/// @file processor.h
/// @brief A user's Python class loaded as an audio processor: attributes, messages, process().
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Host-independent. A processor loads `<scripts_dir>/<name>.py` by path (never through import, so
// no other module can stand in for it), instantiates the class `name` defined there, and describes
// it to the host:
// - the class's type-annotated public fields become attributes (attributes())
// - its public methods become messages, their argument types taken from the hints (messages())
// - a method `process()` becomes the audio callback (process()): `process(self, x: float) -> float`
//   is called once per sample; `process(self, x: np.ndarray) -> np.ndarray` once per vector
// - an optional `prepare(self, sample_rate: float, vector_size: int)` is told the audio settings
//   (prepare()) before the instance first processes audio, and again whenever they change
//
// Threads: load(), prepare(), flush_reports() and the destructor run on the host's main thread;
// set_attribute(), get_attribute() and call() on any non-audio thread; process() on the audio
// thread. Each takes the GIL. A processor that is not loaded (import, class lookup or construction
// failed, or process() raised) outputs silence and ignores attributes and messages until the next
// load().
//
// The audio thread never prints. What goes wrong there — process() raising, returning something
// that is not a number, producing NaN or infinity, or a vector of the wrong length — is recorded
// and the host's report_ready callback is called (it must be real-time safe: in Max, setting a
// qelem); the host then calls flush_reports() on its main thread to print it. Each kind is reported
// once per load. Non-finite output is replaced with 0.0.
//
// The GIL alone does not keep these apart: CPython hands it to a waiting thread every switch
// interval (5 ms), including in the middle of a process() vector or of a reload. So load() builds
// the new binding completely before swapping it in with no Python call in between, and every
// caller holds its own references to what it uses for as long as it uses them. A reload that
// succeeds never interrupts the audio; one that fails silences it.

#pragma once

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tap/python/runtime.h"
#include "tap/python/value.h"

namespace tap::python {

    /// A public, annotated field of the user's class.
    struct attribute_info {
        std::string name;
        value_type  type;
    };

    /// A public method of the user's class, callable through call().
    struct message_info {
        std::string               name;
        std::vector<value_type>   argument_types; ///< one per positional parameter, in order
        std::size_t               required{};     ///< how many of those have no default
        std::optional<value_type> variadic;       ///< the type of *args, if the method takes it
    };

    class processor {
      public:
        /// @param source_name       the module and class name (`<name>.py` defining `class <name>`)
        /// @param log               receives the processor's own diagnostics (may be called on the
        ///                          audio thread)
        /// @param reserved_messages method names the host handles itself; a Python method with one
        ///                          of these names is not exposed as a message (with a diagnostic)
        /// @param report_ready      called on the audio thread when something needs reporting; the
        ///                          host must then call flush_reports() from its main thread. Must be
        ///                          real-time safe (no locks, no allocation).
        explicit processor(std::string source_name, log_function log = {},
                           std::vector<std::string> reserved_messages = {}, std::function<void()> report_ready = {})
            : m_source_name{std::move(source_name)}
            , m_log{std::move(log)}
            , m_reserved_messages{std::move(reserved_messages)}
            , m_report_ready{std::move(report_ready)} {}

        ~processor() {
            if (!Py_IsInitialized()) {
                return;
            }
            gil_lock lock;
            release_binding();
            release_block_buffer();
            clear_carried();
            Py_CLEAR(m_module);
            Py_XDECREF(m_pending_exception.exchange(nullptr));
            Py_XDECREF(m_pending_non_numeric.exchange(nullptr));
        }

        processor(const processor&)            = delete;
        processor& operator=(const processor&) = delete;

        const std::string& source_name() const noexcept { return m_source_name; }

        /// True while a class instance exists (attributes and messages are live).
        bool loaded() const noexcept { return m_instance.load() != nullptr; }

        /// True while process() is bound to the class's process() method.
        bool has_process() const noexcept { return m_process_function.load() != nullptr; }

        /// Load `<scripts_dir>/<name>.py` (executing it only if its source changed since any
        /// processor last loaded it), instantiate the class, carry the previous instance's
        /// attribute values over, and rebuild the attribute and message descriptions. Returns true
        /// when the class was instantiated. Main thread only.
        ///
        /// Until the new class is ready, audio, attributes and messages keep using the previous
        /// instance; on success it is replaced in one step, on failure the processor is unloaded
        /// (its attribute values are kept for the next successful load).
        bool load() {
            if (!Py_IsInitialized()) {
                return false;
            }

            gil_lock lock;

            // 3.4: remember the current attribute values, to carry them onto the new instance. The
            // snapshot outlives a failed load, so fixing the error brings the values back.
            if (PyObject* current = acquire_instance()) {
                capture_attributes(current);
                Py_DECREF(current);
            }

            // D2: the source is <scripts_dir>/<name>.py, loaded by path — never an import that could
            // resolve to another module (a standard-library name, something on sys.path)
            const auto path = scripts_directory() / (m_source_name + ".py");
            if (!is_identifier(m_source_name)) {
                log(log_level::error, "'" + m_source_name
                                          + "' is not a valid Python name: the source must be a .py file named like a "
                                            "Python identifier, defining a class of the same name");
                release_binding();
                return false;
            }
            std::error_code ec;
            if (!std::filesystem::is_regular_file(path, ec)) {
                log(log_level::error, "No file " + path.string());
                release_binding();
                return false;
            }

            bool      executed = false;
            PyObject* module   = load_script(m_source_name, executed);
            if (!module) {
                report_exception();
                log(log_level::error, "Failed to load " + path.string());
                release_binding();
                return false;
            }
            Py_XSETREF(m_module, module);

            PyObject* py_class = PyObject_GetAttrString(m_module, m_source_name.c_str()); // strong
            if (!py_class) {
                PyErr_Clear();
                log(log_level::error, "No class named '" + m_source_name + "' in " + m_source_name + ".py");
                release_binding();
                return false;
            }
            if (!PyCallable_Check(py_class)) {
                Py_DECREF(py_class);
                log(log_level::error, "Cannot instantiate the Python class " + m_source_name);
                release_binding();
                return false;
            }

            binding next;
            next.instance = PyObject_CallNoArgs(py_class);
            if (!next.instance) {
                Py_DECREF(py_class);
                report_exception();
                log(log_level::error, "Failed to instantiate the Python class " + m_source_name);
                release_binding();
                return false;
            }

            collect_attributes(py_class, next);
            Py_DECREF(py_class);
            carry_attributes(next);
            collect_messages(next);

            // Tell the new instance the audio settings before anything can run it.
            if (m_prepared) {
                if (next.block_mode) {
                    ensure_block_buffer(m_vector_size);
                }
                call_prepare(next.prepare_function);
            }

            // The swap: no Python call from here until the old binding is released, so no other
            // thread can take the GIL and see a half-built state.
            binding previous;
            previous.instance         = m_instance.exchange(next.instance);
            previous.process_function = m_process_function.exchange(next.process_function);
            previous.prepare_function = std::exchange(m_prepare_function, next.prepare_function);
            previous.block_mode       = std::exchange(m_block_mode, next.block_mode);
            previous.attributes       = std::exchange(m_attributes, std::move(next.attributes));
            previous.messages         = std::exchange(m_messages, std::move(next.messages));
            next.instance             = nullptr;
            next.process_function     = nullptr;
            next.prepare_function     = nullptr;
            reset_warnings(); // each kind of audio-thread problem is reported once per load

            release(previous); // may run finalizers, which may let other threads in: now safe
            return true;
        }

        /// Tell the processor the audio settings (sample rate in Hz, maximum vector size in samples)
        /// before audio starts and whenever they change; calls the class's prepare(), if it has one,
        /// now and after every later load(). Main thread only.
        void prepare(const double sample_rate, const std::size_t vector_size) {
            m_sample_rate = sample_rate;
            m_vector_size = vector_size;
            m_prepared    = true;
            if (!Py_IsInitialized()) {
                return;
            }

            gil_lock lock;
            if (m_block_mode) {
                ensure_block_buffer(vector_size);
            }
            PyObject* function = m_prepare_function; // bound to the current instance
            Py_XINCREF(function);
            call_prepare(function);
            Py_XDECREF(function);
        }

        /// Print whatever the audio thread recorded since the last flush (see report_ready). Main
        /// thread only; cheap when there is nothing to report.
        void flush_reports() {
            if (!Py_IsInitialized()) {
                return;
            }

            gil_lock lock;
            if (PyObject* exception = m_pending_exception.exchange(nullptr)) {
                PyErr_DisplayException(exception);
                Py_DECREF(exception);
                log(log_level::error,
                    "process() raised an exception — audio disabled until the source is fixed and reloaded");
            }
            if (PyObject* type = m_pending_non_numeric.exchange(nullptr)) {
                log(log_level::error, std::string{"process() returned "}
                                          + reinterpret_cast<PyTypeObject*>(type)->tp_name
                                          + ", not a number — output as 0.0 (reported once per load)");
                Py_DECREF(type);
            }
            if (m_pending_non_finite.exchange(false)) {
                log(log_level::error,
                    "process() produced a non-finite sample (NaN or infinity) — replaced with 0.0 (reported once per "
                    "load)");
            }
            if (const auto returned = m_pending_bad_length.exchange(k_no_length); returned != k_no_length) {
                log(log_level::error, "process() returned " + std::to_string(returned) + " sample(s) for a vector of "
                                          + std::to_string(m_bad_length_expected.load())
                                          + " — that vector is output as silence (reported once per load)");
            }
        }

        /// The class's annotated public fields, in declaration order. Main thread, after load().
        const std::vector<attribute_info>& attributes() const noexcept { return m_attributes; }

        /// The class's public methods other than process(), prepare(), the attributes and the reserved names,
        /// sorted by name. Main thread, after load().
        std::vector<message_info> messages() const {
            std::vector<message_info> result;
            result.reserve(m_messages.size());
            for (const auto& message : m_messages) {
                result.push_back(message.info);
            }
            return result;
        }

        /// Set the instance attribute `name`, converting `v` to the attribute's hinted type (errors,
        /// e.g. an attrs validator rejecting the value, are printed to the console). Returns true on
        /// success.
        bool set_attribute(const std::string_view name, const value& v) {
            if (!loaded()) {
                return false;
            }

            gil_lock lock;

            // the attribute's hinted type decides the conversion (a bool field gets a bool)
            value_type type  = value_type::any;
            const auto found = std::find_if(m_attributes.begin(), m_attributes.end(),
                                            [&](const attribute_info& a) { return a.name == name; });
            if (found != m_attributes.end()) {
                type = found->type;
            }

            PyObject* instance = acquire_instance();
            if (!instance) {
                return false;
            }

            bool      ok       = false;
            PyObject* py_value = to_python(coerce(v, type), type);
            if (py_value) {
                ok = PyObject_SetAttrString(instance, std::string{name}.c_str(), py_value) == 0;
                Py_DECREF(py_value);
            }
            if (!ok) {
                report_exception(); // e.g. an attrs validator rejected the value
            }
            Py_DECREF(instance);
            return ok;
        }

        /// Read the instance attribute `name` as `type`. Empty when there is no instance, no such
        /// attribute, or a value that does not convert (None, or a string in a numeric attribute) —
        /// never an error sentinel. Numbers convert between integer and real (truncating); any
        /// object reads as a symbol through str().
        std::optional<value> get_attribute(const std::string_view name, const value_type type) {
            if (!loaded()) {
                return std::nullopt;
            }

            gil_lock  lock;
            PyObject* instance = acquire_instance();
            if (!instance) {
                return std::nullopt;
            }
            PyObject* py_value = PyObject_GetAttrString(instance, std::string{name}.c_str());
            Py_DECREF(instance);
            if (!py_value) {
                PyErr_Clear();
                return std::nullopt;
            }

            std::optional<value> result = from_python(py_value, type);
            Py_DECREF(py_value);
            return result;
        }

        /// Call the method `name` with `args`, each coerced to its parameter's hinted type (an
        /// unannotated parameter takes the value as it is). The count must fit the method's
        /// signature: its required positional parameters at least, all of them at most unless it
        /// takes *args. Exceptions are printed to the console. Returns true when the method was
        /// called and returned normally.
        bool call(const std::string_view name, const std::span<const value> args) {
            if (!loaded()) {
                return false;
            }

            gil_lock lock;

            const auto found = std::find_if(m_messages.begin(), m_messages.end(),
                                            [&](const bound_message& m) { return m.info.name == name; });
            if (found == m_messages.end()) {
                return false;
            }

            // Take our own copies and references of everything the call uses before anything that
            // could run Python code (even an allocation can trigger a GC finalizer): that could let
            // a reload on another thread replace the message list under us.
            const message_info info     = found->info;
            PyObject*          function = found->function; // bound: call with the arguments only
            Py_INCREF(function);

            const auto positional = info.argument_types.size();
            if (args.size() < info.required || (!info.variadic && args.size() > positional)) {
                Py_DECREF(function);
                std::string expected;
                if (info.variadic) {
                    expected = "at least " + std::to_string(info.required);
                }
                else if (info.required == positional) {
                    expected = std::to_string(positional);
                }
                else {
                    expected = std::to_string(info.required) + " to " + std::to_string(positional);
                }
                log(log_level::error,
                    std::string{name} + ": expected " + expected + " argument(s), got " + std::to_string(args.size()));
                return false;
            }

            std::vector<PyObject*> call_args;
            call_args.reserve(args.size());
            bool converted = true;
            for (std::size_t i = 0; i < args.size(); ++i) {
                const value_type type = i < positional ? info.argument_types[i] : *info.variadic;
                PyObject*        item = to_python(coerce(args[i], type), type);
                if (!item) {
                    converted = false;
                    break;
                }
                call_args.push_back(item);
            }

            PyObject* result =
                converted ? PyObject_Vectorcall(function, call_args.data(), call_args.size(), nullptr) : nullptr;
            for (PyObject* arg : call_args) {
                Py_DECREF(arg);
            }
            Py_DECREF(function);

            if (!result) {
                report_exception();
                return false;
            }
            Py_DECREF(result);
            return true;
        }

        /// The audio callback. Calls the class's process() once per sample, or once per vector when
        /// its input is hinted np.ndarray. `input` and `output` may alias. Outputs silence while
        /// not bound; if process() raises, silences the rest of the vector, unbinds until the next
        /// load() and records the exception for flush_reports(). A reload landing mid-vector does
        /// not affect this vector: it finishes on the binding it started with.
        void process(const double* input, double* output, const std::size_t frame_count) {
            if (!has_process()) {
                std::fill(output, output + frame_count, 0.0);
                return;
            }

            gil_lock lock;

            // the instance and function are swapped together, with no Python call between, so
            // under the GIL they are always a matching pair
            PyObject* function = m_process_function.load();
            PyObject* instance = m_instance.load();
            if (!function || !instance) {
                std::fill(output, output + frame_count, 0.0);
                return;
            }
            Py_INCREF(function);
            Py_INCREF(instance);

            if (m_block_mode) {
                process_block(function, instance, input, output, frame_count);
            }
            else {
                process_samples(function, instance, input, output, frame_count);
            }

            Py_DECREF(instance);
            Py_DECREF(function);
        }

      private:
        struct bound_message {
            message_info info;
            PyObject*    function{}; // strong
        };

        /// Everything load() builds, swapped in as a unit.
        struct binding {
            PyObject*                   instance{};         // strong
            PyObject*                   process_function{}; // strong
            PyObject*                   prepare_function{}; // strong, or null
            bool                        block_mode{};       // process() takes and returns np.ndarray
            std::vector<attribute_info> attributes;
            std::vector<bound_message>  messages;
        };

        static constexpr std::int64_t k_no_length = -1;

        std::string              m_source_name;
        log_function             m_log;
        std::vector<std::string> m_reserved_messages;
        PyObject*                m_module{}; // strong; main thread only
        // Written only under the GIL; atomic so the audio thread's lock-free "is anything
        // bound?" check (and loaded()) is well-defined.
        std::atomic<PyObject*>      m_instance{};         // strong
        std::atomic<PyObject*>      m_process_function{}; // strong
        std::vector<attribute_info> m_attributes;
        std::vector<bound_message>  m_messages;
        // The rest of the binding and the block buffer: read and written only under the GIL.
        PyObject*   m_prepare_function{}; // strong, or null
        bool        m_block_mode{};
        PyObject*   m_block_input{}; // strong: the np.ndarray process() receives, reused every vector
        Py_buffer   m_block_view{};  // held while m_block_input is, so its memory cannot move
        double*     m_block_data{};
        std::size_t m_block_size{};
        // Attribute values captured from the previous instance, for the next one (main thread).
        struct carried_value {
            attribute_info info;
            PyObject*      value{}; // strong
        };
        std::vector<carried_value> m_carried;
        // The audio settings from prepare(); main thread only.
        double      m_sample_rate{};
        std::size_t m_vector_size{};
        bool        m_prepared{};
        // Reports recorded on the audio thread for flush_reports(), and whether each kind has
        // already been recorded since the last load().
        std::function<void()>     m_report_ready;
        std::atomic<PyObject*>    m_pending_exception{};   // strong
        std::atomic<PyObject*>    m_pending_non_numeric{}; // strong: the returned object's type
        std::atomic<bool>         m_pending_non_finite{};
        std::atomic<std::int64_t> m_pending_bad_length{k_no_length};
        std::atomic<std::int64_t> m_bad_length_expected{};
        std::atomic<bool>         m_warned_non_numeric{};
        std::atomic<bool>         m_warned_non_finite{};
        std::atomic<bool>         m_warned_bad_length{};

        void log(const log_level level, const std::string& text) const {
            if (m_log) {
                m_log(level, text);
            }
        }

        /// A new reference to the current instance, or nullptr. Caller holds the GIL.
        PyObject* acquire_instance() const {
            PyObject* instance = m_instance.load();
            Py_XINCREF(instance);
            return instance;
        }

        /// A new reference for `v` as `type` (a boolean becomes a bool), or nullptr with a Python
        /// error set. Caller holds the GIL.
        static PyObject* to_python(const value& v, const value_type type = value_type::any) {
            if (const auto* i = std::get_if<std::int64_t>(&v)) {
                return type == value_type::boolean ? PyBool_FromLong(*i != 0) : PyLong_FromLongLong(*i);
            }
            if (const auto* d = std::get_if<double>(&v)) {
                return PyFloat_FromDouble(*d);
            }
            return PyUnicode_DecodeFSDefault(std::get<std::string>(v).c_str());
        }

        /// `object` as `type`, or empty if it does not convert (see get_attribute). Caller holds
        /// the GIL; never leaves an error set.
        static std::optional<value> from_python(PyObject* object, const value_type type) {
            std::optional<value> result;
            if (object == Py_None) {
                return result;
            }
            switch (type) {
            case value_type::boolean: {
                const int truth = PyObject_IsTrue(object);
                if (truth >= 0) {
                    result = std::int64_t{truth};
                }
                break;
            }
            case value_type::integer:
            case value_type::real: {
                if (!PyNumber_Check(object)) {
                    break;
                }
                const double d = PyFloat_AsDouble(object);
                if (d == -1.0 && PyErr_Occurred()) {
                    break;
                }
                if (type == value_type::real) {
                    result = d;
                }
                else if (PyLong_Check(object)) {
                    const long long i = PyLong_AsLongLong(object);
                    result = (i == -1 && PyErr_Occurred()) ? coerce(d, value_type::integer) : value{std::int64_t{i}};
                }
                else {
                    result = coerce(d, value_type::integer);
                }
                break;
            }
            case value_type::symbol:
            case value_type::any: {
                PyObject* text = PyUnicode_Check(object) ? Py_NewRef(object) : PyObject_Str(object);
                if (text) {
                    if (const char* utf8 = PyUnicode_AsUTF8(text)) {
                        result = std::string{utf8};
                    }
                    Py_DECREF(text);
                }
                break;
            }
            }
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
            return result;
        }

        /// Release a binding's references. Caller holds the GIL. Releasing can run arbitrary
        /// finalizers, which may let other threads take the GIL, so the binding must already be
        /// out of the members.
        static void release(binding& b) {
            Py_CLEAR(b.process_function);
            Py_CLEAR(b.prepare_function);
            Py_CLEAR(b.instance);
            b.attributes.clear();
            for (auto& message : b.messages) {
                Py_CLEAR(message.function);
            }
            b.messages.clear();
        }

        /// Unload: take the binding out of the members, then release it. Caller holds the GIL.
        void release_binding() {
            binding previous;
            previous.instance         = m_instance.exchange(nullptr);
            previous.process_function = m_process_function.exchange(nullptr);
            previous.prepare_function = std::exchange(m_prepare_function, nullptr);
            previous.block_mode       = std::exchange(m_block_mode, false);
            previous.attributes       = std::exchange(m_attributes, {});
            previous.messages         = std::exchange(m_messages, {});
            release(previous);
        }

        /// Unbind process() if it is still bound to `function` (a reload may have replaced it
        /// while the failing vector ran). Caller holds the GIL.
        void unbind_process(PyObject* function) {
            PyObject* expected = function;
            if (m_process_function.compare_exchange_strong(expected, nullptr)) {
                Py_DECREF(function); // the member's reference; the caller still holds its own
            }
        }

        static bool is_identifier(const std::string& name) {
            PyObject* text = PyUnicode_FromString(name.c_str());
            if (!text) {
                PyErr_Clear();
                return false;
            }
            const bool ok = PyUnicode_IsIdentifier(text) == 1;
            Py_DECREF(text);
            return ok;
        }

        /// Caller holds the GIL.
        void clear_carried() {
            auto carried = std::exchange(m_carried, {});
            for (auto& c : carried) {
                Py_CLEAR(c.value);
            }
        }

        /// Snapshot `instance`'s current attribute values. Caller holds the GIL; main thread.
        void capture_attributes(PyObject* instance) {
            clear_carried();
            const auto attributes = m_attributes; // getattr runs Python code: work on a copy
            for (const auto& attribute : attributes) {
                PyObject* value = PyObject_GetAttrString(instance, attribute.name.c_str());
                if (!value) {
                    PyErr_Clear();
                    continue;
                }
                m_carried.push_back({attribute, value});
            }
        }

        /// Set the snapshot onto the new instance, for the attributes it still has with the same
        /// type (a changed type starts from the new class default). Caller holds the GIL.
        void carry_attributes(binding& b) {
            auto carried = std::exchange(m_carried, {});
            for (auto& c : carried) {
                const auto found = std::find_if(b.attributes.begin(), b.attributes.end(),
                                                [&](const attribute_info& a) { return a.name == c.info.name; });
                if (found == b.attributes.end()) {
                    // removed from the class
                }
                else if (found->type != c.info.type) {
                    log(log_level::info,
                        "attribute '" + c.info.name + "' changed type; it starts from the class default");
                }
                else if (PyObject_SetAttrString(b.instance, c.info.name.c_str(), c.value) != 0) {
                    report_exception();
                    log(log_level::error, "could not carry attribute '" + c.info.name + "' over the reload");
                }
                Py_CLEAR(c.value);
            }
        }

        void reset_warnings() {
            m_warned_non_numeric = false;
            m_warned_non_finite  = false;
            m_warned_bad_length  = false;
        }

        void notify_report() const {
            if (m_report_ready) {
                m_report_ready();
            }
        }

        /// Record the pending exception (audio thread; caller holds the GIL). Only the first since
        /// the last flush is kept — the rest of that vector is silenced, and process() unbound.
        void record_exception() {
            PyObject* exception = PyErr_GetRaisedException();
            PyObject* expected  = nullptr;
            if (exception && !m_pending_exception.compare_exchange_strong(expected, exception)) {
                Py_DECREF(exception);
            }
            notify_report();
        }

        /// Record that process() returned `object`, which is not a number. Caller holds the GIL.
        void record_non_numeric(PyObject* object) {
            if (m_warned_non_numeric.exchange(true)) {
                return;
            }
            PyObject* type = reinterpret_cast<PyObject*>(Py_TYPE(object));
            Py_INCREF(type);
            Py_XDECREF(m_pending_non_numeric.exchange(type));
            notify_report();
        }

        /// Replace non-finite samples with 0.0, recording the first occurrence per load.
        void sanitize(double* output, const std::size_t frame_count) {
            bool found = false;
            for (std::size_t i = 0; i < frame_count; ++i) {
                if (!std::isfinite(output[i])) {
                    output[i] = 0.0;
                    found     = true;
                }
            }
            if (found && !m_warned_non_finite.exchange(true)) {
                m_pending_non_finite = true;
                notify_report();
            }
        }

        /// The per-sample path. Caller holds the GIL and references to `function` and `instance`.
        void process_samples(PyObject* function, PyObject* instance, const double* input, double* output,
                             const std::size_t frame_count) {
            for (std::size_t i = 0; i < frame_count; ++i) {
                PyObject* x = PyFloat_FromDouble(input[i]);
                if (!x) {
                    record_exception();
                    std::fill(output + i, output + frame_count, 0.0);
                    return;
                }
                PyObject* const call_args[2] = {instance, x};
                PyObject*       result       = PyObject_Vectorcall(function, call_args, 2, nullptr);
                Py_DECREF(x);

                if (!result) {
                    record_exception();
                    unbind_process(function); // load() re-arms it
                    std::fill(output + i, output + frame_count, 0.0);
                    return;
                }

                double y = PyFloat_AsDouble(result); // also handles ints and other number types
                if (y == -1.0 && PyErr_Occurred()) {
                    PyErr_Clear();
                    record_non_numeric(result);
                    y = 0.0;
                }
                output[i] = y;
                Py_DECREF(result);
            }
            sanitize(output, frame_count);
        }

        /// The block path: one call per vector with the reused input array. Caller holds the GIL
        /// and references to `function` and `instance`.
        void process_block(PyObject* function, PyObject* instance, const double* input, double* output,
                           const std::size_t frame_count) {
            // allocates only when the vector size differs from the one prepare() announced
            if (!ensure_block_buffer(frame_count)) {
                std::fill(output, output + frame_count, 0.0);
                return;
            }

            // copy in and take our reference before calling, which may yield the GIL
            std::memcpy(m_block_data, input, frame_count * sizeof(double));
            PyObject* x = m_block_input;
            Py_INCREF(x);
            PyObject* const call_args[2] = {instance, x};
            PyObject*       result       = PyObject_Vectorcall(function, call_args, 2, nullptr);
            Py_DECREF(x);

            if (!result) {
                record_exception();
                unbind_process(function);
                std::fill(output, output + frame_count, 0.0);
                return;
            }
            copy_block_result(result, output, frame_count);
            Py_DECREF(result);
            sanitize(output, frame_count);
        }

        static bool is_native_double(const Py_buffer& view) {
            const std::string_view format{view.format ? view.format : "B"};
            return view.itemsize == static_cast<Py_ssize_t>(sizeof(double))
                   && (format == "d" || format == "@d" || format == "=d"
                       || (format == "<d" && std::endian::native == std::endian::little));
        }

        /// Copy process()'s block result into `output`: directly from a contiguous float64 buffer,
        /// otherwise through np.ascontiguousarray(result, dtype=float64). Caller holds the GIL.
        void copy_block_result(PyObject* result, double* output, const std::size_t frame_count) {
            const auto expected_bytes = static_cast<Py_ssize_t>(frame_count * sizeof(double));

            Py_buffer view;
            if (PyObject_GetBuffer(result, &view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == 0) {
                if (is_native_double(view) && view.len == expected_bytes) {
                    std::memcpy(output, view.buf, frame_count * sizeof(double));
                    PyBuffer_Release(&view);
                    return;
                }
                PyBuffer_Release(&view);
            }
            else {
                PyErr_Clear();
            }

            // np.ascontiguousarray(None) is a 0-d NaN array, not an error: a process() that forgot
            // its return statement would read as a length mismatch
            if (result == Py_None) {
                record_non_numeric(result);
                std::fill(output, output + frame_count, 0.0);
                return;
            }

            PyObject* converted = nullptr;
            if (PyObject* numpy = PyImport_ImportModule("numpy")) {
                converted = PyObject_CallMethod(numpy, "ascontiguousarray", "Os", result, "float64");
                Py_DECREF(numpy);
            }
            if (!converted) {
                PyErr_Clear();
                record_non_numeric(result);
                std::fill(output, output + frame_count, 0.0);
                return;
            }
            if (PyObject_GetBuffer(converted, &view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) != 0) {
                PyErr_Clear();
                Py_DECREF(converted);
                record_non_numeric(result);
                std::fill(output, output + frame_count, 0.0);
                return;
            }
            if (view.len == expected_bytes) {
                std::memcpy(output, view.buf, frame_count * sizeof(double));
            }
            else {
                std::fill(output, output + frame_count, 0.0);
                if (!m_warned_bad_length.exchange(true)) {
                    m_bad_length_expected = static_cast<std::int64_t>(frame_count);
                    m_pending_bad_length =
                        static_cast<std::int64_t>(view.len / static_cast<Py_ssize_t>(sizeof(double)));
                    notify_report();
                }
            }
            PyBuffer_Release(&view);
            Py_DECREF(converted);
        }

        /// Make sure the block input array holds `size` samples, building a new np.zeros(size) if
        /// not. Caller holds the GIL. The new array is built completely, then swapped in with no
        /// Python call in between (building it can yield the GIL). Returns false if numpy failed.
        bool ensure_block_buffer(const std::size_t size) {
            if (m_block_input && m_block_size == size) {
                return true;
            }

            PyObject* array = nullptr;
            if (PyObject* numpy = PyImport_ImportModule("numpy")) {
                array = PyObject_CallMethod(numpy, "zeros", "n", static_cast<Py_ssize_t>(size));
                Py_DECREF(numpy);
            }
            Py_buffer view;
            if (!array || PyObject_GetBuffer(array, &view, PyBUF_WRITABLE | PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) != 0) {
                PyErr_Clear();
                Py_XDECREF(array);
                return false;
            }
            if (!is_native_double(view)) {
                PyBuffer_Release(&view);
                Py_DECREF(array);
                return false;
            }
            if (m_block_input && m_block_size == size) { // another thread built one meanwhile
                PyBuffer_Release(&view);
                Py_DECREF(array);
                return true;
            }

            PyObject* old_input = std::exchange(m_block_input, array);
            Py_buffer old_view  = std::exchange(m_block_view, view);
            m_block_data        = static_cast<double*>(view.buf);
            m_block_size        = size;
            if (old_input) {
                PyBuffer_Release(&old_view);
                Py_DECREF(old_input);
            }
            return true;
        }

        /// Caller holds the GIL.
        void release_block_buffer() {
            if (m_block_input) {
                PyBuffer_Release(&m_block_view);
                Py_CLEAR(m_block_input);
            }
            m_block_data = nullptr;
            m_block_size = 0;
        }

        /// Call `function` (the class's prepare(), bound to its instance, if it has one) with the
        /// current audio settings; report an exception. Caller holds the GIL; main thread.
        void call_prepare(PyObject* function) {
            if (!function) {
                return;
            }
            PyObject* sample_rate = PyFloat_FromDouble(m_sample_rate);
            PyObject* vector_size = PyLong_FromSize_t(m_vector_size);
            PyObject* result      = nullptr;
            if (sample_rate && vector_size) {
                PyObject* const call_args[2] = {sample_rate, vector_size};
                result                       = PyObject_Vectorcall(function, call_args, 2, nullptr);
            }
            Py_XDECREF(sample_rate);
            Py_XDECREF(vector_size);
            if (!result) {
                report_exception();
                log(log_level::error, "prepare() raised an exception — the instance may not know the audio settings");
                return;
            }
            Py_DECREF(result);
        }

        bool is_reserved(const std::string& name) const {
            return std::find(m_reserved_messages.begin(), m_reserved_messages.end(), name) != m_reserved_messages.end();
        }

        /// A support-module call with one argument; a new reference, or nullptr with an error set.
        static PyObject* call_support(const char* function_name, PyObject* argument) {
            PyObject* function = detail::support(function_name);
            if (!function) {
                PyErr_Format(PyExc_RuntimeError, "the tap.python support function %s is missing", function_name);
                return nullptr;
            }
            return PyObject_CallOneArg(function, argument);
        }

        /// Report a type-hint error the support module caught (a borrowed exception, or None).
        void report_hint_error(PyObject* error, const std::string& what) const {
            if (!error || error == Py_None) {
                return;
            }
            PyErr_DisplayException(error);
            log(log_level::error, "could not resolve the type hints of " + what + "; using the annotations as written");
        }

        static std::string utf8(PyObject* text) {
            const char* s = text ? PyUnicode_AsUTF8(text) : nullptr;
            if (!s) {
                PyErr_Clear();
                return {};
            }
            return s;
        }

        /// Describe the class's annotated public fields as attributes (ClassVars are not fields).
        /// Caller holds the GIL.
        void collect_attributes(PyObject* py_class, binding& b) const {
            PyObject* result = call_support("class_hints", py_class);
            if (!result) {
                report_exception();
                return;
            }
            PyObject* fields = PyTuple_GetItem(result, 0); // borrowed
            report_hint_error(PyTuple_GetItem(result, 1), "class " + m_source_name);

            const Py_ssize_t count = fields ? PyList_Size(fields) : 0;
            for (Py_ssize_t i = 0; i < count; ++i) {
                PyObject*         field = PyList_GetItem(fields, i); // borrowed (name, kind)
                const std::string name  = utf8(PyTuple_GetItem(field, 0));
                const std::string kind  = utf8(PyTuple_GetItem(field, 1));
                if (name.empty() || name[0] == '_' || kind == "ClassVar") {
                    continue;
                }
                b.attributes.push_back({name, value_type_from_hint(kind)});
            }
            Py_DECREF(result);
        }

        /// A method's parameters, as the support module's describe() reports them.
        struct parameter {
            std::string name;
            std::string kind; // the hint kind
            int         parameter_kind{};
            bool        has_default{};
        };

        struct signature {
            std::vector<parameter> parameters;
            std::string            return_kind;
        };

        // inspect.Parameter.kind values
        static constexpr int k_positional_only     = 0;
        static constexpr int k_positional_or_named = 1;
        static constexpr int k_var_positional      = 2;
        static constexpr int k_keyword_only        = 3;

        /// Describe a callable's signature; a hint error is reported. Empty if inspection failed
        /// (reported too). Caller holds the GIL.
        std::optional<signature> describe(PyObject* callable, const std::string& what) const {
            PyObject* result = call_support("describe", callable);
            if (!result) {
                report_exception();
                return std::nullopt;
            }
            signature sig;
            PyObject* parameters = PyTuple_GetItem(result, 0); // borrowed
            sig.return_kind      = utf8(PyTuple_GetItem(result, 1));
            report_hint_error(PyTuple_GetItem(result, 2), what);

            const Py_ssize_t count = parameters ? PyList_Size(parameters) : 0;
            for (Py_ssize_t i = 0; i < count; ++i) {
                PyObject* p = PyList_GetItem(parameters, i); // borrowed
                sig.parameters.push_back({utf8(PyTuple_GetItem(p, 0)), utf8(PyTuple_GetItem(p, 1)),
                                          static_cast<int>(PyLong_AsLong(PyTuple_GetItem(p, 2))),
                                          PyObject_IsTrue(PyTuple_GetItem(p, 3)) == 1});
            }
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }
            Py_DECREF(result);
            return sig;
        }

        /// Bind the instance's public methods as messages, process() as the audio callback and
        /// prepare() as the settings hook. Caller holds the GIL.
        void collect_messages(binding& b) const {
            PyObject* methods = call_support("methods", b.instance);
            if (!methods) {
                report_exception();
                return;
            }

            const auto is_attribute = [&](const std::string& name) {
                return std::any_of(b.attributes.begin(), b.attributes.end(),
                                   [&](const attribute_info& a) { return a.name == name; });
            };

            const Py_ssize_t count = PyList_Size(methods);
            for (Py_ssize_t i = 0; i < count; ++i) {
                PyObject*         entry  = PyList_GetItem(methods, i); // borrowed (name, bound callable)
                const std::string name   = utf8(PyTuple_GetItem(entry, 0));
                PyObject*         method = PyTuple_GetItem(entry, 1); // borrowed
                if (name.empty() || !method || is_attribute(name)) {
                    continue;
                }

                if (name == "prepare") {
                    Py_INCREF(method);
                    b.prepare_function = method;
                }
                else if (name == "process") {
                    bind_process(method, b);
                }
                else if (is_reserved(name)) {
                    log(log_level::error,
                        name + "() is reserved by the host and is not exposed as a message; rename the method");
                }
                else {
                    bind_message(name, method, b);
                }
            }
            Py_DECREF(methods);
        }

        /// Bind the class's process() method as the audio callback: per vector when its first
        /// parameter is hinted np.ndarray, per sample otherwise. Caller holds the GIL.
        void bind_process(PyObject* method, binding& b) const {
            // the audio path calls the function with the instance explicitly (one less indirection
            // per sample), so process() must be a plain instance method
            PyObject* fn = PyMethod_Check(method) ? PyMethod_Function(method) : nullptr; // borrowed
            if (!fn || !PyFunction_Check(fn) || PyMethod_Self(method) != b.instance) {
                log(log_level::error, "process() must be a regular instance method (not a classmethod or "
                                      "staticmethod); audio is not bound");
                return;
            }

            const auto sig = describe(method, "process()");
            if (!sig) {
                return;
            }
            int  in_count    = 0;
            bool block_input = false;
            for (const auto& p : sig->parameters) {
                if (p.parameter_kind == k_positional_only || p.parameter_kind == k_positional_or_named) {
                    if (in_count == 0) {
                        block_input = p.kind == "ndarray";
                    }
                    ++in_count;
                }
            }

            if (in_count > 1) {
                log(log_level::error, "process() declares " + std::to_string(in_count)
                                          + " inputs but only the first is supported (single-channel object)");
            }
            if (sig->return_kind == "tuple") {
                log(log_level::error,
                    "process() returns a tuple — multichannel output is not supported yet; use a single float return");
                return;
            }

            Py_INCREF(fn);
            b.process_function = fn;
            b.block_mode       = block_input;

            log(log_level::info, block_input ? "Audio process() bound: 1 input, 1 output, one call per vector (numpy)"
                                             : "Audio process() bound: 1 input, 1 output, one call per sample");
        }

        /// Bind a public method as a message, described by its signature. A keyword-only parameter
        /// without a default cannot be passed from the host, so such a method is not exposed.
        /// Caller holds the GIL.
        void bind_message(const std::string& name, PyObject* method, binding& b) const {
            const auto sig = describe(method, name + "()");
            if (!sig) {
                return;
            }

            message_info info{name, {}, 0, std::nullopt};
            for (const auto& p : sig->parameters) {
                switch (p.parameter_kind) {
                case k_positional_only:
                case k_positional_or_named:
                    info.argument_types.push_back(value_type_from_hint(p.kind));
                    if (!p.has_default) {
                        info.required = info.argument_types.size();
                    }
                    break;
                case k_var_positional:
                    info.variadic = value_type_from_hint(p.kind);
                    break;
                case k_keyword_only:
                    if (!p.has_default) {
                        log(log_level::error, name + "() has a keyword-only parameter '" + p.name
                                                  + "' without a default, which a message cannot pass; it is not "
                                                    "exposed as a message");
                        return;
                    }
                    break;
                default: // **kwargs: nothing to pass
                    break;
                }
            }

            Py_INCREF(method);
            b.messages.push_back({std::move(info), method});
        }
    };

} // namespace tap::python

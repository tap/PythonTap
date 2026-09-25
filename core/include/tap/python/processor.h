/// @file processor.h
/// @brief A user's Python class loaded as an audio processor: attributes, messages, process().
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Host-independent. A processor imports `<scripts_dir>/<name>.py`, instantiates the class `name`
// defined there, and describes it to the host:
// - the class's type-annotated public fields become attributes (attributes())
// - its public methods become messages, their argument types taken from the hints (messages())
// - a method `process(self, x: float) -> float` becomes the per-sample audio callback (process())
//
// Threads: load() and the destructor run on the host's main thread; set_attribute(),
// get_attribute() and call() on any non-audio thread; process() on the audio thread. Each takes
// the GIL. A processor that is not loaded (import, class lookup or construction failed, or
// process() raised) outputs silence and ignores attributes and messages until the next load().

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
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
        std::string             name;
        std::vector<value_type> argument_types; ///< one per annotated parameter, in order
    };

    class processor {
      public:
        /// @param source_name the module and class name (`<name>.py` defining `class <name>`)
        /// @param log         receives the processor's own diagnostics (may be called on the audio thread)
        explicit processor(std::string source_name, log_function log = {})
            : m_source_name{std::move(source_name)}
            , m_log{std::move(log)} {}

        ~processor() {
            if (!Py_IsInitialized()) {
                return;
            }
            gil_lock lock;
            release_binding();
            Py_CLEAR(m_module);
        }

        processor(const processor&)            = delete;
        processor& operator=(const processor&) = delete;

        const std::string& source_name() const noexcept { return m_source_name; }

        /// True while a class instance exists (attributes and messages are live).
        bool loaded() const noexcept { return m_instance != nullptr; }

        /// True while process() is bound to the class's process() method.
        bool has_process() const noexcept { return m_process_function != nullptr; }

        /// (Re)import the module, instantiate the class, and rebuild the attribute and message
        /// descriptions. Imports on the first call and reloads the module afterwards. Returns true
        /// when the class was instantiated. Main thread only.
        bool load() {
            if (!Py_IsInitialized()) {
                return false;
            }

            gil_lock lock;

            // detach the audio binding first: process() treats a null function as "output silence"
            release_binding();

            if (m_module) {
                PyObject* reloaded = PyImport_ReloadModule(m_module);
                if (!reloaded) {
                    PyErr_Print();
                    log(log_level::error, "Failed to reload module " + m_source_name);
                    return false;
                }
                Py_DECREF(m_module);
                m_module = reloaded;
            }
            else {
                PyObject* name = PyUnicode_DecodeFSDefault(m_source_name.c_str());
                m_module       = name ? PyImport_Import(name) : nullptr;
                Py_XDECREF(name);
                if (!m_module) {
                    PyErr_Print();
                    log(log_level::error, "Failed to load module '" + m_source_name + "' (searched "
                                              + scripts_directory().string() + ")");
                    return false;
                }
            }

            PyObject* module_dict = PyModule_GetDict(m_module);                               // borrowed
            PyObject* py_class    = PyDict_GetItemString(module_dict, m_source_name.c_str()); // borrowed
            if (!py_class) {
                log(log_level::error, "No class named '" + m_source_name + "' in " + m_source_name + ".py");
                return false;
            }
            if (!PyCallable_Check(py_class)) {
                log(log_level::error, "Cannot instantiate the Python class " + m_source_name);
                return false;
            }

            m_instance = PyObject_CallObject(py_class, nullptr);
            if (!m_instance) {
                PyErr_Print();
                log(log_level::error, "Failed to instantiate the Python class " + m_source_name);
                return false;
            }

            collect_attributes(py_class);
            collect_messages();
            return true;
        }

        /// The class's annotated public fields, in declaration order. Main thread, after load().
        const std::vector<attribute_info>& attributes() const noexcept { return m_attributes; }

        /// The class's public methods other than process() and the attributes, sorted by name.
        /// Main thread, after load().
        std::vector<message_info> messages() const {
            std::vector<message_info> result;
            result.reserve(m_messages.size());
            for (const auto& message : m_messages) {
                result.push_back(message.info);
            }
            return result;
        }

        /// Set the instance attribute `name` (errors, e.g. an attrs validator rejecting the value,
        /// are printed to the console). Returns true on success.
        bool set_attribute(const std::string_view name, const value& v) {
            if (!loaded()) {
                return false;
            }

            gil_lock lock;

            if (!m_instance) {
                return false;
            }
            PyObject* py_value = to_python(v);
            if (!py_value) {
                PyErr_Print();
                return false;
            }
            const bool ok = PyObject_SetAttrString(m_instance, std::string{name}.c_str(), py_value) == 0;
            if (!ok) {
                PyErr_Print(); // e.g. an attrs validator rejected the value
            }
            Py_DECREF(py_value);
            return ok;
        }

        /// Read the instance attribute `name` as `type`. Empty when there is no instance or no such
        /// attribute; a value that does not convert reads as CPython's error sentinel (-1).
        std::optional<value> get_attribute(const std::string_view name, const value_type type) {
            if (!loaded()) {
                return std::nullopt;
            }

            gil_lock lock;

            if (!m_instance) {
                return std::nullopt;
            }
            PyObject* py_value = PyObject_GetAttrString(m_instance, std::string{name}.c_str());
            if (!py_value) {
                PyErr_Clear();
                return std::nullopt;
            }

            value result;
            switch (type) {
            case value_type::real:
                result = PyFloat_AsDouble(py_value);
                break;
            case value_type::integer:
                result = static_cast<std::int64_t>(PyLong_AsLongLong(py_value));
                break;
            case value_type::symbol: {
                const char* str = PyUnicode_AsUTF8(py_value);
                result          = std::string{str ? str : ""};
                break;
            }
            }
            if (PyErr_Occurred()) {
                PyErr_Clear();
            }

            Py_DECREF(py_value);
            return result;
        }

        /// Call the method `name` with `args`, each coerced to its parameter's hinted type. The
        /// argument count must match the number of annotated parameters. Exceptions are printed to
        /// the console. Returns true when the method was called and returned normally.
        bool call(const std::string_view name, const std::span<const value> args) {
            if (!loaded()) {
                return false;
            }

            gil_lock lock;

            if (!m_instance) {
                return false;
            }
            const auto found = std::find_if(m_messages.begin(), m_messages.end(),
                                            [&](const bound_message& m) { return m.info.name == name; });
            if (found == m_messages.end()) {
                return false;
            }

            const auto& types = found->info.argument_types;
            if (args.size() != types.size()) {
                log(log_level::error, std::string{name} + ": expected " + std::to_string(types.size())
                                          + " argument(s), got " + std::to_string(args.size()));
                return false;
            }

            PyObject* py_args = PyTuple_New(static_cast<Py_ssize_t>(args.size() + 1));
            if (!py_args) {
                PyErr_Clear();
                return false;
            }
            Py_INCREF(m_instance); // the tuple *steals* a reference
            PyTuple_SetItem(py_args, 0, m_instance);

            for (std::size_t i = 0; i < args.size(); ++i) {
                PyObject* item = to_python(coerce(args[i], types[i]));
                if (!item) {
                    PyErr_Print();
                    Py_DECREF(py_args);
                    return false;
                }
                PyTuple_SetItem(py_args, static_cast<Py_ssize_t>(i + 1), item); // steals the reference
            }

            // hold our own reference: the method may release the GIL, and a reload on another
            // thread would release the binding's
            PyObject* function = found->function;
            Py_INCREF(function);
            PyObject* result = PyObject_Call(function, py_args, nullptr);
            Py_DECREF(function);
            Py_DECREF(py_args);

            if (!result) {
                PyErr_Print();
                return false;
            }
            Py_DECREF(result);
            return true;
        }

        /// The audio callback: calls the Python process() once per sample. `input` and `output`
        /// may alias. Outputs silence while not bound; if process() raises, prints the traceback,
        /// silences the rest of the vector and unbinds until the next load().
        void process(const double* input, double* output, const std::size_t frame_count) {
            if (!m_process_function) {
                std::fill(output, output + frame_count, 0.0);
                return;
            }

            gil_lock lock;

            if (!m_process_function) { // re-check now that we hold the GIL
                std::fill(output, output + frame_count, 0.0);
                return;
            }

            for (std::size_t i = 0; i < frame_count; ++i) {
                PyTuple_SetItem(m_process_args, 1, PyFloat_FromDouble(input[i])); // the tuple steals the float
                PyObject* result = PyObject_CallObject(m_process_function, m_process_args);
                if (!result) {
                    PyErr_Print();
                    log(log_level::error,
                        "process() raised an exception — audio disabled until the source is fixed and reloaded");
                    Py_CLEAR(m_process_function); // load() re-arms it
                    std::fill(output + i, output + frame_count, 0.0);
                    return;
                }

                double y = PyFloat_AsDouble(result); // also handles ints and other number types
                if (PyErr_Occurred()) {
                    PyErr_Clear();
                    y = 0.0;
                }
                output[i] = y;
                Py_DECREF(result);
            }
        }

      private:
        struct bound_message {
            message_info info;
            PyObject*    function{}; // strong
        };

        std::string                 m_source_name;
        log_function                m_log;
        PyObject*                   m_module{};           // strong
        PyObject*                   m_instance{};         // strong
        PyObject*                   m_process_function{}; // strong
        PyObject*                   m_process_args{};     // strong; slot 0 holds a ref to m_instance
        std::vector<attribute_info> m_attributes;
        std::vector<bound_message>  m_messages;

        void log(const log_level level, const std::string& text) const {
            if (m_log) {
                m_log(level, text);
            }
        }

        /// A new reference for `v`, or nullptr with a Python error set. Caller holds the GIL.
        static PyObject* to_python(const value& v) {
            if (const auto* i = std::get_if<std::int64_t>(&v)) {
                return PyLong_FromLongLong(*i);
            }
            if (const auto* d = std::get_if<double>(&v)) {
                return PyFloat_FromDouble(*d);
            }
            return PyUnicode_DecodeFSDefault(std::get<std::string>(v).c_str());
        }

        /// Drop the instance, process() binding and messages. Caller holds the GIL.
        void release_binding() {
            Py_CLEAR(m_process_function);
            Py_CLEAR(m_process_args);
            Py_CLEAR(m_instance);
            m_attributes.clear();

            // releasing a function can run arbitrary finalizers (which may release the GIL), so
            // take the list out of the member before letting go of anything
            auto old_messages = std::move(m_messages);
            m_messages.clear();
            for (auto& message : old_messages) {
                Py_CLEAR(message.function);
            }
        }

        /// Describe the class-level type hints as attributes. Caller holds the GIL.
        void collect_attributes(PyObject* py_class) {
            PyObject* hints = detail::get_type_hints(py_class);
            if (!hints) {
                PyErr_Clear();
                return;
            }

            PyObject*  key;
            PyObject*  hint;
            Py_ssize_t pos = 0;
            while (PyDict_Next(hints, &pos, &key, &hint)) { // key/hint are borrowed
                const char* key_cstr = PyUnicode_AsUTF8(key);
                if (!key_cstr) {
                    PyErr_Clear();
                    continue;
                }
                if (key_cstr[0] == '_') {
                    continue;
                }
                m_attributes.push_back({key_cstr, value_type_from_hint(detail::hint_name(hint, "str"))});
            }
            Py_DECREF(hints);
        }

        bool is_attribute(const std::string& name) const {
            return std::any_of(m_attributes.begin(), m_attributes.end(),
                               [&](const attribute_info& a) { return a.name == name; });
        }

        /// Bind the instance's public methods as messages, and process() as the audio callback.
        /// Caller holds the GIL.
        void collect_messages() {
            PyObject* members = PyObject_Dir(m_instance);
            if (!members) {
                PyErr_Clear();
                return;
            }

            const auto member_count = PyList_Size(members);
            for (Py_ssize_t i = 0; i < member_count; ++i) {
                PyObject*   member    = PyList_GetItem(members, i); // borrowed
                const char* name_cstr = member ? PyUnicode_AsUTF8(member) : nullptr;
                if (!name_cstr) {
                    PyErr_Clear();
                    continue;
                }
                if (name_cstr[0] == '_') {
                    continue;
                }

                const std::string member_name{name_cstr};
                if (is_attribute(member_name)) {
                    continue;
                }

                PyObject* method = PyObject_GetAttrString(m_instance, member_name.c_str());
                if (!method) {
                    PyErr_Clear();
                    continue;
                }

                if (PyMethod_Check(method)) {
                    PyObject* fn = PyMethod_Function(method); // borrowed
                    if (fn && PyFunction_Check(fn)) {
                        PyObject* hints = detail::get_type_hints(method);
                        if (!hints) {
                            PyErr_Clear();
                        }

                        if (member_name == "process") {
                            bind_process(fn, hints);
                        }
                        else {
                            bind_message(member_name, fn, hints);
                        }

                        Py_XDECREF(hints);
                    }
                }
                Py_DECREF(method);
            }
            Py_DECREF(members);
        }

        /// Bind the class's process() method as the per-sample audio callback. Caller holds the GIL.
        void bind_process(PyObject* fn, PyObject* hints) {
            int  in_count      = 0;
            bool returns_tuple = false;

            if (hints) {
                PyObject*  key;
                PyObject*  hint;
                Py_ssize_t pos = 0;
                while (PyDict_Next(hints, &pos, &key, &hint)) { // borrowed
                    const char* key_cstr = PyUnicode_AsUTF8(key);
                    if (!key_cstr) {
                        PyErr_Clear();
                        continue;
                    }
                    if (std::string_view{key_cstr} == "return") {
                        returns_tuple = detail::hint_name(hint, "") == "tuple";
                    }
                    else {
                        ++in_count;
                    }
                }
            }

            if (in_count > 1) {
                log(log_level::error, "process() declares " + std::to_string(in_count)
                                          + " inputs but only the first is supported (single-channel object)");
            }
            if (returns_tuple) {
                log(log_level::error,
                    "process() returns a tuple — multichannel output is not supported yet; use a single float return");
                return;
            }

            m_process_args = PyTuple_New(2);
            if (!m_process_args) {
                PyErr_Clear();
                return;
            }
            Py_INCREF(m_instance); // the tuple *steals* a reference
            PyTuple_SetItem(m_process_args, 0, m_instance);
            PyTuple_SetItem(m_process_args, 1, PyFloat_FromDouble(0.0)); // placeholder; replaced every sample

            Py_INCREF(fn);
            m_process_function = fn;

            log(log_level::info, "Audio process() bound: 1 input, 1 output");
        }

        /// Bind a public method as a message. Caller holds the GIL.
        void bind_message(const std::string& name, PyObject* fn, PyObject* hints) {
            bound_message message{{name, {}}, fn};

            if (hints) {
                PyObject*  key;
                PyObject*  hint;
                Py_ssize_t pos = 0;
                while (PyDict_Next(hints, &pos, &key, &hint)) { // borrowed
                    const char* key_cstr = PyUnicode_AsUTF8(key);
                    if (!key_cstr) {
                        PyErr_Clear();
                        continue;
                    }
                    if (std::string_view{key_cstr} == "return") {
                        continue;
                    }
                    message.info.argument_types.push_back(value_type_from_hint(detail::hint_name(hint, "str")));
                }
            }

            Py_INCREF(fn);
            m_messages.push_back(std::move(message));
        }
    };

} // namespace tap::python

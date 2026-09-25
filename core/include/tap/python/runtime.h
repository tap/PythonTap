/// @file runtime.h
/// @brief The process-wide embedded CPython interpreter: start-up, GIL discipline, console routing.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Host-independent: no Max, no min-api. A host (the Max external, a test harness, a future plugin
// front end) calls initialize() once with where the runtime and the user's scripts live and where
// Python's print() output should go; everything after that takes a gil_lock.

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h> // CPython must precede the standard headers

// standard library
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

namespace tap::python {

    /// Severity of a line of output, whether from Python's sys.stdout/sys.stderr or from the core.
    enum class log_level { info, error };

    /// Receives complete lines (without the trailing newline). May be called from any thread,
    /// including the audio thread, so an implementation must be thread-safe.
    using log_function = std::function<void(log_level, std::string_view)>;

    /// What initialize() needs from the host.
    struct runtime_options {
        std::filesystem::path home;        ///< the CPython installation (PyConfig.home)
        std::filesystem::path scripts_dir; ///< the user's script folder, made importable
        log_function          console;     ///< where Python's stdout (info) and stderr (error) go
    };

    /// The outcome of initialize(): ok, or the reason the interpreter could not start.
    struct runtime_status {
        bool        ok{false};
        std::string error;
    };

    /// RAII guard for the GIL. Every call into the CPython API after initialize() must hold one of
    /// these — hosts call in from several threads (e.g. Max's main, scheduler and audio threads).
    class gil_lock {
      public:
        gil_lock()
            : m_state{PyGILState_Ensure()} {}

        ~gil_lock() { PyGILState_Release(m_state); }

        gil_lock(const gil_lock&)            = delete;
        gil_lock& operator=(const gil_lock&) = delete;

      private:
        PyGILState_STATE m_state;
    };

    namespace detail {

        // _maxconsole: a tiny built-in module that sys.stdout/sys.stderr are rebound to, so that
        // print() and tracebacks reach the host's console instead of disappearing. Output is
        // buffered per stream and forwarded a line at a time.

        struct console_state {
            std::mutex   mutex;
            log_function sink;
            std::string  buffer_out;
            std::string  buffer_err;
        };

        inline console_state& console() {
            static console_state s_console;
            return s_console;
        }

        inline std::filesystem::path& scripts_directory() {
            static std::filesystem::path s_scripts_directory;
            return s_scripts_directory;
        }

        inline void console_post(const char* str, const log_level level) {
            auto&                       state = console();
            std::lock_guard<std::mutex> lock{state.mutex};
            auto&                       buffer = (level == log_level::error) ? state.buffer_err : state.buffer_out;
            buffer += str;
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                if (state.sink) {
                    state.sink(level, std::string_view{buffer}.substr(0, pos));
                }
                buffer.erase(0, pos + 1);
            }
        }

        inline PyObject* console_write(PyObject*, PyObject* args) {
            const char* str{};
            if (!PyArg_ParseTuple(args, "s", &str)) {
                return nullptr;
            }
            console_post(str, log_level::info);
            Py_RETURN_NONE;
        }

        inline PyObject* console_write_err(PyObject*, PyObject* args) {
            const char* str{};
            if (!PyArg_ParseTuple(args, "s", &str)) {
                return nullptr;
            }
            console_post(str, log_level::error);
            Py_RETURN_NONE;
        }

        inline PyMethodDef s_console_methods[] = {
            {"write", console_write, METH_VARARGS, "Write to the host console."},
            {"write_err", console_write_err, METH_VARARGS, "Write to the host console as an error."},
            {nullptr, nullptr, 0, nullptr}};

        inline PyModuleDef s_console_moduledef = {
            PyModuleDef_HEAD_INIT, "_maxconsole", nullptr, -1, s_console_methods, nullptr, nullptr, nullptr, nullptr};

        inline PyObject* console_module_init() {
            return PyModule_Create(&s_console_moduledef);
        }

        inline PyObject* path_to_unicode(const std::filesystem::path& path) {
#ifdef _WIN32
            return PyUnicode_FromWideChar(path.wstring().c_str(), -1);
#else
            return PyUnicode_DecodeFSDefault(path.string().c_str());
#endif
        }

        /// typing.get_type_hints(obj) — a NEW reference to the hints dict, or nullptr with a
        /// Python error set (the caller clears it). Caller holds the GIL.
        inline PyObject* get_type_hints(PyObject* obj) {
            PyObject* typing = PyImport_ImportModule("typing");
            if (!typing) {
                return nullptr;
            }
            PyObject* fn = PyObject_GetAttrString(typing, "get_type_hints");
            Py_DECREF(typing);
            if (!fn) {
                return nullptr;
            }
            PyObject* result = PyObject_CallFunctionObjArgs(fn, obj, nullptr);
            Py_DECREF(fn);
            return result;
        }

        /// `__name__` of a type-hint object, or `fallback` when it has none (e.g. a typing generic).
        /// Caller holds the GIL; never leaves an error set.
        inline std::string hint_name(PyObject* hint, std::string fallback) {
            PyObject* name = PyObject_GetAttrString(hint, "__name__");
            if (!name) {
                PyErr_Clear();
                return fallback;
            }
            if (const char* s = PyUnicode_AsUTF8(name)) {
                fallback = s;
            }
            else {
                PyErr_Clear();
            }
            Py_DECREF(name);
            return fallback;
        }

    } // namespace detail

    /// Replace the console sink (e.g. a test capturing Python's output). Thread-safe.
    inline void set_console(log_function sink) {
        auto&                       state = detail::console();
        std::lock_guard<std::mutex> lock{state.mutex};
        state.sink = std::move(sink);
    }

    /// The user's script folder passed to initialize() (empty before initialization).
    inline const std::filesystem::path& scripts_directory() {
        return detail::scripts_directory();
    }

    /// Initialize the embedded interpreter exactly once per process. Every call returns the outcome
    /// of that one attempt; options passed after the first call are ignored. The GIL is released
    /// before returning; all later API calls take gil_lock.
    ///
    /// The interpreter is never finalized — it lives until the process exits (finalizing and
    /// re-initializing is unsafe for extension modules such as numpy). All clients share it.
    inline runtime_status initialize(const runtime_options& options) {
        static std::once_flag s_once;
        static runtime_status s_status;

        std::call_once(s_once, [&] {
            set_console(options.console);
            detail::scripts_directory() = options.scripts_dir;
            PyImport_AppendInittab("_maxconsole", detail::console_module_init);

            PyConfig config;
            PyConfig_InitIsolatedConfig(&config); // ignore environment variables and user site-packages
            config.install_signal_handlers = 0;   // we are a plugin: never steal signal handling from the host
            config.parse_argv              = 0;

#ifdef _WIN32
            PyStatus status = PyConfig_SetString(&config, &config.home, options.home.wstring().c_str());
#else
            PyStatus status = PyConfig_SetBytesString(&config, &config.home, options.home.string().c_str());
#endif
            if (!PyStatus_Exception(status)) {
                status = Py_InitializeFromConfig(&config);
            }
            PyConfig_Clear(&config);

            if (PyStatus_Exception(status)) {
                s_status.error = status.err_msg ? status.err_msg : "unknown error";
                return;
            }

            // Make the user's script folder importable.
            {
                PyObject* sys_path = PySys_GetObject("path"); // borrowed
                PyObject* dir      = detail::path_to_unicode(options.scripts_dir);
                if (sys_path && dir) {
                    PyList_Insert(sys_path, 0, dir); // does not steal the reference
                }
                Py_XDECREF(dir);
            }

            // Route print() and tracebacks to the host console.
            PyRun_SimpleString("import sys, _maxconsole\n"
                               "class _MaxConsoleStream:\n"
                               "    def __init__(self, write):\n"
                               "        self._write = write\n"
                               "    def write(self, s):\n"
                               "        self._write(s)\n"
                               "        return len(s)\n"
                               "    def flush(self):\n"
                               "        pass\n"
                               "sys.stdout = _MaxConsoleStream(_maxconsole.write)\n"
                               "sys.stderr = _MaxConsoleStream(_maxconsole.write_err)\n");

            PyEval_SaveThread(); // release the GIL; every entry point re-acquires via gil_lock
            s_status.ok = true;
        });

        return s_status;
    }

} // namespace tap::python

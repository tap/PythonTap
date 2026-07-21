/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.
///
/// Embedded-interpreter plumbing for tap.python~:
///  - locating the Max package that contains this external (so all paths are
///    relative to the package, never hardcoded)
///  - one-time, process-wide CPython initialization from the runtime installed
///    in <package>/support by scripts/install-runtime.*
///  - GIL discipline (every entry point takes a scoped gil_lock)
///  - redirection of Python's stdout/stderr to the Max console

#pragma once

#define PY_SSIZE_T_CLEAN
#include <filesystem>
#include <mutex>
#include <string>

#include <Python.h>

#ifdef WIN_VERSION
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "c74_min_api.h"

namespace tap::python {

    /// Full path of this external's binary on disk, found from the address of one of
    /// our own functions. This works no matter where the user installed the package.
    inline std::filesystem::path external_binary_path() {
#ifdef WIN_VERSION
        HMODULE module{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&external_binary_path), &module);
        wchar_t buffer[4096]{};
        GetModuleFileNameW(module, buffer, 4096);
        return std::filesystem::path{buffer};
#else
        Dl_info info{};
        dladdr(reinterpret_cast<void*>(&external_binary_path), &info);
        return std::filesystem::path{info.dli_fname};
#endif
    }

    /// Root of the Max package containing this external.
    /// mac: <package>/externals/tap.python~.mxo/Contents/MacOS/tap.python~
    /// win: <package>/externals/tap.python~.mxe64
    inline std::filesystem::path package_root() {
        auto p = external_binary_path();
#ifdef MAC_VERSION
        return p.parent_path().parent_path().parent_path().parent_path().parent_path();
#else
        return p.parent_path().parent_path();
#endif
    }

    /// RAII guard for the GIL. Every call into the CPython API after initialization
    /// must hold one of these — messages and attribute accessors arrive on the Max
    /// main/scheduler threads while the perform routine runs on the audio thread.
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

    // _maxconsole: a tiny built-in module that Python's sys.stdout/sys.stderr are
    // rebound to, so that print() and tracebacks land in the Max console instead of
    // disappearing (the old code redirected them to a log file at a hardcoded path).
    // Output is buffered per-stream and flushed to the console a line at a time.

    inline std::mutex  s_console_mutex;
    inline std::string s_console_buffer_out;
    inline std::string s_console_buffer_err;

    inline void console_post(std::string& buffer, const char* str, const bool is_error) {
        std::lock_guard<std::mutex> lock{s_console_mutex};
        buffer += str;
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            const auto line = buffer.substr(0, pos);
            if (is_error) {
                c74::max::object_error(nullptr, "python: %s", line.c_str());
            }
            else {
                c74::max::object_post(nullptr, "python: %s", line.c_str());
            }
            buffer.erase(0, pos + 1);
        }
    }

    inline PyObject* console_write(PyObject*, PyObject* args) {
        const char* str{};
        if (!PyArg_ParseTuple(args, "s", &str)) {
            return nullptr;
        }
        console_post(s_console_buffer_out, str, false);
        Py_RETURN_NONE;
    }

    inline PyObject* console_write_err(PyObject*, PyObject* args) {
        const char* str{};
        if (!PyArg_ParseTuple(args, "s", &str)) {
            return nullptr;
        }
        console_post(s_console_buffer_err, str, true);
        Py_RETURN_NONE;
    }

    inline PyMethodDef s_console_methods[] = {
        {"write", console_write, METH_VARARGS, "Write to the Max console."},
        {"write_err", console_write_err, METH_VARARGS, "Write to the Max console as an error."},
        {nullptr, nullptr, 0, nullptr}};

    inline PyModuleDef s_console_moduledef = {
        PyModuleDef_HEAD_INIT, "_maxconsole", nullptr, -1, s_console_methods, nullptr, nullptr, nullptr, nullptr};

    inline PyObject* console_module_init() {
        return PyModule_Create(&s_console_moduledef);
    }

    /// Initialize the embedded interpreter exactly once per Max session, from the
    /// runtime in <package>/support. Returns true if the interpreter is available.
    /// The GIL is released after initialization; all later API calls take gil_lock.
    ///
    /// Note: the interpreter is never finalized — it lives until Max quits. All
    /// tap.python~ instances share it (CPython supports only one interpreter here).
    inline bool initialize(const std::filesystem::path& home, const std::filesystem::path& scripts_dir) {
        static std::once_flag s_once;
        static bool           s_ok = false;

        std::call_once(s_once, [&] {
            PyImport_AppendInittab("_maxconsole", console_module_init);

            PyConfig config;
            PyConfig_InitIsolatedConfig(&config); // ignore environment variables and user site-packages
            config.install_signal_handlers = 0;   // we are a plugin: never steal signal handling from Max
            config.parse_argv              = 0;

#ifdef WIN_VERSION
            PyStatus status = PyConfig_SetString(&config, &config.home, home.wstring().c_str());
#else
        PyStatus status = PyConfig_SetBytesString(&config, &config.home, home.string().c_str());
#endif
            if (!PyStatus_Exception(status))
                status = Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);

            if (PyStatus_Exception(status)) {
                c74::max::object_error(
                    nullptr,
                    "tap.python~: failed to start Python from '%s': %s (run scripts/install-runtime to install the runtime)",
                    home.string().c_str(), status.err_msg ? status.err_msg : "unknown error");
                return;
            }

            // Make <package>/python (the user's script folder) importable.
            {
                PyObject* sys_path = PySys_GetObject("path"); // borrowed
#ifdef WIN_VERSION
                PyObject* dir = PyUnicode_FromWideChar(scripts_dir.wstring().c_str(), -1);
#else
            PyObject* dir = PyUnicode_DecodeFSDefault(scripts_dir.string().c_str());
#endif
                if (sys_path && dir)
                    PyList_Insert(sys_path, 0, dir); // does not steal the reference
                Py_XDECREF(dir);
            }

            // Route print() and tracebacks to the Max console.
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
            s_ok = true;
        });

        return s_ok;
    }

    /// typing.get_type_hints(obj) — returns a NEW reference to the hints dict, or
    /// nullptr with a Python error set (caller clears it).
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

} // namespace tap::python

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
#include <thread>
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

    namespace detail {

        // No CPython *data* symbols (Py_None, Py_True, PyExc_*, Py*_Type, and the macros that expand
        // to them, such as Py_RETURN_NONE, PyFloat_Check or PyMethod_Check) anywhere in the core: the
        // Windows external delay-loads python3xx.dll so that it can load, and say what is missing,
        // without a runtime, and MSVC cannot delay-load a DLL whose data a module imports (LNK1194).
        // Reach them through function calls instead, as below. CI checks the external's imports.

        /// None, as a borrowed reference.
        inline PyObject* none() {
            return Py_GetConstantBorrowed(Py_CONSTANT_NONE);
        }

        /// Set a RuntimeError with `message`, looking the type up in builtins.
        inline void set_runtime_error(const std::string& message) {
            PyObject* builtins = PyImport_ImportModule("builtins");
            PyObject* type     = builtins ? PyObject_GetAttrString(builtins, "RuntimeError") : nullptr;
            Py_XDECREF(builtins);
            if (type) {
                PyErr_SetString(type, message.c_str());
                Py_DECREF(type);
            }
        }

        /// The thread that called initialize(); its thread state belongs to the interpreter.
        inline std::thread::id& init_thread() {
            static std::thread::id s_init_thread;
            return s_init_thread;
        }

        /// Keeps one Python thread state alive for the life of the calling thread, by holding a
        /// PyGILState_Ensure() open without the GIL (plan 1.4). Without it, every gil_lock on a
        /// thread Python did not create (Max's audio and scheduler threads) would create and destroy
        /// a thread state — an allocation and an interpreter-wide lock per audio vector — and wipe
        /// that thread's Python state (threading.local, contexts) every time.
        class thread_state_keeper {
          public:
            thread_state_keeper()
                : m_outer{PyGILState_Ensure()}
                , m_thread_state{PyEval_SaveThread()} {}

            ~thread_state_keeper() {
                // At thread exit, give the thread state back. Never on the interpreter's own thread
                // (its state outlives us), nor once the interpreter is going away.
                if (std::this_thread::get_id() == init_thread() || !Py_IsInitialized() || Py_IsFinalizing()) {
                    return;
                }
                PyEval_RestoreThread(m_thread_state);
                PyGILState_Release(m_outer);
            }

            thread_state_keeper(const thread_state_keeper&)            = delete;
            thread_state_keeper& operator=(const thread_state_keeper&) = delete;

          private:
            PyGILState_STATE m_outer;
            PyThreadState*   m_thread_state;
        };

    } // namespace detail

    /// RAII guard for the GIL. Every call into the CPython API after initialize() must hold one of
    /// these — hosts call in from several threads (e.g. Max's main, scheduler and audio threads).
    /// The first gil_lock on a thread gives it a Python thread state that lives as long as the thread.
    class gil_lock {
      public:
        gil_lock()
            : m_state{acquire()} {}

        ~gil_lock() { PyGILState_Release(m_state); }

        gil_lock(const gil_lock&)            = delete;
        gil_lock& operator=(const gil_lock&) = delete;

      private:
        PyGILState_STATE m_state;

        static PyGILState_STATE acquire() {
            static thread_local detail::thread_state_keeper s_keeper;
            return PyGILState_Ensure();
        }
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

        /// The globals of the support module created once by initialize() (strong; lives as long as
        /// the interpreter): the script loader and the introspection helpers below.
        inline PyObject*& support_globals() {
            static PyObject* s_globals = nullptr;
            return s_globals;
        }

        /// A support-module function by name (borrowed), or nullptr. Caller holds the GIL.
        inline PyObject* support(const char* name) {
            return support_globals() ? PyDict_GetItemString(support_globals(), name) : nullptr;
        }

        // The support module, in Python because that is where the introspection is simplest.
        //
        // load(module_name, path) -> (module, executed). Compiles the file's source itself — never a
        // cached .pyc, whose staleness check (mtime at 1 s resolution + size) can miss a quick second
        // save — and executes it as a fresh module registered in sys.modules under module_name (typing
        // and attrs resolve annotations through sys.modules). Modules are cached by source: loading an
        // unchanged file returns the module already executed, so every instance of one file shares a
        // single execution per save. On failure the previous module stays registered.
        //
        // hint_kind(hint) -> str. The name the host maps a type hint by: 'int', 'float', 'bool', 'str',
        // 'ndarray', 'tuple', ... ('any' for no hint, 'ClassVar' for a class variable). Optional[X] and
        // X | None are X; an unresolved hint written as a string is read by name.
        //
        // class_hints(cls) -> ([(name, kind)], error). The class's annotated fields, base classes
        // first. If typing.get_type_hints fails (e.g. a name imported only under TYPE_CHECKING), falls
        // back to the annotations as written and returns the exception, for the host to report.
        //
        // methods(instance) -> [(name, callable)]. The public methods — functions, classmethods,
        // staticmethods — found without running descriptors, so a property getter never runs just
        // because the class was loaded. Each callable is bound: call it with the arguments only.
        //
        // describe(callable) -> ([(name, kind, parameter_kind, has_default)], return_kind, error).
        // parameter_kind is inspect.Parameter.kind as an int.
        inline constexpr const char* k_support_source = R"(
import inspect, re, sys, types, typing

_cache = {}

def load(module_name, path):
    with open(path, 'rb') as f:
        source = f.read()
    cached = _cache.get(module_name)
    if cached is not None and cached[0] == source:
        return cached[1], False
    code = compile(source, path, 'exec', dont_inherit=True)
    module = types.ModuleType(module_name)
    module.__file__ = path
    previous = sys.modules.get(module_name)
    sys.modules[module_name] = module
    try:
        exec(code, module.__dict__)
    except BaseException:
        if previous is not None:
            sys.modules[module_name] = previous
        else:
            sys.modules.pop(module_name, None)
        raise
    _cache[module_name] = (source, module)
    return module, True

_OPTIONAL = re.compile(r'(?:typing\.)?Optional\[(.*)\]')

def hint_kind(hint):
    if hint is inspect.Parameter.empty:
        return 'any'
    if isinstance(hint, str):
        text = hint.strip()
        match = _OPTIONAL.fullmatch(text)
        if match:
            return hint_kind(match.group(1))
        parts = [p.strip() for p in text.split('|') if p.strip() != 'None']
        if len(parts) == 1 and parts[0] != text:
            return hint_kind(parts[0])
        return text.split('[')[0].split('.')[-1]
    origin = typing.get_origin(hint)
    if origin is typing.ClassVar:
        return 'ClassVar'
    if origin is typing.Union or origin is types.UnionType:
        args = [a for a in typing.get_args(hint) if a is not type(None)]
        return hint_kind(args[0]) if len(args) == 1 else 'str'
    if origin is not None:
        return getattr(origin, '__name__', 'str')
    return getattr(hint, '__name__', 'str')

def class_hints(cls):
    error = None
    try:
        hints = typing.get_type_hints(cls)
    except Exception as e:
        error = e
        hints = {}
        for base in reversed(cls.__mro__):
            if base is not object:
                try:
                    hints.update(inspect.get_annotations(base))
                except Exception:
                    pass
    return [(name, hint_kind(hint)) for name, hint in hints.items()], error

def methods(instance):
    found = []
    for name in dir(instance):
        if name.startswith('_'):
            continue
        try:
            raw = inspect.getattr_static(instance, name)
        except AttributeError:
            continue
        if isinstance(raw, (types.FunctionType, classmethod, staticmethod)):
            found.append((name, getattr(instance, name)))
    return found

def describe(fn):
    error = None
    try:
        hints = typing.get_type_hints(fn)
    except Exception as e:
        error = e
        try:
            hints = inspect.get_annotations(getattr(fn, '__func__', fn))
        except Exception:
            hints = {}
    parameters = [
        (p.name, hint_kind(hints.get(p.name, inspect.Parameter.empty)), int(p.kind),
         p.default is not inspect.Parameter.empty)
        for p in inspect.signature(fn).parameters.values()
    ]
    return parameters, hint_kind(hints.get('return', inspect.Parameter.empty)), error
)";

        /// Create the support module (initialize() calls this with the GIL held).
        inline void create_support() {
            PyObject* globals = PyDict_New();
            if (!globals) {
                PyErr_Clear();
                return;
            }
            PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins()); // does not steal
            if (PyObject* name = PyUnicode_FromString("_tap_python_support")) {
                PyDict_SetItemString(globals, "__name__", name);
                Py_DECREF(name);
            }
            PyObject* result = PyRun_String(k_support_source, Py_file_input, globals, globals);
            if (result) {
                Py_DECREF(result);
                support_globals() = globals; // keeps the reference
                return;
            }
            PyErr_Print(); // start-up only, never user code
            Py_DECREF(globals);
        }

        inline void console_post(const std::string_view text, const log_level level) {
            auto&                       state = console();
            std::lock_guard<std::mutex> lock{state.mutex};
            auto&                       buffer = (level == log_level::error) ? state.buffer_err : state.buffer_out;
            buffer.append(text);
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                if (state.sink) {
                    state.sink(level, std::string_view{buffer}.substr(0, pos));
                }
                buffer.erase(0, pos + 1);
            }
        }

        /// Forward a pending partial line (text without its newline yet) as a line of its own.
        inline void console_flush(const log_level level) {
            auto&                       state = console();
            std::lock_guard<std::mutex> lock{state.mutex};
            auto&                       buffer = (level == log_level::error) ? state.buffer_err : state.buffer_out;
            if (!buffer.empty()) {
                if (state.sink) {
                    state.sink(level, buffer);
                }
                buffer.clear();
            }
        }

        inline log_level console_level(PyObject* args, const char*& str, Py_ssize_t& length, bool& ok) {
            int error = 0;
            ok        = PyArg_ParseTuple(args, "s#p", &str, &length, &error) != 0; // s#: embedded NULs allowed
            return error ? log_level::error : log_level::info;
        }

        inline PyObject* console_write(PyObject*, PyObject* args) {
            const char* str{};
            Py_ssize_t  length{};
            bool        ok{};
            const auto  level = console_level(args, str, length, ok);
            if (!ok) {
                return nullptr;
            }
            console_post(std::string_view{str, static_cast<std::size_t>(length)}, level);
            return Py_NewRef(none());
        }

        inline PyObject* console_flush_method(PyObject*, PyObject* args) {
            int error = 0;
            if (!PyArg_ParseTuple(args, "p", &error)) {
                return nullptr;
            }
            console_flush(error ? log_level::error : log_level::info);
            return Py_NewRef(none());
        }

        inline PyMethodDef s_console_methods[] = {
            {"write", console_write, METH_VARARGS, "write(text, is_error): write to the host console."},
            {"flush", console_flush_method, METH_VARARGS, "flush(is_error): forward a pending partial line."},
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

    } // namespace detail

    /// Print the pending Python exception to sys.stderr (the host console) and clear it. Caller
    /// holds the GIL; does nothing when no exception is set.
    ///
    /// Use this, never PyErr_Print(): for a SystemExit, PyErr_Print() calls Py_Exit() and ends the
    /// host process — so user code calling sys.exit() would quit Max. Here a SystemExit is reported
    /// like any other exception. It also leaves sys.last_exc unset, so a failed call does not keep
    /// its frames (and the objects they reference) alive.
    inline void report_exception() {
        PyObject* exception = PyErr_GetRaisedException();
        if (!exception) {
            return;
        }
        PyErr_DisplayException(exception);
        Py_DECREF(exception);
    }

    /// Replace the console sink (e.g. a test capturing Python's output). Thread-safe.
    inline void set_console(log_function sink) {
        auto&                       state = detail::console();
        std::lock_guard<std::mutex> lock{state.mutex};
        state.sink = std::move(sink);
    }

    /// Load `<scripts_dir>/<name>.py` by path as the module `_tap_python_<name>` (see
    /// detail::k_support_source). Returns a new reference to the module, or nullptr with a Python
    /// error set; `executed` tells whether the source was (re)executed or the cached module reused.
    /// Caller holds the GIL; `name` must be a Python identifier (checked by the caller).
    inline PyObject* load_script(const std::string& name, bool& executed) {
        executed       = false;
        PyObject* load = detail::support("load");
        if (!load) {
            detail::set_runtime_error("the tap.python script loader failed to start");
            return nullptr;
        }
        const auto path   = detail::scripts_directory() / (name + ".py");
        PyObject*  pyname = PyUnicode_FromString(("_tap_python_" + name).c_str());
        PyObject*  pypath = detail::path_to_unicode(path);
        PyObject*  result = (pyname && pypath) ? PyObject_CallFunctionObjArgs(load, pyname, pypath, nullptr) : nullptr;
        Py_XDECREF(pyname);
        Py_XDECREF(pypath);
        if (!result) {
            return nullptr;
        }
        PyObject* module = nullptr;
        if (PyTuple_Check(result) && PyTuple_Size(result) == 2) {
            module = PyTuple_GetItem(result, 0); // borrowed
            Py_INCREF(module);
            executed = PyObject_IsTrue(PyTuple_GetItem(result, 1)) == 1;
        }
        Py_DECREF(result);
        return module;
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
            detail::init_thread()       = std::this_thread::get_id();
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

            // Make the user's script folder importable — at the END of sys.path, so a user's helper
            // module can be imported but a user file named like a standard-library module (random.py,
            // json.py) cannot shadow it. The class files themselves are loaded by path (load_script).
            {
                PyObject* sys_path = PySys_GetObject("path"); // borrowed
                PyObject* dir      = detail::path_to_unicode(options.scripts_dir);
                if (sys_path && dir) {
                    PyList_Append(sys_path, dir); // does not steal the reference
                }
                Py_XDECREF(dir);
            }

            detail::create_support();

            // Route print() and tracebacks to the host console.
            // sys.stdout/sys.stderr are text streams (io.TextIOBase) so that libraries which probe them
            // — encoding, errors, isatty(), fileno() — find what they expect; flush() forwards a
            // pending partial line (e.g. print(..., end='', flush=True)).
            PyRun_SimpleString("import io, sys, _maxconsole\n"
                               "class _MaxConsoleStream(io.TextIOBase):\n"
                               "    encoding = 'utf-8'\n"
                               "    errors = 'backslashreplace'\n"
                               "    def __init__(self, is_error):\n"
                               "        self._is_error = is_error\n"
                               "    def writable(self):\n"
                               "        return True\n"
                               "    def isatty(self):\n"
                               "        return False\n"
                               "    def write(self, s):\n"
                               "        if not isinstance(s, str):\n"
                               "            raise TypeError(f'write() argument must be str, not {type(s).__name__}')\n"
                               "        _maxconsole.write(s, self._is_error)\n"
                               "        return len(s)\n"
                               "    def flush(self):\n"
                               "        _maxconsole.flush(self._is_error)\n"
                               "sys.stdout = _MaxConsoleStream(False)\n"
                               "sys.stderr = _MaxConsoleStream(True)\n");

            PyEval_SaveThread(); // release the GIL; every entry point re-acquires via gil_lock
            s_status.ok = true;
        });

        return s_status;
    }

} // namespace tap::python

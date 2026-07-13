/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#define PY_SSIZE_T_CLEAN
#include <Python.h> // CPython insists on being included before system headers

// c74_min.h must be the FIRST min header in the translation unit: it defines
// C74_MIN_WITH_IMPLEMENTATION so the min wrapper's out-of-line statics get
// emitted here. If c74_min_api.h sneaks in first (e.g. via our helper headers),
// the include guards swallow those definitions and the external fails to link.
#include <cstring>
#include <memory>
#include <unordered_map>

#include "c74_min.h"
#include "tap.python_tilde_attribute.h"
#include "tap.python_tilde_cglue.h"
#include "tap.python_tilde_message.h"
#include "tap.python_tilde_runtime.h"

using namespace c74::min;
namespace runtime = tap::python_runtime;

class python : public object<python>, public vector_operator<> {
  public:
    MIN_DESCRIPTION{
        "Process audio with a Python class. Attributes and messages are generated from the class's type-annotated members, and the class's process() method runs on the audio signal. The source file is watched and hot-reloaded on save."};
    MIN_TAGS{"programming"};
    MIN_AUTHOR{"Tim Place"};
    MIN_RELATED{"js, node.script"};

    inlet<>  m_inlet{this, "(signal) input passed to the Python process() method"};
    outlet<> m_outlet_main{this, "(signal) output returned from the Python process() method", "signal"};

    argument<symbol> m_source_arg{
        this, "source",
        "Python source file in the package's python folder, without the .py extension. It must define a class of the same name."};

    python(const atoms& args = {}) {
        if (maxobj() == NULL) {
            return; // this occurs during dummy construction
        }

        const auto package = runtime::package_root();
        m_scripts_dir      = package / "python";
        const auto home    = package / "support";

        if (!std::filesystem::exists(home)) {
            cerr << "No Python runtime found at " << home.string()
                 << " — run scripts/install-runtime from the package root to install it." << endl;
            return;
        }
        if (!runtime::initialize(home, m_scripts_dir)) {
            return;
        }

        if (args.empty()) {
            m_python_source = "default";
        }
        else {
            m_python_source = to_string(args);
        }

        update_source();

        // watch the source file for changes (delivered as our 'filechanged' message)
        const auto watched_file = m_scripts_dir / (m_python_source + ".py");
        char       filename[c74::max::MAX_PATH_CHARS]{};
        std::strncpy(filename, watched_file.string().c_str(), c74::max::MAX_PATH_CHARS - 1);
        short              path_id{};
        c74::max::t_fourcc filetype{};
        if (c74::max::locatefile_extended(filename, &path_id, &filetype, nullptr, 0) == 0) {
            m_filewatcher = c74::max::filewatcher_new(maxobj(), path_id, filename);
            if (m_filewatcher) {
                c74::max::filewatcher_start(m_filewatcher);
            }
        }
        else {
            cerr << "Unable to watch " << watched_file.string() << " for changes." << endl;
        }
    }

    ~python() {
        if (m_filewatcher) {
            c74::max::object_free(m_filewatcher);
        }

        if (!Py_IsInitialized()) {
            return; // dummy construction, or the runtime never came up
        }

        runtime::gil_lock lock;
        m_python_messages.clear(); // releases strong refs to bound functions
        m_python_attributes.clear();
        Py_CLEAR(m_process_fn);
        Py_CLEAR(m_process_args);
        Py_CLEAR(m_instance);
        Py_CLEAR(m_module);
    }

    /// (Re)import the user's module, instantiate its class, and rebuild the Max
    /// attributes and messages from the class's type hints. Called from the Max
    /// main thread (constructor and file watcher). Holding the GIL for the whole
    /// rebuild also serializes us against the perform routine.
    void update_source() {
        if (!Py_IsInitialized()) {
            return;
        }

        runtime::gil_lock lock;

        // detach the audio binding first: the perform routine treats a null
        // m_process_fn as "output silence"
        Py_CLEAR(m_process_fn);
        Py_CLEAR(m_process_args);
        Py_CLEAR(m_instance);

        if (m_module) {
            PyObject* reloaded = PyImport_ReloadModule(m_module);
            if (!reloaded) {
                PyErr_Print();
                cerr << "Failed to reload module " << m_python_source << endl;
                return;
            }
            Py_DECREF(m_module);
            m_module = reloaded;
        }
        else {
            PyObject* name = PyUnicode_DecodeFSDefault(m_python_source.c_str());
            m_module       = name ? PyImport_Import(name) : nullptr;
            Py_XDECREF(name);
            if (!m_module) {
                PyErr_Print();
                cerr << "Failed to load module '" << m_python_source << "' (searched " << m_scripts_dir.string() << ")"
                     << endl;
                return;
            }
        }

        PyObject* module_dict = PyModule_GetDict(m_module);                                 // borrowed
        PyObject* py_class    = PyDict_GetItemString(module_dict, m_python_source.c_str()); // borrowed
        if (!py_class) {
            cerr << "No class named '" << m_python_source << "' in " << m_python_source << ".py" << endl;
            return;
        }
        if (!PyCallable_Check(py_class)) {
            cerr << "Cannot instantiate the Python class " << m_python_source << endl;
            return;
        }

        m_instance = PyObject_CallObject(py_class, nullptr);
        if (!m_instance) {
            PyErr_Print();
            cerr << "Failed to instantiate the Python class " << m_python_source << endl;
            return;
        }

        create_attributes(py_class);
        create_messages();
    }

    // Max's filewatcher (created in the constructor) sends this message any time
    // our python source file is modified. Declared after update_source() so its
    // initializer lambda can name it: clang (unlike AppleClang/MSVC) does not treat
    // a lambda inside a default member initializer as complete-class context.
    message<> m_filechanged{this, "filechanged",
                            MIN_FUNCTION {
                                cout << "Source file update detected. Reloading." << endl;
                                update_source();
                                return {};
                            }};

    /// Dispatch a Max message to the bound Python method.
    /// Runs on the Max main or scheduler thread.
    void message_gimme(const symbol name, const long ac, const c74::max::t_atom* av) {
        if (!Py_IsInitialized() || !m_instance) {
            return;
        }

        runtime::gil_lock lock;

        auto found = m_python_messages.find(name.c_str());
        if (found == m_python_messages.end()) {
            return;
        }
        auto& mess = found->second;

        const auto& arg_types = mess->arg_types();
        if (static_cast<size_t>(ac) != arg_types.size()) {
            cerr << name << ": expected " << arg_types.size() << " argument(s), got " << ac << endl;
            return;
        }

        PyObject* py_args = PyTuple_New(ac + 1);
        if (!py_args) {
            return;
        }
        Py_INCREF(m_instance); // the tuple *steals* a reference
        PyTuple_SetItem(py_args, 0, m_instance);

        for (auto i = 0; i < ac; ++i) {
            PyObject* value;
            if (arg_types[i] == "int") {
                value = PyLong_FromLong(c74::max::atom_getlong(av + i));
            }
            else if (arg_types[i] == "float") {
                value = PyFloat_FromDouble(c74::max::atom_getfloat(av + i));
            }
            else {
                value = PyUnicode_DecodeFSDefault(c74::max::atom_getsym(av + i)->s_name);
            }
            PyTuple_SetItem(py_args, i + 1, value); // steals the reference
        }

        PyObject* result = PyObject_Call(mess->function(), py_args, nullptr);
        if (result) {
            Py_DECREF(result);
        }
        else {
            PyErr_Print();
        }

        Py_DECREF(py_args);
    }

    void attr_set(const symbol& name, const long argc, const c74::max::t_atom* argv) {
        if (!Py_IsInitialized() || !m_instance) {
            return;
        }
        if (argc != 1) {
            cerr << "Attributes with more than 1 arg not supported" << endl;
            return;
        }

        runtime::gil_lock lock;

        auto found = m_python_attributes.find(name.c_str());
        if (found == m_python_attributes.end()) {
            return;
        }
        auto& attr = found->second;

        PyObject* value;
        if (attr->type() == k_sym_float64) {
            value = PyFloat_FromDouble(c74::max::atom_getfloat(argv));
        }
        else if (attr->type() == k_sym_long) {
            value = PyLong_FromLong(c74::max::atom_getlong(argv));
        }
        else {
            value = PyUnicode_DecodeFSDefault(c74::max::atom_getsym(argv)->s_name);
        }

        if (value) {
            if (PyObject_SetAttrString(m_instance, name.c_str(), value) < 0) {
                PyErr_Print(); // e.g. an attrs validator rejected the value
            }
            Py_DECREF(value);
        }
    }

    void attr_get(const symbol& name, long* argc, c74::max::t_atom** argv) {
        if ((*argc) != 1 || !(*argv)) { // otherwise use memory passed in
            if (*argc && *argv) {
                c74::max::sysmem_freeptr(*argv);
                *argv = NULL;
            }
            *argc = 1;
            *argv = reinterpret_cast<c74::max::t_atom*>(c74::max::sysmem_newptr(sizeof(c74::max::t_atom) * (*argc)));
        }
        c74::max::atom_setfloat(*argv, 0.0); // default in case anything below fails

        if (!Py_IsInitialized() || !m_instance) {
            return;
        }

        runtime::gil_lock lock;

        auto found = m_python_attributes.find(name.c_str());
        if (found == m_python_attributes.end()) {
            return;
        }
        auto& attr = found->second;

        PyObject* value = PyObject_GetAttrString(m_instance, name.c_str());
        if (!value) {
            PyErr_Clear();
            return;
        }

        if (attr->type() == k_sym_float64) {
            c74::max::atom_setfloat(*argv, PyFloat_AsDouble(value));
        }
        else if (attr->type() == k_sym_long) {
            c74::max::atom_setlong(*argv, PyLong_AsLong(value));
        }
        else {
            const char* str = PyUnicode_AsUTF8(value);
            c74::max::atom_setsym(*argv, c74::max::gensym(str ? str : ""));
        }
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }

        Py_DECREF(value);
    }

    /// The audio perform routine. Calls the Python process() method once per
    /// sample — the GIL is taken once per vector, and the GIL is also what
    /// serializes us against update_source() on the main thread.
    void operator()(audio_bundle input, audio_bundle output) {
        if (!m_process_fn) {
            output.clear();
            return;
        }

        runtime::gil_lock lock;

        if (!m_process_fn) { // re-check now that we hold the GIL
            output.clear();
            return;
        }

        auto in  = input.samples(0);
        auto out = output.samples(0);

        for (auto i = 0; i < input.frame_count(); ++i) {
            PyTuple_SetItem(m_process_args, 1, PyFloat_FromDouble(in[i])); // the tuple steals the pyfloat
            PyObject* result = PyObject_CallObject(m_process_fn, m_process_args);
            if (!result) {
                PyErr_Print();
                cerr << "process() raised an exception — audio disabled until the source is fixed and reloaded" << endl;
                Py_CLEAR(m_process_fn); // update_source() re-arms it
                for (; i < output.frame_count(); ++i) {
                    out[i] = 0.0;
                }
                return;
            }

            double y = PyFloat_AsDouble(result); // also handles ints and other number types
            if (PyErr_Occurred()) {
                PyErr_Clear();
                y = 0.0;
            }
            out[i] = y;
            Py_DECREF(result);
        }
    }

  private:
    string                m_python_source{};
    std::filesystem::path m_scripts_dir{};
    void*                 m_filewatcher{};
    PyObject*             m_module{};       // strong
    PyObject*             m_instance{};     // strong
    PyObject*             m_process_fn{};   // strong
    PyObject*             m_process_args{}; // strong; slot 0 holds a ref to m_instance
    std::unordered_map<std::string, std::unique_ptr<python_message>> m_python_messages;
    std::unordered_map<std::string, std::unique_ptr<python_attr>>    m_python_attributes;

    /// Create Max attributes from the class-level type hints.
    /// Caller holds the GIL.
    void create_attributes(PyObject* py_class) {
        PyObject* hints = runtime::get_type_hints(py_class);
        if (!hints) {
            PyErr_Clear();
            return;
        }

        PyObject*  key;
        PyObject*  value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(hints, &pos, &key, &value)) { // key/value are borrowed
            const char* key_cstr = PyUnicode_AsUTF8(key);
            if (!key_cstr || key_cstr[0] == '_') {
                continue;
            }

            string    type_str  = "str";
            PyObject* type_name = PyObject_GetAttrString(value, "__name__");
            if (type_name) {
                if (const char* s = PyUnicode_AsUTF8(type_name)) {
                    type_str = s;
                }
                Py_DECREF(type_name);
            }
            else {
                PyErr_Clear(); // e.g. a typing generic without __name__; treat as str
            }

            // attributes persist across reloads; only create ones we don't have yet
            if (m_python_attributes.find(key_cstr) == m_python_attributes.end()) {
                m_python_attributes[key_cstr] = std::make_unique<python_attr>(maxobj(), key_cstr, type_str);
            }
        }
        Py_DECREF(hints);
    }

    /// Create Max messages from the instance's public methods, and bind process().
    /// Caller holds the GIL.
    void create_messages() {
        // remove the previous incarnation's messages before rebinding
        for (auto& element : m_python_messages) {
            c74::max::object_deletemethod(maxobj(), c74::max::gensym(element.first.c_str()));
        }
        m_python_messages.clear();

        PyObject* members = PyObject_Dir(m_instance);
        if (!members) {
            PyErr_Clear();
            return;
        }

        const auto member_count = PyList_Size(members);
        for (Py_ssize_t i = 0; i < member_count; ++i) {
            PyObject*   member    = PyList_GetItem(members, i); // borrowed
            const char* name_cstr = member ? PyUnicode_AsUTF8(member) : nullptr;
            if (!name_cstr || name_cstr[0] == '_') {
                continue;
            }

            const string member_name{name_cstr};
            if (m_python_attributes.find(member_name) != m_python_attributes.end()) {
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
                    PyObject* hints = runtime::get_type_hints(method);
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

    /// Bind the class's process() method as the per-sample audio callback.
    /// Caller holds the GIL.
    void bind_process(PyObject* fn, PyObject* hints) {
        int  in_count      = 0;
        bool returns_tuple = false;

        if (hints) {
            PyObject*  key;
            PyObject*  value;
            Py_ssize_t pos = 0;
            while (PyDict_Next(hints, &pos, &key, &value)) { // borrowed
                const char* key_cstr = PyUnicode_AsUTF8(key);
                if (!key_cstr) {
                    continue;
                }
                if (string("return") == key_cstr) {
                    PyObject* type_name = PyObject_GetAttrString(value, "__name__");
                    if (type_name) {
                        const char* s = PyUnicode_AsUTF8(type_name);
                        returns_tuple = (s && string("tuple") == s);
                        Py_DECREF(type_name);
                    }
                    else {
                        PyErr_Clear();
                    }
                }
                else {
                    ++in_count;
                }
            }
        }

        if (in_count > 1) {
            cerr << "process() declares " << in_count
                 << " inputs but only the first is supported (single-channel object)" << endl;
        }
        if (returns_tuple) {
            cerr << "process() returns a tuple — multichannel output is not supported yet; use a single float return"
                 << endl;
            return;
        }

        m_process_args = PyTuple_New(2);
        if (!m_process_args) {
            return;
        }
        Py_INCREF(m_instance); // the tuple *steals* a reference
        PyTuple_SetItem(m_process_args, 0, m_instance);
        PyTuple_SetItem(m_process_args, 1, PyFloat_FromDouble(0.0)); // placeholder; replaced every sample

        Py_INCREF(fn);
        m_process_fn = fn;

        cout << "Audio process() bound: 1 input, 1 output" << endl;
    }

    /// Bind a public method as a Max message.
    /// Caller holds the GIL.
    void bind_message(const string& name, PyObject* fn, PyObject* hints) {
        strings argument_types;

        if (hints) {
            PyObject*  key;
            PyObject*  value;
            Py_ssize_t pos = 0;
            while (PyDict_Next(hints, &pos, &key, &value)) { // borrowed
                const char* key_cstr = PyUnicode_AsUTF8(key);
                if (!key_cstr || string("return") == key_cstr) {
                    continue;
                }

                string    type_str  = "str";
                PyObject* type_name = PyObject_GetAttrString(value, "__name__");
                if (type_name) {
                    if (const char* s = PyUnicode_AsUTF8(type_name)) {
                        type_str = s;
                    }
                    Py_DECREF(type_name);
                }
                else {
                    PyErr_Clear();
                }
                argument_types.push_back(type_str);
            }
        }

        m_python_messages[name] = std::make_unique<python_message>(maxobj(), name, fn, argument_types);
    }
};

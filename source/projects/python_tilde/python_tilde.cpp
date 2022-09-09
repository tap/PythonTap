/// @file
///    @copyright    Copyright 2022 Timothy Place. All rights reserved.
///    @license            Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"

#define PY_SSIZE_T_CLEAN
#include <cstdlib>
#include <Python.h>

using namespace c74::min;



c74::max::t_max_err python_attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc, const c74::max::t_atom* argv);
c74::max::t_max_err python_attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc, c74::max::t_atom** argv);


class python : public object<python>, public vector_operator<> {

    class python_attr {
    public:
        python_attr(python* owner, string a_name, string a_type)
        : m_owner(owner)
        , m_name(a_name)
        , m_typename(a_type)
        {
            if (m_owner->maxobj() == NULL)
                return; // this occurs during dummy construction

            symbol max_type;

            if (m_typename == "int")
                m_type = k_sym_long;
            else if (m_typename == "float")
                m_type = k_sym_float64;
            else
                m_type = k_sym_symbol;

            m_attrobj = c74::max::attribute_new(m_name.c_str(), m_type, 0, (c74::max::method)python_attr_get, (c74::max::method)python_attr_set);
            auto err = c74::max::object_addattr(m_owner->maxobj(), m_attrobj);
            if (err)
                ; // TODO: implement
            else {
                err = c74::max::object_attr_addattr_parse(m_owner->maxobj(), m_name.c_str(), "dynamicattr", k_sym_long, 0, "1");
                if (!err) {
                    //printf("setting value %s %f\n", paramname->s_name, atom_getfloat(argv));
                    //err = object_attr_setvalueof(x, paramname, 1, argv);
                    //if (err) {
                    //    object_error((t_object *)x, "failed to set attribute for %s", paramname->s_name);
                    //}
                    //if (x->m_wrapper)
                    //    object_method(x->m_wrapper, gensym("makedynamicattr"), x, paramname);
                }
                else {
                    c74::max::object_error((c74::max::t_object*)m_owner, "failed to make dynamic attribute for %s", m_name.c_str());
                }
            }
        }

        python_attr(const python_attr&) = default;

        symbol type() const {
            return m_type;
        }

    private:
        python*             m_owner;
        string              m_name;
        string              m_typename;
        symbol              m_type;
        c74::max::t_object* m_attrobj;
    };


public:
    MIN_DESCRIPTION {"Run python code."};
    MIN_TAGS        {"programming"};
    MIN_AUTHOR      {"Tim Place"};
    MIN_RELATED     {"python, mxj~"};


    inlet<>  m_inlet        { this, "(signal) post greeting to the max console" };
    outlet<> m_outlet_main  { this, "(signal) Sample value at index", "signal" };

    argument<symbol> m_source_arg { this, "source", "Python source file." };

    watcher m_file_watcher { this,
        MIN_FUNCTION {    // will trigger any time our python source file is modified
                cout << "Source file update detected. Reloading." << endl;
                update_source();
                return {};
        }
    };


    void update_source() {
        m_updating_source = true;

        if (m_module) {
            Py_DECREF(m_module);
            PyImport_ReloadModule(m_module);
        }
        else {
            auto pName = PyUnicode_DecodeFSDefault(m_python_source.c_str());
            m_module = PyImport_Import(pName);
            Py_DECREF(pName);
        }

        if (!m_module) {
            cerr << "Failed to load module " << m_python_source << endl;
            PyErr_Print();
        }

        PyObject* const pModuleDict = PyModule_GetDict(m_module); // borrowed reference
        Py_IncRef(pModuleDict);

        PyObject* pClass = PyDict_GetItemString(pModuleDict, m_python_source.c_str());
        if (pClass == nullptr) {
            PyErr_Print();
            cerr << "Fails to get the Python class named " << m_python_source << endl;
            return;
        }

        // Creates an instance of the class
        if (PyCallable_Check(pClass)) {
            m_instance = PyObject_CallObject(pClass, nullptr);
            Py_DECREF(pClass);
        }
        else {
           cerr << "Cannot instantiate the Python class " << m_python_source << endl;
           Py_DECREF(pClass);
           return;
        }

        string instantiation_str = "me = ";
        instantiation_str += m_python_source;
        instantiation_str += "()\n";
        instantiation_str += "from typing import get_type_hints\n";
        instantiation_str += "attributes = {k: v for k, v in get_type_hints(me).items() if not k.startswith('_')}\n";
        auto ret = PyRun_String(instantiation_str.c_str(), Py_file_input, pModuleDict, pModuleDict);
        if (ret == nullptr) {
            cout << "ERROR" << endl;
            PyErr_Print();
            return;
        }
        auto attribute_dict = PyRun_String("attributes", Py_eval_input, pModuleDict, pModuleDict);
        if (attribute_dict == nullptr) {
            cout << "ERROR" << endl;
            PyErr_Print();
            return;
        }

        if (attribute_dict) {
            auto attribute_dict_pstr = PyObject_Str(attribute_dict);
            string attr_dict_str = PyUnicode_AsUTF8(attribute_dict_pstr);

            auto attr_dict_keys = PyDict_Keys(attribute_dict);
            auto attr_dict_key_count = PyList_Size(attr_dict_keys);
            for (auto i=0; i<attr_dict_key_count; ++i) {
                auto key = PyList_GetItem(attr_dict_keys, i);
                auto value = PyDict_GetItem(attribute_dict, key);
                auto name = PyObject_GetAttrString(value, "__name__");

                string key_str = PyUnicode_AsUTF8(key);
                string type_str = PyUnicode_AsUTF8(name);

                auto* attr = new python_attr(this, key_str, type_str); // TODO: leaking
                m_python_attributes[key_str] = attr;
            }
        }

        auto pDir = PyObject_Dir(m_instance); // returns array of members
        auto member_count = PyList_Size(pDir);
        for (auto i=0; i<member_count; ++i) {
            auto member = PyList_GetItem(pDir, i);
            auto member_name = PyObject_Str(member);
            string member_name_str = PyUnicode_AsUTF8(member_name);

            if (member_name_str[0] == '_')
                continue;
            if (m_python_attributes.find(member_name_str) != m_python_attributes.end())
                continue;

            auto io_dict = PyRun_String("get_type_hints(me.process)", Py_eval_input, pModuleDict, pModuleDict);
            auto io_pstr = PyObject_Str(io_dict);
            auto io_str = PyUnicode_AsUTF8(io_pstr);
            cout << member_name_str << "    "<< io_str << endl;

            auto method = PyObject_GetAttrString(m_instance, member_name_str.c_str());
            if (PyMethod_Check(method)) {
                auto fn = PyMethod_Function(method);
                if (PyFunction_Check(fn)) {
                    if (member_name_str == "process") {
                        m_process_fn = fn;
                        if (!m_process_args)
                            m_process_args = PyTuple_New(2);
                        Py_IncRef(m_instance); // because the tuple will *steal* a reference
                        PyTuple_SetItem(m_process_args, 0, m_instance);
                    }
                    else
                        m_python_messages[member_name_str] = fn;
                }
            }

            Py_DECREF(pModuleDict);
        }
        m_updating_source = false;
    }


    python(const atoms& args = {}) {
        // char pythonhome[] {"PYTHONHOME=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib"};
        // char pythonpath[] {"PYTHONPATH=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib:/Users/tim/Documents/Max 8/Packages/python/misc:/Users/tim/Library/Python/3.8/lib/python/site-packages"};
        char pythonpath[] {"PYTHONPATH=$PYTHONPATH:/Users/tim/Documents/Max 8/Packages/python/misc"};
        // /Users/tim/Library/Python/3.8/lib/python/site-packages
        // /usr/local/lib/python3.10/site-packages

        // putenv(pythonhome);
        putenv(pythonpath);
        Py_Initialize();

        if (args.empty())
            m_python_source = "python_thru";
        else
            m_python_source = to_string(args);

        update_source();

        string source_fullpath {"/Users/tim/Documents/Max 8/Packages/python/misc/"};
        source_fullpath += m_python_source;
        source_fullpath += ".py";
        path p {source_fullpath};
        m_file_watcher.begin(p);
   }


    ~python() {
        Py_XDECREF(m_process_args);
        Py_XDECREF(m_module);
    }


    message<> anything { this, "anything", "Execute python function.",
        MIN_FUNCTION {
            PyObject *function = m_python_messages[args[0]];
            if (function) {
                PyObject *pArgs = PyTuple_New(2);
                PyObject *pValue = PyFloat_FromDouble(args[1]);// PyLong_FromLong(13);
                PyTuple_SetItem(pArgs, 0, m_instance);
                PyTuple_SetItem(pArgs, 1, pValue);

                auto result = PyObject_Call(function, pArgs, NULL);
                if (result) {
                    auto result_str = PyObject_Str(result);
                    Py_ssize_t size;
                    const char* data = PyUnicode_AsUTF8AndSize(result_str, &size);
                    cout << "RESULT: " << data << endl;
                }
                else
                    PyErr_Print();
            }

            return {};
        }
    };


    // the actual attribute for the message
    attribute<symbol> greeting { this, "greeting", "hello world",
        description {
            "Greeting to be posted. "
            "The greeting will be posted to the Max console when a bang is received."
        }
    };


    void attr_set(const symbol& name, const long argc, const c74::max::t_atom* argv) {
        if (argc != 1)
            cerr << "Attributes with more than 1 arg not supported" << endl;
        else {
            auto        attr = m_python_attributes[name.c_str()];
            PyObject*   value;

            if (attr->type() == "float64")
                value = PyFloat_FromDouble(c74::max::atom_getfloat(argv));
            else if (attr->type() == "long")
                value = PyLong_FromLong(c74::max::atom_getlong(argv));
            else // if (attr->type() == "symbol")
                value = PyUnicode_DecodeFSDefault(c74::max::atom_getsym(argv)->s_name);

            PyObject_SetAttrString(m_instance, name.c_str(), value);
        }
    }


    void attr_get(const symbol& name, long* argc, c74::max::t_atom** argv) {
        auto        attr = m_python_attributes[name.c_str()];
        PyObject*   value = PyObject_GetAttrString(m_instance, name.c_str());

        if ((*argc) != 1 || !(*argv)) {         // otherwise use memory passed in
            if (*argc && *argv) {
                sysmem_freeptr(*argv);
                *argv = NULL;
            }
            *argc = 1;
            *argv = reinterpret_cast<c74::max::t_atom*>( c74::max::sysmem_newptr(sizeof(c74::max::t_atom) * (*argc)) );
        }

        if (attr->type() == "float64")
            c74::max::atom_setfloat(*argv, PyFloat_AsDouble(value));
        else if (attr->type() == "long")
            c74::max::atom_setlong(*argv, PyLong_AsLong(value));
        else // if (attr->type() == "symbol")
            c74::max::atom_setsym(*argv, c74::max::gensym(PyUnicode_AsUTF8(value)));
    }



    void operator()(audio_bundle input, audio_bundle output) {
        if (m_updating_source == true || m_process_fn == nullptr)
            output.clear();
        else {
            auto in     = input.samples(0);   // get vector for channel 0 (first channel)
            auto out    = output.samples(0);  // get vector for channel 0 (first channel)
            auto fn     = m_process_fn;
            auto args   = m_process_args;

            for (auto i = 0; i < input.frame_count(); ++i) {
                PyTuple_SetItem(args, 1, PyFloat_FromDouble(in[i])); // pyfloat ref is stolen by the tuple
                auto pOutValue = PyObject_CallObject(fn, args);
                out[i] = PyFloat_AS_DOUBLE(pOutValue);

                Py_DECREF(pOutValue);
            }
        }
    }


private:
    string                                  m_python_source {};
    PyObject*                               m_module {};
    PyObject*                               m_instance {};
    PyObject*                               m_process_fn {};
    PyObject*                               m_process_args {};
    std::unordered_map<string, PyObject*>   m_python_messages;
    std::unordered_map<string, python_attr*> m_python_attributes;
    std::atomic<bool>                       m_updating_source { false };
};


c74::max::t_max_err python_attr_set(c74::max::t_object* x, c74::max::t_object* maxattr, const long argc, const c74::max::t_atom* argv) {
    const symbol    attr_name { static_cast<const c74::max::t_symbol*>(c74::max::object_method(maxattr, k_sym_getname)) };
    auto            self = &(wrapper_find_self<python>(x))->m_min_object;

    self->attr_set(attr_name, argc, argv);
    return c74::max::MAX_ERR_NONE;
}


c74::max::t_max_err python_attr_get(c74::max::t_object* x, c74::max::t_object* maxattr, long* argc, c74::max::t_atom** argv) {
    const symbol    attr_name { static_cast<const c74::max::t_symbol*>(c74::max::object_method(maxattr, k_sym_getname)) };
    auto            self = &(wrapper_find_self<python>(x))->m_min_object;

    self->attr_get(attr_name, argc, argv);
    return c74::max::MAX_ERR_NONE;
}


MIN_EXTERNAL(python);

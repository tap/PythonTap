/// @file
/// @copyright  Copyright 2022 Timothy Place. All rights reserved.
/// @license           Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_min.h"

#define PY_SSIZE_T_CLEAN
#include <cstdlib>
#include <Python.h>

#include "python_tilde_cglue.h"
#include "python_tilde_attribute.h"
#include "python_tilde_message.h"

using namespace c74::min;


class python : public object<python>, public vector_operator<> {
public:
    MIN_DESCRIPTION {"Run python code."};
    MIN_TAGS        {"programming"};
    MIN_AUTHOR      {"Tim Place"};
    MIN_RELATED     {"js"};

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


    python(const atoms& args = {}) {
        if (maxobj() == NULL)
            return; // this occurs during dummy construction

        #ifdef MAC_VERSION
        {
            char pythonhome[] {"PYTHONHOME=/Users/tim/Documents/Max 8/Packages/python/support-mac"};
            char pythonpath[] {"PYTHONPATH=$PYTHONPATH:/Users/tim/Documents/Max 8/Packages/python/python"};
            putenv(pythonhome);
            putenv(pythonpath);
        }
        #endif

        Py_Initialize();

        #ifdef WIN_VERSION
        {
            auto err = PyRun_SimpleString(
                "import os\n"
                "import sys\n"
                "log = open('C:\\\\Users\\\\placetimothy\\\\Documents\\\\Max 8\\\\Packages\\\\python\\\\python.log', 'a')\n"
                "sys.stdout = log\n"
                "print('Hello World')\n"
                "print(sys.path)\n"
                "sys.path.append('C:\\\\Users\\\\placetimothy\\\\Documents\\\\Max 8\\\\Packages\\\\python\\\\site-packages')\n"
                "sys.path.append('C:\\\\Users\\\\placetimothy\\\\Documents\\\\Max 8\\\\Packages\\\\python\\\\misc')\n"
            );
            if (err < 0)
                PyErr_Print();
        }
        #else // MAC or LINUX
        {
            auto err = PyRun_SimpleString(
                "import sys\n"
                "log = open('/Users/tim/Documents/Max 8/Packages/python/python.log', 'a')\n"
                "sys.stdout = log\n"
                "print('Hello World')\n"
                "print(sys.path)\n"
            );
            if (err < 0)
                PyErr_Print();
        }
        #endif

        // TODO: watch the python.log file to access script output

        if (args.empty())
            m_python_source = "default";
        else
            m_python_source = to_string(args);

        update_source();

        string source_fullpath {"/Users/tim/Documents/Max 8/Packages/python/python/"};
        source_fullpath += m_python_source;
        source_fullpath += ".py";
        path p {source_fullpath};
        m_file_watcher.begin(p);
    }


    ~python() {
        Py_XDECREF(m_process_args);
        Py_XDECREF(m_instance);
        Py_XDECREF(m_module);
    }


    void update_source() {
        m_updating_source = true;

        if (m_module) {
            Py_DECREF(m_process_args);
            Py_DECREF(m_instance);

            //auto old_module = m_module;
            m_module = PyImport_ReloadModule(m_module);
            //Py_DECREF(old_module);
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
        }
        else {
           cerr << "Cannot instantiate the Python class " << m_python_source << endl;
           return;
        }
        Py_DECREF(pClass);

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
        else
            Py_DECREF(ret);
        auto attribute_dict = PyRun_String("attributes", Py_eval_input, pModuleDict, pModuleDict);
        if (attribute_dict == nullptr) {
            cout << "ERROR" << endl;
            PyErr_Print();
            return;
        }
        else {
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

                auto* attr = new python_attr(this->maxobj(), key_str, type_str); // TODO: leaking
                m_python_attributes[key_str] = attr;

                Py_DECREF(name);
                Py_DECREF(value);
                Py_DECREF(key);
            }

            Py_DECREF(attr_dict_keys);
            Py_DECREF(attribute_dict_pstr);
            Py_DECREF(attribute_dict);
        }

        std::for_each(m_python_messages.begin(), m_python_messages.end() , [this](std::pair<std::string, python_message*> element){
            auto err = c74::max::object_deletemethod(maxobj(), c74::max::gensym(element.first.c_str()));
            if (err)
                cerr << "Error removing method" << endl;
        });


        auto pDir = PyObject_Dir(m_instance); // returns array of members
        auto member_count = PyList_Size(pDir);
        for (auto i=0; i<member_count; ++i) {
            auto member = PyList_GetItem(pDir, i);
            //auto member_name = PyObject_Str(member);
            //string member_name_str = PyUnicode_AsUTF8(member_name);
            string member_name_str = PyUnicode_AsUTF8(member);

            if (member_name_str[0] == '_')
                continue;
            if (m_python_attributes.find(member_name_str) != m_python_attributes.end())
                continue;

            string run_str { "get_type_hints(me." };
            run_str += member_name_str;
            run_str += ")";
            auto io_dict = PyRun_String(run_str.c_str(), Py_eval_input, pModuleDict, pModuleDict);
            if (io_dict == nullptr)
                PyErr_Print();

            //cout endl; cout << member_name_str << endl;

            auto method = PyObject_GetAttrString(m_instance, member_name_str.c_str());
            if (PyMethod_Check(method)) {
                auto fn = PyMethod_Function(method); // returnes a *borrowed* reference
                if (PyFunction_Check(fn)) {
                    if (member_name_str == "process" && io_dict) {
                        m_process_fn = fn;
                        if (!m_process_args)
                            m_process_args = PyTuple_New(2);
                        Py_IncRef(m_instance); // because the tuple will *steal* a reference
                        PyTuple_SetItem(m_process_args, 0, m_instance);

                        int     in_count = 0;
                        int     out_count = 0;
                        auto    keys = PyDict_Keys(io_dict);
                        auto    key_count = PyDict_Size(io_dict);

                        for (auto k = 0; k < key_count; ++k) {
                            auto key = PyList_GetItem(keys, k);
                            auto value = PyDict_GetItem(io_dict, key);
                            auto name = PyObject_GetAttrString(value, "__name__");

                            auto key_str = PyUnicode_AsUTF8(key);
                            auto type_str = PyUnicode_AsUTF8(name);

                            // cout << "KEY: " << key_str << "    Value: " << type_str << endl;

                            if (string("return") == key_str) { // output
                                if (string("tuple") == type_str) {
                                    // it's a tuple, so we have to test it to find out
                                    // because we are using Python 3.8 which doesn't support full tuple annotations
                                    PyTuple_SetItem(m_process_args, 1, PyFloat_FromDouble(0.0)); // pyfloat ref is stolen by the tuple
                                    auto pOutValue = PyObject_CallObject(m_process_fn, m_process_args);
                                    out_count = PyTuple_Size(pOutValue);
                                    Py_DECREF(pOutValue);
                                }
                                else
                                    out_count = 1;
                            }
                            else { // input
                                ++in_count;
                            }

                            Py_DECREF(name);
                            Py_DECREF(value);
                            Py_DECREF(key);
                        }
                        cout << "Audio inputs: " << in_count << "    outputs: " << out_count << endl;
                    }
                    else {
                        strings argument_types;
                        auto    keys = PyDict_Keys(io_dict);
                        auto    key_count = PyDict_Size(io_dict);

                        for (auto k = 0; k < key_count; ++k) {
                            auto key = PyList_GetItem(keys, k);
                            auto value = PyDict_GetItem(io_dict, key);
                            auto name = PyObject_GetAttrString(value, "__name__");

                            auto key_str = PyUnicode_AsUTF8(key);
                            auto type_str = PyUnicode_AsUTF8(name);

                            // cout << "KEY: " << key_str << "    Value: " << type_str << endl;

                            if (string("return") == key_str) { // output
                                if (string("tuple") == type_str) {
                                }
                            }
                            else {
                                argument_types.push_back(type_str);
                            }

                            Py_DECREF(name);
                            Py_DECREF(value);
                            Py_DECREF(key);
                        }

                        auto* mess = new python_message(this->maxobj(), member_name_str, fn, argument_types); // TODO: leaking
                        m_python_messages[member_name_str] = mess;
                    }
                }
            }
            Py_DECREF(method);
            Py_DECREF(io_dict);
            //Py_DECREF(member_name);
            Py_DECREF(member);
            Py_DECREF(pModuleDict);
        }
        Py_DECREF(pDir);
        m_updating_source = false;
    }

    
    void message_gimme(symbol name, long ac, c74::max::t_atom* av) {
        auto mess = m_python_messages[name.c_str()];
        if (mess) {
            PyObject *pArgs = PyTuple_New(ac+1);
            Py_IncRef(m_instance); // because the tuple will *steal* a reference
            PyTuple_SetItem(pArgs, 0, m_instance);

            auto arg_types = mess->arg_types();
            for (auto i=0; i<ac; ++i){
                PyObject *pValue;

                if (arg_types[i] == "int")
                    pValue = PyLong_FromLong(c74::max::atom_getlong(av));
                else if (arg_types[i] == "float")
                    pValue = PyFloat_FromDouble(c74::max::atom_getfloat(av));
                else if (arg_types[i] == "str")
                    pValue = PyUnicode_DecodeFSDefault(c74::max::atom_getsym(av)->s_name);
                else
                    pValue = PyLong_FromLong(-1974);

                PyTuple_SetItem(pArgs, i+1, pValue);
                // tuple steals ownership
            }

            auto result = PyObject_Call(mess->function(), pArgs, NULL);
            if (result) {
            //    auto result_str = PyObject_Str(result);
            //    Py_ssize_t size;
            //    const char* data = PyUnicode_AsUTF8AndSize(result_str, &size);
            //    cout << "RESULT: " << data << endl;
                Py_DECREF(result);
            }
            else
                PyErr_Print();

            Py_DECREF(pArgs);
        }
    }


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
            Py_DECREF(value);
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

        Py_DECREF(value);
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
    string                                      m_python_source {};
    PyObject*                                   m_module {};
    PyObject*                                   m_instance {};
    PyObject*                                   m_process_fn {}; // borrowed reference
    PyObject*                                   m_process_args {};
    std::unordered_map<string, python_message*> m_python_messages;
    std::unordered_map<string, python_attr*>    m_python_attributes;
    std::atomic<bool>                           m_updating_source { false };
};

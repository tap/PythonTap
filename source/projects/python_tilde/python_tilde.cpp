/// @file
///    @copyright    Copyright 2022 Timothy Place. All rights reserved.
///    @license            Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"

#define PY_SSIZE_T_CLEAN
#include <cstdlib>
#include <Python.h>

using namespace c74::min;


class python : public object<python>, public vector_operator<> {
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

        if (m_module) {
            PyObject *const pDict = PyModule_GetDict(m_module); // borrowed

            PyObject* pKey = nullptr;
            PyObject* pValue = nullptr;
            for (Py_ssize_t i = 0; PyDict_Next(pDict, &i, &pKey, &pValue);) {
                const char *key = PyUnicode_AsUTF8(pKey);
                if (PyFunction_Check(pValue)) {
                    m_python_messages[key] = pValue;

                    string arg_names_symbol {m_python_source};
                    arg_names_symbol += ".";
                    arg_names_symbol += key;
                    arg_names_symbol += ".__code__.co_varnames";

                    PyObject* code = PyObject_GetAttrString(pValue, "__code__");
                    PyObject* co_varnames = PyObject_GetAttrString(code, "co_varnames");

                    auto arg_count = PyTuple_Size(co_varnames);

                    auto co_varnames_str = PyObject_Str(co_varnames);
                    Py_ssize_t size;
                    const char* co_varnames_cstr = PyUnicode_AsUTF8AndSize(co_varnames_str, &size);

                    cout << "Function " << key << " has " << arg_count << " vars -- " << co_varnames_cstr << endl;
                }
            }
        }
        else {
            PyErr_Print();
            cout << "Failed to load script" << endl;
        }
        m_updating_source = false;
    }


    python(const atoms& args = {}) {
        char pythonhome[] {"PYTHONHOME=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib"};
        char pythonpath[] {"PYTHONPATH=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib:/Users/tim/Documents/Max 8/Packages/python/misc"};

        putenv(pythonhome);
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
        Py_XDECREF(m_module);
    }


    message<> anything { this, "anything", "Execute python function.",
        MIN_FUNCTION {
            PyObject *function = m_python_messages[args[0]];
            if (function) {
                PyObject *pArgs = PyTuple_New(1);
                PyObject *pValue = PyLong_FromLong(13);
                PyTuple_SetItem(pArgs, 0, pValue);
                //PyObject *kwargs = Py_BuildValue("{s:i}", "b", 5);
                //auto result = PyObject_Call(function, args, kwargs);

                //PyObject_CallNoArgs(pValue);

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


    void operator()(audio_bundle input, audio_bundle output) {
        auto          in  = input.samples(0);                                     // get vector for channel 0 (first channel)
        auto          out = output.samples(0);                                    // get vector for channel 0 (first channel)

        if (m_updating_source == true) {
            output.clear();
        }
        else {
            auto pFunc = PyObject_GetAttrString(m_module, "process");
            auto pArgs = PyTuple_New(1);

            for (auto i = 0; i < input.frame_count(); ++i) {
                auto pValue = PyFloat_FromDouble(in[i]);

                //PyList_SetItem(pArgs, 0, pValue);
                PyTuple_SetItem(pArgs, 0, pValue);
                pValue = PyObject_CallObject(pFunc, pArgs);
                out[i] = PyFloat_AsDouble(pValue);

                Py_DECREF(pValue);
            }
            Py_DECREF(pArgs);
            Py_XDECREF(pFunc);
        }
    }


private:
    string                                  m_python_source {};
    PyObject*                               m_module {};
    std::unordered_map<string, PyObject*>   m_python_messages;
    std::atomic<bool>                       m_updating_source { false };
};


MIN_EXTERNAL(python);

/// @file
///	@copyright	Copyright 2022 Timothy Place. All rights reserved.
///	@license	        Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"

#define PY_SSIZE_T_CLEAN
#include <cstdlib>
#include <Python.h>

using namespace c74::min;


class python : public object<python> {
public:
    MIN_DESCRIPTION	{"Run python code."};
    MIN_TAGS		{"programming"};
    MIN_AUTHOR		{"Tim Place"};
    MIN_RELATED		{"python~, js, mxj"};


    inlet<>  input	{ this, "(bang) post greeting to the max console" };
    outlet<> output	{ this, "(anything) Run the named python function" };


    argument<symbol> source_arg { this, "source", "Python source file." };


    python(const atoms& args = {}) {
        char pythonhome[] {"PYTHONHOME=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib"};
        char pythonpath[] {"PYTHONPATH=/Users/tim/Documents/Max 8/Packages/python/source/cpython/Lib:/Users/tim/Documents/Max 8/Packages/python/misc"};

        putenv(pythonhome);
        putenv(pythonpath);
        Py_Initialize();

        if (args.empty())
            m_python_source = "tap_test";
        else
            m_python_source = to_string(args);

        auto pName = PyUnicode_DecodeFSDefault(m_python_source.c_str());
        m_module = PyImport_Import(pName);
        Py_DECREF(pName);

        if (m_module) {
            PyObject *const pDict = PyModule_GetDict(m_module); // borrowed

            PyObject* pKey = nullptr;
            PyObject* pValue = nullptr;
            for (Py_ssize_t i = 0; PyDict_Next(pDict, &i, &pKey, &pValue);) {
                const char *key = PyUnicode_AsUTF8(pKey);
                if (PyFunction_Check(pValue)) {
                    m_python_messages[key] = pValue;

                    //tap_test.foo.__code__.co_argcount
                    // names = tap_test.foo.__code__.co_varnames
                                        // the above results in a tupl
                    string arg_names_symbol {m_python_source};
                    arg_names_symbol += ".";
                    arg_names_symbol += key;
                    arg_names_symbol += ".__code__.co_varnames";
                    auto arg_names_pstr = PyUnicode_FromString(arg_names_symbol.c_str());




                    PyObject* code = PyObject_GetAttrString(pValue, "__code__");
                    PyObject* co_varnames = PyObject_GetAttrString(code, "co_varnames");

                    auto arg_count = PyTuple_Size(co_varnames);

                    auto co_varnames_str = PyObject_Str(co_varnames);
                    Py_ssize_t size;
                    const char* co_varnames_cstr = PyUnicode_AsUTF8AndSize(co_varnames_str, &size);

                    cout << "Function " << key << " has " << arg_count << " args -- " << co_varnames_cstr << endl;

/*
                    //PyObject *function = PyObject_GetAttrString(m_module, key);
                    PyObject *args = PyTuple_New(1);
                    PyObject *pValue = PyLong_FromLong(13);
                    PyTuple_SetItem(args, 0, pValue);
                    //PyObject *kwargs = Py_BuildValue("{s:i}", "b", 5);
                    //auto result = PyObject_Call(function, args, kwargs);
                    auto result = PyObject_Call(function, args, NULL);
                    if (result) {
                        auto result_str = PyObject_Str(result);
                        Py_ssize_t size;
                        const char* data = PyUnicode_AsUTF8AndSize(result_str, &size);
                        cout << "RESULT: " << data << endl;
                    }
                    else
                        PyErr_Print();

                   // Py_DECREF(kwargs);
                    Py_DECREF(args);
                    //Py_DECREF(function);
 */
                }
            }

            //int wtf = PyRun_SimpleString("import tap_test \n" "tap_test.bar()\n");
            //if (wtf < 0)
            //    PyErr_Print();
        }
        else {
            PyErr_Print();
            cout << "Failed to load script" << endl;
        }
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


    message<> number { this, "number", "Execute the 'anonymous' method. The number is passed in as the variable 'x'.",
        MIN_FUNCTION {
            int in_value = args[0];
            int out_value = 0;

            auto pFunc = PyObject_GetAttrString(m_module, "foo");
            if (pFunc && PyCallable_Check(pFunc)) {
                auto pArgs = PyTuple_New(1);
                auto pValue = PyLong_FromLong(in_value);
                PyTuple_SetItem(pArgs, 0, pValue);
                pValue = PyObject_CallObject(pFunc, pArgs);
                if (pValue) {
                    out_value = PyLong_AsLong(pValue);
                    output.send(out_value);
                    Py_DECREF(pValue);
                }
                Py_DECREF(pArgs);
            }
            else {
                if (PyErr_Occurred())
                    PyErr_Print();
                cout << "Cannot find function" << endl;
            }
            Py_XDECREF(pFunc);

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


    // respond to the bang message to do something
    message<> bang { this, "bang", "Post the greeting.",
        MIN_FUNCTION {
            symbol the_greeting = greeting;    // fetch the symbol itself from the attribute named greeting

            cout << the_greeting << endl;    // post to the max console
            output.send(the_greeting);       // send out our outlet
            return {};
        }
    };


private:
    string                                  m_python_source {};
    PyObject*                               m_module {};
    std::unordered_map<string, PyObject*>   m_python_messages;
};


MIN_EXTERNAL(python);

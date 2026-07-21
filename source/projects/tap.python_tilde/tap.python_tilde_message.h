/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_min_api.h"
#include "tap.python_tilde_cglue.h"

class python;

using namespace c74::min;

/// A Max message dynamically added to a tap.python~ instance, bound to a method
/// of the user's Python class. Owns a strong reference to the Python function so
/// the binding survives (and is correctly released across) module reloads.
/// Instances are owned via unique_ptr in the object's message map; the caller is
/// responsible for holding the GIL during construction and destruction.
class python_message {
  public:
    python_message(c74::max::t_object* owner, const string& a_name, PyObject* a_function, const strings& in_types)
        : m_owner{owner}
        , m_name{a_name}
        , m_function{a_function}
        , m_in_types{in_types} {
        if (m_owner == nullptr) {
            return; // this occurs during dummy construction
        }

        Py_XINCREF(m_function);

        c74::max::t_max_err err{};

        if (m_name == "int") {
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_int, m_name.c_str(),
                                             c74::max::A_LONG, 0);
        }
        else if (m_name == "float") {
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_float, m_name.c_str(),
                                             c74::max::A_FLOAT, 0);
        }
        else if (m_name == "symbol") {
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_symbol, m_name.c_str(),
                                             c74::max::A_SYM, 0);
        }
        else if (m_name == "bang") {
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_bang, m_name.c_str(), 0);
        }
        else {
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_gimme, m_name.c_str(),
                                             c74::max::A_GIMME, 0);
        }

        if (err) {
            c74::max::object_error(m_owner, "failed to add message for python method '%s'", m_name.c_str());
        }
    }

    ~python_message() { Py_XDECREF(m_function); }

    python_message(const python_message&)            = delete;
    python_message& operator=(const python_message&) = delete;

    PyObject* function() const { return m_function; }

    const strings& arg_types() const { return m_in_types; }

  private:
    c74::max::t_object* m_owner;
    string              m_name;
    PyObject*           m_function; // strong reference
    strings             m_in_types;
};

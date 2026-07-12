/// @file
/// @copyright  Copyright 2022 Timothy Place. All rights reserved.
/// @license           Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_min_api.h"

class python;

using namespace c74::min;


class python_message {
public:
    python_message(c74::max::t_object* owner, string a_name, PyObject* a_function, strings in_types)
    : m_owner(owner)
    , m_name(a_name)
    , m_function(a_function)
    , m_in_types(in_types)
    {
        if (m_owner == nullptr)
            return; // this occurs during dummy construction

        c74::max::t_max_err err;

        if (a_name == "int")
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_int, m_name.c_str(), c74::max::A_LONG, 0);
        else if (a_name == "float")
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_float, m_name.c_str(), c74::max::A_FLOAT, 0);
        else if (a_name == "symbol")
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_symbol, m_name.c_str(), c74::max::A_SYM, 0);
        else if (a_name == "bang")
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_bang, m_name.c_str(), 0);
        else if (in_types.size() == 0 && a_name == "clear")
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_clear, m_name.c_str(), 0);
        else
            err = c74::max::object_addmethod(m_owner, (c74::max::method)python_mess_gimme, m_name.c_str(), c74::max::A_GIMME, 0);


        if (err)
            ; // TODO: implement
    }

    python_message(const python_message&) = default;

    PyObject* function() const {
        return m_function;
    }

    strings arg_types() const {
        return m_in_types;
    }

private:
    c74::max::t_object* m_owner;
    string              m_name;
    PyObject*           m_function;
    strings             m_in_types;
    symbol              m_out_type;
};

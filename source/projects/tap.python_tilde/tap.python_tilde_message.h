/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include <string>

#include "c74_min_api.h"
#include "tap.python_tilde_cglue.h"

namespace tap::python {

    /// A Max message dynamically added to a tap.python~ instance for a method of the
    /// user's Python class. It only registers the Max method; the Python side (the
    /// bound function and its signature) is owned by the core's processor, which
    /// the object's message handler dispatches to by name. Instances are owned via
    /// unique_ptr in the object's message map and rebuilt on every reload.
    class python_message {
      public:
        python_message(c74::max::t_object* owner, const std::string& name)
            : m_owner{owner}
            , m_name{name} {
            if (m_owner == nullptr) {
                return; // this occurs during dummy construction
            }

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

        python_message(const python_message&)            = delete;
        python_message& operator=(const python_message&) = delete;

      private:
        c74::max::t_object* m_owner;
        std::string         m_name;
    };

} // namespace tap::python

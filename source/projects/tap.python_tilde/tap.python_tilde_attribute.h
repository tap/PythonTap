/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include "c74_min_api.h"

class python;

using namespace c74::min;

/// A Max attribute dynamically added to a tap.python~ instance, mirroring an
/// annotated attribute of the user's Python class. Instances are owned via
/// unique_ptr in the object's attribute map and persist across module reloads
/// (the underlying Max attribute object stays registered on the Max object).
class python_attr {
  public:
    python_attr(c74::max::t_object* owner, const string& a_name, const string& a_type)
        : m_owner{owner}
        , m_name{a_name}
        , m_typename{a_type} {
        if (m_typename == "int") {
            m_type = k_sym_long;
        }
        else if (m_typename == "float") {
            m_type = k_sym_float64;
        }
        else {
            m_type = k_sym_symbol;
        }

        m_attrobj = c74::max::attribute_new(m_name.c_str(), m_type, 0, (c74::max::method)python_attr_get,
                                            (c74::max::method)python_attr_set);
        auto err  = c74::max::object_addattr(m_owner, m_attrobj);
        if (err) {
            c74::max::object_error(m_owner, "failed to add attribute for python member '%s'", m_name.c_str());
        }
        else {
            err = c74::max::object_attr_addattr_parse(m_owner, m_name.c_str(), "dynamicattr", k_sym_long, 0, "1");
            if (err) {
                c74::max::object_error(m_owner, "failed to make dynamic attribute for %s", m_name.c_str());
            }
        }
    }

    python_attr(const python_attr&)            = delete;
    python_attr& operator=(const python_attr&) = delete;

    symbol type() const { return m_type; }

  private:
    c74::max::t_object* m_owner;
    string              m_name;
    string              m_typename;
    symbol              m_type;
    c74::max::t_object* m_attrobj{};
};

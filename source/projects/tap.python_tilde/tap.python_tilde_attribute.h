/// @file
/// @copyright  Copyright 2022-2026 Timothy Place. All rights reserved.
/// @license    Use of this source code is governed by the MIT License found in the License.md file.

#pragma once

#include <string>

#include "c74_min_api.h"
#include "tap.python_tilde_cglue.h"
#include "tap/python/value.h"

namespace tap::python {

    /// A Max attribute dynamically added to a tap.python~ instance, mirroring an
    /// annotated attribute of the user's Python class. Instances are owned via
    /// unique_ptr in the object's attribute map; one persists across reloads while the
    /// class keeps the field with the same type, and is removed otherwise.
    class python_attr {
      public:
        python_attr(c74::max::t_object* owner, const std::string& name, const value_type type)
            : m_owner{owner}
            , m_name{name}
            , m_value_type{type} {
            switch (m_value_type) {
            case value_type::integer:
            case value_type::boolean:
                m_type = c74::min::k_sym_long;
                break;
            case value_type::real:
                m_type = c74::min::k_sym_float64;
                break;
            case value_type::symbol:
            case value_type::any:
                m_type = c74::min::k_sym_symbol;
                break;
            }

            m_attrobj = c74::max::attribute_new(m_name.c_str(), m_type, 0, (c74::max::method)python_attr_get,
                                                (c74::max::method)python_attr_set);
            auto err  = c74::max::object_addattr(m_owner, m_attrobj);
            if (err) {
                c74::max::object_error(m_owner, "failed to add attribute for python member '%s'", m_name.c_str());
                return;
            }
            err = c74::max::object_attr_addattr_parse(m_owner, m_name.c_str(), "dynamicattr", c74::min::k_sym_long, 0,
                                                      "1");
            if (err) {
                c74::max::object_error(m_owner, "failed to make dynamic attribute for %s", m_name.c_str());
            }
            if (m_value_type == value_type::boolean) {
                // a bool field shows as a toggle in the inspector and attrui
                c74::max::object_attr_addattr_parse(m_owner, m_name.c_str(), "style", c74::min::k_sym_symbol, 0,
                                                    "onoff");
            }
        }

        python_attr(const python_attr&)            = delete;
        python_attr& operator=(const python_attr&) = delete;

        /// Detach the attribute from the Max object and free it (the class no longer has the field,
        /// or its type changed). Not called at object destruction: Max frees instance attributes then.
        void remove() {
            if (m_attrobj) {
                c74::max::object_deleteattr(m_owner, c74::max::gensym(m_name.c_str()));
                m_attrobj = nullptr;
            }
        }

        /// The Max attribute type (long, float64 or symbol).
        c74::min::symbol type() const { return m_type; }

        /// The core value type the attribute was created with; fixed for the attribute's lifetime.
        tap::python::value_type value_type() const { return m_value_type; }

      private:
        c74::max::t_object*     m_owner;
        std::string             m_name;
        tap::python::value_type m_value_type;
        c74::min::symbol        m_type;
        c74::max::t_object*     m_attrobj{};
    };

} // namespace tap::python

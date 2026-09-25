/// @file value.h
/// @brief The values exchanged between a host and a Python class, and how they coerce.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// The value model mirrors a Max atom (long, float, symbol) so the Max external can pass atoms
// through unchanged, and the coercion rules reproduce Max's own atom accessors (atom_getlong,
// atom_getfloat, atom_getsym) so behavior does not depend on which side converts.

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <variant>

namespace tap::python {

    /// The type a Python annotation maps to.
    enum class value_type { integer, real, symbol };

    /// One argument or attribute value: an integer, a real number, or a symbol (string).
    using value = std::variant<std::int64_t, double, std::string>;

    /// Map a type hint's `__name__` to a value type: `int` → integer, `float` → real, anything
    /// else → symbol. (The documented class contract; richer mapping is Phase 3.2.)
    constexpr value_type value_type_from_hint(const std::string_view hint_name) {
        if (hint_name == "int") {
            return value_type::integer;
        }
        if (hint_name == "float") {
            return value_type::real;
        }
        return value_type::symbol;
    }

    /// Convert `v` to `type` with Max atom-accessor semantics:
    /// - to integer: integers pass, reals truncate toward zero (non-finite → 0, out-of-range
    ///   saturates), symbols → 0
    /// - to real: integers and reals convert, symbols → 0.0
    /// - to symbol: symbols pass, numbers → the empty symbol
    inline value coerce(const value& v, const value_type type) {
        switch (type) {
        case value_type::integer:
            if (const auto* i = std::get_if<std::int64_t>(&v)) {
                return *i;
            }
            if (const auto* d = std::get_if<double>(&v)) {
                constexpr auto k_lowest  = static_cast<double>(std::numeric_limits<std::int64_t>::lowest());
                constexpr auto k_highest = static_cast<double>(std::numeric_limits<std::int64_t>::max());
                if (!std::isfinite(*d)) {
                    return std::int64_t{0};
                }
                if (*d <= k_lowest) {
                    return std::numeric_limits<std::int64_t>::lowest();
                }
                if (*d >= k_highest) {
                    return std::numeric_limits<std::int64_t>::max();
                }
                return static_cast<std::int64_t>(*d);
            }
            return std::int64_t{0};
        case value_type::real:
            if (const auto* i = std::get_if<std::int64_t>(&v)) {
                return static_cast<double>(*i);
            }
            if (const auto* d = std::get_if<double>(&v)) {
                return *d;
            }
            return 0.0;
        case value_type::symbol:
            if (const auto* s = std::get_if<std::string>(&v)) {
                return *s;
            }
            return std::string{};
        }
        return v;
    }

} // namespace tap::python

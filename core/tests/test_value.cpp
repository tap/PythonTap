/// @file test_value.cpp
/// @brief The value model's coercion rules reproduce Max's atom accessors.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <cstdint>
#include <limits>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "tap/python/value.h"

using namespace tap::python;

SCENARIO("Type hints map to value types by name") {
    CHECK(value_type_from_hint("int") == value_type::integer);
    CHECK(value_type_from_hint("float") == value_type::real);
    CHECK(value_type_from_hint("str") == value_type::symbol);
    CHECK(value_type_from_hint("bool") == value_type::boolean);
    CHECK(value_type_from_hint("any") == value_type::any);
    CHECK(value_type_from_hint("ndarray") == value_type::symbol);
    CHECK(value_type_from_hint("") == value_type::symbol);
}

SCENARIO("Coercion to integer follows atom_getlong") {
    CHECK(std::get<std::int64_t>(coerce(std::int64_t{42}, value_type::integer)) == 42);
    CHECK(std::get<std::int64_t>(coerce(2.9, value_type::integer)) == 2);
    CHECK(std::get<std::int64_t>(coerce(-2.9, value_type::integer)) == -2);
    CHECK(std::get<std::int64_t>(coerce(std::string{"7"}, value_type::integer)) == 0);

    THEN("reals that do not fit saturate instead of invoking undefined behavior") {
        constexpr auto k_nan = std::numeric_limits<double>::quiet_NaN();
        constexpr auto k_inf = std::numeric_limits<double>::infinity();
        CHECK(std::get<std::int64_t>(coerce(k_nan, value_type::integer)) == 0);
        CHECK(std::get<std::int64_t>(coerce(k_inf, value_type::integer)) == 0);
        CHECK(std::get<std::int64_t>(coerce(1e300, value_type::integer)) == std::numeric_limits<std::int64_t>::max());
        CHECK(std::get<std::int64_t>(coerce(-1e300, value_type::integer))
              == std::numeric_limits<std::int64_t>::lowest());
    }

    THEN("integers keep all 64 bits (no truncation to a 32-bit long)") {
        constexpr std::int64_t k_big = 1LL << 40;
        CHECK(std::get<std::int64_t>(coerce(k_big, value_type::integer)) == k_big);
    }
}

SCENARIO("Coercion to real follows atom_getfloat") {
    CHECK(std::get<double>(coerce(std::int64_t{3}, value_type::real)) == 3.0);
    CHECK(std::get<double>(coerce(0.25, value_type::real)) == 0.25);
    CHECK(std::get<double>(coerce(std::string{"0.5"}, value_type::real)) == 0.0);
}

SCENARIO("Coercion to symbol follows atom_getsym") {
    CHECK(std::get<std::string>(coerce(std::string{"hello"}, value_type::symbol)) == "hello");
    CHECK(std::get<std::string>(coerce(std::int64_t{1}, value_type::symbol)).empty());
    CHECK(std::get<std::string>(coerce(0.5, value_type::symbol)).empty());
}

SCENARIO("Coercion to boolean is 0 or 1, by atom_getlong") {
    CHECK(std::get<std::int64_t>(coerce(std::int64_t{5}, value_type::boolean)) == 1);
    CHECK(std::get<std::int64_t>(coerce(std::int64_t{0}, value_type::boolean)) == 0);
    CHECK(std::get<std::int64_t>(coerce(0.5, value_type::boolean)) == 0); // truncates first, as atom_getlong
    CHECK(std::get<std::int64_t>(coerce(-2.0, value_type::boolean)) == 1);
    CHECK(std::get<std::int64_t>(coerce(std::string{"true"}, value_type::boolean)) == 0);
}

SCENARIO("Coercion to any leaves the value as the atom carried it") {
    CHECK(std::get<std::int64_t>(coerce(std::int64_t{5}, value_type::any)) == 5);
    CHECK(std::get<double>(coerce(0.5, value_type::any)) == 0.5);
    CHECK(std::get<std::string>(coerce(std::string{"x"}, value_type::any)) == "x");
}

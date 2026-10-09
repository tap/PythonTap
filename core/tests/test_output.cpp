/// @file test_output.cpp
/// @brief tap.python's output (plan 9.1): what a method returns, converted for the host's outlets;
///        the outlet count from the return hints; the audio option; list parameters.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <string>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    std::string got(processor& p) {
        return std::get<std::string>(*p.get_attribute("got", value_type::symbol));
    }

} // namespace

// Pinned before plan 9.1 changes it: a parameter hinted list[float] is not a list to the bridge, so
// one atom arrives coerced to a symbol — a number becomes the empty symbol, as atom_getsym() says —
// and more atoms than parameters are refused.
SCENARIO("Today, a message parameter hinted list[float] receives one atom as a symbol") {
    ensure_runtime();
    log_capture log;
    processor   p{"list_params", log.sink()};
    REQUIRE(p.load());

    REQUIRE(p.call("take", std::vector<value>{1.5}));
    CHECK(got(p) == "''");

    CHECK_FALSE(p.call("take", std::vector<value>{1.0, 2.0, 3.0}));
    CHECK(log.contains("take: expected 1 argument(s), got 3", log_level::error));
}

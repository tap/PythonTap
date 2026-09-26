/// @file test_examples.cpp
/// @brief The package's shipped examples (python/*.py) load and behave as documented.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    /// The examples use attrs (and allpass numpy): CI installs them; locally, point
    /// TAP_PYTHON_TEST_SITE at a `pip install --target` folder. Skipped — visibly — otherwise,
    /// unless TAP_PYTHON_TEST_REQUIRE_EXAMPLES is set (as CI does), which makes a missing
    /// dependency a failure instead.
    bool importable(const std::string& module) {
        bool found = false;
        {
            gil_lock  lock;
            PyObject* m = PyImport_ImportModule(module.c_str());
            if (m) {
                Py_DECREF(m);
                found = true;
            }
            else {
                PyErr_Clear();
            }
        }
        if (std::getenv("TAP_PYTHON_TEST_REQUIRE_EXAMPLES")) {
            REQUIRE(found);
        }
        return found;
    }

} // namespace

SCENARIO("default.py passes audio at unity gain and exposes gain, float, int and greet") {
    ensure_runtime();
    if (!importable("attrs")) {
        SKIP("attrs is not importable by the embedded interpreter");
    }

    processor p{"default"};
    REQUIRE(p.load());

    REQUIRE(p.attributes().size() == 1);
    CHECK(p.attributes()[0].name == "gain");
    CHECK(p.attributes()[0].type == value_type::real);

    std::vector<std::string> names;
    for (const auto& m : p.messages()) {
        names.push_back(m.name);
    }
    CHECK(names == std::vector<std::string>{"float", "greet", "int"});

    CHECK(all_equal(render(p, 0.5), 0.5));

    WHEN("a float message sets the gain") {
        REQUIRE(p.call("float", std::vector<value>{0.25}));
        CHECK(all_equal(render(p, 1.0), 0.25));
    }
    WHEN("an int message sets the gain") {
        REQUIRE(p.call("int", std::vector<value>{std::int64_t{2}}));
        CHECK(all_equal(render(p, 1.0), 2.0));
    }
    WHEN("greet is sent") {
        console().clear();
        REQUIRE(p.call("greet", std::vector<value>{std::string{"max"}}));
        CHECK(console().contains("hello max, from python!", log_level::info));
    }
}

SCENARIO("allpass.py is an allpass: an impulse's energy is preserved") {
    ensure_runtime();
    if (!importable("attrs") || !importable("numpy")) {
        SKIP("attrs and numpy are not importable by the embedded interpreter");
    }

    processor p{"allpass"};
    REQUIRE(p.load());

    // prepare() sets the sample rate: 1 ms at 96 kHz is a 96-sample delay (alpha 0.5)
    p.prepare(96000.0, 64);
    constexpr std::size_t k_length = 96 * 400;
    std::vector<double>   in(k_length, 0.0);
    std::vector<double>   out(k_length);
    in[0] = 1.0;
    p.process(in.data(), out.data(), k_length);

    double energy = 0.0;
    for (const double s : out) {
        energy += s * s;
    }
    CHECK(std::abs(energy - 1.0) < 1e-9);
    CHECK(out[0] == 0.5);   // alpha * x[0]
    CHECK(out[95] == 0.0);  // nothing until the delay
    CHECK(out[96] == 0.75); // x[0] - alpha * y[0]

    THEN("alpha must stay strictly inside the unit circle") {
        console().clear();
        CHECK_FALSE(p.set_attribute("alpha", 1.0));
        CHECK(console().contains("alpha must be strictly between -1.0 and 1.0", log_level::error));
    }
}

SCENARIO("numpy_gain.py processes a vector per call") {
    ensure_runtime();
    if (!importable("attrs") || !importable("numpy")) {
        SKIP("attrs and numpy are not importable by the embedded interpreter");
    }

    log_capture log;
    processor   p{"numpy_gain", log.sink()};
    REQUIRE(p.load());
    p.prepare(48000.0, 64);
    CHECK(log.contains("one call per vector (numpy)", log_level::info));
    CHECK(all_equal(render(p, 0.5, 64), 0.5));
    REQUIRE(p.set_attribute("gain", 0.5));
    CHECK(all_equal(render(p, 0.5, 64), 0.25));
}

/// @file test_examples.cpp
/// @brief The package's shipped examples (python/*.py) load and behave as documented.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include <catch2/generators/catch_generators.hpp>

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
    CHECK(p.block_mode());
    CHECK(all_equal(render(p, 0.5, 64), 0.5));
    REQUIRE(p.set_attribute("gain", 0.5));
    CHECK(all_equal(render(p, 0.5, 64), 0.25));
}

SCENARIO("numpy_allpass.py computes exactly what allpass.py does, a vector at a time") {
    ensure_runtime();
    if (!importable("attrs") || !importable("numpy")) {
        SKIP("attrs and numpy are not importable by the embedded interpreter");
    }

    processor per_sample{"allpass"};
    processor per_vector{"numpy_allpass"};
    REQUIRE(per_sample.load());
    REQUIRE(per_vector.load());
    REQUIRE(per_vector.block_mode());

    // a delay shorter than the vector (the vector is split), then longer than it
    const auto            delay_ms = GENERATE(0.5, 1.0, 5.0);
    constexpr std::size_t k_vector = 64;
    for (auto* p : {&per_sample, &per_vector}) {
        REQUIRE(p->set_attribute("delay", delay_ms));
        p->prepare(48000.0, k_vector);
    }

    std::uint64_t       seed = 12345; // a fixed pseudo-random input in [-1, 1)
    std::vector<double> in(k_vector);
    std::vector<double> a(k_vector);
    std::vector<double> b(k_vector);
    bool                identical = true;
    for (int vector = 0; vector < 200; ++vector) {
        for (auto& s : in) {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            s    = static_cast<double>(seed >> 11) / static_cast<double>(1ULL << 52) - 1.0;
        }
        if (vector == 100) { // and a delay change mid-stream: both clear, and carry on alike
            for (auto* p : {&per_sample, &per_vector}) {
                REQUIRE(p->set_attribute("delay", delay_ms * 2.0));
            }
        }
        per_sample.process(in.data(), a.data(), k_vector);
        per_vector.process(in.data(), b.data(), k_vector);
        identical = identical && a == b;
    }
    CHECK(identical);
}

SCENARIO("stereo_width.py has two inputs and two outputs, and sets the width") {
    ensure_runtime();
    if (!importable("attrs") || !importable("numpy")) {
        SKIP("attrs and numpy are not importable by the embedded interpreter");
    }

    processor p{"stereo_width"};
    REQUIRE(p.load());
    p.prepare(48000.0, 4);
    CHECK(p.input_count() == 2);
    CHECK(p.output_count() == 2);
    CHECK(p.input_names() == std::vector<std::string>{"left", "right"});

    std::vector<double> left{1.0, 0.5, 0.0, -0.5};
    std::vector<double> right{0.0, 0.5, 1.0, 0.5};
    std::vector<double> out_left(4);
    std::vector<double> out_right(4);
    const double*       in[2]  = {left.data(), right.data()};
    double*             out[2] = {out_left.data(), out_right.data()};

    WHEN("the width is 1") {
        p.process(in, 2, out, 2, 4);
        THEN("the pair passes as it was") {
            CHECK(out_left == left);
            CHECK(out_right == right);
        }
    }
    WHEN("the width is 0") {
        REQUIRE(p.set_attribute("width", 0.0));
        p.process(in, 2, out, 2, 4);
        THEN("both channels are the mono mix") {
            CHECK(out_left == std::vector<double>{0.5, 0.5, 0.5, 0.0});
            CHECK(out_right == out_left);
        }
    }
    THEN("a negative width is refused") {
        CHECK_FALSE(p.set_attribute("width", -1.0));
    }
}

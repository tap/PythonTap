/// @file test_channels.cpp
/// @brief process() with several signal inputs and outputs (plan 2.4).
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include <catch2/generators/catch_generators.hpp>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    using channel_data = std::vector<std::vector<double>>;

    /// Run the host's `inputs` (one vector per channel, all the same length) through `p` into
    /// `output_count` output channels, each starting as a sentinel so untouched samples show.
    channel_data render_channels(processor& p, const channel_data& inputs, const std::size_t output_count,
                                 const std::size_t frame_count) {
        channel_data               outputs(output_count, std::vector<double>(frame_count, 12345.0));
        std::vector<const double*> in;
        std::vector<double*>       out;
        for (const auto& channel : inputs) {
            in.push_back(channel.data());
        }
        for (auto& channel : outputs) {
            out.push_back(channel.data());
        }
        p.process(in.data(), in.size(), out.data(), out.size(), frame_count);
        return outputs;
    }

    bool numpy_importable() {
        gil_lock  lock;
        PyObject* numpy = PyImport_ImportModule("numpy");
        if (!numpy) {
            PyErr_Clear();
        }
        Py_XDECREF(numpy);
        if (std::getenv("TAP_PYTHON_TEST_REQUIRE_EXAMPLES")) {
            REQUIRE(numpy != nullptr);
        }
        return numpy != nullptr;
    }

} // namespace

SCENARIO("process()'s parameters are its inputs and its return hint its outputs") {
    ensure_runtime();
    const auto [name, block] = GENERATE(std::pair{"two_by_two", false}, std::pair{"block_two_by_two", true});
    if (block && !numpy_importable()) {
        SKIP("numpy is not importable by the embedded interpreter");
    }
    processor p{name};
    REQUIRE(p.load());
    p.prepare(48000.0, 4);
    CHECK(p.input_count() == 2);
    CHECK(p.output_count() == 2);
    CHECK(p.block_mode() == block);

    THEN("each input arrives, and each output leaves, on its own channel") {
        const auto out = render_channels(p, {{1.0, 2.0, 3.0, 4.0}, {0.5, 0.5, 1.0, 1.0}}, 2, 4);
        CHECK(out[0] == std::vector<double>{1.5, 2.5, 4.0, 5.0});
        CHECK(out[1] == std::vector<double>{0.5, 1.5, 2.0, 3.0});
    }
    WHEN("the host gives one input, and three outputs") {
        const auto out = render_channels(p, {{1.0, 2.0, 3.0, 4.0}}, 3, 4);
        THEN("the missing input is silence, and the extra output is silent") {
            CHECK(out[0] == std::vector<double>{1.0, 2.0, 3.0, 4.0});
            CHECK(out[1] == std::vector<double>{1.0, 2.0, 3.0, 4.0});
            CHECK(all_equal(out[2], 0.0));
        }
    }
    WHEN("the host gives three inputs, and one output") {
        const auto out = render_channels(p, {{1.0, 1.0, 1.0, 1.0}, {2.0, 2.0, 2.0, 2.0}, {9.0, 9.0, 9.0, 9.0}}, 1, 4);
        THEN("the extra input is ignored, and the second output is dropped") {
            CHECK(all_equal(out[0], 3.0));
        }
    }
    WHEN("the channels are the same memory (in place)") {
        std::vector<double> a{1.0, 2.0, 3.0, 4.0};
        std::vector<double> b{0.5, 0.5, 1.0, 1.0};
        const double*       in[2]  = {a.data(), b.data()};
        double*             out[2] = {a.data(), b.data()};
        p.process(in, 2, out, 2, 4);
        THEN("every input sample is read before it is overwritten") {
            CHECK(a == std::vector<double>{1.5, 2.5, 4.0, 5.0});
            CHECK(b == std::vector<double>{0.5, 1.5, 2.0, 3.0});
        }
    }
}

SCENARIO("process() with two inputs and one output gets both inputs") {
    ensure_runtime();
    processor p{"two_inputs"};
    REQUIRE(p.load());
    CHECK(p.input_count() == 2);
    CHECK(p.output_count() == 1);
    const auto out = render_channels(p, {{3.0, 3.0}, {1.0, 2.0}}, 1, 2);
    CHECK(out[0] == std::vector<double>{2.0, 1.0});
}

SCENARIO("process() with no inputs is a generator") {
    ensure_runtime();
    processor p{"generator"};
    REQUIRE(p.load());
    CHECK(p.input_count() == 0);
    CHECK(p.output_count() == 1);
    // the object still has a left inlet (for messages); a signal there is ignored
    const auto out = render_channels(p, {{7.0, 7.0, 7.0}}, 1, 3);
    CHECK(out[0] == std::vector<double>{1.0, 2.0, 3.0});
}

SCENARIO("A process() whose inputs or outputs cannot be told is not bound, and says why") {
    ensure_runtime();
    const auto [name, why] =
        GENERATE(std::pair{"mixed_inputs", "mixes np.ndarray inputs (per vector) with others (per sample)"},
                 std::pair{"star_inputs", "takes *args, but its parameters are the object's signal inputs"},
                 std::pair{"tuple_return", "returns a tuple without saying how many values"});
    log_capture log;
    processor   p{name, log.sink()};
    forget_loaded(name); // this load runs the file, so says what is wrong with the class
    REQUIRE(p.load());
    CHECK_FALSE(p.has_process());
    CHECK(log.contains(why, log_level::error));
    CHECK(log.contains("audio is not bound", log_level::error));
    CHECK(all_equal(render_channels(p, {{1.0, 1.0}}, 1, 2)[0], 0.0));
}

SCENARIO("A process() returning the wrong number of values is silent, and reported once") {
    ensure_runtime();
    log_capture log;
    processor   p{"wrong_count", log.sink()};
    REQUIRE(p.load());
    CHECK(p.output_count() == 2);
    const auto out = render_channels(p, {{1.0, 1.0}}, 2, 2);
    CHECK(all_equal(out[0], 0.0));
    CHECK(all_equal(out[1], 0.0));
    render_channels(p, {{1.0, 1.0}}, 2, 2);
    p.flush_reports();
    const auto lines = log.lines();
    CHECK(std::count_if(lines.begin(), lines.end(),
                        [](const log_capture::line& l) {
                            return l.text.find("returned 3 value(s) for 2 outputs") != std::string::npos;
                        })
          == 1);
}

SCENARIO("The inputs and outputs follow the class across reloads, and survive a failed one") {
    ensure_runtime();
    write_script("reshaped", "class reshaped:\n"
                             "    def process(self, x: float) -> float:\n"
                             "        return x\n");
    processor p{"reshaped"};
    REQUIRE(p.load());
    CHECK(p.input_count() == 1);
    CHECK(p.output_count() == 1);

    write_script("reshaped", "class reshaped:\n"
                             "    def process(self, a: float, b: float, c: float) -> tuple[float, float]:\n"
                             "        return a, b + c\n");
    REQUIRE(p.load());
    CHECK(p.input_count() == 3);
    CHECK(p.output_count() == 2);
    CHECK(render_channels(p, {{1.0}, {2.0}, {3.0}}, 2, 1) == channel_data{{1.0}, {5.0}});

    write_script("reshaped", "class reshaped:\n    def (\n");
    CHECK_FALSE(p.load());
    CHECK(p.input_count() == 3);
    CHECK(p.output_count() == 2);
}

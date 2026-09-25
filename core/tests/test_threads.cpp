/// @file test_threads.cpp
/// @brief A real audio thread against main-thread attributes and messages.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <atomic>
#include <thread>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

SCENARIO("Attributes and messages from another thread are safe while audio runs") {
    ensure_runtime();
    processor p{"gain"};
    REQUIRE(p.load());

    std::atomic<bool> stop{false};
    std::atomic<int>  bad_samples{0};
    std::atomic<int>  vectors{0};

    // the audio thread: constant input 1.0, so every output is one of the levels set below
    std::thread audio{[&] {
        std::vector<double> in(64, 1.0);
        std::vector<double> out(64);
        while (!stop.load()) {
            p.process(in.data(), out.data(), out.size());
            for (const double s : out) {
                if (s != 1.0 && s != 0.25 && s != 0.5) {
                    ++bad_samples;
                }
            }
            ++vectors;
        }
    }};

    // this thread plays Max's main/scheduler threads
    for (int i = 0; i < 2000; ++i) {
        if (i % 2 == 0) {
            p.set_attribute("level", 0.25);
        }
        else {
            p.call("set_level", std::vector<value>{0.5});
        }
        p.get_attribute("level", value_type::real);
    }
    while (vectors.load() < 100) {
        std::this_thread::yield();
    }
    stop = true;
    audio.join();

    CHECK(bad_samples.load() == 0);
    CHECK(p.has_process());
}

/// @file test_safety.cpp
/// @brief Phase 1 of the production plan: user Python cannot take the host down.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

// 1.1 — CPython's PyErr_Print() calls Py_Exit() for a SystemExit, which would quit Max. Every
// path that reports a user exception must report a SystemExit instead of honoring it.

SCENARIO("sys.exit() in a message is reported, not honored") {
    ensure_runtime();
    processor p{"exits"};
    REQUIRE(p.load());

    console().clear();
    CHECK_FALSE(p.call("quit", std::vector<value>{}));
    CHECK(console().contains("SystemExit: 3", log_level::error));

    console().clear();
    CHECK_FALSE(p.call("raise_system_exit", std::vector<value>{}));
    CHECK(console().contains("SystemExit: raised directly", log_level::error));

    THEN("the processor carries on") {
        CHECK(p.loaded());
        CHECK(all_equal(render(p, 0.5), 0.5));
    }
}

SCENARIO("sys.exit() in process() is reported and silences the processor") {
    ensure_runtime();
    log_capture log;
    processor   p{"exits", log.sink()};
    REQUIRE(p.load());

    console().clear();
    CHECK(all_equal(render(p, 1.0), 0.0));
    CHECK(console().contains("SystemExit: from process()", log_level::error));
    CHECK(log.contains("process() raised an exception", log_level::error));
    CHECK_FALSE(p.has_process());
}

SCENARIO("sys.exit() in an attribute setter is reported, not honored") {
    ensure_runtime();
    processor p{"exits"};
    REQUIRE(p.load());

    console().clear();
    CHECK_FALSE(p.set_attribute("level", -1.0));
    CHECK(console().contains("SystemExit: from an attribute setter", log_level::error));
    CHECK(p.loaded());
}

SCENARIO("sys.exit() while the module is imported fails the load") {
    ensure_runtime();
    log_capture log;
    processor   p{"exits_on_import", log.sink()};

    console().clear();
    CHECK_FALSE(p.load());
    CHECK(console().contains("SystemExit: from module top level", log_level::error));
    CHECK(log.contains("Failed to load module 'exits_on_import'", log_level::error));
    CHECK(all_equal(render(p, 1.0), 0.0));
}

SCENARIO("sys.exit() in the constructor fails the load") {
    ensure_runtime();
    log_capture log;
    processor   p{"exits_in_constructor", log.sink()};

    console().clear();
    CHECK_FALSE(p.load());
    CHECK(console().contains("SystemExit: from __init__", log_level::error));
    CHECK(log.contains("Failed to instantiate the Python class exits_in_constructor", log_level::error));
}

// 1.2 — CPython hands the GIL to a waiting thread every switch interval, including in the middle
// of a process() vector. A reload on another thread must never pull the binding out from under
// the audio loop, and a reload that succeeds must never interrupt the audio at all.

SCENARIO("Reloading while audio runs never crashes and never interrupts the audio") {
    ensure_runtime();
    processor p{"slow_process"};
    REQUIRE(p.load());

    // make GIL hand-offs as frequent as possible, so reloads land mid-vector
    REQUIRE(run("import sys\nsys.setswitchinterval(1e-6)"));

    std::atomic<bool> stop{false};
    std::atomic<int>  wrong_samples{0};
    std::atomic<int>  vectors{0};

    std::thread audio{[&] {
        std::vector<double> in(256, 0.5);
        std::vector<double> out(256);
        while (!stop.load()) {
            p.process(in.data(), out.data(), out.size());
            for (const double s : out) {
                if (s != 0.5) {
                    ++wrong_samples;
                }
            }
            ++vectors;
        }
    }};

    int reloads = 0;
    for (; reloads < 200; ++reloads) {
        if (!p.load()) {
            break;
        }
    }
    while (vectors.load() < 50) {
        std::this_thread::yield();
    }
    stop = true;
    audio.join();
    REQUIRE(run("import sys\nsys.setswitchinterval(0.005)"));

    CHECK(reloads == 200);
    CHECK(wrong_samples.load() == 0); // no silent (or partial) vectors across 200 reloads
    CHECK(p.has_process());
}

// 1.3 — a Python method must not replace a message the host itself relies on (in Max, a method
// named filechanged would silently disable hot reload).

SCENARIO("Methods named like reserved host messages are not exposed") {
    ensure_runtime();
    log_capture log;
    processor   p{"reserved", log.sink(), {"filechanged", "dsp64"}};
    REQUIRE(p.load());

    std::vector<std::string> names;
    for (const auto& m : p.messages()) {
        names.push_back(m.name);
    }
    CHECK(names == std::vector<std::string>{"allowed"});
    CHECK(log.contains("filechanged() is reserved", log_level::error));
    CHECK(log.contains("dsp64() is reserved", log_level::error));
    CHECK_FALSE(p.call("filechanged", std::vector<value>{}));
    CHECK(p.has_process());
}

// 1.4 — the audio thread keeps one Python thread state for its lifetime, rather than creating
// and destroying one (with its allocations and interpreter-wide lock) every vector.

SCENARIO("Per-thread Python state on the audio thread survives from one vector to the next") {
    ensure_runtime();
    processor p{"thread_state"};
    REQUIRE(p.load());

    std::vector<double> first(4);
    std::vector<double> second(4);
    std::thread         audio{[&] {
        std::vector<double> in(4, 0.0);
        p.process(in.data(), first.data(), first.size());
        p.process(in.data(), second.data(), second.size());
    }};
    audio.join();

    CHECK(first == std::vector<double>{1.0, 2.0, 3.0, 4.0});
    CHECK(second == std::vector<double>{5.0, 6.0, 7.0, 8.0});
}

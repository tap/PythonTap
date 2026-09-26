/// @file test_realtime.cpp
/// @brief Phase 2.1–2.3 of the production plan: nothing printed on the audio thread, the numpy
///        block path, and the prepare() hook.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    std::int64_t get_int(processor& p, const char* name) {
        return std::get<std::int64_t>(*p.get_attribute(name, value_type::integer));
    }

    double get_real(processor& p, const char* name) {
        return std::get<double>(*p.get_attribute(name, value_type::real));
    }

    /// The processor, loaded and prepared, with a counter for report_ready.
    struct harness {
        log_capture      log;
        std::atomic<int> notified{0};
        processor        p;

        explicit harness(const char* name)
            : p{name, log.sink(), {}, [this] { ++notified; }} {}
    };

    bool numpy_available() {
        gil_lock  lock;
        PyObject* numpy = PyImport_ImportModule("numpy");
        if (!numpy) {
            PyErr_Clear();
            return false;
        }
        Py_DECREF(numpy);
        return true;
    }

    void require_numpy() {
        const bool available = numpy_available();
        if (std::getenv("TAP_PYTHON_TEST_REQUIRE_EXAMPLES")) {
            REQUIRE(available);
        }
        if (!available) {
            SKIP("numpy is not importable by the embedded interpreter");
        }
    }

} // namespace

// 2.1 — reports leave the audio thread

SCENARIO("A clean vector records nothing and never notifies the host") {
    ensure_runtime();
    harness h{"gain"};
    REQUIRE(h.p.load());
    h.log.clear(); // drop load()'s own "process() bound" line
    console().clear();

    CHECK(all_equal(render(h.p, 0.5), 0.5));
    CHECK(h.notified.load() == 0);
    h.p.flush_reports();
    CHECK(h.log.lines().empty());
    CHECK(console().lines().empty());
}

SCENARIO("A non-numeric return from process() is output as 0.0 and reported once per load") {
    ensure_runtime();
    harness h{"non_number"};
    REQUIRE(h.p.load());
    h.log.clear(); // drop load()'s own "process() bound" line

    CHECK(all_equal(render(h.p, 1.0), 0.0));
    CHECK(h.p.has_process()); // the audio keeps running
    CHECK(h.log.lines().empty());
    CHECK(h.notified.load() == 1);

    h.p.flush_reports();
    CHECK(h.log.contains("process() returned str, not a number", log_level::error));

    h.log.clear();
    render(h.p, 1.0);
    h.p.flush_reports();
    CHECK(h.log.lines().empty()); // once per load
    CHECK(h.notified.load() == 1);

    REQUIRE(h.p.load());
    render(h.p, 1.0);
    h.p.flush_reports();
    CHECK(h.log.contains("process() returned str", log_level::error)); // again after a load
}

SCENARIO("Non-finite output is replaced with 0.0 and reported once per load") {
    ensure_runtime();
    harness             h{"nan_samples"};
    std::vector<double> in{0.5, 1.0, 0.25, 1.0};
    std::vector<double> out(4);
    REQUIRE(h.p.load());

    h.p.process(in.data(), out.data(), out.size());
    CHECK(out == std::vector<double>{0.5, 0.0, 0.25, 0.0});
    CHECK(h.notified.load() == 1);
    h.p.flush_reports();
    CHECK(h.log.contains("non-finite sample (NaN or infinity)", log_level::error));

    h.log.clear();
    h.p.process(in.data(), out.data(), out.size());
    h.p.flush_reports();
    CHECK(h.log.lines().empty());
}

// 2.2 — the numpy block path

SCENARIO("A process() hinted np.ndarray is called once per vector") {
    ensure_runtime();
    require_numpy();
    harness h{"block_gain"};
    REQUIRE(h.p.load());
    h.p.prepare(48000.0, 64);
    CHECK(h.log.contains("one call per vector (numpy)", log_level::info));

    CHECK(all_equal(render(h.p, 0.5, 64), 0.25));
    CHECK(all_equal(render(h.p, 1.0, 64), 0.5));
    CHECK(get_int(h.p, "calls") == 2);

    WHEN("an attribute changes") {
        REQUIRE(h.p.set_attribute("level", 2.0));
        THEN("the next vector uses it") {
            CHECK(all_equal(render(h.p, 0.5, 64), 1.0));
        }
    }
}

SCENARIO("The per-sample path is called once per sample") {
    ensure_runtime();
    harness h{"gain"};
    REQUIRE(h.p.load());
    CHECK(h.log.contains("one call per sample", log_level::info));
}

SCENARIO("The block input array is one array, reused every vector") {
    ensure_runtime();
    require_numpy();
    harness h{"block_modes"};
    REQUIRE(h.p.load());
    h.p.prepare(48000.0, 16);

    render(h.p, 0.5, 16);
    const auto first = get_int(h.p, "last_input_id");
    render(h.p, 0.5, 16);
    CHECK(get_int(h.p, "last_input_id") == first);
}

SCENARIO("A block result in another shape is converted, or reported") {
    ensure_runtime();
    require_numpy();
    harness h{"block_modes"};
    REQUIRE(h.p.load());
    h.p.prepare(48000.0, 16);

    WHEN("it is a float64 array") {
        CHECK(all_equal(render(h.p, 0.25, 16), 0.5));
    }
    WHEN("it is float32") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{1}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.5));
    }
    WHEN("it is the input array, modified in place") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{2}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.5));
    }
    WHEN("it is a list") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{5}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.5));
    }
    WHEN("it has the wrong length") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{3}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.0));
        h.p.flush_reports();
        CHECK(h.log.contains("process() returned 15 sample(s) for a vector of 16", log_level::error));
        CHECK(h.p.has_process());
    }
    WHEN("it is not an array at all") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{4}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.0));
        h.p.flush_reports();
        CHECK(h.log.contains("process() returned NoneType, not a number", log_level::error));
    }
    WHEN("it holds infinities") {
        REQUIRE(h.p.set_attribute("mode", std::int64_t{6}));
        CHECK(all_equal(render(h.p, 0.25, 16), 0.0));
        h.p.flush_reports();
        CHECK(h.log.contains("non-finite sample", log_level::error));
    }
}

SCENARIO("The block path follows the vector size") {
    ensure_runtime();
    require_numpy();
    harness h{"block_gain"};
    REQUIRE(h.p.load());

    WHEN("prepare() never ran") {
        THEN("the first vector sizes the buffer") {
            CHECK(all_equal(render(h.p, 1.0, 16), 0.5));
        }
    }
    WHEN("prepare() announces a new vector size") {
        h.p.prepare(48000.0, 64);
        CHECK(all_equal(render(h.p, 1.0, 64), 0.5));
        h.p.prepare(48000.0, 32);
        CHECK(all_equal(render(h.p, 1.0, 32), 0.5));
    }
    WHEN("a vector arrives in a size prepare() did not announce") {
        h.p.prepare(48000.0, 64);
        CHECK(all_equal(render(h.p, 1.0, 16), 0.5));
    }
}

SCENARIO("prepare() while block audio runs never crashes or corrupts a vector") {
    ensure_runtime();
    require_numpy();
    processor p{"block_gain"};
    REQUIRE(p.load());
    p.prepare(48000.0, 64);
    REQUIRE(run("import sys\nsys.setswitchinterval(1e-6)"));

    std::atomic<bool> stop{false};
    std::atomic<int>  wrong{0};
    std::atomic<int>  vectors{0};
    std::thread       audio{[&] {
        std::vector<double> in(64, 1.0);
        std::vector<double> out(64);
        while (!stop.load()) {
            p.process(in.data(), out.data(), out.size());
            for (const double s : out) {
                if (s != 0.5) {
                    ++wrong;
                }
            }
            ++vectors;
        }
    }};
    for (int i = 0; i < 200; ++i) {
        p.prepare(48000.0, (i % 2) ? 64 : 128); // the audio thread's 64 keeps disagreeing
    }
    while (vectors.load() < 50) {
        std::this_thread::yield();
    }
    stop = true;
    audio.join();
    REQUIRE(run("import sys\nsys.setswitchinterval(0.005)"));
    CHECK(wrong.load() == 0);
}

// 2.3 — prepare()

SCENARIO("prepare() tells the class the audio settings") {
    ensure_runtime();
    harness h{"prepared"};
    REQUIRE(h.p.load());
    h.p.prepare(44100.0, 128);

    CHECK(get_real(h.p, "sample_rate") == 44100.0);
    CHECK(get_int(h.p, "vector_size") == 128);
    CHECK(get_int(h.p, "prepare_calls") == 1);
    CHECK(all_equal(render(h.p, 0.0), 44100.0));

    THEN("prepare() is not exposed as a message") {
        for (const auto& m : h.p.messages()) {
            CHECK(m.name != "prepare");
        }
    }
    THEN("a reloaded instance is prepared before it processes a single vector") {
        REQUIRE(h.p.load());
        CHECK(all_equal(render(h.p, 0.0), 44100.0));
        // the count carried over from the old instance (3.4), plus the new instance's own call
        CHECK(get_int(h.p, "prepare_calls") == 2);
    }
    THEN("new settings reach the instance") {
        h.p.prepare(96000.0, 64);
        CHECK(all_equal(render(h.p, 0.0), 96000.0));
        CHECK(get_int(h.p, "prepare_calls") == 2);
    }
}

SCENARIO("Settings announced before the class loads reach it when it does") {
    ensure_runtime();
    harness h{"prepared"};
    h.p.prepare(96000.0, 32);
    REQUIRE(h.p.load());
    CHECK(all_equal(render(h.p, 0.0), 96000.0));
    CHECK(get_int(h.p, "vector_size") == 32);
}

SCENARIO("A prepare() that raises is reported and the instance stays loaded") {
    ensure_runtime();
    harness h{"prepare_raises"};
    REQUIRE(h.p.load());
    console().clear();

    h.p.prepare(48000.0, 64);
    CHECK(console().contains("RuntimeError: prepare() failed on purpose", log_level::error));
    CHECK(h.log.contains("prepare() raised an exception", log_level::error));
    CHECK(h.p.loaded());
    CHECK(all_equal(render(h.p, 0.5), 0.5));
}

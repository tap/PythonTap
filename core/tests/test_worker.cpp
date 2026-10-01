/// @file test_worker.cpp
/// @brief Worker mode (plan 2.5): process() on a thread of its own, a fixed latency behind the
///        audio thread.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <catch2/generators/catch_generators.hpp>

#include "support.h"
#include "tap/python/worker.h"

using namespace tap::python;
using namespace tap::python::test;

namespace {

    constexpr std::size_t k_frames = 64;

    using channel_data = std::vector<std::vector<double>>;

    /// A processor run by a worker, with one counter for both's report_ready.
    struct harness {
        log_capture      log;
        std::atomic<int> notified{0};
        processor        p;
        worker           w; // declared after p: stopped (and joined) before p goes

        explicit harness(const char* name)
            : p{name, log.sink(), {}, [this] { ++notified; }}
            , w{p, log.sink(), [this] { ++notified; }} {}

        /// One vector through the worker, as the audio thread calls it: each input channel holds
        /// one value throughout; returns `output_count` channels.
        channel_data push(const std::vector<double>& values, const std::size_t output_count,
                          const std::size_t frame_count = k_frames) {
            channel_data               inputs;
            channel_data               outputs(output_count, std::vector<double>(frame_count, 12345.0));
            std::vector<const double*> in;
            std::vector<double*>       out;
            for (const auto v : values) {
                inputs.emplace_back(frame_count, v);
            }
            for (const auto& channel : inputs) {
                in.push_back(channel.data());
            }
            for (auto& channel : outputs) {
                out.push_back(channel.data());
            }
            w.process(in.data(), in.size(), out.data(), out.size(), frame_count);
            return outputs;
        }

        /// Wait until the worker has processed `count` vectors (a test may wait; the audio thread
        /// never does).
        void wait_for(const std::uint64_t count) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{20};
            while (w.processed() < count && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::microseconds{100});
            }
            REQUIRE(w.processed() >= count);
        }

        std::size_t lines_containing(const std::string& fragment) const {
            const auto lines = log.lines();
            return static_cast<std::size_t>(std::count_if(lines.begin(), lines.end(), [&](const auto& line) {
                return line.text.find(fragment) != std::string::npos;
            }));
        }
    };

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

SCENARIO("In worker mode the output is the direct output, the latency later (plan 2.5)") {
    ensure_runtime();
    const auto [name, block] = GENERATE(std::pair{"two_by_two", false}, std::pair{"block_two_by_two", true});
    if (block && !numpy_importable()) {
        SKIP("numpy is not importable by the embedded interpreter");
    }
    const std::size_t latency = GENERATE(1, 2, 3);
    harness           h{name};
    REQUIRE(h.p.load());
    h.p.prepare(48000.0, k_frames);
    h.w.start(2, 2, k_frames, latency, 48000.0);
    CHECK(h.w.running());
    CHECK(h.w.latency_samples() == latency * k_frames);

    for (std::size_t k = 0; k < 12; ++k) {
        const auto a       = static_cast<double>(k + 1);
        const auto outputs = h.push({a, a / 4.0}, 2);
        h.wait_for(k + 1);
        if (k < latency) {
            CHECK(all_equal(outputs[0], 0.0)); // the first `latency` vectors are silence
            CHECK(all_equal(outputs[1], 0.0));
        }
        else {
            const auto was = static_cast<double>(k - latency + 1); // vector k - latency's input
            CHECK(all_equal(outputs[0], was * 1.25));
            CHECK(all_equal(outputs[1], was * 0.75));
        }
    }
    CHECK(h.notified.load() == 0);
}

SCENARIO("A latency in milliseconds is whole vectors, rounded up, at least one (plan 2.5)") {
    CHECK(worker::latency_vectors(10.0, 96000.0, 64) == 15);
    CHECK(worker::latency_vectors(10.0, 44100.0, 64) == 7);  // 6.9
    CHECK(worker::latency_vectors(10.0, 48000.0, 512) == 1); // 0.94
    CHECK(worker::latency_vectors(4.0, 48000.0, 64) == 3);   // exactly 3, despite rounding error
    CHECK(worker::latency_vectors(0.0, 48000.0, 64) == 1);
    CHECK(worker::latency_vectors(-5.0, 48000.0, 64) == 1);
    CHECK(worker::latency_vectors(10.0, 0.0, 64) == 1);
    CHECK(worker::latency_vectors(std::numeric_limits<double>::quiet_NaN(), 48000.0, 64) == 1);
}

SCENARIO("The host's channels are matched to the worker's ring (plan 2.5)") {
    ensure_runtime();
    harness h{"two_by_two"};
    REQUIRE(h.p.load());
    h.w.start(2, 2, k_frames, 1, 48000.0);

    WHEN("the host gives one input and takes three outputs") {
        h.push({2.0}, 3);
        h.wait_for(1);
        const auto outputs = h.push({2.0}, 3);
        THEN("the missing input is silence and the extra output silent") {
            CHECK(all_equal(outputs[0], 2.0)); // 2 + 0
            CHECK(all_equal(outputs[1], 2.0)); // 2 - 0
            CHECK(all_equal(outputs[2], 0.0));
        }
    }
    WHEN("the host gives a shorter vector") {
        h.push({1.0, 1.0}, 2, 16);
        h.wait_for(1);
        const auto outputs = h.push({1.0, 1.0}, 2, 16);
        THEN("that many frames come back") {
            CHECK(all_equal(outputs[0], 2.0));
        }
    }
}

SCENARIO("A worker that falls behind outputs silence, keeps its latency, and says so once per load "
         "(plan 2.5)") {
    ensure_runtime();
    harness h{"stalls"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 48000.0);
    std::uint64_t k     = 0;
    const auto    paced = [&] { // one vector, waiting for the worker: never late
        const auto outputs = h.push({static_cast<double>(k + 1)}, 1);
        h.wait_for(++k);
        return outputs;
    };
    const auto stall = [&] { // the worker stalls 0.2 s; the next vectors arrive meanwhile
        REQUIRE(h.p.set_attribute("stall", value{0.2}));
        channel_data outputs;
        for (int i = 0; i < 5; ++i) {
            outputs.push_back(h.push({static_cast<double>(++k)}, 1)[0]);
        }
        h.wait_for(k);
        return outputs;
    };
    for (int i = 0; i < 4; ++i) {
        paced();
    }

    const auto during = stall();
    THEN("a vector due while it stalled is output as silence") {
        CHECK(all_equal(during[2], 0.0)); // due two vectors after the stalled one
    }
    THEN("once it has caught up, the output is the input two vectors before") {
        const auto outputs = paced(); // vector k is in; vector k - 2 is out
        CHECK(all_equal(outputs[0], static_cast<double>(k - 2)));
    }
    THEN("it is reported, from the main thread, once") {
        CHECK(h.notified.load() == 1);
        h.w.flush_reports();
        CHECK(h.log.contains("late for", log_level::error));
        CHECK(h.lines_containing("late for") == 1);

        AND_WHEN("it stalls again") {
            stall();
            h.w.flush_reports();
            THEN("nothing more is reported") {
                CHECK(h.notified.load() == 1);
                CHECK(h.lines_containing("late for") == 1);
            }
        }
        AND_WHEN("the class is reloaded and it stalls again") {
            REQUIRE(h.p.load());
            stall();
            h.w.flush_reports();
            THEN("it is reported again") {
                CHECK(h.notified.load() == 2);
                CHECK(h.lines_containing("late for") == 2);
            }
        }
    }
}

SCENARIO("A worker a whole ring behind drops new inputs until it catches up (plan 2.5)") {
    ensure_runtime();
    harness h{"stalls"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 0.0); // no sample rate: the least backlog, 16 vectors (18 slots)
    h.push({1.0}, 1);
    h.push({2.0}, 1);
    h.wait_for(2);

    REQUIRE(h.p.set_attribute("stall", value{0.3}));
    for (std::uint64_t k = 2; k < 32; ++k) { // the worker is stuck on vector 2
        h.push({static_cast<double>(k + 1)}, 1);
    }
    h.wait_for(32);
    h.w.flush_reports();
    THEN("the vectors beyond the ring were dropped, and it says so") {
        CHECK(h.log.contains("input of 12 vector(s) was dropped", log_level::error));
    }
    THEN("a dropped vector's output is silence, without counting it late") {
        h.w.flush_reports();                    // what was late during the stall has been reported
        const auto outputs = h.push({33.0}, 1); // vector 30's output is due: dropped
        h.wait_for(33);
        CHECK(all_equal(outputs[0], 0.0));
        h.w.flush_reports();
        CHECK(h.lines_containing("late for") == 1);
    }
    THEN("afterwards it runs at the same latency") {
        h.push({33.0}, 1);
        h.wait_for(33);
        h.push({34.0}, 1);
        h.wait_for(34);
        const auto outputs = h.push({35.0}, 1); // vector 34 in, vector 32 (input 33.0) out
        CHECK(all_equal(outputs[0], 33.0));
    }
}

SCENARIO("A worker can be restarted with new settings, and stopped (plan 2.5)") {
    ensure_runtime();
    harness h{"stalls"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 48000.0);
    h.push({1.0}, 1);
    h.push({2.0}, 1);

    WHEN("it is restarted with another vector size and latency") {
        h.w.start(1, 1, 128, 3, 48000.0);
        THEN("the latency is the new one, and what was queued is gone") {
            CHECK(h.w.latency_samples() == 3 * 128);
            for (std::uint64_t k = 0; k < 5; ++k) {
                const auto outputs = h.push({static_cast<double>(k + 10)}, 1, 128);
                h.wait_for(k + 1);
                CHECK(all_equal(outputs[0], k < 3 ? 0.0 : static_cast<double>(k - 3 + 10)));
            }
        }
    }
    WHEN("it is stopped") {
        h.w.stop();
        THEN("it outputs silence, and nothing is running") {
            CHECK_FALSE(h.w.running());
            CHECK(h.w.latency_samples() == 0);
            CHECK(all_equal(h.push({5.0}, 1)[0], 0.0));
        }
    }
    WHEN("it is destroyed while the class stalls") {
        REQUIRE(h.p.set_attribute("stall", value{0.2}));
        h.push({3.0}, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds{50}); // into the stall
        THEN("stopping waits for the vector in progress") {
            h.w.stop(); // joins: returns once the stall is over
            CHECK_FALSE(h.w.running());
        }
    }
}

// 8.3 — a worker never hangs the host

SCENARIO("A process() that never returns is interrupted when the worker stops, and audio stays bound (plan 8.3)") {
    ensure_runtime();
    harness h{"hangs"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 48000.0);
    h.push({1.0}, 1); // the worker enters process() and never leaves it
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    REQUIRE(h.w.processed() == 0);

    const auto before = std::chrono::steady_clock::now();
    h.w.stop();
    const auto took = std::chrono::steady_clock::now() - before;
    THEN("stop() returns within a second, having joined the thread") {
        CHECK(took < std::chrono::seconds{1});
        CHECK_FALSE(h.w.running());
        CHECK_FALSE(h.w.has_abandoned_thread());
    }
    THEN("the interruption is reported, from the main thread, and the class's audio is still bound") {
        CHECK(h.notified.load() == 1);
        h.p.flush_reports();
        CHECK(h.log.contains("interrupted", log_level::error));
        CHECK_FALSE(h.log.contains("audio disabled", log_level::error));
        CHECK(h.p.has_process());
    }
    THEN("the same class runs again once it returns") {
        REQUIRE(h.p.set_attribute("forever", std::int64_t{0}));
        h.w.start(1, 1, k_frames, 1, 48000.0);
        h.push({2.0}, 1);
        h.wait_for(1);
        CHECK(all_equal(h.push({3.0}, 1)[0], 2.0));
    }
}

SCENARIO("A process() blocked in a call Python cannot interrupt is abandoned, not waited for (plan 8.3)") {
    ensure_runtime();
    // the abandoned thread may wake and touch the processor and worker later: leaked, as a host must
    auto* h = new harness{"sleeps"}; // NOLINT(cppcoreguidelines-owning-memory)
    REQUIRE(h->p.load());
    h->w.start(1, 1, k_frames, 2, 48000.0);
    h->push({1.0}, 1); // the worker sleeps 3 s inside process()
    std::this_thread::sleep_for(std::chrono::milliseconds{50});

    const auto before = std::chrono::steady_clock::now();
    h->w.stop();
    const auto took = std::chrono::steady_clock::now() - before;
    THEN("stop() returns within a second, having abandoned the thread, and says so") {
        CHECK(took < std::chrono::seconds{1});
        CHECK_FALSE(h->w.running());
        CHECK(h->w.has_abandoned_thread());
        CHECK(h->log.contains("abandoned", log_level::error));
    }
    THEN("a new worker runs the same class once it no longer blocks") {
        REQUIRE(h->p.set_attribute("seconds", 0.0));
        h->w.start(1, 1, k_frames, 1, 48000.0);
        h->push({2.0}, 1);
        h->wait_for(1);
        CHECK(all_equal(h->push({3.0}, 1)[0], 2.0));
        h->w.stop();
    }
    if (!h->w.has_abandoned_thread()) {
        delete h; // NOLINT(cppcoreguidelines-owning-memory)
    }
}

SCENARIO("A vector that is merely slow is waited for, not interrupted (plan 8.3)") {
    ensure_runtime();
    harness h{"stalls"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 48000.0);
    REQUIRE(h.p.set_attribute("stall", value{0.05}));
    h.push({1.0}, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds{10}); // into the 50 ms stall
    h.w.stop();
    h.p.flush_reports();
    CHECK(h.notified.load() == 0);
    CHECK_FALSE(h.log.contains("interrupted", log_level::error));
    CHECK_FALSE(h.w.has_abandoned_thread());
}

SCENARIO("The worker's stack takes recursion through a C boundary to the recursion limit (plan 8.3)") {
    ensure_runtime();
    harness h{"deep_recursion"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, 4, 1, 48000.0);
    h.push({0.5}, 1, 4);
    h.wait_for(1);
    const auto outputs = h.push({0.5}, 1, 4);
    CHECK(all_equal(outputs[0], 0.5)); // every sample made it down and back
    CHECK(h.notified.load() == 0);     // no exception: the recursion did not hit a limit
}

SCENARIO("A reload while the worker runs takes effect without a pause on the audio thread (plan 2.5)") {
    ensure_runtime();
    write_script("worker_reload",
                 "class worker_reload:\n    def process(self, x: float) -> float:\n        return x\n");
    harness h{"worker_reload"};
    REQUIRE(h.p.load());
    h.w.start(1, 1, k_frames, 2, 48000.0);

    std::atomic<bool>          running{true};
    std::atomic<std::uint64_t> pushed{0};
    std::thread                audio{[&] { // the audio thread, at about real time (64 frames at 48 kHz)
        while (running.load()) {
            h.push({1.0}, 1);
            ++pushed;
            std::this_thread::sleep_for(std::chrono::microseconds{1333});
        }
    }};
    for (int i = 0; i < 10; ++i) {
        write_script("worker_reload",
                     "class worker_reload:\n    def process(self, x: float) -> float:\n        return x * "
                         + std::to_string(i % 2 == 0 ? 2 : 1) + "\n");
        REQUIRE(h.p.load());
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
    running = false;
    audio.join();

    const auto queued = pushed.load();
    h.wait_for(queued); // all it was given while reloading
    h.push({1.0}, 1);
    h.wait_for(queued + 1);
    h.push({1.0}, 1);
    h.wait_for(queued + 2);
    const auto outputs = h.push({1.0}, 1); // vector `queued` out: the first after the reloads
    THEN("the last reload is what plays") {
        CHECK(all_equal(outputs[0], 1.0)); // the tenth load: x * 1
    }
}

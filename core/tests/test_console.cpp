/// @file test_console.cpp
/// @brief Plan 8.4: nothing a user prints is posted from a thread the host did not nominate.
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

namespace {

    /// Nominate the calling thread as the host's main thread for the console, counting the calls
    /// for queued lines; put back the battery's default (every thread posts at once) when it goes.
    struct nominated_main_thread {
        std::atomic<int> ready{0};

        nominated_main_thread() {
            const auto main = std::this_thread::get_id();
            set_console_threading([main] { return std::this_thread::get_id() == main; }, [this] { ++ready; });
        }
        ~nominated_main_thread() { set_console_threading({}, {}); }
    };

    /// Run Python source on a thread of its own, to completion.
    void run_elsewhere(const std::string& source) {
        std::thread other{[&] { REQUIRE(run(source)); }};
        other.join();
    }

} // namespace

SCENARIO("A line printed off the main thread is posted by the host's flush, not by the printing thread (plan 8.4)") {
    ensure_runtime();
    nominated_main_thread host;
    console().clear();

    run_elsewhere("print('from another thread')");
    THEN("it has not reached the sink, and the host was told to flush") {
        CHECK_FALSE(console().contains("from another thread", log_level::info));
        CHECK(host.ready.load() >= 1);
    }
    THEN("flush_console() posts it") {
        flush_console();
        CHECK(console().contains("from another thread", log_level::info));
    }
    THEN("a line printed on the main thread still posts at once") {
        REQUIRE(run("print('from the main thread')"));
        CHECK(console().contains("from the main thread", log_level::info));
    }
    THEN("a partial line flushed by Python off the main thread is queued whole") {
        run_elsewhere("import sys\nprint('no newline yet', end='', flush=True)");
        CHECK_FALSE(console().contains("no newline yet", log_level::info));
        flush_console();
        CHECK(console().contains("no newline yet", log_level::info));
    }
}

SCENARIO("Two threads printing a line in pieces never produce a mixed line (plan 8.4)") {
    ensure_runtime();
    nominated_main_thread host;
    console().clear();
    REQUIRE(run("import sys\n_switch_interval = sys.getswitchinterval()\nsys.setswitchinterval(1e-6)"));

    const auto pieces = [](const char letter) {
        return std::string{"for _ in range(40):\n    for _ in range(60):\n        print('"} + letter
               + "', end='')\n    print()\n";
    };
    std::thread a{[&] { REQUIRE(run(pieces('a'))); }};
    std::thread b{[&] { REQUIRE(run(pieces('b'))); }};
    a.join();
    b.join();
    REQUIRE(run("import sys\nsys.setswitchinterval(_switch_interval)"));
    flush_console();

    std::size_t lines = 0;
    for (const auto& line : console().lines()) {
        if (line.text.find_first_of("ab") == std::string::npos) {
            continue;
        }
        ++lines;
        const auto first = line.text[0];
        CHECK(line.text == std::string(60, first)); // all one letter, the whole line
    }
    CHECK(lines == 80);
}

SCENARIO("A flood printed off the main thread never blocks it, and the overflow is counted (plan 8.4)") {
    ensure_runtime();
    nominated_main_thread host;
    console().clear();

    const auto before = std::chrono::steady_clock::now();
    run_elsewhere("for i in range(10000):\n    print('flood', i)");
    const auto took = std::chrono::steady_clock::now() - before;
    CHECK(took < std::chrono::seconds{5}); // bounded by Python, not by the console

    flush_console();
    std::size_t flood        = 0;
    bool        dropped_line = false;
    for (const auto& line : console().lines()) {
        if (line.text.rfind("flood ", 0) == 0) {
            ++flood;
        }
        else if (line.text.find("console line(s) were dropped") != std::string::npos) {
            dropped_line = true;
            CHECK(line.text.find(std::to_string(10000 - flood)) != std::string::npos);
        }
    }
    CHECK(flood == detail::console_queue::k_slots); // what the queue holds; the rest dropped
    CHECK(dropped_line);
}

SCENARIO("A line longer than a queue slot is cut with an ellipsis, not lost (plan 8.4)") {
    ensure_runtime();
    nominated_main_thread host;
    console().clear();
    run_elsewhere("print('x' * 2000)");
    flush_console();
    const auto lines = console().lines();
    REQUIRE(lines.size() == 1);
    CHECK(lines[0].text.size() == detail::console_queue::k_line_bytes);
    CHECK(lines[0].text.ends_with("\u2026"));
}

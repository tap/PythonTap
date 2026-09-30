/// @file reload_bench.cpp
/// @brief What a reload costs the audio thread: how long process() waits while load() runs (plan 2.6).
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// An audio thread plays Max's audio callback: every I/O buffer (512 samples) it calls
// processor::process() for each signal vector in it (64 samples), back to back, and times the
// buffer — scheduled as a real-time audio thread, so that other work on the machine does not
// preempt it. The main thread saves the class file with a new revision and reloads it every so
// often, as the object does on a save. load() holds the GIL to compile, run and describe the class,
// so a process() that arrives meanwhile waits: CPython makes the holder let go after its switch
// interval (5 ms by default), and never inside a long run of C code. A buffer not done within its
// period is a dropout.
//
//   tap_python_reload_bench                       # the default switch interval, then smaller ones
//   tap_python_reload_bench --seconds 10 --every 50

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "tap/python/processor.h"
#include "tap/python/runtime.h"

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <pthread/qos.h>
#endif

namespace {

    using tap::python::processor;
    using clock_type = std::chrono::steady_clock;

    constexpr double      k_sample_rate   = 96000.0;
    constexpr std::size_t k_vector_size   = 64;
    constexpr std::size_t k_io_size       = 512; // an I/O buffer: 8 vectors, computed in one go
    constexpr double      k_quiet_seconds = 2.0;
    constexpr double k_switch_intervals[] = {0.005, 0.001, 0.0005, 0.0002, 0.0001}; // seconds; CPython's default first

    struct subject {
        const char* name;    // the class written for the benchmark
        const char* example; // the shipped example it copies
    };
    constexpr subject k_subjects[] = {{"bench_reload_sample", "allpass"}, {"bench_reload_block", "numpy_allpass"}};

    std::string read_file(const std::string& path) {
        std::ifstream      in{path};
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    /// The example's source as class `name`, at `revision`: each revision is a real change, so
    /// every reload compiles and runs the file again.
    std::string revision_source(const std::string& example_source, const std::string& example, const std::string& name,
                                const int revision) {
        std::string source    = example_source;
        const auto  statement = "class " + example + ":";
        source.replace(source.find(statement), statement.size(), "class " + name + ":");
        return "REVISION = " + std::to_string(revision) + "\n" + source;
    }

    /// Schedule the calling thread as an audio thread is (macOS: the time-constraint policy Core
    /// Audio's I/O threads use), so that other work on the machine does not preempt it and the
    /// measurement shows the wait for the GIL rather than the machine's load.
    void make_realtime(const double period_seconds) {
#ifdef __APPLE__
        mach_timebase_info_data_t timebase{};
        mach_timebase_info(&timebase);
        const auto ticks = [&](const double seconds) {
            return static_cast<uint32_t>(seconds * 1e9 * timebase.denom / timebase.numer);
        };
        thread_time_constraint_policy_data_t policy{ticks(period_seconds), ticks(period_seconds * 0.5),
                                                    ticks(period_seconds), 1};
        thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY,
                          reinterpret_cast<thread_policy_t>(&policy), THREAD_TIME_CONSTRAINT_POLICY_COUNT);
#else
        (void)period_seconds;
#endif
    }

    /// The main thread, as a user-facing app's is (macOS: the user-interactive QoS class).
    void make_interactive() {
#ifdef __APPLE__
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
    }

    struct summary {
        double median{};
        double p99{};
        double max{};
    };

    summary summarize(std::vector<double> microseconds) {
        if (microseconds.empty()) {
            return {};
        }
        std::sort(microseconds.begin(), microseconds.end());
        return {microseconds[microseconds.size() / 2], microseconds[microseconds.size() * 99 / 100],
                microseconds.back()};
    }

    void run_case(const subject& s, const std::string& scripts, const double switch_interval, const double seconds,
                  const int every_ms) {
        const auto example_source = read_file(scripts + "/" + s.example + ".py");
        const auto path           = scripts + "/" + s.name + ".py";
        int        revision       = 0;
        std::ofstream{path, std::ios::trunc} << revision_source(example_source, s.example, s.name, revision);

        processor p{s.name, [](auto, auto) {}};
        if (!p.load()) {
            std::fprintf(stderr, "could not load %s\n", s.name);
            std::exit(1);
        }
        p.prepare(k_sample_rate, k_vector_size);
        {
            tap::python::gil_lock lock;
            PyRun_SimpleString(("import sys\nsys.setswitchinterval(" + std::to_string(switch_interval) + ")").c_str());
        }

        const auto period =
            std::chrono::duration_cast<clock_type::duration>(std::chrono::duration<double>{k_io_size / k_sample_rate});
        const auto          vectors = static_cast<std::size_t>((k_quiet_seconds + seconds) * k_sample_rate / k_io_size);
        std::vector<double> quiet;
        std::vector<double> reloading;
        quiet.reserve(vectors);
        reloading.reserve(vectors);
        std::atomic<bool> stop{false};
        std::atomic<bool> busy{false}; // the main thread has started reloading

        std::thread audio{[&] {
            make_realtime(k_io_size / k_sample_rate);
            std::vector<double> in(k_vector_size, 0.25);
            std::vector<double> out(k_vector_size);
            auto                next = clock_type::now();
            while (!stop.load()) {
                std::this_thread::sleep_until(next);
                next += period;
                const auto start = clock_type::now();
                for (std::size_t v = 0; v < k_io_size / k_vector_size; ++v) {
                    p.process(in.data(), out.data(), k_vector_size);
                }
                const std::chrono::duration<double, std::micro> took = clock_type::now() - start;
                (busy.load() ? reloading : quiet).push_back(took.count());
            }
        }};

        std::this_thread::sleep_for(std::chrono::duration<double>{k_quiet_seconds});
        busy = true;
        std::vector<double> loads;
        const auto          until = clock_type::now() + std::chrono::duration<double>{seconds};
        while (clock_type::now() < until) {
            std::ofstream{path, std::ios::trunc} << revision_source(example_source, s.example, s.name, ++revision);
            const auto start = clock_type::now();
            p.load();
            const std::chrono::duration<double, std::micro> took = clock_type::now() - start;
            loads.push_back(took.count());
            std::this_thread::sleep_for(std::chrono::milliseconds{every_ms});
        }
        stop = true;
        audio.join();

        const auto   q        = summarize(quiet);
        const auto   r        = summarize(reloading);
        const auto   l        = summarize(loads);
        const double deadline = std::chrono::duration<double, std::micro>{period}.count();
        const auto   late = std::count_if(reloading.begin(), reloading.end(), [&](double t) { return t > deadline; });
        std::printf(
            "%-20s switch %5.2f ms | buffer: quiet p50 %6.0f max %6.0f us | reloading p50 %6.0f p99 %6.0f max %6.0f us"
            " | %3zu of %5zu buffers over %.0f us | load p50 %5.0f max %6.0f us (%zu)\n",
            s.name, switch_interval * 1000.0, q.median, q.max, r.median, r.p99, r.max, static_cast<std::size_t>(late),
            reloading.size(), deadline, l.median, l.max, loads.size());
    }

} // namespace

int main(int argc, char** argv) {
    double seconds  = 5.0;
    int    every_ms = 100;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--seconds" && i + 1 < argc) {
            seconds = std::atof(argv[++i]);
        }
        else if (arg == "--every" && i + 1 < argc) {
            every_ms = std::max(1, std::atoi(argv[++i]));
        }
        else {
            std::fprintf(stderr, "usage: %s [--seconds <reloading seconds>] [--every <ms between reloads>]\n", argv[0]);
            return 2;
        }
    }

    make_interactive();
    const auto status = tap::python::initialize({TAP_PYTHON_BENCH_HOME, TAP_PYTHON_BENCH_SCRIPTS_DIR, {}});
    if (!status.ok) {
        std::fprintf(stderr, "could not start Python: %s\n", status.error.c_str());
        return 1;
    }
    std::printf("96 kHz, 64-sample vectors in 512-sample buffers (a %.0f us period); a save and reload every %d ms\n",
                k_io_size / k_sample_rate * 1e6, every_ms);
    for (const auto& s : k_subjects) {
        for (const double interval : k_switch_intervals) {
            run_case(s, TAP_PYTHON_BENCH_SCRIPTS_DIR, interval, seconds, every_ms);
        }
    }
    return 0;
}

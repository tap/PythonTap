/// @file process_bench.cpp
/// @brief What a class's process() costs, timed as Max's audio thread calls it (plan 6.3).
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.
//
// Each subject is loaded into a processor, told the audio settings with prepare(), and run through
// processor::process() one signal vector at a time — the call the external makes from Max's audio
// thread, taking the GIL once per vector. The result is the time it takes to process one second of
// audio: the share of one CPU core the object needs at that sample rate. Every measurement is taken
// once per round, for several rounds after an untimed one, and keeps its fastest (other work on
// the machine can only ever add time); the median is reported too, as a measure of that noise.
//
//   tap_python_bench                  # print the table
//   tap_python_bench --json out.json  # also write the measurements (scripts/update-perf-docs.py)
//   tap_python_bench --seconds 4 --runs 7
//
// It also times a message to an object without audio (tap.python, plan 9.1): a list of 64 numbers
// through processor::call_with_output(), converted in and back out, as the external's messages are
// called from Max's main or scheduler thread.
//
// Run it on an otherwise idle machine, from a Release build.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

#include "tap/python/processor.h"
#include "tap/python/runtime.h"

namespace {

    using tap::python::processor;

    struct subject {
        const char* name;        // the class file, <scripts_dir>/<name>.py
        const char* description; // what it is, for the table
    };

    // The per-sample path, then the block path; each starts with a class that does nothing, so the
    // cost of the bridge itself is separate from the cost of the Python code.
    constexpr subject k_subjects[] = {
        {"bench_identity", "per sample: returns its input (the bridge alone)"},
        {"default", "per sample: default.py, a gain (an attrs class)"},
        {"allpass", "per sample: allpass.py, a Schroeder allpass filter"},
        {"bench_block_identity", "per vector: returns its input (the bridge alone)"},
        {"numpy_gain", "per vector: numpy_gain.py, a gain"},
        {"numpy_allpass", "per vector: numpy_allpass.py, the same allpass filter"},
    };
    // Messages to an object without audio, each with a list of k_message_atoms numbers: the bridge
    // alone, then an example doing numpy work.
    struct message_subject {
        const char* name;        // the class file
        const char* message;     // the method
        const char* description; // for the table
    };
    constexpr message_subject k_message_subjects[] = {
        {"bench_list_identity", "list", "returns its list (the bridge alone)"},
        {"scale", "list", "scale.py, the list as one array, scaled with numpy"},
    };
    constexpr std::size_t k_message_atoms = 64;
    constexpr std::size_t k_message_calls = 20000; // per round

    constexpr double      k_sample_rates[] = {48000.0, 96000.0};
    constexpr std::size_t k_vector_sizes[] = {64, 512}; // Max's default signal vector, and a large one

    /// One subject at one sample rate and vector size, and the time each round took.
    struct measurement {
        std::string         subject;
        std::string         description;
        bool                block{};
        double              sample_rate{};
        std::size_t         vector_size{};
        std::vector<double> loads; // seconds of processing per second of audio, one per round

        /// The fastest round: other work on the machine can only ever add time.
        double load() const { return *std::min_element(loads.begin(), loads.end()); }

        double median() const {
            auto sorted = loads;
            std::sort(sorted.begin(), sorted.end());
            return sorted[sorted.size() / 2];
        }
    };

    /// The time `p` takes to process one second of audio at `sample_rate` in vectors of
    /// `vector_size`, from `audio_seconds` of it.
    double run_once(processor& p, const double sample_rate, const std::size_t vector_size, const double audio_seconds) {
        p.prepare(sample_rate, vector_size);

        std::vector<double> input(vector_size);
        std::vector<double> output(vector_size);
        for (std::size_t i = 0; i < vector_size; ++i) { // a 440 Hz tone at half scale
            input[i] = 0.5 * std::sin(2.0 * std::numbers::pi * 440.0 * static_cast<double>(i) / sample_rate);
        }

        const auto vectors = static_cast<std::size_t>(audio_seconds * sample_rate / static_cast<double>(vector_size));
        const auto seconds_of_audio = static_cast<double>(vectors * vector_size) / sample_rate;

        const auto start = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < vectors; ++i) {
            p.process(input.data(), output.data(), vector_size);
        }
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        return elapsed.count() / seconds_of_audio;
    }

    /// One message subject, and the time each round's call took.
    struct call_measurement {
        std::string         subject;
        std::string         description;
        std::vector<double> seconds; // per call, one per round

        double fastest() const { return *std::min_element(seconds.begin(), seconds.end()); }

        double median() const {
            auto sorted = seconds;
            std::sort(sorted.begin(), sorted.end());
            return sorted[sorted.size() / 2];
        }
    };

    /// The time one call of `message` with `args` takes on `p`, from `calls` of them.
    double call_once(processor& p, const char* message, const std::vector<tap::python::value>& args,
                     const std::size_t calls) {
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < calls; ++i) {
            const auto out = p.call_with_output(message, args);
            if (out.empty()) {
                std::fprintf(stderr, "%s output nothing\n", message);
                std::exit(1);
            }
        }
        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        return elapsed.count() / static_cast<double>(calls);
    }

    void write_json(const std::string& path, const std::vector<measurement>& results,
                    const std::vector<call_measurement>& calls) {
        std::ofstream out{path};
        out << "{\n  \"python\": \"" << Py_GetVersion() << "\",\n  \"measurements\": [\n";
        for (std::size_t i = 0; i < results.size(); ++i) {
            const auto& r = results[i];
            out << "    {\"subject\": \"" << r.subject << "\", \"description\": \"" << r.description
                << "\", \"block\": " << (r.block ? "true" : "false") << ", \"sample_rate\": " << r.sample_rate
                << ", \"vector_size\": " << r.vector_size << ", \"load\": " << r.load()
                << ", \"median\": " << r.median() << "}" << (i + 1 < results.size() ? ",\n" : "\n");
        }
        out << "  ],\n  \"calls\": [\n";
        for (std::size_t i = 0; i < calls.size(); ++i) {
            const auto& c = calls[i];
            out << "    {\"subject\": \"" << c.subject << "\", \"description\": \"" << c.description
                << "\", \"atoms\": " << k_message_atoms << ", \"seconds\": " << c.fastest()
                << ", \"median\": " << c.median() << "}" << (i + 1 < calls.size() ? ",\n" : "\n");
        }
        out << "  ]\n}\n";
    }

} // namespace

int main(int argc, char** argv) {
    std::string json_path;
    double      audio_seconds = 2.0;
    int         runs          = 7;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--json" && i + 1 < argc) {
            json_path = argv[++i];
        }
        else if (arg == "--seconds" && i + 1 < argc) {
            audio_seconds = std::atof(argv[++i]);
        }
        else if (arg == "--runs" && i + 1 < argc) {
            runs = std::max(1, std::atoi(argv[++i]));
        }
        else {
            std::fprintf(stderr, "usage: %s [--json <path>] [--seconds <audio seconds per round>] [--runs <rounds>]\n",
                         argv[0]);
            return 2;
        }
    }

    const auto log = [](const tap::python::log_level level, const std::string_view text) {
        if (level == tap::python::log_level::error) {
            std::fprintf(stderr, "%.*s\n", static_cast<int>(text.size()), text.data());
        }
    };
    const auto status = tap::python::initialize({TAP_PYTHON_BENCH_HOME, TAP_PYTHON_BENCH_SCRIPTS_DIR, log});
    if (!status.ok) {
        std::fprintf(stderr, "could not start Python: %s\n", status.error.c_str());
        return 1;
    }

    // Load every subject, then time every measurement once per round, round after round: work
    // elsewhere on the machine then falls on all of them alike, and each keeps its fastest round.
    std::vector<std::unique_ptr<processor>> processors;
    std::vector<std::size_t>                owner; // results[i] is measured with processors[owner[i]]
    std::vector<measurement>                results;
    for (const auto& s : k_subjects) {
        processors.push_back(std::make_unique<processor>(s.name, log));
        if (!processors.back()->load() || !processors.back()->has_process()) {
            std::fprintf(stderr, "could not load %s (are attrs and numpy importable?)\n", s.name);
            return 1;
        }
        const bool block = std::string_view{s.description}.starts_with("per vector");
        for (const auto vector_size : k_vector_sizes) {
            if (!block && vector_size != k_vector_sizes[0]) {
                continue; // per sample, the vector size only changes how often the GIL is taken
            }
            for (const auto sample_rate : k_sample_rates) {
                results.push_back({s.name, s.description, block, sample_rate, vector_size, {}});
                owner.push_back(processors.size() - 1);
            }
        }
    }
    std::vector<std::unique_ptr<processor>> message_processors;
    std::vector<call_measurement>           calls;
    std::vector<tap::python::value>         list_args;
    for (std::size_t i = 0; i < k_message_atoms; ++i) {
        list_args.emplace_back(0.5 * static_cast<double>(i));
    }
    for (const auto& s : k_message_subjects) {
        message_processors.push_back(std::make_unique<processor>(s.name, log, std::vector<std::string>{},
                                                                 std::function<void()>{},
                                                                 std::function<bool(const std::string&)>{}, false));
        if (!message_processors.back()->load()) {
            std::fprintf(stderr, "could not load %s (are attrs and numpy importable?)\n", s.name);
            return 1;
        }
        calls.push_back({s.name, s.description, {}});
    }

    for (int round = -1; round < runs; ++round) { // round -1 warms up, untimed
        for (std::size_t i = 0; i < results.size(); ++i) {
            auto&        r    = results[i];
            const double load = run_once(*processors[owner[i]], r.sample_rate, r.vector_size, audio_seconds);
            if (round >= 0) {
                r.loads.push_back(load);
            }
        }
        for (std::size_t i = 0; i < calls.size(); ++i) {
            const double seconds =
                call_once(*message_processors[i], k_message_subjects[i].message, list_args, k_message_calls);
            if (round >= 0) {
                calls[i].seconds.push_back(seconds);
            }
        }
    }

    std::printf("%-22s %-7s %-5s %-7s %-7s %s\n", "class", "rate", "vs", "load", "median", "cost");
    for (const auto& r : results) {
        const double load = r.load();
        const double per_call =
            r.block ? load / r.sample_rate * static_cast<double>(r.vector_size) * 1e6 : load / r.sample_rate * 1e9;
        std::printf("%-22s %-7.0f %-5zu %5.2f%% %5.2f%% %.3g %s\n", r.subject.c_str(), r.sample_rate, r.vector_size,
                    load * 100.0, r.median() * 100.0, per_call, r.block ? "us/vector" : "ns/sample");
    }

    std::printf("\n%-22s %-9s %-9s %s\n", "message", "atoms", "fastest", "median");
    for (const auto& c : calls) {
        std::printf("%-22s %-9zu %6.2f us %6.2f us\n", c.subject.c_str(), k_message_atoms, c.fastest() * 1e6,
                    c.median() * 1e6);
    }

    if (!json_path.empty()) {
        write_json(json_path, results, calls);
    }
    return 0;
}

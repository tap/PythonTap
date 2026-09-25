/// @file support.h
/// @brief Shared helpers for the core battery: one interpreter per test process, captured output.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#pragma once

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "tap/python/processor.h"
#include "tap/python/runtime.h"

namespace tap::python::test {

    /// Thread-safe capture of logged lines (the console, or one processor's diagnostics).
    class log_capture {
      public:
        struct line {
            log_level   level;
            std::string text;
        };

        log_function sink() {
            return [this](const log_level level, const std::string_view text) {
                std::lock_guard<std::mutex> lock{m_mutex};
                m_lines.push_back({level, std::string{text}});
            };
        }

        std::vector<line> lines() const {
            std::lock_guard<std::mutex> lock{m_mutex};
            return m_lines;
        }

        void clear() {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_lines.clear();
        }

        /// True if any line at `level` contains `fragment`.
        bool contains(const std::string_view fragment, const log_level level) const {
            std::lock_guard<std::mutex> lock{m_mutex};
            return std::any_of(m_lines.begin(), m_lines.end(), [&](const line& l) {
                return l.level == level && l.text.find(fragment) != std::string::npos;
            });
        }

      private:
        mutable std::mutex m_mutex;
        std::vector<line>  m_lines;
    };

    /// Python's stdout/stderr for this test process.
    inline log_capture& console() {
        static log_capture s_console;
        return s_console;
    }

    inline std::filesystem::path scripts_dir() {
        return TAP_PYTHON_TEST_SCRIPTS_DIR;
    }

    /// Start the interpreter (once per process) against the build's scripts folder. When
    /// TAP_PYTHON_TEST_SITE is set, that folder is appended to sys.path (e.g. a
    /// `pip install --target` of attrs and numpy for the example tests).
    inline void ensure_runtime() {
        static const bool k_ready = [] {
            const auto status = initialize({TAP_PYTHON_TEST_HOME, scripts_dir(), console().sink()});
            if (!status.ok) {
                return false;
            }
            if (const char* site = std::getenv("TAP_PYTHON_TEST_SITE")) {
                gil_lock  lock;
                PyObject* sys_path = PySys_GetObject("path"); // borrowed
                PyObject* dir      = PyUnicode_DecodeFSDefault(site);
                if (sys_path && dir) {
                    PyList_Append(sys_path, dir);
                }
                Py_XDECREF(dir);
            }
            return true;
        }();
        REQUIRE(k_ready);
    }

    /// Run Python source in __main__; true if it raised nothing.
    inline bool run(const std::string& source) {
        gil_lock lock;
        return PyRun_SimpleString(source.c_str()) == 0;
    }

    /// Write `<scripts_dir>/<name>.py` (tests that reload rewrite their own uniquely named module).
    inline void write_script(const std::string& name, const std::string& source) {
        std::ofstream file{scripts_dir() / (name + ".py"), std::ios::trunc};
        file << source;
    }

    /// Run `frame_count` samples of a constant `input` through `p`.
    inline std::vector<double> render(processor& p, const double input, const std::size_t frame_count = 64) {
        std::vector<double> in(frame_count, input);
        std::vector<double> out(frame_count, 12345.0); // a sentinel, so untouched samples show
        p.process(in.data(), out.data(), frame_count);
        return out;
    }

    inline bool all_equal(const std::vector<double>& samples, const double expected) {
        return std::all_of(samples.begin(), samples.end(), [&](const double s) { return s == expected; });
    }

} // namespace tap::python::test

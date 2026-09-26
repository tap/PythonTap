/// @file test_runtime.cpp
/// @brief Interpreter start-up and the console that print() and tracebacks are routed to.
// SPDX-License-Identifier: MIT
// Copyright 2022-2026 Timothy Place.

#include "support.h"

using namespace tap::python;
using namespace tap::python::test;

SCENARIO("The interpreter starts once and every later call reports the same outcome") {
    ensure_runtime();
    CHECK(Py_IsInitialized());

    const auto again = initialize({"/nonexistent", "/nonexistent", {}}); // ignored: already initialized
    CHECK(again.ok);
    CHECK(again.error.empty());
    CHECK(scripts_directory() == scripts_dir());
}

SCENARIO("The GIL is released after start-up, so any thread can take it") {
    ensure_runtime();
    gil_lock lock;
    CHECK(PyGILState_Check() == 1);
}

SCENARIO("The user's script folder is importable after the standard library") {
    // D2: a helper module next to the class files can be imported, but a user file named like a
    // standard-library module (random.py, json.py) cannot shadow it
    ensure_runtime();
    CHECK(run("import os, sys, sysconfig\n"
              "scripts = '" TAP_PYTHON_TEST_SCRIPTS_DIR "'\n"
              "assert sys.path.count(scripts) == 1, sys.path\n"
              "stdlib = sysconfig.get_path('stdlib')\n"
              "assert sys.path.index(scripts) > sys.path.index(stdlib), sys.path\n"));
}

SCENARIO("print() reaches the console a line at a time") {
    ensure_runtime();
    console().clear();

    WHEN("a complete line is printed") {
        REQUIRE(run("print('hello console')"));
        THEN("it arrives as one info line without its newline") {
            CHECK(console().contains("hello console", log_level::info));
        }
    }

    WHEN("a line is printed in pieces") {
        REQUIRE(run("print('partial ', end='')"));
        THEN("nothing arrives until the newline") {
            CHECK_FALSE(console().contains("partial", log_level::info));
        }
        REQUIRE(run("print('line')"));
        THEN("the pieces arrive joined") {
            CHECK(console().contains("partial line", log_level::info));
        }
    }

    WHEN("several lines are printed at once") {
        REQUIRE(run("print('first\\nsecond')"));
        THEN("each arrives separately") {
            CHECK(console().contains("first", log_level::info));
            CHECK(console().contains("second", log_level::info));
        }
    }
}

SCENARIO("stderr and tracebacks reach the console as errors") {
    ensure_runtime();
    console().clear();

    REQUIRE(run("import sys\nsys.stderr.write('to stderr\\n')"));
    CHECK(console().contains("to stderr", log_level::error));

    CHECK_FALSE(run("raise KeyError('traceback marker')"));
    CHECK(console().contains("KeyError: 'traceback marker'", log_level::error));
}

SCENARIO("sys.stdout and sys.stderr are text streams libraries can probe") {
    ensure_runtime();
    console().clear();
    CHECK(run("import io, sys\n"
              "for s in (sys.stdout, sys.stderr):\n"
              "    assert isinstance(s, io.TextIOBase)\n"
              "    assert s.encoding == 'utf-8' and s.writable() and not s.isatty()\n"
              "try:\n"
              "    sys.stdout.fileno()\n"
              "    raise AssertionError('fileno() should not exist')\n"
              "except io.UnsupportedOperation:\n"
              "    pass\n"));

    WHEN("a partial line is flushed") {
        REQUIRE(run("print('progress', end='', flush=True)"));
        THEN("it arrives without waiting for a newline") {
            CHECK(console().contains("progress", log_level::info));
        }
    }
    WHEN("text contains a NUL character") {
        REQUIRE(run("print('before\\x00after')"));
        THEN("it is written rather than rejected") {
            CHECK(console().contains("before", log_level::info));
        }
    }
}

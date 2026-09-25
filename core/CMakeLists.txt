# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# tap::python_core — the host-independent CPython layer behind tap.python~ (header-only).
# Consumers include core/include and link a CPython 3.13 of their choosing (the Max external
# links the runtime in <package>/support). Built standalone, this also builds the core's own
# test battery against the CPython that find_package(Python3) finds:
#
#   cmake -S core -B build-core && cmake --build build-core && ctest --test-dir build-core

cmake_minimum_required(VERSION 3.24)
project(tap-python-core LANGUAGES CXX)

add_library(tap_python_core INTERFACE)
add_library(tap::python_core ALIAS tap_python_core)
target_include_directories(tap_python_core INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(tap_python_core INTERFACE cxx_std_20)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
endif ()

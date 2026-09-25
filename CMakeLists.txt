# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Build for the tap.python package (Min-API based Max externals).
# Requires the embedded Python runtime in support/ — run
# scripts/install-runtime.sh (macOS) or scripts/install-runtime.ps1 (Windows)
# from the package root first. On Linux (development only: the mock-kernel
# unit test), a system CPython 3.13 is embedded instead.

cmake_minimum_required(VERSION 3.19)
project(tap-python)

enable_testing()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

if (APPLE)
    # Build universal binaries (Apple Silicon + Intel) by default. Max 9 runs
    # natively on Apple Silicon; never ship an Intel-only external.
    #
    # ...but never build wider than the embedded Python runtime in support/,
    # which the external links against: a universal build over a native-only
    # runtime fails at link with "symbol(s) not found for architecture ...".
    # install-runtime.sh installs native by default and universal with
    # --universal (as CI does), so when the runtime is present, match its
    # architectures.
    #
    # The detected value is remembered, and detection re-runs on every configure
    # for as long as CMAKE_OSX_ARCHITECTURES still holds it — so reinstalling the
    # runtime (native <-> universal) is picked up by an existing build folder.
    # A value you set yourself is never overwritten, but it is checked below.
    set(_tap_python_dylib "")
    set(_tap_runtime_archs "")
    file(GLOB _tap_python_dylib "${CMAKE_CURRENT_SOURCE_DIR}/support/lib/libpython3.*.dylib")
    if (_tap_python_dylib)
        list(GET _tap_python_dylib 0 _tap_python_dylib)
        execute_process(COMMAND lipo -archs "${_tap_python_dylib}"
                        OUTPUT_VARIABLE _tap_runtime_archs
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET)
        string(REPLACE " " ";" _tap_runtime_archs "${_tap_runtime_archs}")
    endif ()

    if (NOT CMAKE_OSX_ARCHITECTURES OR "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "${TAP_PYTHON_AUTO_ARCHITECTURES}")
        if (_tap_runtime_archs)
            set(_tap_python_archs "${_tap_runtime_archs}")
            set(_tap_python_archs_source "matching the support/ runtime")
        else ()
            set(_tap_python_archs "arm64;x86_64")
            set(_tap_python_archs_source "universal default; no support/ runtime yet")
        endif ()
        set(CMAKE_OSX_ARCHITECTURES "${_tap_python_archs}" CACHE STRING "macOS architectures" FORCE)
        set(TAP_PYTHON_AUTO_ARCHITECTURES "${_tap_python_archs}" CACHE INTERNAL
            "The CMAKE_OSX_ARCHITECTURES value last detected from the runtime")
        message(STATUS "CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES} (${_tap_python_archs_source})")
    endif ()

    # Whatever the source, refuse to configure a build the runtime cannot link.
    if (_tap_runtime_archs)
        foreach (_arch ${CMAKE_OSX_ARCHITECTURES})
            if (NOT _arch IN_LIST _tap_runtime_archs)
                message(FATAL_ERROR
                    "CMAKE_OSX_ARCHITECTURES includes ${_arch}, but the runtime in support/ only has "
                    "${_tap_runtime_archs} — the link would fail. Run ./scripts/install-runtime.sh "
                    "--universal for a universal build, or configure with a narrower "
                    "-DCMAKE_OSX_ARCHITECTURES.")
            endif ()
        endforeach ()
    endif ()

    # Minimum macOS is 11.0, tracking Max 9's system requirement. Min's
    # min-pretarget.cmake force-pins CMAKE_OSX_DEPLOYMENT_TARGET to 10.11 in
    # every object and CMake has no per-target deployment property, so append
    # the deployment flag globally instead — the later flag wins on the
    # compile/link command line.
    add_compile_options(-mmacosx-version-min=11.0)
    add_link_options(-mmacosx-version-min=11.0)
endif ()

# Min-DevKit package helpers (defines SUBDIRLIST, packaging, etc.)
include(${CMAKE_CURRENT_SOURCE_DIR}/source/min-api/script/min-package.cmake)

# Generate a build target for every object folder in source/projects.
#
# C++20 everywhere (the core in core/ requires it): Min's posttarget pins each external to C++17
# and the unit-test harness pins each _test target to C++17. Force C++20 on every target an object
# folder creates (the external and its test) here, in one place, as TapTools-Max does.
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message(STATUS "Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
        get_property(_proj_targets
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}
            PROPERTY BUILDSYSTEM_TARGETS)
        foreach (_t ${_proj_targets})
            set_property(TARGET ${_t} PROPERTY CXX_STANDARD 20)
            set_property(TARGET ${_t} PROPERTY CXX_STANDARD_REQUIRED ON)
            # The min unit-test harness pins tests to C++17 because its bundled Catch calls
            # std::uncaught_exception() — removed in C++20 (Apple's libc++ enforces this; libstdc++
            # still keeps it). It papers over that on Apple by defining
            # CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS, which forces the *removed* singular form. Now
            # that tests are C++20, cancel that opt-out (our option comes later on the command line and
            # wins) so Catch auto-selects the C++20-valid std::uncaught_exceptions().
            if (APPLE AND _t MATCHES "_test$")
                target_compile_options(${_t} PRIVATE
                    -UCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS
                    -DCATCH_CONFIG_CPP17_UNCAUGHT_EXCEPTIONS)
            endif ()
        endforeach ()
    endif ()
endforeach ()

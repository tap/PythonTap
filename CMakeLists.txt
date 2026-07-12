# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.
#
# Build for the tap.python package (Min-API based Max externals).
# Requires the embedded Python runtime in support/ — run
# scripts/install-runtime.sh (macOS) or scripts/install-runtime.ps1 (Windows)
# from the package root first.

cmake_minimum_required(VERSION 3.19)
project(tap-python)

enable_testing()

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")

if (APPLE)
    # Build universal binaries (Apple Silicon + Intel) by default. Max 9 runs
    # natively on Apple Silicon; never ship an Intel-only external.
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
        message(STATUS "CMAKE_OSX_ARCHITECTURES set to ${CMAKE_OSX_ARCHITECTURES}")
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
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir}/CMakeLists.txt")
        message(STATUS "Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_dir})
    endif ()
endforeach ()

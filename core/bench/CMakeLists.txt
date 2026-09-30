# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.
#
# The process() benchmark (plan 6.3): how much of a CPU core a class's audio callback needs.
# Measure from a Release build; scripts/update-perf-docs.py builds it, runs it, and writes the
# ReadMe's table.

find_package(Python3 3.13 EXACT REQUIRED COMPONENTS Interpreter Development.Embed)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix, end='')"
    OUTPUT_VARIABLE TAP_PYTHON_BENCH_HOME
    COMMAND_ERROR_IS_FATAL ANY
)

add_executable(tap_python_bench process_bench.cpp)
target_link_libraries(tap_python_bench PRIVATE tap::python Python3::Python)

set(_scripts_dir "${CMAKE_CURRENT_BINARY_DIR}/scripts")
target_compile_definitions(tap_python_bench PRIVATE
    TAP_PYTHON_BENCH_HOME="${TAP_PYTHON_BENCH_HOME}"
    TAP_PYTHON_BENCH_SCRIPTS_DIR="${_scripts_dir}"
)

if (MSVC)
    target_compile_options(tap_python_bench PRIVATE /W4)
else ()
    target_compile_options(tap_python_bench PRIVATE -Wall -Wextra)
endif ()

# The script folder: the benchmark's own classes plus the package's shipped examples.
add_custom_command(TARGET tap_python_bench POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/python" "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../python" "${_scripts_dir}"
)

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

set(_scripts_dir "${CMAKE_CURRENT_BINARY_DIR}/scripts")

# The script folder: the benchmarks' own classes plus the package's shipped examples.
add_custom_target(tap_python_bench_scripts
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/python" "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../python" "${_scripts_dir}"
)

# tap_python_bench: what process() costs (6.3); tap_python_reload_bench: what a reload costs the
# audio thread (2.6)
foreach (_bench IN ITEMS tap_python_bench tap_python_reload_bench)
    if (_bench STREQUAL "tap_python_bench")
        add_executable(${_bench} process_bench.cpp)
    else ()
        add_executable(${_bench} reload_bench.cpp)
    endif ()
    target_link_libraries(${_bench} PRIVATE tap::python Python3::Python)
    add_dependencies(${_bench} tap_python_bench_scripts)
    target_compile_definitions(${_bench} PRIVATE
        TAP_PYTHON_BENCH_HOME="${TAP_PYTHON_BENCH_HOME}"
        TAP_PYTHON_BENCH_SCRIPTS_DIR="${_scripts_dir}"
    )
    if (MSVC)
        target_compile_options(${_bench} PRIVATE /W4)
    else ()
        target_compile_options(${_bench} PRIVATE -Wall -Wextra)
    endif ()
endforeach ()
find_package(Threads REQUIRED)
target_link_libraries(tap_python_reload_bench PRIVATE Threads::Threads)

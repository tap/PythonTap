# SPDX-License-Identifier: MIT
# Copyright 2022-2026 Timothy Place.

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.0
    FIND_PACKAGE_ARGS 3
)
FetchContent_MakeAvailable(Catch2)
include(Catch)

# The runtime version is pinned (D4 in docs/PRODUCTION-PLAN.md); point Python3_ROOT_DIR or
# Python3_EXECUTABLE at a 3.13 if the default search finds another.
find_package(Python3 3.13 EXACT REQUIRED COMPONENTS Interpreter Development.Embed)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix, end='')"
    OUTPUT_VARIABLE TAP_PYTHON_TEST_HOME
    COMMAND_ERROR_IS_FATAL ANY
)
message(STATUS "Core tests embed ${Python3_EXECUTABLE} (home ${TAP_PYTHON_TEST_HOME})")

set(TAP_PYTHON_SANITIZE "" CACHE STRING
    "Sanitizers for the core tests, e.g. 'address,undefined' or 'thread' (GCC/Clang)")

add_executable(tap_python_core_tests
    test_value.cpp
    test_runtime.cpp
    test_processor.cpp
    test_threads.cpp
    test_examples.cpp
    test_safety.cpp
    test_realtime.cpp
)
target_link_libraries(tap_python_core_tests PRIVATE tap::python_core Python3::Python Catch2::Catch2WithMain)

set(_scripts_dir "${CMAKE_CURRENT_BINARY_DIR}/scripts")
target_compile_definitions(tap_python_core_tests PRIVATE
    TAP_PYTHON_TEST_HOME="${TAP_PYTHON_TEST_HOME}"
    TAP_PYTHON_TEST_SCRIPTS_DIR="${_scripts_dir}"
)

if (MSVC)
    target_compile_options(tap_python_core_tests PRIVATE /W4)
else ()
    target_compile_options(tap_python_core_tests PRIVATE -Wall -Wextra)
endif ()

if (TAP_PYTHON_SANITIZE)
    target_compile_options(tap_python_core_tests PRIVATE -fsanitize=${TAP_PYTHON_SANITIZE} -fno-omit-frame-pointer)
    target_link_options(tap_python_core_tests PRIVATE -fsanitize=${TAP_PYTHON_SANITIZE})
endif ()

# The script folder the tests' interpreter imports from: the test fixtures plus the package's
# shipped examples, refreshed on every build. Tests that exercise reload write into it too.
add_custom_command(TARGET tap_python_core_tests POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/python" "${_scripts_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../python" "${_scripts_dir}"
)

# One process per test case: the interpreter is process-wide and never finalized. The interpreter
# is intentionally leaked at exit, so LeakSanitizer stays off.
catch_discover_tests(tap_python_core_tests
    PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0"
)

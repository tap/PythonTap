# Copyright 2022-2026 Timothy Place. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.19)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# EMBEDDED PYTHON RUNTIME
#############################################################
# Installed into <package>/support by scripts/install-runtime.*
# (python-build-standalone). We link libpython from there and, on the Mac,
# resolve it at load time through an rpath relative to the external:
#   <package>/externals/tap.python~.mxo/Contents/MacOS/<binary>
#   -> @loader_path/../../../../support/lib
# On Windows, Max adds the package's support folder to the DLL search path,
# which is where the runtime places python3xx.dll.

set(SUPPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../support")

if (APPLE)
    file(GLOB PYTHON_INCLUDE_DIR "${SUPPORT_DIR}/include/python3.*")
    file(GLOB PYTHON_LIB "${SUPPORT_DIR}/lib/libpython3.*.dylib")
elseif (WIN32)
    set(PYTHON_INCLUDE_DIR "${SUPPORT_DIR}/include")
    # python3[0-9]* excludes python3.lib, the limited-ABI stub — we use the full API
    file(GLOB PYTHON_LIB "${SUPPORT_DIR}/libs/python3[0-9]*.lib")
else ()
    # Max does not run on Linux, but the external and its mock-kernel unit test
    # still build and run there — against the CPython 3.13 find_package() finds
    # (point Python3_EXECUTABLE at one) instead of a support/ runtime. This is
    # the fast feedback loop for the Max glue; macOS and Windows stay the targets.
    find_package(Python3 3.13 EXACT REQUIRED COMPONENTS Interpreter Development.Embed)
    set(PYTHON_INCLUDE_DIR "${Python3_INCLUDE_DIRS}")
    set(PYTHON_LIB "${Python3_LIBRARIES}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix, end='')"
        OUTPUT_VARIABLE TAP_PYTHON_HOME
        COMMAND_ERROR_IS_FATAL ANY
    )
    # directory scope, so the unit-test target created below gets it too
    add_compile_definitions(TAP_PYTHON_HOME="${TAP_PYTHON_HOME}")
endif ()

if (NOT PYTHON_LIB OR NOT EXISTS "${PYTHON_INCLUDE_DIR}")
    message(FATAL_ERROR
        "No Python runtime found in ${SUPPORT_DIR}.\n"
        "Run scripts/install-runtime.sh (macOS) or scripts/install-runtime.ps1 (Windows) "
        "from the package root, then re-run CMake.")
endif ()

list(GET PYTHON_LIB 0 PYTHON_LIB)
message(STATUS "Embedding Python: ${PYTHON_LIB}")

# The host-independent CPython layer (tap::python), shared with the core's own Linux test battery.
set(TAP_PYTHON_CORE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../core/include")

include_directories(
    "${C74_INCLUDES}"
    "${PYTHON_INCLUDE_DIR}"
    "${TAP_PYTHON_CORE_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)


#############################################################
# MAX EXTERNAL
#############################################################

set(SOURCE_FILES
    ${PROJECT_NAME}.h
    ${PROJECT_NAME}_package.h
    ${PROJECT_NAME}_cglue.h
    ${PROJECT_NAME}_attribute.h
    ${PROJECT_NAME}_message.h
    ${PROJECT_NAME}.cpp
)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

# The external must load even without a runtime, so it can say what is missing (plan 4.2) instead
# of Max failing to load it: libpython is linked weakly on macOS, and delay-loaded on Windows —
# which MSVC allows only because the core imports no CPython data symbols (see runtime.h; CI checks
# with scripts/check-data-imports.sh). runtime_library_loadable() checks before the first call.
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC "-Wl,-weak_library,${PYTHON_LIB}")
elseif (WIN32)
    get_filename_component(PYTHON_DLL_NAME "${PYTHON_LIB}" NAME_WE)
    set(PYTHON_DLL_NAME "${PYTHON_DLL_NAME}.dll")
    target_link_libraries(${PROJECT_NAME} PUBLIC "${PYTHON_LIB}" delayimp)
    target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:${PYTHON_DLL_NAME}")
    # directory scope, so the unit-test target created below gets it too
    add_compile_definitions(TAP_PYTHON_DLL="${PYTHON_DLL_NAME}")
else ()
    target_link_libraries(${PROJECT_NAME} PUBLIC "${PYTHON_LIB}")
endif ()

if (APPLE)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-rpath,@loader_path/../../../../support/lib")
    # Only that relative rpath: without this CMake also embeds the absolute path of
    # the build machine's support/lib, which would leak into every shipped binary.
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH ON)
endif ()

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)
# (C++20 is forced on this target and its test by the root CMakeLists.txt.)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)

if (TARGET ${PROJECT_NAME}_test)
    target_link_libraries(${PROJECT_NAME}_test PRIVATE "${PYTHON_LIB}")
    if (APPLE)
        # the test binary runs from the build tree, so give it an absolute
        # rpath to the installed runtime
        get_filename_component(PYTHON_LIB_DIR "${PYTHON_LIB}" DIRECTORY)
        target_link_options(${PROJECT_NAME}_test PRIVATE "-Wl,-rpath,${PYTHON_LIB_DIR}")
    endif ()
endif ()

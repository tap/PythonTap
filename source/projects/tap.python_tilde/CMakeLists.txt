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
else ()
    set(PYTHON_INCLUDE_DIR "${SUPPORT_DIR}/include")
    file(GLOB PYTHON_LIB "${SUPPORT_DIR}/libs/python3*.lib")
endif ()

if (NOT PYTHON_LIB OR NOT EXISTS "${PYTHON_INCLUDE_DIR}")
    message(FATAL_ERROR
        "No Python runtime found in ${SUPPORT_DIR}.\n"
        "Run scripts/install-runtime.sh (macOS) or scripts/install-runtime.ps1 (Windows) "
        "from the package root, then re-run CMake.")
endif ()

list(GET PYTHON_LIB 0 PYTHON_LIB)
message(STATUS "Embedding Python: ${PYTHON_LIB}")

include_directories(
    "${C74_INCLUDES}"
    "${PYTHON_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)


#############################################################
# MAX EXTERNAL
#############################################################

set(SOURCE_FILES
    ${PROJECT_NAME}.h
    ${PROJECT_NAME}_runtime.h
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

target_link_libraries(${PROJECT_NAME} PUBLIC "${PYTHON_LIB}")

if (APPLE)
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,-rpath,@loader_path/../../../../support/lib")
endif ()

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

# Min's posttarget pins C++17; the object uses C++20.
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)


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

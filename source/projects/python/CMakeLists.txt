# Copyright 2018 The Min-DevKit Authors. All rights reserved.
# Use of this source code is governed by the MIT License found in the License.md file.

cmake_minimum_required(VERSION 3.0)
set(CMAKE_OSX_ARCHITECTURES x86_64)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


if (APPLE)
    include_directories(
        "${C74_INCLUDES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../cpython"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../cpython/include"
    )
else()
    include_directories(
        "${C74_INCLUDES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../support/include"
    )
endif()


set( SOURCE_FILES
    ${PROJECT_NAME}.cpp
)


add_library(
    ${PROJECT_NAME}
    MODULE
    ${SOURCE_FILES}
)

if (APPLE)
    SET(PYTHON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../cpython/libpython3.10.a")
else()
    SET(PYTHON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../../cpython/PCBuild/amd64/python310.lib")
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC ${PYTHON_LIB})

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)

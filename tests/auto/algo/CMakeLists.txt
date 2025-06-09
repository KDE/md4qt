# SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
# SPDX-License-Identifier: MIT

project(test.algo)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif(ENABLE_COVERAGE)

if(MSVC)
    add_compile_options(/bigobj)
    add_compile_options(/utf-8)
endif()

set(SRC main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty)

if(BUILD_MD4QT_QT_TESTS)
    find_package(Qt6Core REQUIRED)

    add_executable(test.algo.qt ${SRC})
    target_compile_definitions(test.algo.qt PUBLIC TRAIT=MD::QStringTrait)
    target_compile_definitions(test.algo.qt PUBLIC MD4QT_QT_SUPPORT)
    target_link_libraries(test.algo.qt Qt6::Core)

    add_test(NAME test.algo.qt
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../../../bin/test.algo.qt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../../bin)
endif()

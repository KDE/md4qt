# SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
# SPDX-License-Identifier: MIT

project(tests)

kde_enable_exceptions()

add_subdirectory(test_funcs)
add_subdirectory(test_parser)
add_subdirectory(commonmark)
add_subdirectory(gfm)
add_subdirectory(test_build)
add_subdirectory(internal_string)
add_subdirectory(algo)
add_subdirectory(poscache)
add_subdirectory(html)

# SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
# SPDX-License-Identifier: MIT

project(plugins)

kde_enable_exceptions()

if(BUILD_MD4QT_QT_TESTS)
    add_subdirectory(emphasis)
    add_subdirectory(yaml)
endif()

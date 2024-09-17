
/*
    SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

#define MD4QT_QT_SUPPORT
#define MD4QT_ICU_STL_SUPPORT
#include <md4qt/doc.h>
#include <md4qt/parser.h>

#include <QFile>

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm.h>
#include <registry.h>

int main(int argc, char **argv)
{
    // md4qt
    {
        const auto start = std::chrono::high_resolution_clock::now();

        MD::Parser<MD::UnicodeStringTrait> parser;

        const auto doc = parser.parse(MD::UnicodeString("tests/manual/complex.md"), false);

        const auto end = std::chrono::high_resolution_clock::now();

        const auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "md4qt with ICU parsing: " << d.count() << " us" << std::endl;
    }

    // md4qt
    {
        const auto start = std::chrono::high_resolution_clock::now();

        MD::Parser<MD::QStringTrait> parser;

        const auto doc = parser.parse(QStringLiteral("tests/manual/complex.md"), false);

        const auto end = std::chrono::high_resolution_clock::now();

        const auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "md4qt with Qt6 parsing: " << d.count() << " us" << std::endl;
    }

    // cmark-gfm
    {
        const auto start = std::chrono::high_resolution_clock::now();

        QFile file(QStringLiteral("tests/manual/complex.md"));

        if (file.open(QIODevice::ReadOnly)) {
            const auto md = file.readAll();

            file.close();

            cmark_gfm_core_extensions_ensure_registered();

            auto doc = cmark_parse_document(md.constData(), md.size(), CMARK_OPT_FOOTNOTES);

            cmark_node_free(doc);

            cmark_release_plugins();
        } else
            std::cout << "failed to open complex.md" << std::endl;

        const auto end = std::chrono::high_resolution_clock::now();

        const auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "cmark-gfm parsing: " << d.count() << " us" << std::endl;
    }

    return 0;
}

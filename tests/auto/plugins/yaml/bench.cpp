/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// Qt include.
#include <QObject>
#include <QTest>

// md4qt include.
#include <md4qt/parser.h>
#include <md4qt/plugins.h>

class Bench final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void with_yaml_plugin()
    {
        QBENCHMARK {
            MD::Parser<TRAIT> p;
            p.addBlockPlugin(std::make_shared<MD::YAMLBlockPlugin<TRAIT>>());
            p.parse(TRAIT::latin1ToString("tests/plugins/yaml/data/002.md"));
        }
    }

    void without_yaml_plugin()
    {
        QBENCHMARK {
            MD::Parser<TRAIT> p;
            p.parse(TRAIT::latin1ToString("tests/plugins/yaml/data/002.md"));
        }
    }
};

QTEST_GUILESS_MAIN(Bench)

#include "bench.moc"

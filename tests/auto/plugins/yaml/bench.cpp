/*
    SPDX-FileCopyrightText: 2026 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// Qt include.
#include <QObject>
#include <QTest>

// md4qt include.
#include "atx_heading_parser.h"
#include "blockquote_parser.h"
#include "fenced_code_parser.h"
#include "footnote_parser.h"
#include "html_parser.h"
#include "indented_code_parser.h"
#include "list_parser.h"
#include "paragraph_parser.h"
#include "parser.h"
#include "setext_heading_parser.h"
#include "table_parser.h"
#include "thematic_break_parser.h"

MD::Parser::BlockParsers makeBlockParsersPipelineWithoutYaml(MD::Parser *parser)
{
    MD::Parser::BlockParsers parsers;

    MD::Parser::appendBlockParser<MD::BlockquoteParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::SetextHeadingParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::ThematicBreakParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::ListParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::ATXHeadingParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::FencedCodeParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::HTMLParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::IndentedCodeParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::FootnoteParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::TableParser>(parsers, parser);
    MD::Parser::appendBlockParser<MD::ParagraphParser>(parsers, parser);

    return parsers;
}

class Bench final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void with_yaml_plugin()
    {
        QBENCHMARK {
            MD::Parser p;
            p.parse(QStringLiteral("tests/plugins/yaml/data/002.md"));
        }
    }

    void without_yaml_plugin()
    {
        QBENCHMARK {
            MD::Parser p;
            p.setBlockParsers(makeBlockParsersPipelineWithoutYaml(&p));
            p.parse(QStringLiteral("tests/plugins/yaml/data/002.md"));
        }
    }
};

QTEST_GUILESS_MAIN(Bench)

#include "bench.moc"

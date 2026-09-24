/*
    SPDX-FileCopyrightText: 2026 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// md4qt include.
#include "parser.h"
#include "yaml_parser.h"

// Qt include.
#include <QDirIterator>
#include <QString>
#include <QTextStream>

bool compare(QSharedPointer<MD::Block> original,
             QSharedPointer<MD::Block> serialised);

bool compare(QSharedPointer<MD::Item> original,
             QSharedPointer<MD::Item> serialised)
{
    REQUIRE(original->type() == serialised->type());

    switch (original->type()) {
    case MD::ItemType::Heading: {
        if (!compare(original.staticCast<MD::Heading>()->text().staticCast<MD::Block>(),
                     serialised.staticCast<MD::Heading>()->text().staticCast<MD::Block>())) {
            return false;
        }
    } break;

    case MD::ItemType::Text: {
        const auto ot = original.staticCast<MD::Text>();
        const auto st = serialised.staticCast<MD::Text>();

        REQUIRE(ot->text() == st->text());
    } break;

    case MD::ItemType::LineBreak: {
        return true;
    } break;

    case MD::ItemType::Link: {
        const auto ol = original.staticCast<MD::Link>();
        const auto sl = serialised.staticCast<MD::Link>();

        REQUIRE(ol->url() == sl->url());

        if (!compare(ol->p().staticCast<MD::Block>(), sl->p().staticCast<MD::Block>())) {
            return false;
        }
    } break;

    case MD::ItemType::Image: {
        const auto oi = original.staticCast<MD::Image>();
        const auto si = serialised.staticCast<MD::Image>();

        REQUIRE(oi->url() == si->url());

        if (!compare(oi->p().staticCast<MD::Block>(), si->p().staticCast<MD::Block>())) {
            return false;
        }
    } break;

    case MD::ItemType::Code: {
        const auto oc = original.staticCast<MD::Code>();
        const auto sc = serialised.staticCast<MD::Code>();

        REQUIRE(oc->text() == sc->text());
    } break;

    case MD::ItemType::Table: {
        const auto ot = original.staticCast<MD::Table>();
        const auto st = serialised.staticCast<MD::Table>();

        REQUIRE(ot->rows().size() == st->rows().size());
        REQUIRE(ot->columnsCount() == st->columnsCount());

        for (int i = 0; i < ot->columnsCount(); ++i) {
            REQUIRE(ot->columnAlignment(i) == st->columnAlignment(i));
        }

        for (int i = 0; i < ot->rows().count(); ++i) {
            const auto orow = ot->rows().at(i);
            const auto srow = st->rows().at(i);

            REQUIRE(orow->cells().size() == srow->cells().size());

            for (int i = 0; i < orow->cells().size(); ++i) {
                if (!compare(orow->cells().at(i).staticCast<MD::Block>(),
                             srow->cells().at(i).staticCast<MD::Block>())) {
                    return false;
                }
            }
        }
    } break;

    case MD::ItemType::FootnoteRef: {
        const auto oref = original.staticCast<MD::FootnoteRef>();
        const auto sref = serialised.staticCast<MD::FootnoteRef>();

        REQUIRE(oref->id() == sref->id());
    } break;

    case MD::ItemType::HorizontalLine: {
        return true;
    } break;

    case MD::ItemType::RawHtml: {
        const auto oh = original.staticCast<MD::RawHtml>();
        const auto sh = serialised.staticCast<MD::RawHtml>();

        REQUIRE(oh->text().simplified() == sh->text().simplified());
    } break;

    case MD::ItemType::Math: {
        const auto om = original.staticCast<MD::Math>();
        const auto sm = serialised.staticCast<MD::Math>();

        REQUIRE(om->expr().simplified() == sm->expr().simplified());
    } break;

    case MD::ItemType::Anchor: {
        const auto oa = original.staticCast<MD::Anchor>();
        const auto sa = serialised.staticCast<MD::Anchor>();

        REQUIRE(oa->label() == sa->label());
    } break;

    case static_cast<MD::ItemType>(static_cast<int>(MD::ItemType::UserDefined) + 1): {
        const auto oy = original.staticCast<MD::YAMLHeader>();
        const auto sy = serialised.staticCast<MD::YAMLHeader>();

        REQUIRE(oy->yaml() == sy->yaml());
    } break;

    default:
        return false;
    }

    return true;
}

bool compare(QSharedPointer<MD::Block> original,
             QSharedPointer<MD::Block> serialised)
{
    REQUIRE(original->type() == serialised->type());
    REQUIRE(original->items().count() == serialised->items().count());

    for (int i = 0; i < original->items().count(); ++i) {
        switch (original->items().at(i)->type()) {
        case MD::ItemType::Paragraph:
        case MD::ItemType::Blockquote:
        case MD::ItemType::List:
        case MD::ItemType::ListItem:
        case MD::ItemType::Footnote: {
            if (!compare(original->items().at(i).staticCast<MD::Block>(),
                         serialised->items().at(i).staticCast<MD::Block>())) {
                return false;
            }
        } break;

        default: {
            if (!compare(original->items().at(i), serialised->items().at(i))) {
                return false;
            }
        } break;
        }
    }

    return true;
}

bool compare(QSharedPointer<MD::Document> original,
             QSharedPointer<MD::Document> serialised)
{
    if (!compare(original.staticCast<MD::Block>(), serialised.staticCast<MD::Block>())) {
        return false;
    }

    REQUIRE(original->footnotesMap().size() == serialised->footnotesMap().size());

    for (auto it = original->footnotesMap().cbegin(), last = original->footnotesMap().cend(); it != last; ++it) {
        REQUIRE(serialised->footnotesMap().contains(it.key()));

        if (!compare(it.value().m_footnote.staticCast<MD::Block>(),
                     serialised->footnotesMap()[it.key()].m_footnote.staticCast<MD::Block>())) {
            return false;
        }
    }

    for (auto it = original->labeledLinks().cbegin(), last = original->labeledLinks().cend(); it != last; ++it) {
        REQUIRE(serialised->labeledLinks().contains(it.key()));

        if (!compare(it.value().staticCast<MD::Item>(), serialised->labeledLinks()[it.key()].staticCast<MD::Item>())) {
            return false;
        }
    }

    for (auto it = original->labeledHeadings().cbegin(), last = original->labeledHeadings().cend(); it != last; ++it) {
        REQUIRE(serialised->labeledHeadings().contains(it.key()));

        if (!compare(it.value().staticCast<MD::Item>(),
                     serialised->labeledHeadings()[it.key()].staticCast<MD::Item>())) {
            return false;
        }
    }

    return true;
}

void checkDirectory(QDirIterator it,
                    MD::Parser &p)
{
    while (it.hasNext()) {
        const QString file = it.next();

        if (file.endsWith(QStringLiteral(".md"))) {
            const QFileInfo info(file);
            auto originalDoc = p.parse(file, false);
            QString serialised;
            QTextStream stream(&serialised);
            stream << *originalDoc;
            stream.seek(0);
            const auto serialisedDoc = p.parse(stream, info.absolutePath(), info.fileName());
            REQUIRE(compare(originalDoc, serialisedDoc));
        }
    }
}

TEST_CASE("parser")
{
    MD::Parser p;

    checkDirectory(QDirIterator(QStringLiteral("tests/parser/data"), QDir::Files, QDirIterator::NoIteratorFlags), p);
}

TEST_CASE("commonmark/0.30")
{
    MD::Parser p;
    p.setAutolinkUriValidation(MD::Parser::AutolinkUriValidation::CommonMark);
    p.setBlockParsers(MD::Parser::makeCommonMarkBlockParsersPipeline(&p));
    p.setInlineParsers(MD::Parser::makeCommonMarkInlineParsersPipeline());

    checkDirectory(QDirIterator(QStringLiteral("tests/commonmark/0.30"), QDir::Files, QDirIterator::NoIteratorFlags),
                   p);
}

TEST_CASE("commonmark/0.31.2")
{
    MD::Parser p;
    p.setAutolinkUriValidation(MD::Parser::AutolinkUriValidation::CommonMark);
    p.setBlockParsers(MD::Parser::makeCommonMarkBlockParsersPipeline(&p));
    p.setInlineParsers(MD::Parser::makeCommonMarkInlineParsersPipeline());

    checkDirectory(QDirIterator(QStringLiteral("tests/commonmark/0.31.2"), QDir::Files, QDirIterator::NoIteratorFlags),
                   p);
}

TEST_CASE("gfm")
{
    MD::Parser p;

    checkDirectory(QDirIterator(QStringLiteral("tests/gfm/data"), QDir::Files, QDirIterator::NoIteratorFlags), p);
}

TEST_CASE("html")
{
    MD::Parser p;

    checkDirectory(QDirIterator(QStringLiteral("tests/html/data"), QDir::Files, QDirIterator::NoIteratorFlags), p);
}

TEST_CASE("yaml")
{
    MD::Parser p;

    checkDirectory(QDirIterator(QStringLiteral("tests/plugins/yaml/data"), QDir::Files, QDirIterator::NoIteratorFlags),
                   p);
}

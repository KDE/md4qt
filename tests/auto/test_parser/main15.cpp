/*
    SPDX-FileCopyrightText: 2026 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// md4qt include.
#include "parser.h"

#include "test_utils.h"

/*
> 1. first
> 2. second
>    1. nested
>    2. another nested item
> 3. third

*/
TEST_CASE("376")
{
    MD::Parser parser;

    auto doc = parser.parse(QStringLiteral("tests/parser/data/376.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Blockquote);
    auto b = static_cast<MD::Blockquote *>(doc->items().at(1).get());
    REQUIRE(b->items().size() == 1);
    REQUIRE(b->items().at(0)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List *>(b->items().at(0).get());
    REQUIRE(l->items().size() == 3);
    REQUIRE(l->items().at(1)->type() == MD::ItemType::ListItem);
    auto li = static_cast<MD::ListItem *>(l->items().at(1).get());
    REQUIRE(li->items().size() == 2);
    REQUIRE(li->items().at(0)->type() == MD::ItemType::Paragraph);
    REQUIRE(li->items().at(1)->type() == MD::ItemType::List);
    auto nl = static_cast<MD::List *>(li->items().at(1).get());
    REQUIRE(nl->items().size() == 2);
}

/*
a*b**

*/
TEST_CASE("377")
{
    MD::Parser parser;

    auto doc = parser.parse(QStringLiteral("tests/parser/data/377.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph *>(doc->items().at(1).get());
    REQUIRE(p->items().size() == 1);
    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text *>(p->items().at(0).get());
    REQUIRE(t->text() == QStringLiteral("a*b**"));
}

/*
x *b**y

*/
TEST_CASE("378")
{
    MD::Parser parser;

    auto doc = parser.parse(QStringLiteral("tests/parser/data/378.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph *>(doc->items().at(1).get());
    REQUIRE(p->items().size() == 1);
    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text *>(p->items().at(0).get());
    REQUIRE(t->text() == QStringLiteral("x *b**y"));
}

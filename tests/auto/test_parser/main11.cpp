
/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

/*
*   text
   * text
   *


*/
TEST_CASE("312")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/312.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(l->items().size() == 3);
}

/*
>

*/
TEST_CASE("313")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/313.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Blockquote);
    auto b = static_cast<MD::Blockquote<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(b->startColumn() == 0);
    REQUIRE(b->startLine() == 0);
    REQUIRE(b->endColumn() == 1);
    REQUIRE(b->endLine() == 0);
}

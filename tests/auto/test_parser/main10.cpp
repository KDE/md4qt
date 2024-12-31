
/*
    SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

/*
 - 1
  - 2
   - 3
    - 4
     - 5
      - 6
       - 7
      - 6
     -
    - 4
   - 3
  - 2
 - 1

*/
TEST_CASE("281")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/281.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 4);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT>*>(doc->items().at(1).get());
    REQUIRE(l->items().size() == 3);
    REQUIRE(l->items().at(2)->type() == MD::ItemType::ListItem);

    {
        auto li = static_cast<MD::ListItem<TRAIT>*>(l->items().at(2).get());
        REQUIRE(li->items().size() == 2);

        REQUIRE(li->items().at(1)->type() == MD::ItemType::List);
        auto l = static_cast<MD::List<TRAIT>*>(li->items().at(1).get());
        REQUIRE(l->items().size() == 5);
    }

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Code);

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::List);
        auto l = static_cast<MD::List<TRAIT>*>(doc->items().at(3).get());
        REQUIRE(l->items().size() == 3);
    }
}

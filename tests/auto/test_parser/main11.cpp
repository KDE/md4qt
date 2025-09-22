
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

/*
* list

  ```cpp
  code
*

  ```

*/
TEST_CASE("314")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/314.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(l->items().size() == 2);
    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Code);
}

/*
*

  ```cpp
  code
*

  ```

*/
TEST_CASE("315")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/315.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(l->items().size() == 1);
    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Code);
}

/*
*


*

*/
TEST_CASE("316")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/316.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(l->items().size() == 2);
}

/*
> ```cpp
>

*/
TEST_CASE("317")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/317.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Blockquote);
    auto b = static_cast<MD::Blockquote<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(b->startColumn() == 0);
    REQUIRE(b->startLine() == 0);
    REQUIRE(b->endColumn() == 1);
    REQUIRE(b->endLine() == 1);
    REQUIRE(b->items().size() == 1);
    REQUIRE(b->items().at(0)->type() == MD::ItemType::Code);
    auto c = static_cast<MD::Code<TRAIT> *>(b->items().at(0).get());
    REQUIRE(c->startColumn() == 2);
    REQUIRE(c->startLine() == 1);
    REQUIRE(c->endColumn() == 2);
    REQUIRE(c->endLine() == 1);
    REQUIRE(c->text().isEmpty());
}

/*
```cpp

*/
TEST_CASE("318")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/318.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Code);
    auto c = static_cast<MD::Code<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(c->startColumn() == 6);
    REQUIRE(c->startLine() == 0);
    REQUIRE(c->endColumn() == 6);
    REQUIRE(c->endLine() == 0);
}

/*
### ---

*/
TEST_CASE("319")
{
    MD::Parser<TRAIT> parser;

    auto doc = parser.parse(TRAIT::latin1ToString("tests/parser/data/319.md"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);
    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Heading);
    auto h = static_cast<MD::Heading<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->startColumn() == 0);
    REQUIRE(h->startLine() == 0);
    REQUIRE(h->endColumn() == 6);
    REQUIRE(h->endLine() == 0);
    REQUIRE(h->text()->items().size() == 1);
    REQUIRE(h->text()->startColumn() == 4);
    REQUIRE(h->text()->startLine() == 0);
    REQUIRE(h->text()->endColumn() == 6);
    REQUIRE(h->text()->endLine() == 0);
    REQUIRE(h->text()->items().at(0)->type() == MD::ItemType::HorizontalLine);
    auto l = static_cast<MD::HorizontalLine<TRAIT> *>(h->text()->items().at(0).get());
    REQUIRE(l->startColumn() == 4);
    REQUIRE(l->startLine() == 0);
    REQUIRE(l->endColumn() == 6);
    REQUIRE(l->endLine() == 0);
}

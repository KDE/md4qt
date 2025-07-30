
/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// md4qt include.
#include <md4qt/parser.h>
#include <md4qt/plugins.h>

template<class Trait>
std::shared_ptr<MD::Document<Trait>> loadTest(const typename Trait::String &name)
{
    MD::Parser<TRAIT> p;
    p.addBlockPlugin(std::make_shared<MD::YAMLBlockPlugin<Trait>>());
    return p.parse(Trait::latin1ToString("tests/plugins/yaml/data/") + name + Trait::latin1ToString(".md"));
}

TEST_CASE("001")
{
    auto doc = loadTest<TRAIT>(TRAIT::latin1ToString("001"));
    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 1);
    REQUIRE(doc->items().at(0)->type() == MD::ItemType::Anchor);
}

/*


---
id: 1
...
text

*/
TEST_CASE("002")
{
    auto doc = loadTest<TRAIT>(TRAIT::latin1ToString("002"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    REQUIRE(doc->items().at(1)->type() == static_cast<MD::ItemType>(static_cast<int>(MD::ItemType::UserDefined) + 1));
    auto h = static_cast<MD::YAMLHeader<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->startColumn() == 0);
    REQUIRE(h->startLine() == 2);
    REQUIRE(h->endColumn() == 3);
    REQUIRE(h->endLine() == 4);
    REQUIRE(h->yaml() == TRAIT::latin1ToString("id: 1\n"));
    REQUIRE(h->startDelim() == MD::WithPosition{0, 2, 2, 2});
    REQUIRE(h->endDelim() == MD::WithPosition{0, 4, 3, 4});

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);

    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);

    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->startColumn() == 0);
    REQUIRE(t->startLine() == 5);
    REQUIRE(t->endColumn() == 3);
    REQUIRE(t->endLine() == 5);
    REQUIRE(t->text() == TRAIT::latin1ToString("text"));
}

/*
---
id: 1
...
text

*/
TEST_CASE("003")
{
    auto doc = loadTest<TRAIT>(TRAIT::latin1ToString("003"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    REQUIRE(doc->items().at(1)->type() == static_cast<MD::ItemType>(static_cast<int>(MD::ItemType::UserDefined) + 1));
    auto h = static_cast<MD::YAMLHeader<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->startColumn() == 0);
    REQUIRE(h->startLine() == 0);
    REQUIRE(h->endColumn() == 3);
    REQUIRE(h->endLine() == 2);
    REQUIRE(h->yaml() == TRAIT::latin1ToString("id: 1\n"));
    REQUIRE(h->startDelim() == MD::WithPosition{0, 0, 2, 0});
    REQUIRE(h->endDelim() == MD::WithPosition{0, 2, 3, 2});

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);

    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);

    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->startColumn() == 0);
    REQUIRE(t->startLine() == 3);
    REQUIRE(t->endColumn() == 3);
    REQUIRE(t->endLine() == 3);
    REQUIRE(t->text() == TRAIT::latin1ToString("text"));
}

/*
text
---
id: 1
...
text

*/
TEST_CASE("004")
{
    auto doc = loadTest<TRAIT>(TRAIT::latin1ToString("004"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Heading);
    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
}

/*
> ---
> id: 1
> ...
> text

*/
TEST_CASE("005")
{
    auto doc = loadTest<TRAIT>(TRAIT::latin1ToString("005"));

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Blockquote);
    auto b = static_cast<MD::Blockquote<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(b->items().size() == 2);
    REQUIRE(b->items().at(0)->type() == MD::ItemType::HorizontalLine);
    REQUIRE(b->items().at(1)->type() == MD::ItemType::Paragraph);
}

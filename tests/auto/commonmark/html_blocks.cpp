
/*
    SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// 4.6 HTML blocks

TEST_CASE("148")
{
    const auto doc = load_test(148);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 4);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<table><tr><td>\n<pre>\n**Hello**,"));
    }

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 3);

    {
        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::ItalicText);
        REQUIRE(t->text() == TRAIT::latin1ToString("world"));
    }

    {
        REQUIRE(p->items().at(1)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(1).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("."));
    }

    {
        REQUIRE(p->items().at(2)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(p->items().at(2).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</pre>"));
    }

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</td></tr></table>"));
    }
}

TEST_CASE("149")
{
    const auto doc = load_test(149);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text()
                == TRAIT::latin1ToString("<table>\n  <tr>\n    <td>\n"
                                         "           hi\n    </td>\n  </tr>\n</table>"));
    }

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);

    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->opts() == MD::TextWithoutFormat);
    REQUIRE(t->text() == TRAIT::latin1ToString("okay."));
}

TEST_CASE("150")
{
    const auto doc = load_test(150);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString(" <div>\n  *hello*\n         <foo><a>"));
    }
}

TEST_CASE("151")
{
    const auto doc = load_test(151);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</div>\n*foo*"));
    }
}

TEST_CASE("152")
{
    const auto doc = load_test(152);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 4);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<DIV CLASS=\"foo\">"));
    }

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);

    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->opts() == MD::ItalicText);
    REQUIRE(t->text() == TRAIT::latin1ToString("Markdown"));

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</DIV>"));
    }
}

TEST_CASE("153")
{
    const auto doc = load_test(153);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div id=\"foo\"\n  class=\"bar\">\n</div>"));
    }
}

TEST_CASE("154")
{
    const auto doc = load_test(154);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div id=\"foo\" class=\"bar\n  baz\">\n</div>"));
    }
}

TEST_CASE("155")
{
    const auto doc = load_test(155);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div>\n*foo*"));
    }

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);

    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->opts() == MD::ItalicText);
    REQUIRE(t->text() == TRAIT::latin1ToString("bar"));
}

TEST_CASE("156")
{
    const auto doc = load_test(156);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div id=\"foo\"\n*hi*"));
    }
}

TEST_CASE("157")
{
    const auto doc = load_test(157);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div class\nfoo"));
    }
}

TEST_CASE("158")
{
    const auto doc = load_test(158);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div *?\?\?-&&&-<---\n*foo*"));
    }
}

TEST_CASE("159")
{
    const auto doc = load_test(159);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div><a href=\"bar\">*foo*</a></div>"));
    }
}

TEST_CASE("160")
{
    const auto doc = load_test(160);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<table><tr><td>\nfoo\n</td></tr></table>"));
    }
}

TEST_CASE("161")
{
    const auto doc = load_test(161);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div></div>\n``` c\nint x = 33;\n```"));
    }
}

TEST_CASE("162")
{
    const auto doc = load_test(162);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<a href=\"foo\">\n*bar*\n</a>"));
}

TEST_CASE("163")
{
    const auto doc = load_test(163);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<Warning>\n*bar*\n</Warning>"));
}

TEST_CASE("164")
{
    const auto doc = load_test(164);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<i class=\"foo\">\n*bar*\n</i>"));
}

TEST_CASE("165")
{
    const auto doc = load_test(165);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("</ins>\n*bar*"));
}

TEST_CASE("166")
{
    const auto doc = load_test(166);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<del>\n*foo*\n</del>"));
}

TEST_CASE("167")
{
    const auto doc = load_test(167);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 4);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<del>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::ItalicText);
        REQUIRE(t->text() == TRAIT::latin1ToString("foo"));
    }

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</del>"));
    }
}

TEST_CASE("168")
{
    const auto doc = load_test(168);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(p->items().size() == 3);

        {
            REQUIRE(p->items().at(0)->type() == MD::ItemType::RawHtml);
            auto h = static_cast<MD::RawHtml<TRAIT> *>(p->items().at(0).get());
            REQUIRE(h->text() == TRAIT::latin1ToString("<del>"));
        }

        REQUIRE(p->items().at(1)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(1).get());
        REQUIRE(t->opts() == MD::ItalicText);
        REQUIRE(t->text() == TRAIT::latin1ToString("foo"));

        {
            REQUIRE(p->items().at(2)->type() == MD::ItemType::RawHtml);
            auto h = static_cast<MD::RawHtml<TRAIT> *>(p->items().at(2).get());
            REQUIRE(h->text() == TRAIT::latin1ToString("</del>"));
        }
    }
}

TEST_CASE("169")
{
    const auto doc = load_test(169);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text()
                == TRAIT::latin1ToString("<pre language=\"haskell\"><code>\n"
                                         "import Text.HTML.TagSoup\n\n"
                                         "main :: IO ()\n"
                                         "main = print $ parseTags tags\n"
                                         "</code></pre>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("170")
{
    const auto doc = load_test(170);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text()
                == TRAIT::latin1ToString("<script type=\"text/javascript\">\n"
                                         "// JavaScript example\n\n"
                                         "document.getElementById(\"demo\").innerHTML = \"Hello JavaScript!\";\n"
                                         "</script>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("171")
{
    const auto doc = load_test(171);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text()
            == TRAIT::latin1ToString("<textarea>\n\n"
                                     "*foo*\n\n"
                                     "_bar_\n\n"
                                     "</textarea>"));
}

TEST_CASE("172")
{
    const auto doc = load_test(172);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text()
                == TRAIT::latin1ToString("<style\n"
                                         "  type=\"text/css\">\n"
                                         "h1 {color:red;}\n\n"
                                         "p {color:blue;}\n"
                                         "</style>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("173")
{
    const auto doc = load_test(173);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text()
            == TRAIT::latin1ToString("<style\n"
                                     "  type=\"text/css\">\n\n"
                                     "foo"));
}

TEST_CASE("174")
{
    const auto doc = load_test(174);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Blockquote);
    auto b = static_cast<MD::Blockquote<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(b->items().size() == 1);

    REQUIRE(b->items().at(0)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(b->items().at(0).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<div>\nfoo"));

    REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
    REQUIRE(p->items().size() == 1);
    REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
    auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
    REQUIRE(t->opts() == MD::TextWithoutFormat);
    REQUIRE(t->text() == TRAIT::latin1ToString("bar"));
}

TEST_CASE("175")
{
    const auto doc = load_test(175);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::List);
    auto l = static_cast<MD::List<TRAIT> *>(doc->items().at(1).get());

    REQUIRE(l->items().size() == 2);

    {
        REQUIRE(l->items().at(0)->type() == MD::ItemType::ListItem);
        auto li = static_cast<MD::ListItem<TRAIT> *>(l->items().at(0).get());

        REQUIRE(li->items().at(0)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(li->items().at(0).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div>"));
    }

    {
        REQUIRE(l->items().at(1)->type() == MD::ItemType::ListItem);
        auto li = static_cast<MD::ListItem<TRAIT> *>(l->items().at(1).get());

        REQUIRE(li->items().at(0)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(li->items().at(0).get());

        REQUIRE(p->items().size() == 1);
        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("foo"));
    }
}

TEST_CASE("176")
{
    const auto doc = load_test(176);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<style>p{color:red;}</style>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::ItalicText);
        REQUIRE(t->text() == TRAIT::latin1ToString("foo"));
    }
}

TEST_CASE("177")
{
    const auto doc = load_test(177);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<!-- foo -->*bar*"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::ItalicText);
        REQUIRE(t->text() == TRAIT::latin1ToString("baz"));
    }
}

TEST_CASE("178")
{
    const auto doc = load_test(178);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<script>\nfoo\n</script>1. *bar*"));
}

TEST_CASE("179")
{
    const auto doc = load_test(179);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<!-- Foo\n\nbar\n   baz -->"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("180")
{
    const auto doc = load_test(180);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<?php\n\n  echo '>';\n\n?>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("181")
{
    const auto doc = load_test(181);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<!DOCTYPE html>"));
}

TEST_CASE("182")
{
    const auto doc = load_test(182);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text()
                == TRAIT::latin1ToString("<![CDATA[\n"
                                         "function matchwo(a,b)\n"
                                         "{\n"
                                         "  if (a < b && a < 0) then {\n"
                                         "    return 1;\n\n"
                                         "  } else {\n\n"
                                         "    return 0;\n"
                                         "  }\n"
                                         "}\n"
                                         "]]>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("okay"));
    }
}

TEST_CASE("183")
{
    const auto doc = load_test(183);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("  <!-- foo -->"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Code);
        auto c = static_cast<MD::Code<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(c->text() == TRAIT::latin1ToString("<!-- foo -->"));
    }
}

TEST_CASE("184")
{
    const auto doc = load_test(184);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("  <div>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Code);
        auto c = static_cast<MD::Code<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(c->text() == TRAIT::latin1ToString("<div>"));
    }
}

TEST_CASE("185")
{
    const auto doc = load_test(185);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 3);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(p->items().size() == 1);

        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("Foo"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div>\nbar\n</div>"));
    }
}

TEST_CASE("186")
{
    const auto doc = load_test(186);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<div>\nbar\n</div>\n*foo*"));
}

TEST_CASE("187")
{
    const auto doc = load_test(187);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::Paragraph);
    auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(p->items().size() == 3);

    {
        REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("Foo"));
    }

    REQUIRE(p->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(p->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<a href=\"bar\">"));

    {
        REQUIRE(p->items().at(2)->type() == MD::ItemType::Text);
        auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(2).get());
        REQUIRE(t->opts() == MD::TextWithoutFormat);
        REQUIRE(t->text() == TRAIT::latin1ToString("baz"));
    }
}

TEST_CASE("188")
{
    const auto doc = load_test(188);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 4);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<div>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::Paragraph);
        auto p = static_cast<MD::Paragraph<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(p->items().size() == 2);

        {
            REQUIRE(p->items().at(0)->type() == MD::ItemType::Text);
            auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(0).get());
            REQUIRE(t->opts() == MD::ItalicText);
            REQUIRE(t->text() == TRAIT::latin1ToString("Emphasized"));
        }

        {
            REQUIRE(p->items().at(1)->type() == MD::ItemType::Text);
            auto t = static_cast<MD::Text<TRAIT> *>(p->items().at(1).get());
            REQUIRE(t->opts() == MD::TextWithoutFormat);
            REQUIRE(t->text() == TRAIT::latin1ToString("text."));
        }
    }

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</div>"));
    }
}

TEST_CASE("189")
{
    const auto doc = load_test(189);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 2);

    REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
    auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
    REQUIRE(h->text() == TRAIT::latin1ToString("<div>\n*Emphasized* text.\n</div>"));
}

TEST_CASE("190")
{
    const auto doc = load_test(190);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 6);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<table>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<tr>"));
    }

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<td>\nHi\n</td>"));
    }

    {
        REQUIRE(doc->items().at(4)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(4).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</tr>"));
    }

    {
        REQUIRE(doc->items().at(5)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(5).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</table>"));
    }
}

TEST_CASE("191")
{
    const auto doc = load_test(191);

    REQUIRE(doc->isEmpty() == false);
    REQUIRE(doc->items().size() == 6);

    {
        REQUIRE(doc->items().at(1)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(1).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("<table>"));
    }

    {
        REQUIRE(doc->items().at(2)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(2).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("  <tr>"));
    }

    {
        REQUIRE(doc->items().at(3)->type() == MD::ItemType::Code);
        auto c = static_cast<MD::Code<TRAIT> *>(doc->items().at(3).get());
        REQUIRE(c->text() == TRAIT::latin1ToString("<td>\n  Hi\n</td>"));
    }

    {
        REQUIRE(doc->items().at(4)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(4).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("  </tr>"));
    }

    {
        REQUIRE(doc->items().at(5)->type() == MD::ItemType::RawHtml);
        auto h = static_cast<MD::RawHtml<TRAIT> *>(doc->items().at(5).get());
        REQUIRE(h->text() == TRAIT::latin1ToString("</table>"));
    }
}


/*
    SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// doctest include.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

typename TRAIT::String fullPath(int num)
{
    typename TRAIT::String wd =
#ifdef MD4QT_QT_SUPPORT
        QDir().absolutePath()
#else
        typename TRAIT::String(std::filesystem::canonical(std::filesystem::current_path()).u8string())
#endif
        + TRAIT::latin1ToString("/tests/html/data/");

    auto numStr = std::to_string(num);
    numStr = std::string(3 - numStr.size(), '0') + numStr;

    wd += TRAIT::latin1ToString(numStr.c_str()) + TRAIT::latin1ToString(".md");

#ifndef MD4QT_QT_SUPPORT
    std::string tmp;
    wd.toUTF8String(tmp);
    std::replace(tmp.begin(), tmp.end(), '\\', '/');
    wd = icu::UnicodeString::fromUTF8(tmp);
#endif

    return wd;
}

/*
**[google](www.google.com)* text*

*/
TEST_CASE("001")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/001.md")), false, {}, false);
    REQUIRE(html
            == TRAIT::latin1ToString("<p><em><em><a href=\"www.google.com\"> google </a>"
                                     "</em> text </em></p>"));
}

/*
**text* text*

*/
TEST_CASE("002")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/002.md")), false, {}, false);
    REQUIRE(html == TRAIT::latin1ToString("<p><em><em> text text </em></em></p>"));
}

/*
# heading

*/
TEST_CASE("003")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/003.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("\n<h1 id=\"heading/") + fullPath(3) + TRAIT::latin1ToString("\"> heading </h1>\n");
    REQUIRE(html == required);
}

/*
| heading1 | heading2 | heading3 |
| :--- | :---: | ---: |
| data1 | data2 | data3 |

*/
TEST_CASE("004")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/004.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
        "\n<table><thead><tr>\n"
        "<th align=\"left\">\n heading1 \n</th>\n"
        "<th align=\"center\">\n heading2 \n</th>\n"
        "<th align=\"right\">\n heading3 \n</th>\n"
        "</tr></thead><tbody>\n<tr>\n\n"
        "<td align=\"left\">\n data1 \n</td>\n\n"
        "<td align=\"center\">\n data2 \n</td>\n\n"
        "<td align=\"right\">\n data3 \n</td>\n\n</tr>\n</tbody></table>\n");
    REQUIRE(html == required);
}

/*
**bold** ~strike~

*/
TEST_CASE("005")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/005.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("<p><strong> bold </strong><del> strike </del></p>");
    REQUIRE(html == required);
}

/*
$a \ne 0$

*/
TEST_CASE("006")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/006.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("<p>$ a \\ne 0 $</p>");
    REQUIRE(html == required);
}

/*
```cpp
int i = 0;
```

*/
TEST_CASE("007")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/007.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("\n<pre><code class=\"language-cpp\">int i = 0;</code></pre>\n");
    REQUIRE(html == required);
}

/*
`code`

*/
TEST_CASE("008")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/008.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("<p><code>code</code></p>");
    REQUIRE(html == required);
}

/*
> blockquote

*/
TEST_CASE("009")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/009.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("\n<blockquote><p> blockquote </p></blockquote>\n");
    REQUIRE(html == required);
}

/*
* list

<!-- -->

1. list


*/
TEST_CASE("010")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/010.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
        "\n<ul>\n<li>\n list </li>\n</ul>\n"
        "<!-- -->\n<ol>\n<li value=\"1\">\n list </li>\n</ol>\n");
    REQUIRE(html == required);
}

/*
* [ ] task
1. [x] list

*/
TEST_CASE("011")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/011.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
        "\n<ul class=\"contains-task-list\">\n"
        "<li class=\"task-list-item\"><input type=\"checkbox\" id=\"\" "
        "disabled=\"\" class=\"task-list-item-checkbox\">\n"
        " task </li>\n</ul>\n\n"
        "<ol class=\"contains-task-list\">\n"
        "<li class=\"task-list-item\"><input type=\"checkbox\" id=\"\" "
        "disabled=\"\" class=\"task-list-item-checkbox\" checked=\"\" value=\"1\">\n"
        " list </li>\n</ol>\n");
    REQUIRE(html == required);
}

/*
text[^1]

[^1]: footnote

*/
TEST_CASE("012")
{
    const auto path = fullPath(12);
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/012.md")), false, TRAIT::latin1ToString("qrc://ref.png"), false);
    const auto required = TRAIT::latin1ToString("<p> text<sup><a href=\"##^1/") + path + TRAIT::latin1ToString("\" id=\"ref-#^1/") + path
        + TRAIT::latin1ToString(
                              "-1\">1</a></sup></p>"
                              "<section class=\"footnotes\"><ol><li id=\"#^1/")
        + path + TRAIT::latin1ToString("\"><p> footnote <a href=\"#ref-#^1/") + path
        + TRAIT::latin1ToString("-1\"><img src=\"qrc://ref.png\" /></a></p></li></ol></section>\n");
    REQUIRE(html == required);
}

/*
![](https://www.google.com)

*/
TEST_CASE("013")
{
    const auto path = fullPath(13);
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/013.md")), false, {}, true);
    const auto required = TRAIT::latin1ToString(
                              "<article class=\"markdown-body\">\n"
                              "<div id=\"")
        + path
        + TRAIT::latin1ToString(
                              "\"></div>\n<p>"
                              "<img src=\"https://www.google.com\" alt=\"\" style=\"max-width:100%;\" /></p></article>\n");
    REQUIRE(html == required);
}

/*
text<space><space>
text
text
___


*/
TEST_CASE("014")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/014.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString("<p> text <br />\n text \n text </p><hr />");
    REQUIRE(html == required);
}

/*
| h1 | h2 |
| - | - |
| d1 |


*/
TEST_CASE("015")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/015.md")), true, {}, false);
    const auto required = TRAIT::latin1ToString(
        "<!DOCTYPE html>\n<html><head></head><body>\n\n"
        "<table><thead><tr>\n<th align=\"left\">\n h1 \n</th>\n"
        "<th align=\"left\">\n h2 \n</th>\n</tr></thead><tbody>\n"
        "<tr>\n\n<td align=\"left\">\n d1 \n</td>\n<td></td>\n</tr>\n"
        "</tbody></table>\n</body></html>\n");
    REQUIRE(html == required);
}

/*
> # heading
>
>     code
>
> * list
>
> | h |
> | - |
> | d |
>
> ---
>
> <table></table>
>
> text <a></a>
>
> > nested quote

*/
TEST_CASE("016")
{
    const auto path = fullPath(16);
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/016.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
                              "\n<blockquote>\n"
                              "<h1 id=\"heading/")
        + path
        + TRAIT::latin1ToString(
                              "\"> heading </h1>\n\n"
                              "<pre><code>code</code></pre>\n\n"
                              "<ul>\n<li>\n list </li>\n</ul>\n\n"
                              "<table><thead><tr>\n<th align=\"left\">\n h \n</th>\n</tr></thead>"
                              "<tbody>\n<tr>\n\n<td align=\"left\">\n d \n</td>\n\n</tr>\n</tbody></table>\n"
                              "<hr />"
                              "<table></table>"
                              "<p> text <a></a></p>\n"
                              "<blockquote><p> nested quote </p></blockquote>\n"
                              "</blockquote>\n");
    REQUIRE(html == required);
}

/*
* list

  # Heading

      code

  > quote

  | t |
  | - |
  | d |

  <div></div>

  ___


*/
TEST_CASE("017")
{
    const auto path = fullPath(17);
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/017.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
                              "\n<ul>\n<li>\n<p> list </p>\n"
                              "<h1 id=\"heading/")
        + path
        + TRAIT::latin1ToString(
                              "\"> Heading </h1>\n\n"
                              "<pre><code>code</code></pre>\n\n"
                              "<blockquote><p> quote </p></blockquote>\n\n"
                              "<table><thead><tr>\n<th align=\"left\">\n t \n</th>\n</tr></thead>"
                              "<tbody>\n<tr>\n\n<td align=\"left\">\n d \n</td>\n\n</tr>\n</tbody></table>\n"
                              "<div></div><hr /></li>\n</ul>\n");
    REQUIRE(html == required);
}

/*
| head |
| ---- |
| `code` [google](https://www.google.com)[^1] ![](https://www.google.com) $a /ne 0$ |

[^1]: # heading

        code

    > quote

    * list

    | t |
    | - |
    | d |

    <div></div>

    ___


*/
TEST_CASE("018")
{
    const auto path = fullPath(18);
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/018.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
                              "\n<table><thead><tr>\n<th align=\"left\">\n head \n</th>\n</tr></thead>"
                              "<tbody>\n<tr>\n\n<td align=\"left\">\n"
                              "<code>code</code>"
                              "<a href=\"https://www.google.com\"> google </a>"
                              "<sup><a href=\"##^1/")
        + path + TRAIT::latin1ToString("\" id=\"ref-#^1/") + path
        + TRAIT::latin1ToString(
                              "-1\">1</a></sup>"
                              "<img src=\"https://www.google.com\" alt=\"\" style=\"max-width:100%;\" />"
                              "$ a /ne 0 $\n</td>\n\n</tr>\n</tbody></table>\n"
                              "<section class=\"footnotes\"><ol><li id=\"#^1/")
        + path
        + TRAIT::latin1ToString(
                              "\">\n"
                              "<h1 id=\"heading/")
        + path
        + TRAIT::latin1ToString(
                              "\"> heading </h1>\n\n"
                              "<pre><code>code</code></pre>\n\n"
                              "<blockquote><p> quote </p></blockquote>\n\n"
                              "<ul>\n<li>\n list </li>\n</ul>\n\n"
                              "<table><thead><tr>\n<th align=\"left\">\n t \n</th>\n</tr></thead>"
                              "<tbody>\n<tr>\n\n<td align=\"left\">\n d \n</td>\n\n</tr>\n</tbody></table>\n"
                              "<div></div><hr /></li></ol></section>\n");
    REQUIRE(html == required);
}

/*
+ Create a list by starting a line with `+`, `-`, or `*`
+ Sub-lists are made by indenting 2 spaces:
  - Marker character change forces new list start:
    * Ac tristique libero volutpat at
    + Facilisis in pretium nisl aliquet
    - Nulla volutpat aliquam velit
+ Very easy!

*/
TEST_CASE("019")
{
    MD::Parser<TRAIT> p;
    auto html = MD::toHtml(p.parse(TRAIT::latin1ToString("tests/html/data/019.md")), false, {}, false);
    const auto required = TRAIT::latin1ToString(
        "\n<ul>\n<li>\n Create a list by starting a line with "
        "<code>+</code>, <code>-</code>, or <code>*</code></li>\n"
        "<li>\n Sub-lists are made by indenting 2 spaces: \n"
        "<ul>\n<li>\n Marker character change forces new list start: \n"
        "<ul>\n<li>\n Ac tristique libero volutpat at </li>\n</ul>\n\n"
        "<ul>\n<li>\n Facilisis in pretium nisl aliquet </li>\n</ul>\n\n"
        "<ul>\n<li>\n Nulla volutpat aliquam velit </li>\n</ul>\n"
        "</li>\n</ul>\n</li>\n<li>\n Very easy! </li>\n</ul>\n");
    REQUIRE(html == required);
}


/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include <md4qt/parser.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// doctest include.
#include <doctest/doctest.h>

TEST_CASE("test_replace_one")
{
    {
        const auto str = TRAIT::latin1ToString("aaabbbccc");
        TRAIT::InternalString s(str);

        REQUIRE(s.length() == 9);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[7] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[8] == TRAIT::latin1ToChar('c'));

        s.replaceOne(3, 3, TRAIT::latin1ToString("ddd"));

        REQUIRE(s.length() == 9);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[7] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[8] == TRAIT::latin1ToChar('c'));

        s.replaceOne(2, 5, TRAIT::latin1ToString("ddd"));

        REQUIRE(s.length() == 7);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('c'));
    }

    {
        const auto str = TRAIT::latin1ToString("aaa");
        TRAIT::InternalString s(str);

        REQUIRE(s.length() == 3);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('a'));

        s.replaceOne(0, 4, TRAIT::latin1ToString("ddd"));

        REQUIRE(s.length() == 3);

        REQUIRE(s[0] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('d'));

        s.replaceOne(0, 4, TRAIT::latin1ToString("ccc"));

        REQUIRE(s.length() == 3);

        REQUIRE(s[0] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('c'));
    }

    {
        const auto str = TRAIT::latin1ToString("aaabbb");
        TRAIT::InternalString s(str);

        REQUIRE(s.length() == 6);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('b'));

        s.replaceOne(0, 1, TRAIT::latin1ToString("ddd"));

        REQUIRE(s.length() == 8);

        REQUIRE(s[0] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[7] == TRAIT::latin1ToChar('b'));

        s.replaceOne(5, 3, TRAIT::latin1ToString("ccc"));

        REQUIRE(s.length() == 8);

        REQUIRE(s[0] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[7] == TRAIT::latin1ToChar('c'));

        s.replaceOne(7, 1, TRAIT::latin1ToString("eee"));

        REQUIRE(s.length() == 10);

        REQUIRE(s[0] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('d'));
        REQUIRE(s[3] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[4] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[5] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[6] == TRAIT::latin1ToChar('c'));
        REQUIRE(s[7] == TRAIT::latin1ToChar('e'));
        REQUIRE(s[8] == TRAIT::latin1ToChar('e'));
        REQUIRE(s[9] == TRAIT::latin1ToChar('e'));

        s.replaceOne(0, 10, TRAIT::latin1ToString("1"));

        REQUIRE(s.length() == 1);

        REQUIRE(s[0] == TRAIT::latin1ToChar('1'));
    }

    {
        const auto str = TRAIT::latin1ToString("a");
        TRAIT::InternalString s(str);

        REQUIRE(s.length() == 1);

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));

        s.replaceOne(0, 4, TRAIT::latin1ToString(""));

        REQUIRE(s.length() == 0);
        REQUIRE(s.isEmpty());
    }

    {
        const auto str = TRAIT::latin1ToString("abc");
        TRAIT::InternalString s(str);

        REQUIRE(s.length() == 3);

        s.indexOf(QString());

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('b'));
        REQUIRE(s[2] == TRAIT::latin1ToChar('c'));

        s.replaceOne(1, 1, TRAIT::latin1ToString(""));

        REQUIRE(s.length() == 2);
        REQUIRE(!s.isEmpty());

        REQUIRE(s[0] == TRAIT::latin1ToChar('a'));
        REQUIRE(s[1] == TRAIT::latin1ToChar('c'));
    }
}

TEST_CASE("replace_remove_1")
{
    const auto str = TRAIT::latin1ToString("abcde");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(-1) == -1);
    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 3);
    REQUIRE(s.virginPos(4) == 4);
    REQUIRE(s.virginPos(5) == 5);

    s.replace(TRAIT::latin1ToString("b"), TRAIT::latin1ToString("bb"));

    REQUIRE(s == TRAIT::latin1ToString("abbcde"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 1);
    REQUIRE(s.virginPos(3) == 2);
    REQUIRE(s.virginPos(4) == 3);
    REQUIRE(s.virginPos(5) == 4);
    REQUIRE(s.virginPos(6) == 5);

    s.replace(TRAIT::latin1ToString("bb"), TRAIT::latin1ToString("b"));

    REQUIRE(s == TRAIT::latin1ToString("abcde"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 3);
    REQUIRE(s.virginPos(4) == 4);

    s.replace(TRAIT::latin1ToString("b"), TRAIT::latin1ToString(""));

    REQUIRE(s == TRAIT::latin1ToString("acde"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 2);
    REQUIRE(s.virginPos(2) == 3);
    REQUIRE(s.virginPos(3) == 4);

    s.remove(0, 1);

    REQUIRE(s == TRAIT::latin1ToString("cde"));

    REQUIRE(s.virginPos(0) == 2);
    REQUIRE(s.virginPos(1) == 3);
    REQUIRE(s.virginPos(2) == 4);
}

TEST_CASE("replace_remove_2")
{
    const auto str = TRAIT::latin1ToString("xxxxxx");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(1) == 1);

    s.replace(TRAIT::latin1ToString("xx"), TRAIT::latin1ToString("x"));

    REQUIRE(s == TRAIT::latin1ToString("xxx"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 2);
    REQUIRE(s.virginPos(2) == 4);

    s.remove(1, 1);

    REQUIRE(s == TRAIT::latin1ToString("xx"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 4);

    s.remove(0, 1);

    REQUIRE(s == TRAIT::latin1ToString("x"));

    REQUIRE(s.virginPos(0) == 4);
}

TEST_CASE("replace_remove_3")
{
    const auto str = TRAIT::latin1ToString("xxxxxx");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(1) == 1);

    s.remove(0, 3);

    REQUIRE(s == TRAIT::latin1ToString("xxx"));

    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 4);
    REQUIRE(s.virginPos(2) == 5);

    s.replace(TRAIT::latin1ToString("xx"), TRAIT::latin1ToString("x"));

    REQUIRE(s == TRAIT::latin1ToString("xx"));

    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 5);

    s.remove(0, 1);

    REQUIRE(s == TRAIT::latin1ToString("x"));

    REQUIRE(s.virginPos(0) == 5);
}

TEST_CASE("replace_remove_4")
{
    const auto str = TRAIT::latin1ToString("xxxxxx");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(1) == 1);

    s.replace(TRAIT::latin1ToString("xxx"), TRAIT::latin1ToString("bbbb"));

    REQUIRE(s == TRAIT::latin1ToString("bbbbbbbb"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 2);

    REQUIRE(s.virginPos(4) == 3);
    REQUIRE(s.virginPos(5) == 4);
    REQUIRE(s.virginPos(6) == 5);
    REQUIRE(s.virginPos(7) == 5);
}

TEST_CASE("replace_remove_5")
{
    const auto str = TRAIT::latin1ToString("xxxxxx");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(1) == 1);

    s.replace(TRAIT::latin1ToString("xxx"), TRAIT::latin1ToString("bbb"));

    REQUIRE(s == TRAIT::latin1ToString("bbbbbb"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 3);
    REQUIRE(s.virginPos(4) == 4);
    REQUIRE(s.virginPos(5) == 5);
}

TEST_CASE("replace_remove_6")
{
    const auto str = TRAIT::latin1ToString("\tParagraph\t");
    TRAIT::InternalString s(str);

    REQUIRE(s.virginPos(1) == 1);

    s.replace(TRAIT::latin1ToString("\t"), TRAIT::latin1ToString("    "));

    REQUIRE(s == TRAIT::latin1ToString("    Paragraph    "));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 0);
    REQUIRE(s.virginPos(2) == 0);
    REQUIRE(s.virginPos(3) == 0);
    REQUIRE(s.virginPos(4) == 1);
    REQUIRE(s.virginPos(5) == 2);
    REQUIRE(s.virginPos(13) == 10);
    REQUIRE(s.virginPos(14) == 10);
    REQUIRE(s.virginPos(15) == 10);
    REQUIRE(s.virginPos(16) == 10);
    REQUIRE(s.virginPos(17) == 11);
    REQUIRE(s.virginPos(18) == 11);

    s.remove(0, 4);

    REQUIRE(s == TRAIT::latin1ToString("Paragraph    "));

    REQUIRE(s.virginPos(0) == 1);
    REQUIRE(s.virginPos(1) == 2);
    REQUIRE(s.virginPos(2) == 3);
    REQUIRE(s.virginPos(9) == 10);
    REQUIRE(s.virginPos(10) == 10);
    REQUIRE(s.virginPos(11) == 10);
    REQUIRE(s.virginPos(12) == 10);
    REQUIRE(s.virginPos(13) == 11);
    REQUIRE(s.virginPos(14) == 11);
}

TEST_CASE("simplified")
{
    const auto str1 = TRAIT::latin1ToString("   a   b   c   ");
    TRAIT::InternalString s(str1);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a b c"));

    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 4);
    REQUIRE(s.virginPos(2) == 7);
    REQUIRE(s.virginPos(3) == 8);
    REQUIRE(s.virginPos(4) == 11);

    const auto str2 = TRAIT::latin1ToString("   a b c   ");
    s = TRAIT::InternalString(str2);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a b c"));

    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 4);
    REQUIRE(s.virginPos(2) == 5);
    REQUIRE(s.virginPos(3) == 6);
    REQUIRE(s.virginPos(4) == 7);

    const auto str3 = TRAIT::latin1ToString("a b c");
    s = TRAIT::InternalString(str3);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a b c"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 3);
    REQUIRE(s.virginPos(4) == 4);

    const auto str4 = TRAIT::latin1ToString("a b  c");
    s = TRAIT::InternalString(str4);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a b c"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 2);
    REQUIRE(s.virginPos(3) == 3);
    REQUIRE(s.virginPos(4) == 5);

    const auto str5 = TRAIT::latin1ToString("");
    s = TRAIT::InternalString(str5);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString(""));
    REQUIRE(s.isEmpty());

    const auto str6 = TRAIT::latin1ToString("   ");
    s = TRAIT::InternalString(str6);

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString(""));
    REQUIRE(s.isEmpty());
}

TEST_CASE("split")
{
    const auto str1 = TRAIT::latin1ToString("|a|b|c|");
    TRAIT::InternalString s(str1);

    auto r = s.split(TRAIT::InternalString(TRAIT::latin1ToString("|")));

    REQUIRE(r.size() == 3);

    REQUIRE(r.at(0) == TRAIT::latin1ToString("a"));
    REQUIRE(r.at(0).virginPos(0) == 1);

    REQUIRE(r.at(1) == TRAIT::latin1ToString("b"));
    REQUIRE(r.at(1).virginPos(0) == 3);

    REQUIRE(r.at(2) == TRAIT::latin1ToString("c"));
    REQUIRE(r.at(2).virginPos(0) == 5);

    const auto str2 = TRAIT::latin1ToString(" | a | b | c | ");
    s = TRAIT::InternalString(str2);

    r = s.split(TRAIT::InternalString(TRAIT::latin1ToString("|")));

    REQUIRE(r.size() == 5);

    REQUIRE(r.at(0) == TRAIT::latin1ToString(" "));
    REQUIRE(r.at(0).virginPos(0) == 0);

    REQUIRE(r.at(1) == TRAIT::latin1ToString(" a "));
    REQUIRE(r.at(1).virginPos(1) == 3);

    REQUIRE(r.at(2) == TRAIT::latin1ToString(" b "));
    REQUIRE(r.at(2).virginPos(1) == 7);

    REQUIRE(r.at(3) == TRAIT::latin1ToString(" c "));
    REQUIRE(r.at(3).virginPos(1) == 11);

    REQUIRE(r.at(4) == TRAIT::latin1ToString(" "));
    REQUIRE(r.at(4).virginPos(0) == 14);

    const auto str3 = TRAIT::latin1ToString("abc");
    s = TRAIT::InternalString(str3);

    r = s.split(TRAIT::InternalString(TRAIT::latin1ToString("")));

    REQUIRE(r.size() == 3);

    REQUIRE(r.at(0) == TRAIT::latin1ToString("a"));
    REQUIRE(r.at(0).virginPos(0) == 0);

    REQUIRE(r.at(1) == TRAIT::latin1ToString("b"));
    REQUIRE(r.at(1).virginPos(0) == 1);

    REQUIRE(r.at(2) == TRAIT::latin1ToString("c"));
    REQUIRE(r.at(2).virginPos(0) == 2);

    const auto str4 = TRAIT::latin1ToString(" | a | b | c | ");
    s = TRAIT::InternalString(str4);
    s = s.simplified();

    r = s.split(TRAIT::InternalString(TRAIT::latin1ToString("|")));

    REQUIRE(r.size() == 3);

    REQUIRE(r.at(0) == TRAIT::latin1ToString(" a "));
    REQUIRE(r.at(0).virginPos(1) == 3);

    REQUIRE(r.at(1) == TRAIT::latin1ToString(" b "));
    REQUIRE(r.at(1).virginPos(1) == 7);

    REQUIRE(r.at(2) == TRAIT::latin1ToString(" c "));
    REQUIRE(r.at(2).virginPos(1) == 11);
}

TEST_CASE("sliced")
{
    const auto str1 = TRAIT::latin1ToString("aaabbbccc");
    TRAIT::InternalString s(str1);

    s = s.sliced(3, 3);

    REQUIRE(s == TRAIT::latin1ToString("bbb"));
    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 4);
    REQUIRE(s.virginPos(2) == 5);

    const auto str2 = TRAIT::latin1ToString("aaabbbccc");
    s = TRAIT::InternalString(str2);

    s = s.sliced(3);

    REQUIRE(s == TRAIT::latin1ToString("bbbccc"));
    REQUIRE(s.virginPos(0) == 3);
    REQUIRE(s.virginPos(1) == 4);
    REQUIRE(s.virginPos(2) == 5);
    REQUIRE(s.virginPos(3) == 6);
    REQUIRE(s.virginPos(4) == 7);
    REQUIRE(s.virginPos(5) == 8);
}

TEST_CASE("right")
{
    const auto str = TRAIT::latin1ToString("aaabbbccc");
    TRAIT::InternalString s(str);

    s = s.right(3);

    REQUIRE(s == TRAIT::latin1ToString("ccc"));
    REQUIRE(s.virginPos(0) == 6);
    REQUIRE(s.virginPos(1) == 7);
    REQUIRE(s.virginPos(2) == 8);
}

TEST_CASE("insert")
{
    const auto str = TRAIT::latin1ToString("a");
    TRAIT::InternalString s(str);

    s = s.insert(0, TRAIT::latin1ToChar('b'));

    REQUIRE(s == TRAIT::latin1ToString("ba"));
    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 0);
}

TEST_CASE("double_remove")
{
    const auto str1 = TRAIT::latin1ToString("a b c d");
    TRAIT::InternalString s(str1);

    s.remove(2, 1);

    REQUIRE(s == TRAIT::latin1ToString("a  c d"));

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a c d"));

    s.remove(2, 1);

    REQUIRE(s == TRAIT::latin1ToString("a  d"));

    s = s.simplified();

    REQUIRE(s == TRAIT::latin1ToString("a d"));

    REQUIRE(s.virginPos(0) == 0);
    REQUIRE(s.virginPos(1) == 1);
    REQUIRE(s.virginPos(2) == 6);
}

TEST_CASE("replace_tabs")
{
    {
        const auto str = TRAIT::latin1ToString("\tcode\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginPos(0) == 0);
        REQUIRE(s.virginPos(1) == 0);
        REQUIRE(s.virginPos(2) == 0);
        REQUIRE(s.virginPos(3) == 0);
        REQUIRE(s.virginPos(4) == 1);
        REQUIRE(s.virginPos(5) == 2);
        REQUIRE(s.virginPos(6) == 3);
        REQUIRE(s.virginPos(7) == 4);
        REQUIRE(s.virginPos(8) == 5);
        REQUIRE(s.virginPos(9) == 5);
        REQUIRE(s.virginPos(10) == 5);
        REQUIRE(s.virginPos(11) == 5);
    }

    {
        const auto str = TRAIT::latin1ToString("\tcode\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);

        REQUIRE(s.virginPos(0) == 0);
        REQUIRE(s.virginPos(1) == 0);
        REQUIRE(s.virginPos(2) == 1);
        REQUIRE(s.virginPos(3) == 2);
        REQUIRE(s.virginPos(4) == 3);
        REQUIRE(s.virginPos(5) == 4);
        REQUIRE(s.virginPos(6) == 5);
        REQUIRE(s.virginPos(7) == 5);
        REQUIRE(s.virginPos(8) == 5);
        REQUIRE(s.virginPos(9) == 5);
    }

    {
        const auto str = TRAIT::latin1ToString("\tcode\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);
        s.remove(8, 2);

        REQUIRE(s.virginPos(0) == 0);
        REQUIRE(s.virginPos(1) == 0);
        REQUIRE(s.virginPos(2) == 1);
        REQUIRE(s.virginPos(3) == 2);
        REQUIRE(s.virginPos(4) == 3);
        REQUIRE(s.virginPos(5) == 4);
        REQUIRE(s.virginPos(6) == 5);
        REQUIRE(s.virginPos(7) == 5);
    }
}

TEST_CASE("replace_spaces")
{
    const auto str = TRAIT::latin1ToString("    code    c");
    TRAIT::InternalString s(str);

    s.remove(0, 4);
    s.remove(4, 4);

    REQUIRE(s.virginPos(0) == 4);
    REQUIRE(s.virginPos(1) == 5);
    REQUIRE(s.virginPos(2) == 6);
    REQUIRE(s.virginPos(3) == 7);
    REQUIRE(s.virginPos(4) == 12);
}

TEST_CASE("virgin_string")
{
    {
        const auto str = TRAIT::latin1ToString("\tcode");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginSubString() == str);
    }

    {
        const auto str = TRAIT::latin1ToString("\tcode");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("  code"));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("  \tcode"));
    }

    {
        const auto str = TRAIT::latin1ToString("\tcode\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("\tcode\t"));
    }

    {
        const auto str = TRAIT::latin1ToString("\tcode\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("  code\t"));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode\t\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, 2);
        s.remove(s.length() - 2, 2);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("  \tcode\t  "));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode\t\t");
        TRAIT::InternalString s(str);

        s.remove(3, 2);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("\t\tcode\t\t"));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode\t\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, s.length());

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("\t\tcode\t\t"));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode\t\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        s.remove(0, s.length());

        REQUIRE(s.virginSubString(-1, s.length() + 1) == TRAIT::latin1ToString("\t\tcode\t\t"));
    }

    {
        const auto str = TRAIT::latin1ToString("text");
        TRAIT::InternalString s(str);

        s.remove(0, 2);
        s.remove(s.length() - 1, 1);

        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("x"));
    }

    {
        const auto str = TRAIT::latin1ToString("text");
        TRAIT::InternalString s(str);

        REQUIRE(s.virginSubString(1, 1) == TRAIT::latin1ToString("e"));
    }

    {
        const auto str = TRAIT::latin1ToString("\t\tcode\t\t");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginSubString(0, 12) == TRAIT::latin1ToString("\t\tcode"));
    }

    {
        const auto str = TRAIT::latin1ToString("\ta\t\tb");
        TRAIT::InternalString s(str);

        MD::replaceTabs<TRAIT>(s);

        REQUIRE(s.virginSubString(4) == TRAIT::latin1ToString("a\t\tb"));
    }
}

TEST_CASE("backslash")
{
    {
        const auto str = TRAIT::latin1ToString("\\|");
        TRAIT::InternalString s(str);

        s = MD::removeBackslashes<TRAIT>(s);

        REQUIRE(s.virginPos(0) == 1);
        REQUIRE(s.virginSubString() == TRAIT::latin1ToString("|"));
    }

    {
        const auto str = TRAIT::latin1ToString("abcde\\|");
        TRAIT::InternalString s(str);

        s.replace(TRAIT::latin1ToString("\\|"), TRAIT::latin1ToString("|"));

        REQUIRE(s.virginPos(5) == 5);
        REQUIRE(s.virginSubString(5) == TRAIT::latin1ToString("\\|"));
    }
}

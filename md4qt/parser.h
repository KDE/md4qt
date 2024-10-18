/*
    SPDX-FileCopyrightText: 2022-2024 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

#ifndef MD4QT_MD_PARSER_HPP_INCLUDED
#define MD4QT_MD_PARSER_HPP_INCLUDED

// md4qt include.
#include "doc.h"
#include "entities_map.h"
#include "traits.h"
#include "utils.h"

#ifdef MD4QT_QT_SUPPORT

// Qt include.
#include <QDir>
#include <QFile>
#include <QTextStream>

#endif // MD4QT_QT_SUPPORT

#ifdef MD4QT_ICU_STL_SUPPORT

// C++ include.
#include <exception>

#endif // MD4QT_ICU_STL_SUPPORT

// C++ include.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace MD
{

static const char *s_startComment = "<!--";

inline bool
indentInList(const std::vector<long long int> *indents,
             long long int indent,
             bool codeIndentedBySpaces)
{
    if (indents && !indents->empty()) {
        return (std::find_if(indents->cbegin(),
                             indents->cend(),
                             [indent, codeIndentedBySpaces](const auto &v) {
                                 return (indent >= v && (codeIndentedBySpaces ?
                                         true : indent <= v + 3));
                             })
                != indents->cend());
    } else {
        return false;
    }
}

// Skip spaces in line from pos \p i.
template<class Trait>
inline long long int
skipSpaces(long long int i, const typename Trait::String &line)
{
    const auto length = line.length();

    while (i < length && line[i].isSpace()) {
        ++i;
    }

    return i;
} // skipSpaces

// Returns last non-space character position.
template<class Trait>
inline long long int
lastNonSpacePos(const typename Trait::String &line)
{
    long long int i = line.length() - 1;

    while (i > 0 && line[i].isSpace()) {
        --i;
    }

    return i;
} // lastNonSpacePos

//! \return Starting sequence of the same characters.
template<class Trait>
inline typename Trait::String
startSequence(const typename Trait::String &line)
{
    auto pos = skipSpaces<Trait>(0, line);

    if (pos >= line.length()) {
        return {};
    }

    const auto sch = line[pos];
    const auto start = pos;

    ++pos;

    while (pos < line.length() && line[pos] == sch) {
        ++pos;
    }

    return line.sliced(start, pos - start);
}

//! \return Is string an ordered list.
template<class Trait>
inline bool
isOrderedList(const typename Trait::String &s,
              int *num = nullptr,
              int *len = nullptr,
              typename Trait::Char *delim = nullptr,
              bool *isFirstLineEmpty = nullptr)
{
    long long int p = skipSpaces<Trait>(0, s);

    long long int dp = p;

    for (; p < s.size(); ++p) {
        if (!s[p].isDigit()) {
            break;
        }
    }

    if (dp != p && p < s.size()) {
        const auto digits = s.sliced(dp, p - dp);

        if (digits.size() > 9) {
            return false;
        }

        const auto i = digits.toInt();

        if (num) {
            *num = i;
        }

        if (len) {
            *len = p - dp;
        }

        if (s[p] == Trait::latin1ToChar('.') || s[p] == Trait::latin1ToChar(')')) {
            if (delim) {
                *delim = s[p];
            }

            ++p;

            long long int tmp = skipSpaces<Trait>(p, s);

            if (isFirstLineEmpty) {
                *isFirstLineEmpty = (tmp == s.size());
            }

            if ((p < s.size() && s[p] == Trait::latin1ToChar(' ')) || p == s.size()) {
                return true;
            }
        }
    }

    return false;
}

//
// RawHtmlBlock
//

//! Internal structure.
template<class Trait>
struct RawHtmlBlock {
    std::shared_ptr<RawHtml<Trait>> m_html = {};
    std::shared_ptr<Block<Trait>> m_parent = {};
    std::shared_ptr<Block<Trait>> m_topParent = {};
    using SequenceOfBlock = std::vector<std::pair<std::shared_ptr<Block<Trait>>, long long int>>;
    SequenceOfBlock m_blocks = {};
    std::unordered_map<std::shared_ptr<Block<Trait>>, SequenceOfBlock> m_toAdjustLastPos = {};
    int m_htmlBlockType = -1;
    bool m_continueHtml = false;
    bool m_onLine = false;

    std::shared_ptr<Block<Trait>>
    findParent(long long int indent) const
    {
        for (auto it = m_blocks.crbegin(), last = m_blocks.crend(); it != last; ++it) {
            if (indent >= it->second) {
                return it->first;
            }
        }

        return nullptr;
    }
}; // struct RawHtmlBlock

template<class Trait>
inline void resetHtmlTag(RawHtmlBlock<Trait> &html)
{
    html.m_html.reset();
    html.m_parent.reset();
    html.m_htmlBlockType = -1;
    html.m_continueHtml = false;
    html.m_onLine = false;
}

//
// MdLineData
//

//! Internal structure.
struct MdLineData {
    long long int m_lineNumber = -1;
    using CommentData = std::pair<char, bool>;
    using CommentDataMap = std::map<long long int, CommentData>;
    // std::pair< closed, valid >
    CommentDataMap m_htmlCommentData = {};
}; // struct MdLineData

//
// MdBlock
//

//! Internal structure.
template<class Trait>
struct MdBlock {
    using Line = std::pair<typename Trait::InternalString, MdLineData>;
    using Data = typename Trait::template Vector<Line>;

    Data m_data;
    long long int m_emptyLinesBefore = 0;
    bool m_emptyLineAfter = true;
}; // struct MdBlock

//
// StringListStream
//

//! Wrapper for typename Trait::StringList to be behaved like a stream.
template<class Trait>
class StringListStream final
{
public:
    StringListStream(typename MdBlock<Trait>::Data &stream)
        : m_stream(stream)
        , m_pos(0)
    {
    }

    bool atEnd() const
    {
        return (m_pos >= (long long int)m_stream.size());
    }
    typename Trait::InternalString readLine()
    {
        return m_stream.at(m_pos++).first;
    }
    long long int currentLineNumber() const
    {
        return (m_pos < size() ? m_stream.at(m_pos).second.m_lineNumber : size());
    }
    typename Trait::InternalString lineAt(long long int pos)
    {
        return m_stream.at(pos).first;
    }
    long long int size() const
    {
        return m_stream.size();
    }

private:
    typename MdBlock<Trait>::Data &m_stream;
    long long int m_pos;
}; // class StringListStream

inline bool
checkStack(std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
           const std::pair<std::pair<long long int, bool>, int> &v,
           size_t idx)
{
    int value = -v.first.first;

    for (long long int i = s.size() - 1; i >= 0; --i) {
        if (s[i].second == v.second && s[i].first.first > 0) {
            // Check for rule of multiplies of 3. Look at CommonMark 0.30 example 411.
            if (!((s[i].first.second || v.first.second) &&
                (s[i].first.first + value) % 3 == 0 &&
                !(s[i].first.first % 3 == 0 && value % 3 == 0))) {
                if (s[i].first.first - value <= 0) {
                    if (i == (long long int)idx) {
                        return true;
                    }

                    value -= s[i].first.first;

                    s.erase(s.cbegin() + i, s.cend());

                    if (value == 0) {
                        break;
                    }
                } else {
                    s[i].first.first -= value;

                    s.erase(s.cbegin() + i + 1, s.cend());

                    break;
                }
            }
        }

        if (i == 0) {
            break;
        }
    }

    return false;
}

//! \return Is string a footnote?
template<class Trait>
inline bool
isFootnote(const typename Trait::String &s)
{
    long long int p = skipSpaces<Trait>(0, s);

    if (s.size() - p < 5) {
        return false;
    }

    if (s[p++] != Trait::latin1ToChar('[')) {
        return false;
    }

    if (s[p++] != Trait::latin1ToChar('^')) {
        return false;
    }

    if (s[p] == Trait::latin1ToChar(']') || s[p].isSpace()) {
        return false;
    }

    for (; p < s.size(); ++p) {
        if (s[p] == Trait::latin1ToChar(']')) {
            break;
        } else if (s[p].isSpace()) {
            return false;
        }
    }

    ++p;

    if (p < s.size() && s[p] == Trait::latin1ToChar(':')) {
        return true;
    } else {
        return false;
    }
}

//! \return Is string a code fences?
template<class Trait>
inline bool
isCodeFences(const typename Trait::String &s, bool closing = false)
{
    auto p = skipSpaces<Trait>(0, s);

    if (p > 3 || p == s.length()) {
        return false;
    }

    const auto ch = s[p];

    if (ch != Trait::latin1ToChar('~') && ch != Trait::latin1ToChar('`')) {
        return false;
    }

    bool space = false;

    long long int c = 1;
    ++p;

    for (; p < s.length(); ++p) {
        if (s[p].isSpace()) {
            space = true;
        } else if (s[p] == ch) {
            if (space && (closing ? true : ch == Trait::latin1ToChar('`'))) {
                return false;
            }

            if (!space) {
                ++c;
            }
        } else if (closing) {
            return false;
        } else {
            break;
        }
    }

    if (c < 3) {
        return false;
    }

    if (ch == Trait::latin1ToChar('`')) {
        for (; p < s.length(); ++p) {
            if (s[p] == Trait::latin1ToChar('`')) {
                return false;
            }
        }
    }

    return true;
}

template<class Trait>
inline typename Trait::String
readEscapedSequence(long long int i,
                    const typename Trait::String &str,
                    long long int *endPos = nullptr)
{
    bool backslash = false;
    const auto start = i;

    if (start >= str.length()) {
        return {};
    }

    while (i < str.length()) {
        bool now = false;

        if (str[i] == Trait::latin1ToChar('\\') && !backslash) {
            backslash = true;
            now = true;
        } else if (str[i].isSpace() && !backslash) {
            break;
        }

        if (!now) {
            backslash = false;
        }

        ++i;
    }

    if (endPos) {
        *endPos = i - 1;
    }

    return str.sliced(start, i - start);
}

template<class Trait>
static const typename Trait::String s_canBeEscaped =
    Trait::latin1ToString("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

template<class String, class Trait>
inline String
removeBackslashes(const String &s)
{
    String r = s;
    bool backslash = false;
    long long int extra = 0;

    for (long long int i = 0; i < s.length(); ++i) {
        bool now = false;

        if (s[i] == Trait::latin1ToChar('\\') && !backslash && i != s.length() - 1) {
            backslash = true;
            now = true;
        } else if (s_canBeEscaped<Trait>.contains(s[i]) && backslash) {
            r.remove(i - extra - 1, 1);
            ++extra;
        }

        if (!now) {
            backslash = false;
        }
    }

    return r;
}

//! \return Is string a start of code?
template<class Trait>
inline bool
isStartOfCode(const typename Trait::String &str,
              typename Trait::String *syntax = nullptr,
              WithPosition *delim = nullptr,
              WithPosition *syntaxPos = nullptr)
{
    long long int p = skipSpaces<Trait>(0, str);

    if (delim) {
        delim->setStartColumn(p);
    }

    if (p > 3) {
        return false;
    }

    if (str.size() - p < 3) {
        return false;
    }

    const bool c96 = str[p] == Trait::latin1ToChar('`');
    const bool c126 = str[p] == Trait::latin1ToChar('~');

    if (c96 || c126) {
        ++p;
        long long int c = 1;

        while (p < str.length()) {
            if (str[p] != (c96 ? Trait::latin1ToChar('`') : Trait::latin1ToChar('~'))) {
                break;
            }

            ++c;
            ++p;
        }

        if (delim) {
            delim->setEndColumn(p - 1);
        }

        if (c < 3) {
            return false;
        }

        if (syntax) {
            p = skipSpaces<Trait>(p, str);
            long long int endSyntaxPos = p;

            if (p < str.size()) {
                *syntax = removeBackslashes<typename Trait::String, Trait>(
					readEscapedSequence<Trait>(p, str, &endSyntaxPos));

                if (syntaxPos) {
                    syntaxPos->setStartColumn(p);
                    syntaxPos->setEndColumn(endSyntaxPos);
                }
            }
        }

        return true;
    }

    return false;
}

//! \return Is string a horizontal line?
template<class Trait>
inline bool
isHorizontalLine(const typename Trait::String &s)
{
    if (s.size() < 3) {
        return false;
    }

    typename Trait::Char c;

    if (s[0] == Trait::latin1ToChar('*')) {
        c = Trait::latin1ToChar('*');
    } else if (s[0] == Trait::latin1ToChar('-')) {
        c = Trait::latin1ToChar('-');
    } else if (s[0] == Trait::latin1ToChar('_')) {
        c = Trait::latin1ToChar('_');
    } else {
        return false;
    }

    long long int p = 1;
    long long int count = 1;

    for (; p < s.size(); ++p) {
        if (s[p] != c && !s[p].isSpace()) {
            break;
        } else if (s[p] == c) {
            ++count;
        }
    }

    if (count < 3) {
        return false;
    }

    if (p == s.size()) {
        return true;
    }

    return false;
}

//! \return Is string a column alignment?
template<class Trait>
inline bool
isColumnAlignment(const typename Trait::String &s)
{
    long long int p = skipSpaces<Trait>(0, s);

    static const typename Trait::String s_legitime = Trait::latin1ToString(":-");

    if (!s_legitime.contains(s[p])) {
        return false;
    }

    if (s[p] == Trait::latin1ToChar(':')) {
        ++p;
    }

    for (; p < s.size(); ++p) {
        if (s[p] != Trait::latin1ToChar('-')) {
            break;
        }
    }

    if (p == s.size()) {
        return true;
    }

    if (s[p] != Trait::latin1ToChar(':') && !s[p].isSpace()) {
        return false;
    }

    ++p;

    for (; p < s.size(); ++p) {
        if (!s[p].isSpace()) {
            return false;
        }
    }

    return true;
}

template<class Trait>
typename Trait::StringList
splitString(const typename Trait::String &str, const typename Trait::Char &ch);

#ifdef MD4QT_ICU_STL_SUPPORT

template<>
inline typename UnicodeStringTrait::StringList
splitString<UnicodeStringTrait>(const UnicodeString &str, const UnicodeChar &ch)
{
    return str.split(ch);
}

#endif

#ifdef MD4QT_QT_SUPPORT

template<>
inline typename QStringTrait::StringList
splitString<QStringTrait>(const QString &str, const QChar &ch)
{
    return str.split(ch, Qt::SkipEmptyParts);
}

#endif

//! \return Number of columns?
template<class Trait>
inline int
isTableAlignment(const typename Trait::String &s)
{
    const auto columns = splitString<Trait>(s.simplified(), Trait::latin1ToChar('|'));

    for (const auto &c : columns) {
        if (!isColumnAlignment<Trait>(c)) {
            return 0;
        }
    }

    return columns.size();
}

//! \return Is given string a HTML comment.
template<class Trait>
inline bool
isHtmlComment(const typename Trait::String &s)
{
    auto c = s;

    if (s.startsWith(Trait::latin1ToString(s_startComment))) {
        c.remove(0, 4);
    } else {
        return false;
    }

    long long int p = -1;
    bool endFound = false;

    while ((p = c.indexOf(Trait::latin1ToString("--"), p + 1)) > -1) {
        if (c.size() > p + 2 && c[p + 2] == Trait::latin1ToChar('>')) {
            if (!endFound) {
                endFound = true;
            } else {
                return false;
            }
        } else if (p - 2 >= 0 && c.sliced(p - 2, 4) == Trait::latin1ToString("<!--")) {
            return false;
        } else if (c.size() > p + 3 && c.sliced(p, 4) == Trait::latin1ToString("--!>")) {
            return false;
        }
    }

    return endFound;
}

template<class Trait>
inline typename Trait::String
replaceEntity(const typename Trait::String &s)
{
    long long int p1 = 0;

    typename Trait::String res;
    long long int i = 0;

    while ((p1 = s.indexOf(Trait::latin1ToChar('&'), p1)) != -1) {
        if (p1 > 0 && s[p1 - 1] == Trait::latin1ToChar('\\')) {
            ++p1;

            continue;
        }

        const auto p2 = s.indexOf(Trait::latin1ToChar(';'), p1);

        if (p2 != -1) {
            const auto en = s.sliced(p1, p2 - p1 + 1);

            if (en.size() > 2 && en[1] == Trait::latin1ToChar('#')) {
                if (en.size() > 3 && en[2].toLower() == Trait::latin1ToChar('x')) {
                    const auto hex = en.sliced(3, en.size() - 4);

                    if (hex.size() <= 6 && hex.size() > 0) {
                        bool ok = false;

                        const char32_t c = hex.toInt(&ok, 16);

                        if (ok) {
                            res.push_back(s.sliced(i, p1 - i));
                            i = p2 + 1;

                            if (c) {
                                Trait::appendUcs4(res, c);
                            } else {
                                res.push_back(typename Trait::Char(0xFFFD));
                            }
                        }
                    }
                } else {
                    const auto dec = en.sliced(2, en.size() - 3);

                    if (dec.size() <= 7 && dec.size() > 0) {
                        bool ok = false;

                        const char32_t c = dec.toInt(&ok, 10);

                        if (ok) {
                            res.push_back(s.sliced(i, p1 - i));
                            i = p2 + 1;

                            if (c) {
                                Trait::appendUcs4(res, c);
                            } else {
                                res.push_back(typename Trait::Char(0xFFFD));
                            }
                        }
                    }
                }
            } else {
                const auto it = s_entityMap<Trait>.find(en);

                if (it != s_entityMap<Trait>.cend()) {
                    res.push_back(s.sliced(i, p1 - i));
                    i = p2 + 1;
                    res.push_back(Trait::utf16ToString(it->second));
                }
            }
        } else {
            break;
        }

        p1 = p2 + 1;
    }

    res.push_back(s.sliced(i, s.size() - i));

    return res;
}

template<class Trait>
inline typename MdBlock<Trait>::Data
removeBackslashes(const typename MdBlock<Trait>::Data &d)
{
    auto tmp = d;

    for (auto &line : tmp) {
        line.first = removeBackslashes<typename Trait::InternalString, Trait>(line.first);
    }

    return tmp;
}

//! Type of the paragraph's optimization.
enum class OptimizeParagraphType {
    //! Full optimization.
    Full,
    //! Semi optimization, optimization won't concatenate text
    //! items if style delimiters will be in the middle.
    Semi,
    //! Full optimization, but raw text data won't be concatenated (will be untouched).
    FullWithoutRawData,
    //! Semi optimization, but raw text data won't be concatenated (will be untouched).
    SemiWithoutRawData
};

//
// TextPlugin
//

enum TextPlugin : int {
    UnknownPluginID = 0,
    GitHubAutoLinkPluginID = 1,
    UserDefinedPluginID = 255
}; // enum TextPlugin

//
// Style
//

enum class Style { Italic1, Italic2, Bold1, Bold2, Strikethrough, Unknown };

inline TextOption
styleToTextOption(Style s)
{
    switch (s) {
    case Style::Italic1:
    case Style::Italic2:
        return ItalicText;

    case Style::Bold1:
    case Style::Bold2:
        return BoldText;

    case Style::Strikethrough:
        return StrikethroughText;

    default:
        return TextWithoutFormat;
    }
}

//
// TextPluginFunc
//

template<class Trait>
struct TextParsingOpts;

//! Functor type for text plugin.
template<class Trait>
using TextPluginFunc = std::function<void(std::shared_ptr<Paragraph<Trait>>,
                                          TextParsingOpts<Trait> &,
                                          const typename Trait::StringList &)>;

//
// TextPluginsMap
//

template<class Trait>
using TextPluginsMap = std::map<int, std::tuple<TextPluginFunc<Trait>,
                                                bool,
                                                typename Trait::StringList>>;

//
// TextParsingOpts
//

template<class Trait>
struct TextParsingOpts {
    MdBlock<Trait> &m_fr;
    std::shared_ptr<Block<Trait>> m_parent;
    std::shared_ptr<RawHtml<Trait>> m_tmpHtml;
    std::shared_ptr<Document<Trait>> m_doc;
    typename Trait::StringList &m_linksToParse;
    typename Trait::String m_workingPath;
    typename Trait::String m_fileName;
    bool m_collectRefLinks;
    bool m_ignoreLineBreak;
    RawHtmlBlock<Trait> &m_html;
    const TextPluginsMap<Trait> &m_textPlugins;
    std::shared_ptr<Text<Trait>> m_lastText = {};
    bool m_isSpaceBefore = false;
    bool m_wasRefLink = false;
    bool m_checkLineOnNewType = false;
    bool m_firstInParagraph = true;

    struct TextData {
        typename Trait::String m_str;
        long long int m_pos = -1;
        long long int m_line = -1;
        bool m_spaceBefore = false;
        bool m_spaceAfter = false;
    };

    std::vector<TextData> m_rawTextData = {};

    inline void
    concatenateAuxText(long long int start, long long int end)
    {
        if (start < end && (end - start > 1)) {
            for (auto i = start + 1; i < end; ++i) {
                m_rawTextData[start].m_str += m_rawTextData[i].m_str;
            }

            m_rawTextData.erase(m_rawTextData.cbegin() + start + 1, m_rawTextData.cbegin() + end);
        }
    }

    enum class Detected { Nothing = 0, Table = 1, HTML = 2, List = 3, Code = 4 }; // enum class Detected

    Detected m_detected = Detected::Nothing;

    inline bool
    shouldStopParsing() const
    {
        switch (m_detected) {
        case Detected::Table:
        case Detected::List:
        case Detected::Code:
            return true;

        default:
            return false;
        }
    }

    long long int m_line = 0;
    long long int m_pos = 0;
    long long int m_startTableLine = -1;
    long long int m_lastTextLine = -1;
    long long int m_lastTextPos = -1;
    int m_columnsCount = -1;
    int m_opts = TextWithoutFormat;
    std::vector<std::pair<Style, long long int>> m_styles = {};
    typename ItemWithOpts<Trait>::Styles m_openStyles = {};
    std::shared_ptr<ItemWithOpts<Trait>> m_lastItemWithStyle = nullptr;
}; // struct TextParsingOpts

//
// virginSubstr
//

//! \return Substring from fragment with given virgin positions.
template<class Trait>
inline typename Trait::String
virginSubstr(const MdBlock<Trait> &fr, const WithPosition &virginPos)
{
    if (fr.m_data.empty()) {
        return {};
    }

    long long int startLine = virginPos.startLine() < fr.m_data.at(0).second.m_lineNumber ?
        (virginPos.endLine() < fr.m_data.at(0).second.m_lineNumber ? -1 : 0) :
        virginPos.startLine() - fr.m_data.at(0).second.m_lineNumber;

    if (startLine >= static_cast<long long int>(fr.m_data.size()) || startLine < 0) {
        return {};
    }

    auto spos = virginPos.startColumn() - fr.m_data.at(startLine).first.virginPos(0);

    if (spos < 0) {
        spos = 0;
    }

    long long int epos = 0;
    long long int linesCount = virginPos.endLine() - virginPos.startLine() -
        (virginPos.startLine() < fr.m_data.at(0).second.m_lineNumber ?
            fr.m_data.at(0).second.m_lineNumber - virginPos.startLine() : 0);

    if (startLine + linesCount > static_cast<long long int>(fr.m_data.size())) {
        linesCount = fr.m_data.size() - startLine - 1;
        epos = fr.m_data.back().first.length();
    } else {
        epos = virginPos.endColumn() - fr.m_data.at(linesCount + startLine).first.virginPos(0) + 1;
    }

    if (epos < 0) {
        epos = 0;
    }

    if (epos > fr.m_data.at(linesCount + startLine).first.length()) {
        epos = fr.m_data.at(linesCount + startLine).first.length();
    }

    typename Trait::String str =
        (linesCount ? fr.m_data.at(startLine).first.sliced(spos).asString() :
            fr.m_data.at(startLine).first.sliced(spos, epos - spos).asString());

    long long int i = startLine + 1;

    for (; i < startLine + linesCount; ++i) {
        str.push_back(Trait::latin1ToString("\n"));
        str.push_back(fr.m_data.at(i).first.asString());
    }

    if (linesCount) {
        str.push_back(Trait::latin1ToString("\n"));
        str.push_back(fr.m_data.at(i).first.sliced(0, epos).asString());
    }

    return str;
}

//
// localPosFromVirgin
//

//! \return Local position ( { column, line } ) in fragment for given virgin position if exists.
//! \return { -1, -1 } if there is no given position.
template<class Trait>
inline std::pair<long long int, long long int>
localPosFromVirgin(const MdBlock<Trait> &fr, long long int virginColumn, long long int virginLine)
{
    if (fr.m_data.empty()) {
        return {-1, -1};
    }

    if (fr.m_data.front().second.m_lineNumber > virginLine ||
        fr.m_data.back().second.m_lineNumber < virginLine) {
        return {-1, -1};
    }

    auto line = virginLine - fr.m_data.front().second.m_lineNumber;

    if (fr.m_data.at(line).first.isEmpty()) {
        return {-1, -1};
    }

    const auto vzpos = fr.m_data.at(line).first.virginPos(0);

    if (vzpos > virginColumn || virginColumn > vzpos + fr.m_data.at(line).first.length() - 1) {
        return {-1, -1};
    }

    return {virginColumn - vzpos, line};
}

//
// GitHubAutolinkPlugin
//

/*
    "^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?"
    "(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$"
*/
template<class Trait>
inline bool
isEmail(const typename Trait::String &url)
{
    auto isAllowed = [](const typename Trait::Char &ch) -> bool {
        const auto unicode = ch.unicode();
        return ((unicode >= 48 && unicode <= 57) || (unicode >= 97 && unicode <= 122) ||
                (unicode >= 65 && unicode <= 90));
    };

    auto isAdditional = [](const typename Trait::Char &ch) -> bool {
        const auto unicode = ch.unicode();
        return (unicode == 33 || (unicode >= 35 && unicode <= 39) ||
                unicode == 42 || unicode == 43 || (unicode >= 45 && unicode <= 47) ||
                unicode == 61 || unicode == 63 || (unicode >= 94 && unicode <= 96) ||
                (unicode >= 123 && unicode <= 126));
    };

    static const auto s_delim = Trait::latin1ToChar('-');
    static const auto s_dog = Trait::latin1ToChar('@');
    static const auto s_dot = Trait::latin1ToChar('.');

    long long int i = (url.startsWith(Trait::latin1ToString("mailto:")) ? 7 : 0);
    const auto dogPos = url.indexOf(s_dog, i);

    if (dogPos != -1) {
        if (i == dogPos) {
            return false;
        }

        for (; i < dogPos; ++i) {
            if (!isAllowed(url[i]) && !isAdditional(url[i])) {
                return false;
            }
        }

        auto checkToDot = [&](long long int start, long long int dotPos) -> bool {
            static const long long int maxlen = 63;

            if (dotPos - start > maxlen ||
                start + 1 > dotPos ||
                start >= url.length() ||
                dotPos > url.length()) {
                return false;
            }

            if (url[start] == s_delim) {
                return false;
            }

            if (url[dotPos - 1] == s_delim) {
                return false;
            }

            for (; start < dotPos; ++start) {
                if (!isAllowed(url[start]) && url[start] != s_delim) {
                    return false;
                }
            }

            return true;
        };

        long long int dotPos = url.indexOf(s_dot, dogPos + 1);

        if (dotPos != -1) {
            i = dogPos + 1;

            while (dotPos != -1) {
                if (!checkToDot(i, dotPos)) {
                    return false;
                }

                i = dotPos + 1;
                dotPos = url.indexOf(s_dot, i);
            }

            if (!checkToDot(i, url.length())) {
                return false;
            }

            return true;
        }
    }

    return false;
}

template<class Trait>
inline bool
isValidUrl(const typename Trait::String &url);

template<class Trait>
inline bool
isGitHubAutolink(const typename Trait::String &url);

#ifdef MD4QT_QT_SUPPORT

template<>
inline bool
isValidUrl<QStringTrait>(const QString &url)
{
    const QUrl u(url, QUrl::StrictMode);

    return (u.isValid() && !u.isRelative());
}

template<>
inline bool
isGitHubAutolink<QStringTrait>(const QString &url)
{
    const QUrl u(url, QUrl::StrictMode);

    return (u.isValid()
            && ((!u.scheme().isEmpty() && !u.host().isEmpty())
                || (url.startsWith(QStringLiteral("www.")) && url.length() >= 7 &&
                    url.indexOf(QLatin1Char('.'), 4) != -1)));
}

#endif

#ifdef MD4QT_ICU_STL_SUPPORT

template<>
inline bool
isValidUrl<UnicodeStringTrait>(const UnicodeString &url)
{
    const UrlUri u(url);

    return (u.isValid() && !u.isRelative());
}

template<>
inline bool
isGitHubAutolink<UnicodeStringTrait>(const UnicodeString &url)
{
    const UrlUri u(url);

    return (u.isValid()
            && ((!u.scheme().isEmpty() && !u.host().isEmpty())
                || (url.startsWith(UnicodeString("www.")) && url.length() >= 7 &&
                    url.indexOf(UnicodeChar('.'), 4) != -1)));
}

#endif

template<class Trait>
inline long long int
processGitHubAutolinkExtension(std::shared_ptr<Paragraph<Trait>> p,
                               TextParsingOpts<Trait> &po,
                               long long int idx)
{
    if (idx < 0 || idx >= (long long int)po.m_rawTextData.size()) {
        return idx;
    }

    static const auto s_delims = Trait::latin1ToString("*_~()<>");
    auto s = po.m_rawTextData[idx];
    bool first = true;
    long long int j = 0;
    auto end = typename Trait::Char(0x00);
    bool skipSpace = true;
    long long int ret = idx;

    while (s.m_str.length()) {
        long long int i = 0;
        end = typename Trait::Char(0x00);

        for (; i < s.m_str.length(); ++i) {
            if (first) {
                if (s.m_str[i] == Trait::latin1ToChar('(')) {
                    end = Trait::latin1ToChar(')');
                }

                if (s_delims.indexOf(s.m_str[i]) == -1 && !s.m_str[i].isSpace()) {
                    first = false;
                    j = i;
                }
            } else {
                if (s.m_str[i].isSpace() || i == s.m_str.length() - 1 || s.m_str[i] == end) {
                    auto tmp = s.m_str.sliced(j, i - j +
                        (i == s.m_str.length() - 1 && s.m_str[i] != end && !s.m_str[i].isSpace() ?
                            1 : 0));
                    skipSpace = s.m_str[i].isSpace();

                    const auto email = isEmail<Trait>(tmp);

                    if (isGitHubAutolink<Trait>(tmp) || email) {
                        auto ti = textAtIdx(p, idx);

                        if (ti >= 0 && ti < static_cast<long long int>(p->items().size())) {
                            typename ItemWithOpts<Trait>::Styles openStyles, closeStyles;
                            const auto opts = std::static_pointer_cast<Text<Trait>>(p->items().at(ti))->opts();

                            if (j == 0 || s.m_str.sliced(0, j).simplified().isEmpty()) {
                                openStyles = std::static_pointer_cast<ItemWithOpts<Trait>>(p->items().at(ti))->openStyles();
                                closeStyles = std::static_pointer_cast<ItemWithOpts<Trait>>(p->items().at(ti))->closeStyles();
                                p->removeItemAt(ti);
                                po.m_rawTextData.erase(po.m_rawTextData.cbegin() + idx);
                                --ret;
                            } else {
                                const auto tmp = s.m_str.sliced(0, j);

                                auto t = std::static_pointer_cast<Text<Trait>>(p->items().at(ti));
                                t->setEndColumn(po.m_fr.m_data.at(s.m_line).first.virginPos(s.m_pos + j - 1));
                                closeStyles = t->closeStyles();
                                t->closeStyles() = {};
                                po.m_rawTextData[idx].m_str = tmp;
                                ++idx;
                                auto text = replaceEntity<Trait>(tmp.simplified());
                                text = removeBackslashes<typename Trait::String, Trait>(text);
                                t->setText(text);
                                t->setSpaceAfter(true);
                                t->setSpaceBefore(s.m_pos > 0 ? po.m_fr.m_data[s.m_line].first[s.m_pos - 1].isSpace() : true);
                                ++ti;
                            }

                            std::shared_ptr<Link<Trait>> lnk(new Link<Trait>);
                            lnk->setStartColumn(po.m_fr.m_data.at(s.m_line).first.virginPos(s.m_pos + j));
                            lnk->setStartLine(po.m_fr.m_data.at(s.m_line).second.m_lineNumber);
                            lnk->setEndColumn(
                                po.m_fr.m_data.at(s.m_line).first.virginPos(s.m_pos + i -
                                    (i == s.m_str.length() - 1 && s.m_str[i] != end && !s.m_str[i].isSpace() ?
                                        0 : 1)));
                            lnk->setEndLine(po.m_fr.m_data.at(s.m_line).second.m_lineNumber);
                            lnk->openStyles() = openStyles;
                            lnk->setTextPos({lnk->startColumn(), lnk->startLine(), lnk->endColumn(), lnk->endLine()});
                            lnk->setUrlPos(lnk->textPos());

                            if (email && !tmp.toLower().startsWith(Trait::latin1ToString("mailto:"))) {
                                tmp = Trait::latin1ToString("mailto:") + tmp;
                            }

                            if (!email && tmp.toLower().startsWith(Trait::latin1ToString("www."))) {
                                tmp = Trait::latin1ToString("http://") + tmp;
                            }

                            lnk->setUrl(tmp);
                            lnk->setOpts(opts);
                            p->insertItem(ti, lnk);

                            s.m_pos += i + (s.m_str[i] == end ? 0 : 1);
                            s.m_str.remove(0, i + (s.m_str[i] == end ? 0 : 1));
                            s.m_spaceBefore = true;
                            j = 0;
                            i = 0;

                            if (s.m_str.simplified().isEmpty()) {
                                s.m_str.clear();
                            }

                            if (!s.m_str.isEmpty()) {
                                po.m_rawTextData.insert(po.m_rawTextData.cbegin() + idx, s);
                                ++ret;

                                auto t = std::make_shared<Text<Trait>>();
                                t->setStartColumn(po.m_fr.m_data[s.m_line].first.virginPos(s.m_pos));
                                t->setStartLine(po.m_fr.m_data.at(s.m_line).second.m_lineNumber);
                                t->setEndLine(po.m_fr.m_data.at(s.m_line).second.m_lineNumber);
                                t->setEndColumn(po.m_fr.m_data.at(s.m_line).first.virginPos(s.m_pos + s.m_str.length() - 1));
                                auto text = replaceEntity<Trait>(s.m_str);
                                text = removeBackslashes<typename Trait::String, Trait>(text);
                                t->setText(text);
                                t->setSpaceAfter(s.m_spaceAfter);
                                t->setSpaceBefore(s.m_pos > 0 ? po.m_fr.m_data[s.m_line].first[s.m_pos - 1].isSpace() : true);
                                t->closeStyles() = closeStyles;
                                p->insertItem(ti + 1, t);
                            } else {
                                lnk->closeStyles() = closeStyles;
                            }

                            break;
                        }
                    }

                    j = i + (skipSpace ? 1 : 0);
                }
            }
        }

        first = true;

        if (i == s.m_str.length()) {
            break;
        }
    }

    return ret;
}

template<class Trait>
inline void
githubAutolinkPlugin(std::shared_ptr<Paragraph<Trait>> p,
                     TextParsingOpts<Trait> &po,
                     const typename Trait::StringList &)
{
    if (!po.m_collectRefLinks) {
        long long int i = 0;

        while (i >= 0 && i < (long long int)po.m_rawTextData.size()) {
            i = processGitHubAutolinkExtension(p, po, i);

            ++i;
        }
    }
}

//
// Parser
//

//! MD parser.
template<class Trait>
class Parser final
{
public:
    Parser()
    {
        addTextPlugin(GitHubAutoLinkPluginID, githubAutolinkPlugin<Trait>, false, {});
    }

    ~Parser() = default;

    //! \return Parsed Markdown document.
    std::shared_ptr<Document<Trait>>
    parse(
        //! File name of the Markdown document.
        const typename Trait::String &fileName,
        //! Should parsing be recursive? If recursive all links to existing Markdown
        //! files will be parsed and presented in the returned document.
        bool recursive = true,
        //! Allowed extensions for Markdonw document files. If Markdown file doesn't
        //! have given extension it will be ignored.
        const typename Trait::StringList &ext = {Trait::latin1ToString("md"), Trait::latin1ToString("markdown")},
        //! Make full optimization, or just semi one. In full optimization
        //! text items with one style but with some closing delimiters
        //! in the middle will be concatenated in one, like in **text* text*,
        //! here in full optimization will be "text text" with 2 open/close
        //! style delimiters, but one closing delimiter is in the middle.
        bool fullyOptimizeParagraphs = true);

    //! \return Parsed Markdown document.
    std::shared_ptr<Document<Trait>>
    parse(
        //! Stream to parse.
        typename Trait::TextStream &stream,
        //! Absolute path to the root folder for the document.
        //! This path will be used to resolve local links.
        const typename Trait::String &path,
        //! This argument needed only for anchor.
        const typename Trait::String &fileName,
        //! Make full optimization, or just semi one. In full optimization
        //! text items with one style but with some closing delimiters
        //! in the middle will be concatenated in one, like in **text* text*,
        //! here in full optimization will be "text text" with 2 open/close
        //! style delimiters, but one closing delimiter is in the middle.
        bool fullyOptimizeParagraphs = true);

    //! Add text plugin.
    void
    addTextPlugin(
        //! ID of a plugin. Use TextPlugin::UserDefinedPluginID value for start ID.
        int id,
        //! Function of a plugin, that will be invoked to processs raw text.
        TextPluginFunc<Trait> plugin,
        //! Should this plugin be used in parsing of internals of links?
        bool processInLinks,
        //! User data that will be passed to plugin function.
        const typename Trait::StringList &userData)
    {
        m_textPlugins.insert({id, {plugin, processInLinks, userData}});
    }

    //! Remove text plugin.
    void
    removeTextPlugin(
        //! ID of plugin that should be removed.
        int id)
    {
        m_textPlugins.erase(id);
    }

private:
    void
    parseFile(const typename Trait::String &fileName,
              bool recursive,
              std::shared_ptr<Document<Trait>> doc,
              const typename Trait::StringList &ext,
              typename Trait::StringList *parentLinks = nullptr);

    void
    parseStream(typename Trait::TextStream &stream,
                const typename Trait::String &workingPath,
                const typename Trait::String &fileName,
                bool recursive,
                std::shared_ptr<Document<Trait>> doc,
                const typename Trait::StringList &ext,
                typename Trait::StringList *parentLinks = nullptr);

    void
    clearCache();

    enum class BlockType {
        Unknown,
        EmptyLine,
        Text,
        List,
        ListWithFirstEmptyLine,
        CodeIndentedBySpaces,
        Code,
        Blockquote,
        Heading,
        SomethingInList,
        FensedCodeInList,
        Footnote
    }; // enum BlockType

    struct ListIndent {
        long long int m_level = -1;
        long long int m_indent = -1;
    }; // struct ListIndent

    BlockType
    whatIsTheLine(typename Trait::InternalString &str,
                  bool inList = false,
                  bool inListWithFirstEmptyLine = false,
                  bool fensedCodeInList = false,
                  typename Trait::String *startOfCode = nullptr,
                  ListIndent *indent = nullptr,
                  bool emptyLinePreceded = false,
                  bool calcIndent = false,
                  const std::vector<long long int> *indents = nullptr);

    long long int
    parseFragment(MdBlock<Trait> &fr,
                  std::shared_ptr<Block<Trait>> parent,
                  std::shared_ptr<Document<Trait>> doc,
                  typename Trait::StringList &linksToParse,
                  const typename Trait::String &workingPath,
                  const typename Trait::String &fileName,
                  bool collectRefLinks,
                  RawHtmlBlock<Trait> &html);

    void
    parseText(MdBlock<Trait> &fr,
              std::shared_ptr<Block<Trait>> parent,
              std::shared_ptr<Document<Trait>> doc,
              typename Trait::StringList &linksToParse,
              const typename Trait::String &workingPath,
              const typename Trait::String &fileName,
              bool collectRefLinks,
              RawHtmlBlock<Trait> &html);

    void
    parseBlockquote(MdBlock<Trait> &fr,
                    std::shared_ptr<Block<Trait>> parent,
                    std::shared_ptr<Document<Trait>> doc,
                    typename Trait::StringList &linksToParse,
                    const typename Trait::String &workingPath,
                    const typename Trait::String &fileName,
                    bool collectRefLinks,
                    RawHtmlBlock<Trait> &html);

    long long int
    parseList(MdBlock<Trait> &fr,
              std::shared_ptr<Block<Trait>> parent,
              std::shared_ptr<Document<Trait>> doc,
              typename Trait::StringList &linksToParse,
              const typename Trait::String &workingPath,
              const typename Trait::String &fileName,
              bool collectRefLinks,
              RawHtmlBlock<Trait> &html);

    void
    parseCode(MdBlock<Trait> &fr, std::shared_ptr<Block<Trait>> parent, bool collectRefLinks);

    void
    parseCodeIndentedBySpaces(MdBlock<Trait> &fr,
                              std::shared_ptr<Block<Trait>> parent,
                              bool collectRefLinks,
                              int indent,
                              const typename Trait::String &syntax,
                              long long int emptyColumn,
                              long long int startLine,
                              bool fensedCode,
                              const WithPosition &startDelim = {},
                              const WithPosition &endDelim = {},
                              const WithPosition &syntaxPos = {});

    long long int
    parseListItem(MdBlock<Trait> &fr,
                  std::shared_ptr<Block<Trait>> parent,
                  std::shared_ptr<Document<Trait>> doc,
                  typename Trait::StringList &linksToParse,
                  const typename Trait::String &workingPath,
                  const typename Trait::String &fileName,
                  bool collectRefLinks,
                  RawHtmlBlock<Trait> &html,
                  std::shared_ptr<ListItem<Trait>> *resItem = nullptr);

    void
    parseHeading(MdBlock<Trait> &fr,
                 std::shared_ptr<Block<Trait>> parent,
                 std::shared_ptr<Document<Trait>> doc,
                 typename Trait::StringList &linksToParse,
                 const typename Trait::String &workingPath,
                 const typename Trait::String &fileName,
                 bool collectRefLinks);

    void
    parseFootnote(MdBlock<Trait> &fr,
                  std::shared_ptr<Block<Trait>> parent,
                  std::shared_ptr<Document<Trait>> doc,
                  typename Trait::StringList &linksToParse,
                  const typename Trait::String &workingPath,
                  const typename Trait::String &fileName,
                  bool collectRefLinks);

    void
    parseTable(MdBlock<Trait> &fr,
               std::shared_ptr<Block<Trait>> parent,
               std::shared_ptr<Document<Trait>> doc,
               typename Trait::StringList &linksToParse,
               const typename Trait::String &workingPath,
               const typename Trait::String &fileName,
               bool collectRefLinks,
               int columnsCount);

    void
    parseParagraph(MdBlock<Trait> &fr,
                   std::shared_ptr<Block<Trait>> parent,
                   std::shared_ptr<Document<Trait>> doc,
                   typename Trait::StringList &linksToParse,
                   const typename Trait::String &workingPath,
                   const typename Trait::String &fileName,
                   bool collectRefLinks,
                   RawHtmlBlock<Trait> &html);

    void
    parseFormattedTextLinksImages(MdBlock<Trait> &fr,
                                  std::shared_ptr<Block<Trait>> parent,
                                  std::shared_ptr<Document<Trait>> doc,
                                  typename Trait::StringList &linksToParse,
                                  const typename Trait::String &workingPath,
                                  const typename Trait::String &fileName,
                                  bool collectRefLinks,
                                  bool ignoreLineBreak,
                                  RawHtmlBlock<Trait> &html,
                                  bool inLink);

    RawHtmlBlock<Trait>
    parse(StringListStream<Trait> &stream,
          std::shared_ptr<Block<Trait>> parent,
          std::shared_ptr<Document<Trait>> doc,
          typename Trait::StringList &linksToParse,
          const typename Trait::String &workingPath,
          const typename Trait::String &fileName,
          bool collectRefLinks,
          bool top = false,
          bool dontProcessLastFreeHtml = false);

    struct ParserContext {
        typename Trait::template Vector<MdBlock<Trait>> m_splitted;
        typename MdBlock<Trait>::Data m_fragment;
        bool m_emptyLineInList = false;
        bool m_fensedCodeInList = false;
        long long int m_emptyLinesCount = 0;
        long long int m_lineCounter = 0;
        std::vector<long long int> m_indents;
        ListIndent m_indent;
        RawHtmlBlock<Trait> m_html;
        long long int m_emptyLinesBefore = 0;
        MdLineData::CommentDataMap m_htmlCommentData;
        typename Trait::String m_startOfCode;
        typename Trait::String m_startOfCodeInList;
        BlockType m_type = BlockType::EmptyLine;
        BlockType m_lineType = BlockType::Unknown;
        BlockType m_prevLineType = BlockType::Unknown;
    }; // struct ParserContext

    void
    parseFragment(ParserContext &ctx,
                  std::shared_ptr<Block<Trait>> parent,
                  std::shared_ptr<Document<Trait>> doc,
                  typename Trait::StringList &linksToParse,
                  const typename Trait::String &workingPath,
                  const typename Trait::String &fileName,
                  bool collectRefLinks);

    void
    eatFootnote(ParserContext &ctx,
                StringListStream<Trait> &stream,
                std::shared_ptr<Block<Trait>> parent,
                std::shared_ptr<Document<Trait>> doc,
                typename Trait::StringList &linksToParse,
                const typename Trait::String &workingPath,
                const typename Trait::String &fileName,
                bool collectRefLinks);

    void
    finishHtml(ParserContext &ctx,
               std::shared_ptr<Block<Trait>> parent,
               std::shared_ptr<Document<Trait>> doc,
               bool collectRefLinks,
               bool top,
               bool dontProcessLastFreeHtml);

    void
    makeLineMain(ParserContext &ctx,
                 const typename Trait::InternalString &line,
                 long long int emptyLinesCount,
                 const ListIndent &currentIndent,
                 long long int ns,
                 long long int currentLineNumber);

    void
    parseFragmentAndMakeNextLineMain(ParserContext &ctx,
                                     std::shared_ptr<Block<Trait>> parent,
                                     std::shared_ptr<Document<Trait>> doc,
                                     typename Trait::StringList &linksToParse,
                                     const typename Trait::String &workingPath,
                                     const typename Trait::String &fileName,
                                     bool collectRefLinks,
                                     const typename Trait::InternalString &line,
                                     const ListIndent &currentIndent,
                                     long long int ns,
                                     long long int currentLineNumber);

    bool
    isListType(BlockType t);

    typename Trait::InternalString
    readLine(ParserContext &ctx, StringListStream<Trait> &stream);

    std::shared_ptr<Image<Trait>>
    makeImage(const typename Trait::String &url,
              const typename MdBlock<Trait>::Data &text,
              TextParsingOpts<Trait> &po,
              bool doNotCreateTextOnFail,
              long long int startLine,
              long long int startPos,
              long long int lastLine,
              long long int lastPos,
              const WithPosition &textPos,
              const WithPosition &urlPos);

    std::shared_ptr<Link<Trait>>
    makeLink(const typename Trait::String &url,
             const typename MdBlock<Trait>::Data &text,
             TextParsingOpts<Trait> &po,
             bool doNotCreateTextOnFail,
             long long int startLine,
             long long int startPos,
             long long int lastLine,
             long long int lastPos,
             const WithPosition &textPos,
             const WithPosition &urlPos);

    struct Delimiter {
        enum DelimiterType {
            // (
            ParenthesesOpen,
            // )
            ParenthesesClose,
            // [
            SquareBracketsOpen,
            // ]
            SquareBracketsClose,
            // ![
            ImageOpen,
            // ~~
            Strikethrough,
            // *
            Emphasis1,
            // _
            Emphasis2,
            // `
            InlineCode,
            // <
            Less,
            // >
            Greater,
            // $
            Math,
            HorizontalLine,
            H1,
            H2,
            Unknown
        }; // enum DelimiterType

        DelimiterType m_type = Unknown;
        long long int m_line = -1;
        long long int m_pos = -1;
        long long int m_len = 0;
        bool m_spaceBefore = false;
        bool m_spaceAfter = false;
        bool m_isWordBefore = false;
        bool m_backslashed = false;
        bool m_leftFlanking = false;
        bool m_rightFlanking = false;
    }; // struct Delimiter

    using Delims = typename Trait::template Vector<Delimiter>;

    bool
    createShortcutImage(const typename MdBlock<Trait>::Data &text,
                        TextParsingOpts<Trait> &po,
                        long long int startLine,
                        long long int startPos,
                        long long int lastLineForText,
                        long long int lastPosForText,
                        typename Delims::const_iterator lastIt,
                        const typename MdBlock<Trait>::Data &linkText,
                        bool doNotCreateTextOnFail,
                        const WithPosition &textPos,
                        const WithPosition &linkTextPos);

    typename Delims::const_iterator
    checkForImage(typename Delims::const_iterator it,
                  typename Delims::const_iterator last,
                  TextParsingOpts<Trait> &po);

    bool
    createShortcutLink(const typename MdBlock<Trait>::Data &text,
                       TextParsingOpts<Trait> &po,
                       long long int startLine,
                       long long int startPos,
                       long long int lastLineForText,
                       long long int lastPosForText,
                       typename Delims::const_iterator lastIt,
                       const typename MdBlock<Trait>::Data &linkText,
                       bool doNotCreateTextOnFail,
                       const WithPosition &textPos,
                       const WithPosition &linkTextPos);

    typename Delims::const_iterator
    checkForLink(typename Delims::const_iterator it,
                 typename Delims::const_iterator last,
                 TextParsingOpts<Trait> &po);

    Delims
    collectDelimiters(const typename MdBlock<Trait>::Data &fr);

    std::pair<typename Trait::String, bool>
    readHtmlTag(typename Delims::const_iterator it, TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    findIt(typename Delims::const_iterator it,
           typename Delims::const_iterator last,
           TextParsingOpts<Trait> &po);

    void
    finishRule1HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po,
                       bool skipFirst);

    void
    finishRule2HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    void
    finishRule3HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    void
    finishRule4HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    void
    finishRule5HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    void
    finishRule6HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    typename Parser<Trait>::Delims::const_iterator
    finishRule7HtmlTag(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    finishRawHtmlTag(typename Delims::const_iterator it,
                     typename Delims::const_iterator last,
                     TextParsingOpts<Trait> &po,
                     bool skipFirst);

    int
    htmlTagRule(typename Delims::const_iterator it,
                typename Delims::const_iterator last,
                TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    checkForRawHtml(typename Delims::const_iterator it,
                    typename Delims::const_iterator last,
                    TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    checkForMath(typename Delims::const_iterator it,
                 typename Delims::const_iterator last,
                 TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    checkForAutolinkHtml(typename Delims::const_iterator it,
                         typename Delims::const_iterator last,
                         TextParsingOpts<Trait> &po,
                         bool updatePos);

    typename Delims::const_iterator
    checkForInlineCode(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po);

    std::pair<typename MdBlock<Trait>::Data, typename Delims::const_iterator>
    readTextBetweenSquareBrackets(typename Delims::const_iterator start,
                                  typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po,
                                  bool doNotCreateTextOnFail,
                                  WithPosition *pos = nullptr);

    std::pair<typename MdBlock<Trait>::Data, typename Delims::const_iterator>
    checkForLinkText(typename Delims::const_iterator it,
                     typename Delims::const_iterator last,
                     TextParsingOpts<Trait> &po,
                     WithPosition *pos = nullptr);

    std::pair<typename MdBlock<Trait>::Data, typename Delims::const_iterator>
    checkForLinkLabel(typename Delims::const_iterator it,
                      typename Delims::const_iterator last,
                      TextParsingOpts<Trait> &po,
                      WithPosition *pos = nullptr);

    std::tuple<typename Trait::String, typename Trait::String, typename Delims::const_iterator, bool>
    checkForInlineLink(typename Delims::const_iterator it,
                       typename Delims::const_iterator last,
                       TextParsingOpts<Trait> &po,
                       WithPosition *urlPos = nullptr);

    inline std::tuple<typename Trait::String, typename Trait::String, typename Delims::const_iterator, bool>
    checkForRefLink(typename Delims::const_iterator it,
                    typename Delims::const_iterator last,
                    TextParsingOpts<Trait> &po,
                    WithPosition *urlPos = nullptr);

    typename Trait::String
    toSingleLine(const typename MdBlock<Trait>::Data &d);

    template<class Func>
    typename Delims::const_iterator
    checkShortcut(typename Delims::const_iterator it,
                  typename Delims::const_iterator last,
                  TextParsingOpts<Trait> &po,
                  Func functor)
    {
        const auto start = it;

        typename MdBlock<Trait>::Data text;

        WithPosition labelPos;
        std::tie(text, it) = checkForLinkLabel(start, last, po, &labelPos);

        if (it != start && !toSingleLine(text).simplified().isEmpty()) {
            if ((this->*functor)(text, po, start->m_line, start->m_pos, start->m_line,
                start->m_pos + start->m_len, it, {}, false, labelPos, {})) {
                return it;
            }
        }

        return start;
    }

    void
    createStyles(std::vector<std::pair<Style, long long int>> &s,
                 long long int l,
                 typename Delimiter::DelimiterType t,
                 long long int &count);

    bool
    isSequence(typename Delims::const_iterator it,
               long long int itLine,
               long long int itPos,
               typename Delimiter::DelimiterType t);

    typename Delims::const_iterator
    readSequence(typename Delims::const_iterator it,
                 typename Delims::const_iterator last,
                 long long int &line,
                 long long int &pos,
                 long long int &len,
                 typename Delims::const_iterator &current);

    int
    emphasisToInt(typename Delimiter::DelimiterType t);

    std::pair<bool, size_t>
    checkEmphasisSequence(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
                          size_t idx);

    std::vector<std::pair<std::pair<long long int, bool>, int>>
    fixSequence(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s);

    std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>>
    closedSequences(const std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> &vars,
                    size_t idx);

    std::vector<std::pair<Style, long long int>>
    createStyles(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
                 size_t i,
                 typename Delimiter::DelimiterType t,
                 long long int &count);

    std::tuple<bool, std::vector<std::pair<Style, long long int>>, long long int, long long int>
    isStyleClosed(typename Delims::const_iterator it,
                  typename Delims::const_iterator last,
                  TextParsingOpts<Trait> &po);

    typename Delims::const_iterator
    incrementIterator(typename Delims::const_iterator it,
                      typename Delims::const_iterator last,
                      long long int count);

    typename Delims::const_iterator
    checkForStyle(typename Delims::const_iterator first,
                  typename Delims::const_iterator it,
                  typename Delims::const_iterator last,
                  TextParsingOpts<Trait> &po);

    bool
    isListOrQuoteAfterHtml(TextParsingOpts<Trait> &po);

    void
    parseTableInParagraph(TextParsingOpts<Trait> &po,
                          std::shared_ptr<Paragraph<Trait>> parent,
                          std::shared_ptr<Document<Trait>> doc,
                          typename Trait::StringList &linksToParse,
                          const typename Trait::String &workingPath,
                          const typename Trait::String &fileName,
                          bool collectRefLinks);

    bool
    isNewBlockIn(MdBlock<Trait> &fr,
                 long long int startLine,
                 long long int endLine);

    void
    makeInlineCode(long long int startLine,
                   long long int startPos,
                   long long int lastLine,
                   long long int lastPos,
                   TextParsingOpts<Trait> &po,
                   typename Delims::const_iterator startDelimIt,
                   typename Delims::const_iterator endDelimIt);

    OptimizeParagraphType
    defaultParagraphOptimization() const
    {
        return (m_fullyOptimizeParagraphs ? OptimizeParagraphType::Full :
            OptimizeParagraphType::Semi);
    }

private:
    friend struct PrivateAccess;

private:
    typename Trait::StringList m_parsedFiles;
    TextPluginsMap<Trait> m_textPlugins;
    bool m_fullyOptimizeParagraphs = true;

    MD_DISABLE_COPY(Parser)
}; // class Parser

//
// Parser
//

template<class Trait>
inline std::shared_ptr<Document<Trait>>
Parser<Trait>::parse(const typename Trait::String &fileName,
                     bool recursive,
                     const typename Trait::StringList &ext,
                     bool fullyOptimizeParagraphs)
{
    m_fullyOptimizeParagraphs = fullyOptimizeParagraphs;

    std::shared_ptr<Document<Trait>> doc(new Document<Trait>);

    parseFile(fileName, recursive, doc, ext);

    clearCache();

    return doc;
}

template<class Trait>
inline std::shared_ptr<Document<Trait>>
Parser<Trait>::parse(typename Trait::TextStream &stream,
                     const typename Trait::String &path,
                     const typename Trait::String &fileName,
                     bool fullyOptimizeParagraphs)
{
    m_fullyOptimizeParagraphs = fullyOptimizeParagraphs;

    std::shared_ptr<Document<Trait>> doc(new Document<Trait>);

    parseStream(stream, path, fileName, false, doc, typename Trait::StringList());

    clearCache();

    return doc;
}

template<class Trait>
class TextStream;

#ifdef MD4QT_QT_SUPPORT

//! Wrapper for QTextStream.
template<>
class TextStream<QStringTrait>
{
public:
    TextStream(QTextStream &stream)
        : m_stream(stream)
        , m_lastBuf(false)
        , m_pos(0)
    {
    }

    bool
    atEnd() const
    {
        return (m_lastBuf && m_pos == m_buf.size());
    }

    QString
    readLine()
    {
        QString line;
        bool rFound = false;

        while (!atEnd()) {
            const auto c = getChar();

            if (rFound && c != QLatin1Char('\n')) {
                --m_pos;

                return line;
            }

            if (c == QLatin1Char('\r')) {
                rFound = true;

                continue;
            } else if (c == QLatin1Char('\n')) {
                return line;
            }

            if (!c.isNull()) {
                line.push_back(c);
            }
        }

        return line;
    }

private:
    void
    fillBuf()
    {
        m_buf = m_stream.read(512);

        if (m_stream.atEnd()) {
            m_lastBuf = true;
        }

        m_pos = 0;
    }

    QChar
    getChar()
    {
        if (m_pos < m_buf.size()) {
            return m_buf.at(m_pos++);
        } else if (!atEnd()) {
            fillBuf();

            return getChar();
        } else {
            return QChar();
        }
    }

private:
    QTextStream &m_stream;
    QString m_buf;
    bool m_lastBuf;
    long long int m_pos;
}; // class TextStream

#endif

#ifdef MD4QT_ICU_STL_SUPPORT

//! Wrapper for std::istream.
template<>
class TextStream<UnicodeStringTrait>
{
public:
    TextStream(std::istream &stream)
        : m_pos(0)
    {
        std::vector<unsigned char> content;

        stream.seekg(0, std::ios::end);
        const auto ssize = stream.tellg();
        content.resize((size_t)ssize + 1);
        stream.seekg(0, std::ios::beg);
        stream.read((char *)&content[0], ssize);
        content[(size_t)ssize] = 0;

        const auto z = std::count(content.cbegin(), content.cend(), 0);

        if (z > 1) {
            std::vector<unsigned char> tmp;
            tmp.resize(content.size() + (z - 1) * 2);

            for (size_t i = 0, j = 0; i < content.size() - 1; ++i, ++j) {
                if (content[i] == 0) {
                    // 0xFFFD - replacement character in UTF-8.
                    tmp[j++] = 0xEF;
                    tmp[j++] = 0xBF;
                    tmp[j] = 0xBD;
                } else {
                    tmp[j] = content[i];
                }
            }

            tmp[tmp.size() - 1] = 0;

            std::swap(content, tmp);
        }

        m_str = UnicodeString::fromUTF8((char *)&content[0]);
    }

    bool
    atEnd() const
    {
        return m_pos == m_str.size();
    }

    UnicodeString
    readLine()
    {
        UnicodeString line;

        bool rFound = false;

        while (!atEnd()) {
            const auto c = getChar();

            if (rFound && c != UnicodeChar('\n')) {
                --m_pos;

                return line;
            }

            if (c == UnicodeChar('\r')) {
                rFound = true;

                continue;
            } else if (c == UnicodeChar('\n')) {
                return line;
            }

            if (!c.isNull()) {
                line.push_back(c);
            }
        }

        return line;
    }

private:
    UnicodeChar
    getChar()
    {
        if (!atEnd()) {
            return m_str[m_pos++];
        } else {
            return UnicodeChar();
        }
    }

private:
    UnicodeString m_str;
    long long int m_pos;
};

#endif

template<class Trait>
inline bool
checkForEndHtmlComments(const typename Trait::String &line,
                        long long int pos)
{
    const long long int e = line.indexOf(Trait::latin1ToString("-->"), pos);

    if (e != -1) {
        return isHtmlComment<Trait>(line.sliced(0, e + 3));
    }

    return false;
}

template<class Trait>
inline void
checkForHtmlComments(const typename Trait::InternalString &line,
                     StringListStream<Trait> &stream,
                     MdLineData::CommentDataMap &res)
{
    long long int p = 0, l = stream.currentLineNumber();

    const auto &str = line.asString();

    while ((p = str.indexOf(Trait::latin1ToString(s_startComment), p)) != -1) {
        bool addNegative = false;

        auto c = str.sliced(p);

        if (c.startsWith(Trait::latin1ToString("<!-->"))) {
            res.insert({line.virginPos(p), {0, true}});

            p += 5;

            continue;
        } else if (c.startsWith(Trait::latin1ToString("<!--->"))) {
            res.insert({line.virginPos(p), {1, true}});

            p += 6;

            continue;
        }

        if (checkForEndHtmlComments<Trait>(c, 4)) {
            res.insert({line.virginPos(p), {2, true}});
        } else {
            addNegative = true;

            for (; l < stream.size(); ++l) {
                c.push_back(Trait::latin1ToChar(' '));
                c.push_back(stream.lineAt(l).asString());

                if (checkForEndHtmlComments<Trait>(c, 4)) {
                    res.insert({line.virginPos(p), {2, true}});

                    addNegative = false;

                    break;
                }
            }
        }

        if (addNegative) {
            res.insert({line.virginPos(p), {-1, false}});
        }

        ++p;
    }
}

template<class Trait>
inline void
Parser<Trait>::parseFragment(typename Parser<Trait>::ParserContext &ctx,
                             std::shared_ptr<Block<Trait>> parent,
                             std::shared_ptr<Document<Trait>> doc,
                             typename Trait::StringList &linksToParse,
                             const typename Trait::String &workingPath,
                             const typename Trait::String &fileName,
                             bool collectRefLinks)
{
    if (!ctx.m_fragment.empty()) {
        MdBlock<Trait> block = {ctx.m_fragment, ctx.m_emptyLinesBefore, ctx.m_emptyLinesCount > 0};

        ctx.m_emptyLinesBefore = ctx.m_emptyLinesCount;

        ctx.m_splitted.push_back(block);

        long long int line = 0;

        while (line >= 0) {
            line = parseFragment(block, parent, doc, linksToParse, workingPath,
                fileName, collectRefLinks, ctx.m_html);

            assert(line != 0);

            if (line > 0) {
                if (ctx.m_html.m_html) {
                    if (!collectRefLinks) {
                        ctx.m_html.m_parent->appendItem(ctx.m_html.m_html);
                    }

                    resetHtmlTag<Trait>(ctx.m_html);
                }

                const auto it = std::find_if(ctx.m_fragment.cbegin(), ctx.m_fragment.cend(), [line](const auto &d) {
                    return (d.second.m_lineNumber == line);
                });

                block.m_data.clear();
                std::copy(it, ctx.m_fragment.cend(), std::back_inserter(block.m_data));
                block.m_emptyLinesBefore = 0;
            }
        }

        ctx.m_fragment.clear();
    }

    ctx.m_type = BlockType::EmptyLine;
    ctx.m_emptyLineInList = false;
    ctx.m_fensedCodeInList = false;
    ctx.m_emptyLinesCount = 0;
    ctx.m_lineCounter = 0;
    ctx.m_indents.clear();
    ctx.m_indent = {-1, -1};
    ctx.m_startOfCode.clear();
    ctx.m_startOfCodeInList.clear();
}

template<class Trait>
inline void
replaceTabs(typename Trait::InternalString &s)
{
    unsigned char size = 4;
    long long int len = s.length();

    for (long long int i = 0; i < len; ++i, --size) {
        if (s[i] == Trait::latin1ToChar('\t')) {
            s.replaceOne(i, 1, typename Trait::String(size, Trait::latin1ToChar(' ')));

            len += size - 1;
            i += size - 1;
            size = 5;
        }

        if (size == 1) {
            size = 5;
        }
    }
}

template<class Trait>
inline void
Parser<Trait>::eatFootnote(typename Parser<Trait>::ParserContext &ctx,
                           StringListStream<Trait> &stream,
                           std::shared_ptr<Block<Trait>> parent,
                           std::shared_ptr<Document<Trait>> doc,
                           typename Trait::StringList &linksToParse,
                           const typename Trait::String &workingPath,
                           const typename Trait::String &fileName,
                           bool collectRefLinks)
{
    long long int emptyLinesCount = 0;
    bool wasEmptyLine = false;

    while (!stream.atEnd()) {
        const auto currentLineNumber = stream.currentLineNumber();

        auto line = readLine(ctx, stream);

        replaceTabs<Trait>(line);

        const auto ns = skipSpaces<Trait>(0, line.asString());

        if (ns == line.length() || line.asString().startsWith(Trait::latin1ToString("    "))) {
            if (ns == line.length()) {
                ++emptyLinesCount;
                wasEmptyLine = true;
            } else {
                emptyLinesCount = 0;
            }

            ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
        } else if (!wasEmptyLine) {
            if (isFootnote<Trait>(line.sliced(ns).asString())) {
                parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

                ctx.m_lineType = BlockType::Footnote;

                makeLineMain(ctx, line, emptyLinesCount, ctx.m_indent, ns, currentLineNumber);

                continue;
            } else {
                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
            }
        } else {
            parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

            ctx.m_lineType =
                whatIsTheLine(line, false, false, false, &ctx.m_startOfCodeInList, &ctx.m_indent,
                    ctx.m_lineType == BlockType::EmptyLine, true, &ctx.m_indents);

            makeLineMain(ctx, line, emptyLinesCount, ctx.m_indent, ns, currentLineNumber);

            if (ctx.m_type == BlockType::Footnote) {
                wasEmptyLine = false;

                continue;
            } else {
                break;
            }
        }
    }

    if (stream.atEnd() && !ctx.m_fragment.empty()) {
        parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishHtml(ParserContext &ctx,
                          std::shared_ptr<Block<Trait>> parent,
                          std::shared_ptr<Document<Trait>> doc,
                          bool collectRefLinks,
                          bool top,
                          bool dontProcessLastFreeHtml)
{
    if (!collectRefLinks || top) {
        if (ctx.m_html.m_html->isFreeTag()) {
            if (!dontProcessLastFreeHtml) {
                if (ctx.m_html.m_parent) {
                    ctx.m_html.m_parent->appendItem(ctx.m_html.m_html);

                    updateLastPosInList(ctx.m_html);
                } else {
                    parent->appendItem(ctx.m_html.m_html);
                }
            }
        } else {
            std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);
            p->appendItem(ctx.m_html.m_html);
            p->setStartColumn(ctx.m_html.m_html->startColumn());
            p->setStartLine(ctx.m_html.m_html->startLine());
            p->setEndColumn(ctx.m_html.m_html->endColumn());
            p->setEndLine(ctx.m_html.m_html->endLine());
            doc->appendItem(p);
        }
    }

    if (!dontProcessLastFreeHtml) {
        resetHtmlTag(ctx.m_html);
    }

    ctx.m_html.m_toAdjustLastPos.clear();
}

template<class Trait>
inline void
Parser<Trait>::makeLineMain(ParserContext &ctx,
                            const typename Trait::InternalString &line,
                            long long int emptyLinesCount,
                            const ListIndent &currentIndent,
                            long long int ns,
                            long long int currentLineNumber)
{
    if (ctx.m_html.m_htmlBlockType >= 6) {
        ctx.m_html.m_continueHtml = (emptyLinesCount <= 0);
    }

    ctx.m_type = ctx.m_lineType;

    switch (ctx.m_type) {
    case BlockType::List:
    case BlockType::ListWithFirstEmptyLine: {
        if (ctx.m_indents.empty())
            ctx.m_indents.push_back(currentIndent.m_indent);

        ctx.m_indent = currentIndent;
    } break;

    case BlockType::Code:
        ctx.m_startOfCode = startSequence<Trait>(line.asString());
        break;

    default:
        break;
    }

    if (!line.isEmpty() && ns < line.length()) {
        ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
    }

    ctx.m_lineCounter = 1;
    ctx.m_emptyLinesCount = 0;
}

template<class Trait>
inline void
Parser<Trait>::parseFragmentAndMakeNextLineMain(ParserContext &ctx,
                                                std::shared_ptr<Block<Trait>> parent,
                                                std::shared_ptr<Document<Trait>> doc,
                                                typename Trait::StringList &linksToParse,
                                                const typename Trait::String &workingPath,
                                                const typename Trait::String &fileName,
                                                bool collectRefLinks,
                                                const typename Trait::InternalString &line,
                                                const ListIndent &currentIndent,
                                                long long int ns,
                                                long long int currentLineNumber)
{
    const auto empty = ctx.m_emptyLinesCount;

    parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

    makeLineMain(ctx, line, empty, currentIndent, ns, currentLineNumber);
}

template<class Trait>
inline bool
Parser<Trait>::isListType(BlockType t)
{
    switch (t) {
    case BlockType::List:
    case BlockType::ListWithFirstEmptyLine:
        return true;

    default:
        return false;
    }
}

template<class Trait>
typename Trait::InternalString
Parser<Trait>::readLine(typename Parser<Trait>::ParserContext &ctx,
                        StringListStream<Trait> &stream)
{
    ctx.m_htmlCommentData.clear();

    auto line = stream.readLine();

    static const char16_t c_zeroReplaceWith[2] = {0xFFFD, 0};

    line.replace(typename Trait::Char(0), Trait::utf16ToString(&c_zeroReplaceWith[0]));

    checkForHtmlComments(line, stream, ctx.m_htmlCommentData);

    return line;
}

template<class Trait>
inline RawHtmlBlock<Trait>
Parser<Trait>::parse(StringListStream<Trait> &stream,
                     std::shared_ptr<Block<Trait>> parent,
                     std::shared_ptr<Document<Trait>> doc,
                     typename Trait::StringList &linksToParse,
                     const typename Trait::String &workingPath,
                     const typename Trait::String &fileName,
                     bool collectRefLinks,
                     bool top,
                     bool dontProcessLastFreeHtml)
{
    ParserContext ctx;

    while (!stream.atEnd()) {
        const auto currentLineNumber = stream.currentLineNumber();

        auto line = readLine(ctx, stream);

        if (ctx.m_lineType != BlockType::Unknown) {
            ctx.m_prevLineType = ctx.m_lineType;
        }

        ctx.m_lineType = whatIsTheLine(line,
                                       (ctx.m_emptyLineInList || isListType(ctx.m_type)),
                                       ctx.m_prevLineType == BlockType::ListWithFirstEmptyLine,
                                       ctx.m_fensedCodeInList,
                                       &ctx.m_startOfCodeInList,
                                       &ctx.m_indent,
                                       ctx.m_lineType == BlockType::EmptyLine,
                                       true,
                                       &ctx.m_indents);

        if (isListType(ctx.m_type) && ctx.m_lineType == BlockType::FensedCodeInList) {
            ctx.m_fensedCodeInList = !ctx.m_fensedCodeInList;
        }

        const auto currentIndent = ctx.m_indent;

        const auto ns = skipSpaces<Trait>(0, line.asString());

        const auto indentInListValue = indentInList(&ctx.m_indents, ns, true);

        if (isListType(ctx.m_lineType) && !ctx.m_fensedCodeInList && ctx.m_indent.m_level > -1) {
            if (ctx.m_indent.m_level < (long long int)ctx.m_indents.size()) {
                ctx.m_indents.erase(ctx.m_indents.cbegin() + ctx.m_indent.m_level, ctx.m_indents.cend());
            }

            ctx.m_indents.push_back(ctx.m_indent.m_indent);
        }

        if (ctx.m_type == BlockType::CodeIndentedBySpaces && ns > 3) {
            ctx.m_lineType = BlockType::CodeIndentedBySpaces;
        }

        if (ctx.m_type == BlockType::ListWithFirstEmptyLine && ctx.m_lineCounter == 2 &&
            !isListType(ctx.m_lineType)) {
            if (ctx.m_emptyLinesCount > 0) {
                parseFragmentAndMakeNextLineMain(ctx,
                                                 parent,
                                                 doc,
                                                 linksToParse,
                                                 workingPath,
                                                 fileName,
                                                 collectRefLinks,
                                                 line,
                                                 currentIndent,
                                                 ns,
                                                 currentLineNumber);

                continue;
            } else {
                ctx.m_emptyLineInList = false;
                ctx.m_emptyLinesCount = 0;
            }
        }

        if (ctx.m_type == BlockType::ListWithFirstEmptyLine && ctx.m_lineCounter == 2) {
            ctx.m_type = BlockType::List;
        }

        // Footnote.
        if (ctx.m_lineType == BlockType::Footnote) {
            parseFragmentAndMakeNextLineMain(ctx,
                                             parent,
                                             doc,
                                             linksToParse,
                                             workingPath,
                                             fileName,
                                             collectRefLinks,
                                             line,
                                             currentIndent,
                                             ns,
                                             currentLineNumber);

            eatFootnote(ctx, stream, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

            continue;
        }

        // First line of the fragment.
        if (ns != line.length() && ctx.m_type == BlockType::EmptyLine) {
            makeLineMain(ctx, line, ctx.m_emptyLinesCount, currentIndent, ns, currentLineNumber);

            continue;
        } else if (ns == line.length() && ctx.m_type == BlockType::EmptyLine) {
            continue;
        }

        ++ctx.m_lineCounter;

        // Got new empty line.
        if (ns == line.length()) {
            ++ctx.m_emptyLinesCount;

            switch (ctx.m_type) {
            case BlockType::Blockquote: {
                parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

                continue;
            }

            case BlockType::Text:
            case BlockType::CodeIndentedBySpaces:
                continue;
                break;

            case BlockType::Code: {
                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
                ctx.m_emptyLinesCount = 0;

                continue;
            }

            case BlockType::List:
            case BlockType::ListWithFirstEmptyLine: {
                ctx.m_emptyLineInList = true;

                continue;
            }

            default:
                break;
            }
        }
        //! Empty new line in list.
        else if (ctx.m_emptyLineInList) {
            if (indentInListValue || isListType(ctx.m_lineType) || ctx.m_lineType == BlockType::SomethingInList) {
                for (long long int i = 0; i < ctx.m_emptyLinesCount; ++i) {
                    ctx.m_fragment.push_back({typename Trait::String(),
                        {currentLineNumber - ctx.m_emptyLinesCount + i, {}}});
                }

                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});

                ctx.m_emptyLineInList = false;
                ctx.m_emptyLinesCount = 0;

                continue;
            } else {
                const auto empty = ctx.m_emptyLinesCount;

                parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);

                ctx.m_lineType = whatIsTheLine(line, false, false, false, nullptr, nullptr,
                    true, false, &ctx.m_indents);

                makeLineMain(ctx, line, empty, currentIndent, ns, currentLineNumber);

                continue;
            }
        } else if (ctx.m_emptyLinesCount > 0) {
            if (ctx.m_type == BlockType::CodeIndentedBySpaces &&
                ctx.m_lineType == BlockType::CodeIndentedBySpaces) {
                const auto indent = skipSpaces<Trait>(0, ctx.m_fragment.front().first.asString());

                for (long long int i = 0; i < ctx.m_emptyLinesCount; ++i) {
                    ctx.m_fragment.push_back({typename Trait::String(indent, Trait::latin1ToChar(' ')),
                        {currentLineNumber - ctx.m_emptyLinesCount + i, {}}});
                }

                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
                ctx.m_emptyLinesCount = 0;
            } else {
                parseFragmentAndMakeNextLineMain(ctx,
                                                 parent,
                                                 doc,
                                                 linksToParse,
                                                 workingPath,
                                                 fileName,
                                                 collectRefLinks,
                                                 line,
                                                 currentIndent,
                                                 ns,
                                                 currentLineNumber);
            }

            continue;
        }

        // Something new and first block is not a code block or a list, blockquote.
        if (ctx.m_type != ctx.m_lineType && ctx.m_type != BlockType::Code &&
            !isListType(ctx.m_type) && ctx.m_type != BlockType::Blockquote) {
            if (ctx.m_type == BlockType::Text && ctx.m_lineType == BlockType::CodeIndentedBySpaces) {
                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
            }
            else {
                if (ctx.m_type == BlockType::Text && isListType(ctx.m_lineType)) {
                    if (ctx.m_lineType != BlockType::ListWithFirstEmptyLine) {
                        int num = 0;

                        if (isOrderedList<Trait>(line.asString(), &num)) {
                            if (num != 1) {
                                ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});

                                continue;
                            }
                        }
                    } else {
                        ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});

                        continue;
                    }
                }

                parseFragmentAndMakeNextLineMain(ctx,
                                                 parent,
                                                 doc,
                                                 linksToParse,
                                                 workingPath,
                                                 fileName,
                                                 collectRefLinks,
                                                 line,
                                                 currentIndent,
                                                 ns,
                                                 currentLineNumber);
            }
        }
        // End of code block.
        else if (ctx.m_type == BlockType::Code && ctx.m_type == ctx.m_lineType &&
                 !ctx.m_startOfCode.isEmpty() &&
                 startSequence<Trait>(line.asString()).contains(ctx.m_startOfCode) &&
                 isCodeFences<Trait>(line.asString(), true)) {
            ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});

            parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
        }
        // Not a continue of list.
        else if (ctx.m_type != ctx.m_lineType && isListType(ctx.m_type) &&
                 ctx.m_lineType != BlockType::SomethingInList &&
                 ctx.m_lineType != BlockType::FensedCodeInList && !isListType(ctx.m_lineType)) {
            parseFragmentAndMakeNextLineMain(ctx,
                                             parent,
                                             doc,
                                             linksToParse,
                                             workingPath,
                                             fileName,
                                             collectRefLinks,
                                             line,
                                             currentIndent,
                                             ns,
                                             currentLineNumber);
        } else if (ctx.m_type == BlockType::Heading) {
            parseFragmentAndMakeNextLineMain(ctx,
                                             parent,
                                             doc,
                                             linksToParse,
                                             workingPath,
                                             fileName,
                                             collectRefLinks,
                                             line,
                                             currentIndent,
                                             ns,
                                             currentLineNumber);
        } else {
            ctx.m_fragment.push_back({line, {currentLineNumber, ctx.m_htmlCommentData}});
        }

        ctx.m_emptyLinesCount = 0;
    }

    if (!ctx.m_fragment.empty()) {
        if (ctx.m_type == BlockType::Code) {
            ctx.m_fragment.push_back({ctx.m_startOfCode, {-1, {}}});
        }

        parseFragment(ctx, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
    }

    if (top) {
        resetHtmlTag(ctx.m_html);

        for (long long int i = 0; i < (long long int)ctx.m_splitted.size(); ++i) {
            long long int line = 0;

            auto &data = ctx.m_splitted[i];

            while (line >= 0) {
                line = parseFragment(data, parent, doc, linksToParse, workingPath, fileName, false, ctx.m_html);

                assert(line != 0);

                if (line > 0) {
                    if (ctx.m_html.m_html) {
                        ctx.m_html.m_parent->appendItem(ctx.m_html.m_html);

                        resetHtmlTag<Trait>(ctx.m_html);
                    }

                    const auto it = std::find_if(data.m_data.cbegin(), data.m_data.cend(), [line](const auto &d) {
                        return (d.second.m_lineNumber == line);
                    });

                    data.m_data.erase(data.m_data.cbegin(), it);
                }
            }

            if (ctx.m_html.m_htmlBlockType >= 6) {
                ctx.m_html.m_continueHtml = (!ctx.m_splitted[i].m_emptyLineAfter);
            }

            if (ctx.m_html.m_html && !ctx.m_html.m_continueHtml) {
                finishHtml(ctx, parent, doc, collectRefLinks, top, dontProcessLastFreeHtml);
            } else if (!ctx.m_html.m_html) {
                ctx.m_html.m_toAdjustLastPos.clear();
            }
        }
    }

    if (ctx.m_html.m_html) {
        finishHtml(ctx, parent, doc, collectRefLinks, top, dontProcessLastFreeHtml);
    }

    return ctx.m_html;
}

#ifdef MD4QT_QT_SUPPORT

template<>
inline void
Parser<QStringTrait>::parseFile(const QString &fileName,
                                bool recursive,
                                std::shared_ptr<Document<QStringTrait>> doc,
                                const QStringList &ext,
                                QStringList *parentLinks)
{
    QFileInfo fi(fileName);

    if (fi.exists() && ext.contains(fi.suffix().toLower())) {
        QFile f(fileName);

        if (f.open(QIODevice::ReadOnly)) {
            QTextStream s(f.readAll());
            f.close();

            parseStream(s, fi.absolutePath(), fi.fileName(), recursive, doc, ext, parentLinks);
        }
    }
}

#endif

#ifdef MD4QT_ICU_STL_SUPPORT

template<>
inline void
Parser<UnicodeStringTrait>::parseFile(const UnicodeString &fileName,
                                      bool recursive,
                                      std::shared_ptr<Document<UnicodeStringTrait>> doc,
                                      const std::vector<UnicodeString> &ext,
                                      std::vector<UnicodeString> *parentLinks)
{
    if (UnicodeStringTrait::fileExists(fileName)) {
        std::string fn;
        fileName.toUTF8String(fn);

        try {
            auto e = UnicodeString::fromUTF8(std::filesystem::u8path(fn).extension().u8string());

            if (!e.isEmpty()) {
                e.remove(0, 1);
            }

            if (std::find(ext.cbegin(), ext.cend(), e.toLower()) != ext.cend()) {
                auto path = std::filesystem::canonical(std::filesystem::u8path(fn));
                std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

                if (file.good()) {
                    const auto fileNameS = path.filename().u8string();
                    auto workingDirectory = path.remove_filename().u8string();

                    if (!workingDirectory.empty()) {
                        workingDirectory.erase(workingDirectory.size() - 1, 1);
                    }

                    std::replace(workingDirectory.begin(), workingDirectory.end(), '\\', '/');

                    parseStream(file, UnicodeString::fromUTF8(workingDirectory),
                        UnicodeString::fromUTF8(fileNameS), recursive, doc, ext, parentLinks);

                    file.close();
                }
            }
        } catch (const std::exception &) {
        }
    }
}

#endif

template<class Trait>
void
resolveLinks(typename Trait::StringList &linksToParse,
             std::shared_ptr<Document<Trait>> doc)
{
    for (auto it = linksToParse.begin(), last = linksToParse.end(); it != last; ++it) {
        auto nextFileName = *it;

        if (nextFileName.startsWith(Trait::latin1ToString("#"))) {
            const auto lit = doc->labeledLinks().find(nextFileName);

            if (lit != doc->labeledLinks().cend()) {
                nextFileName = lit->second->url();
            } else {
                continue;
            }
        }

        if (Trait::fileExists(nextFileName)) {
            *it = Trait::absoluteFilePath(nextFileName);
        }
    }
}

template<class Trait>
inline void
Parser<Trait>::parseStream(typename Trait::TextStream &s,
                           const typename Trait::String &workingPath,
                           const typename Trait::String &fileName,
                           bool recursive,
                           std::shared_ptr<Document<Trait>> doc,
                           const typename Trait::StringList &ext,
                           typename Trait::StringList *parentLinks)
{
    typename Trait::StringList linksToParse;

    const auto path = workingPath.isEmpty() ? typename Trait::String(fileName) :
        typename Trait::String(workingPath + Trait::latin1ToString("/") + fileName);

    doc->appendItem(std::shared_ptr<Anchor<Trait>>(new Anchor<Trait>(path)));

    typename MdBlock<Trait>::Data data;

    {
        TextStream<Trait> stream(s);

        long long int i = 0;

        while (!stream.atEnd()) {
            data.push_back(std::pair<typename Trait::InternalString, MdLineData>(stream.readLine(), {i}));
            ++i;
        }
    }

    StringListStream<Trait> stream(data);

    parse(stream, doc, doc, linksToParse, workingPath, fileName, true, true);

    m_parsedFiles.push_back(path);

    resolveLinks<Trait>(linksToParse, doc);

    // Parse all links if parsing is recursive.
    if (recursive && !linksToParse.empty()) {
        const auto tmpLinks = linksToParse;

        while (!linksToParse.empty()) {
            auto nextFileName = linksToParse.front();
            linksToParse.erase(linksToParse.cbegin());

            if (parentLinks) {
                const auto pit = std::find(parentLinks->cbegin(), parentLinks->cend(), nextFileName);

                if (pit != parentLinks->cend()) {
                    continue;
                }
            }

            if (nextFileName.startsWith(Trait::latin1ToString("#"))) {
                continue;
            }

            const auto pit = std::find(m_parsedFiles.cbegin(), m_parsedFiles.cend(), nextFileName);

            if (pit == m_parsedFiles.cend()) {
                if (!doc->isEmpty() && doc->items().back()->type() != ItemType::PageBreak) {
                    doc->appendItem(std::shared_ptr<PageBreak<Trait>>(new PageBreak<Trait>));
                }

                parseFile(nextFileName, recursive, doc, ext, &linksToParse);
            }
        }

        if (parentLinks) {
            std::copy(tmpLinks.cbegin(), tmpLinks.cend(), std::back_inserter(*parentLinks));
        }
    }
}

template<class Trait>
inline long long int
posOfListItem(const typename Trait::String &s,
              bool ordered)
{
    long long int p = 0;

    for (; p < s.size(); ++p) {
        if (!s[p].isSpace()) {
            break;
        }
    }

    if (ordered) {
        for (; p < s.size(); ++p) {
            if (!s[p].isDigit()) {
                break;
            }
        }
    }

    ++p;

    long long int sc = 0;

    for (; p < s.size(); ++p) {
        if (!s[p].isSpace()) {
            break;
        } else {
            ++sc;
        }
    }

    if (p == s.length() || sc > 4) {
        p = p - sc + 1;
    } else if (sc == 0) {
        ++p;
    }

    return p;
}

inline long long int
listLevel(const std::vector<long long int> &indents,
          long long int pos)
{
    long long int level = indents.size();

    for (auto it = indents.crbegin(), last = indents.crend(); it != last; ++it) {
        if (pos >= *it) {
            break;
        } else {
            --level;
        }
    }

    return level;
}

template<class Trait>
inline typename Parser<Trait>::BlockType
Parser<Trait>::whatIsTheLine(typename Trait::InternalString &str,
                             bool inList,
                             bool inListWithFirstEmptyLine,
                             bool fensedCodeInList,
                             typename Trait::String *startOfCode,
                             ListIndent *indent,
                             bool emptyLinePreceded,
                             bool calcIndent,
                             const std::vector<long long int> *indents)
{
    replaceTabs<Trait>(str);

    const auto first = skipSpaces<Trait>(0, str.asString());

    if (first < str.length()) {
        auto s = str.sliced(first);

        const bool isBlockquote = s.asString().startsWith(Trait::latin1ToString(">"));
        const bool indentIn = indentInList(indents, first, false);
        bool isHeading = false;

        if (first < 4 && isFootnote<Trait>(s.asString())) {
            return BlockType::Footnote;
        }

        if (s.asString().startsWith(Trait::latin1ToString("#")) &&
            (indent ? first - indent->m_indent < 4 : first < 4)) {
            long long int c = 0;

            while (c < s.length() && s[c] == Trait::latin1ToChar('#')) {
                ++c;
            }

            if (c <= 6 && ((c < s.length() && s[c].isSpace()) || c == s.length())) {
                isHeading = true;
            }
        }

        if (inList) {
            bool isFirstLineEmpty = false;
            const auto orderedList = isOrderedList<Trait>(str.asString(), nullptr, nullptr, nullptr,
                &isFirstLineEmpty);
            const bool fensedCode = isCodeFences<Trait>(s.asString());
            const auto codeIndentedBySpaces = emptyLinePreceded && first >= 4 &&
                !indentInList(indents, first, true);

            if (fensedCodeInList) {
                if (indentInList(indents, first, true)) {
                    if (fensedCode) {
                        if (startOfCode && startSequence<Trait>(s.asString()).contains(*startOfCode)) {
                            return BlockType::FensedCodeInList;
                        }
                    }

                    return BlockType::SomethingInList;
                }
            }

            if (fensedCode && indentIn) {
                if (startOfCode) {
                    *startOfCode = startSequence<Trait>(s.asString());
                }

                return BlockType::FensedCodeInList;
            } else if ((((s.asString().startsWith(Trait::latin1ToString("-")) ||
                          s.asString().startsWith(Trait::latin1ToString("+")) ||
                          s.asString().startsWith(Trait::latin1ToString("*"))) &&
                         ((s.length() > 1 && s[1] == Trait::latin1ToChar(' ')) || s.length() == 1)) ||
                         orderedList) && (first < 4 || indentIn)) {
                if (codeIndentedBySpaces) {
                    return BlockType::CodeIndentedBySpaces;
                }

                if (indent && calcIndent) {
                    indent->m_indent = posOfListItem<Trait>(str.asString(), orderedList);
                    indent->m_level = (indents ? listLevel(*indents, first) : -1);
                }

                if (s.simplified().length() == 1 || isFirstLineEmpty) {
                    return BlockType::ListWithFirstEmptyLine;
                } else {
                    return BlockType::List;
                }
            } else if (indentInList(indents, first, true)) {
                return BlockType::SomethingInList;
            }
            else {
                if (!isHeading && !isBlockquote &&
                    !(fensedCode && first < 4) && !emptyLinePreceded && !inListWithFirstEmptyLine) {
                    return BlockType::SomethingInList;
                }
            }
        } else {
            bool isFirstLineEmpty = false;

            const auto orderedList = isOrderedList<Trait>(str.asString(), nullptr, nullptr, nullptr,
                &isFirstLineEmpty);
            const bool isHLine = first < 4 && isHorizontalLine<Trait>(s.asString());

            if (!isHLine &&
                (((s.asString().startsWith(Trait::latin1ToString("-")) || s.asString().startsWith(Trait::latin1ToString("+")) ||
                      s.asString().startsWith(Trait::latin1ToString("*"))) &&
                     ((s.length() > 1 && s[1] == Trait::latin1ToChar(' ')) || s.length() == 1)) ||
                    orderedList) && first < 4) {
                if (indent && calcIndent) {
                    indent->m_indent = posOfListItem<Trait>(str.asString(), orderedList);
                    indent->m_level = (indents ? listLevel(*indents, first) : -1);
                }

                if (s.simplified().length() == 1 || isFirstLineEmpty) {
                    return BlockType::ListWithFirstEmptyLine;
                } else {
                    return BlockType::List;
                }
            }
        }

        if (str.asString().startsWith(typename Trait::String(4, Trait::latin1ToChar(' ')))) {
            return BlockType::CodeIndentedBySpaces;
        } else if (isCodeFences<Trait>(str.asString())) {
            return BlockType::Code;
        } else if (isBlockquote) {
            return BlockType::Blockquote;
        } else if (isHeading) {
            return BlockType::Heading;
        }
    } else {
        return BlockType::EmptyLine;
    }

    return BlockType::Text;
}

template<class Trait>
inline long long int
Parser<Trait>::parseFragment(MdBlock<Trait> &fr,
                             std::shared_ptr<Block<Trait>> parent,
                             std::shared_ptr<Document<Trait>> doc,
                             typename Trait::StringList &linksToParse,
                             const typename Trait::String &workingPath,
                             const typename Trait::String &fileName,
                             bool collectRefLinks,
                             RawHtmlBlock<Trait> &html)
{
    if (html.m_continueHtml) {
        parseText(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, html);
    } else {
        if (html.m_html) {
            if (!collectRefLinks) {
                parent->appendItem(html.m_html);
            }

            resetHtmlTag(html);
        }

        switch (whatIsTheLine(fr.m_data.front().first)) {
        case BlockType::Footnote:
            parseFootnote(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
            break;

        case BlockType::Text:
            parseText(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, html);
            break;

        case BlockType::Blockquote:
            parseBlockquote(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, html);
            break;

        case BlockType::Code:
            parseCode(fr, parent, collectRefLinks);
            break;

        case BlockType::CodeIndentedBySpaces: {
            int indent = 1;

            if (fr.m_data.front().first.asString().startsWith(Trait::latin1ToString("    "))) {
                indent = 4;
            }

            parseCodeIndentedBySpaces(fr, parent, collectRefLinks, indent, {}, -1, -1, false);
        } break;

        case BlockType::Heading:
            parseHeading(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
            break;

        case BlockType::List:
        case BlockType::ListWithFirstEmptyLine:
            return parseList(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, html);

        default:
            break;
        }
    }

    return -1;
}

template<class Trait>
inline void
Parser<Trait>::clearCache()
{
    m_parsedFiles.clear();
}

template<class Trait>
inline int
isTableHeader(const typename Trait::String &s)
{
    if (s.contains(Trait::latin1ToChar('|'))) {
        int c = 0;

        const auto tmp = s.simplified();
        const auto p = tmp.startsWith(Trait::latin1ToString("|")) ? 1 : 0;
        const auto n = tmp.size() - p - (tmp.endsWith(Trait::latin1ToString("|")) && tmp.size() > 1 ? 1 : 0);
        const auto v = tmp.sliced(p, n);

        bool backslash = false;

        for (long long int i = 0; i < v.size(); ++i) {
            bool now = false;

            if (v[i] == Trait::latin1ToChar('\\') && !backslash) {
                backslash = true;
                now = true;
            } else if (v[i] == Trait::latin1ToChar('|') && !backslash) {
                ++c;
            }

            if (!now) {
                backslash = false;
            }
        }

        ++c;

        return c;
    } else {
        return 0;
    }
}

template<class Trait>
inline void
Parser<Trait>::parseText(MdBlock<Trait> &fr,
                         std::shared_ptr<Block<Trait>> parent,
                         std::shared_ptr<Document<Trait>> doc,
                         typename Trait::StringList &linksToParse,
                         const typename Trait::String &workingPath,
                         const typename Trait::String &fileName,
                         bool collectRefLinks,
                         RawHtmlBlock<Trait> &html)
{
    const auto h = isTableHeader<Trait>(fr.m_data.front().first.asString());
    const auto c = fr.m_data.size() > 1 ? isTableAlignment<Trait>(fr.m_data[1].first.asString()) : 0;

    if (c && h && c == h && !html.m_continueHtml) {
        parseTable(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, c);

        if (!fr.m_data.empty()) {
            StringListStream<Trait> stream(fr.m_data);

            Parser<Trait>::parse(stream, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
        }
    } else {
        parseParagraph(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks, html);
    }
}

template<class Trait>
inline std::pair<typename Trait::String, WithPosition>
findAndRemoveHeaderLabel(typename Trait::InternalString &s)
{
    const auto start = s.asString().indexOf(Trait::latin1ToString("{#"));

    if (start >= 0) {
        long long int p = start + 2;

        for (; p < s.length(); ++p) {
            if (s[p] == Trait::latin1ToChar('}')) {
                break;
            }
        }

        if (p < s.length() && s[p] == Trait::latin1ToChar('}')) {
            WithPosition pos;
            pos.setStartColumn(s.virginPos(start));
            pos.setEndColumn(s.virginPos(p));

            const auto label = s.sliced(start, p - start + 1).asString();
            s.remove(start, p - start + 1);
            return {label, pos};
        }
    }

    return {};
}

template<class Trait>
inline typename Trait::String
stringToLabel(const typename Trait::String &s)
{
    typename Trait::String res;

    for (long long int i = 0; i < s.length(); ++i) {
        const auto c = s[i];

        if (c.isLetter() || c.isDigit() || c == Trait::latin1ToChar('-') ||
            c == Trait::latin1ToChar('_')) {
            res.push_back(c.toLower());
        } else if (c.isSpace() && !res.isEmpty()) {
            res.push_back(Trait::latin1ToString("-"));
        }
    }

    return res;
}

template<class Trait>
inline typename Trait::String
paragraphToLabel(Paragraph<Trait> *p)
{
    typename Trait::String l;

    if (!p) {
        return l;
    }

    long long int line = -1;

    for (auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it) {
        typename Trait::String tmp;
        const bool newLine = ((*it)->startLine() != line);
        line = (*it)->endLine();

        switch ((*it)->type()) {
        case ItemType::Text: {
            auto t = static_cast<Text<Trait> *>(it->get());
            const auto text = t->text().simplified();
            tmp = stringToLabel<Trait>(text);
        } break;

        case ItemType::Image: {
            auto i = static_cast<Image<Trait> *>(it->get());

            if (!i->p()->isEmpty()) {
                tmp = paragraphToLabel(i->p().get());
            } else if (!i->text().simplified().isEmpty()) {
                tmp = stringToLabel<Trait>(i->text().simplified());
            }
        } break;

        case ItemType::Link: {
            auto link = static_cast<Link<Trait> *>(it->get());

            if (!link->p()->isEmpty()) {
                tmp = paragraphToLabel(link->p().get());
            } else if (!link->text().simplified().isEmpty()) {
                tmp = stringToLabel<Trait>(link->text().simplified());
            }
        } break;

        case ItemType::Code: {
            auto c = static_cast<Code<Trait> *>(it->get());

            if (!c->text().simplified().isEmpty()) {
                tmp = stringToLabel<Trait>(c->text().simplified());
            }
        } break;

        default:
            break;
        }

        if (!l.isEmpty() && !tmp.isEmpty() && !newLine) {
            l.push_back(Trait::latin1ToString("-"));
        }

        l.push_back(tmp);
    }

    return l;
}

template<class Trait>
inline WithPosition
findAndRemoveClosingSequence(typename Trait::InternalString &s)
{
    long long int end = -1;
    long long int start = -1;

    for (long long int i = s.length() - 1; i >= 0; --i) {
        if (!s[i].isSpace() && s[i] != Trait::latin1ToChar('#') && end == -1) {
            return {};
        }

        if (s[i] == Trait::latin1ToChar('#')) {
            if (end == -1) {
                end = i;
            }

            if (i - 1 >= 0) {
                if (s[i - 1].isSpace()) {
                    start = i;
                    break;
                } else if (s[i - 1] != Trait::latin1ToChar('#')) {
                    return {};
                }
            } else {
                start = 0;
            }
        }
    }

    WithPosition ret;

    if (start != -1 && end != -1) {
        ret.setStartColumn(s.virginPos(start));
        ret.setEndColumn(s.virginPos(end));

        s.remove(start, end - start + 1);
    }

    return ret;
}

template<class Trait>
inline void
Parser<Trait>::parseHeading(MdBlock<Trait> &fr,
                            std::shared_ptr<Block<Trait>> parent,
                            std::shared_ptr<Document<Trait>> doc,
                            typename Trait::StringList &linksToParse,
                            const typename Trait::String &workingPath,
                            const typename Trait::String &fileName,
                            bool collectRefLinks)
{
    if (!fr.m_data.empty() && !collectRefLinks) {
        auto line = fr.m_data.front().first;

        std::shared_ptr<Heading<Trait>> h(new Heading<Trait>);
        h->setStartColumn(line.virginPos(skipSpaces<Trait>(0, line.asString())));
        h->setStartLine(fr.m_data.front().second.m_lineNumber);
        h->setEndColumn(line.virginPos(line.length() - 1));
        h->setEndLine(h->startLine());

        long long int pos = 0;
        pos = skipSpaces<Trait>(pos, line.asString());

        if (pos > 0) {
            line = line.sliced(pos);
        }

        pos = 0;
        int lvl = 0;

        while (pos < line.length() && line[pos] == Trait::latin1ToChar('#')) {
            ++lvl;
            ++pos;
        }

        WithPosition startDelim = {h->startColumn(), h->startLine(),
            line.virginPos(pos - 1), h->startLine()};

        pos = skipSpaces<Trait>(pos, line.asString());

        if (pos > 0) {
            fr.m_data.front().first = line.sliced(pos);
        }

        auto label = findAndRemoveHeaderLabel<Trait>(fr.m_data.front().first);

        typename Heading<Trait>::Delims delims = {startDelim};

        auto endDelim = findAndRemoveClosingSequence<Trait>(fr.m_data.front().first);

        if (endDelim.startColumn() != -1) {
            endDelim.setStartLine(fr.m_data.front().second.m_lineNumber);
            endDelim.setEndLine(endDelim.startLine());

            delims.push_back(endDelim);
        }

        h->setDelims(delims);

        h->setLevel(lvl);

        if (!label.first.isEmpty()) {
            h->setLabel(label.first.sliced(1, label.first.length() - 2) + Trait::latin1ToString("/") +
                        (!workingPath.isEmpty() ? workingPath + Trait::latin1ToString("/") :
                            Trait::latin1ToString("")) + fileName);

            label.second.setStartLine(fr.m_data.front().second.m_lineNumber);
            label.second.setEndLine(label.second.startLine());

            h->setLabelPos(label.second);
        }

        std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);

        typename MdBlock<Trait>::Data tmp;
        tmp.push_back(fr.m_data.front());
        tmp.front().first = tmp.front().first.simplified();
        MdBlock<Trait> block = {tmp, 0};

        RawHtmlBlock<Trait> html;

        parseFormattedTextLinksImages(block, p, doc, linksToParse, workingPath, fileName,
            false, false, html, false);

        fr.m_data.erase(fr.m_data.cbegin());

        if (p->items().size() && p->items().at(0)->type() == ItemType::Paragraph) {
            h->setText(std::static_pointer_cast<Paragraph<Trait>>(p->items().at(0)));
        } else {
            h->setText(p);
        }

        if (h->isLabeled()) {
            doc->insertLabeledHeading(h->label(), h);
        } else {
            typename Trait::String label = Trait::latin1ToString("#") +
                paragraphToLabel(h->text().get());

            label += Trait::latin1ToString("/") +
                (!workingPath.isEmpty() ? workingPath + Trait::latin1ToString("/") :
                    Trait::latin1ToString("")) + fileName;

            h->setLabel(label);

            doc->insertLabeledHeading(label, h);
        }

        parent->appendItem(h);
    }
}

template<class Trait>
inline typename Trait::InternalString
prepareTableData(typename Trait::InternalString s)
{
    s.replace(Trait::latin1ToString("\\|"), Trait::latin1ToString("|"));

    return s;
}

template<class Trait>
inline std::pair<typename Trait::InternalStringList, std::vector<long long int>>
splitTableRow(const typename Trait::InternalString &s)
{
    typename Trait::InternalStringList res;
    std::vector<long long int> columns;

    bool backslash = false;
    long long int start = 0;

    for (long long int i = 0; i < s.length(); ++i) {
        bool now = false;

        if (s[i] == Trait::latin1ToChar('\\') && !backslash) {
            backslash = true;
            now = true;
        } else if (s[i] == Trait::latin1ToChar('|') && !backslash) {
            res.push_back(prepareTableData<Trait>(s.sliced(start, i - start).simplified()));
            columns.push_back(s.virginPos(i));
            start = i + 1;
        }

        if (!now) {
            backslash = false;
        }
    }

    res.push_back(prepareTableData<Trait>(s.sliced(start, s.length() - start).simplified()));

    return {res, columns};
}

template<class Trait>
inline void
Parser<Trait>::parseTable(MdBlock<Trait> &fr,
                          std::shared_ptr<Block<Trait>> parent,
                          std::shared_ptr<Document<Trait>> doc,
                          typename Trait::StringList &linksToParse,
                          const typename Trait::String &workingPath,
                          const typename Trait::String &fileName,
                          bool collectRefLinks,
                          int columnsCount)
{
    static const char sep = '|';

    if (fr.m_data.size() >= 2) {
        std::shared_ptr<Table<Trait>> table(new Table<Trait>);
        table->setStartColumn(fr.m_data.front().first.virginPos(0));
        table->setStartLine(fr.m_data.front().second.m_lineNumber);
        table->setEndColumn(fr.m_data.back().first.virginPos(fr.m_data.back().first.length() - 1));
        table->setEndLine(fr.m_data.back().second.m_lineNumber);

        auto parseTableRow = [&](const typename MdBlock<Trait>::Line &lineData) -> bool {
            const auto &row = lineData.first;

            if (row.asString().startsWith(Trait::latin1ToString("    "))) {
                return false;
            }

            auto line = row.simplified();

            if (line.asString().startsWith(typename Trait::String(Trait::latin1ToChar(sep)))) {
                line.remove(0, 1);
            }

            if (line.asString().endsWith(typename Trait::String(Trait::latin1ToChar(sep)))) {
                line.remove(line.length() - 1, 1);
            }

            auto columns = splitTableRow<Trait>(line);
            columns.second.insert(columns.second.begin(), row.virginPos(0));
            columns.second.push_back(row.virginPos(row.length() - 1));

            std::shared_ptr<TableRow<Trait>> tr(new TableRow<Trait>);
            tr->setStartColumn(row.virginPos(0));
            tr->setStartLine(lineData.second.m_lineNumber);
            tr->setEndColumn(row.virginPos(row.length() - 1));
            tr->setEndLine(lineData.second.m_lineNumber);

            int col = 0;

            for (auto it = columns.first.begin(), last = columns.first.end(); it != last; ++it, ++col) {
                if (col == columnsCount) {
                    break;
                }

                std::shared_ptr<TableCell<Trait>> c(new TableCell<Trait>);
                c->setStartColumn(columns.second.at(col));
                c->setStartLine(lineData.second.m_lineNumber);
                c->setEndColumn(columns.second.at(col + 1));
                c->setEndLine(lineData.second.m_lineNumber);

                if (!it->isEmpty()) {
                    it->replace(Trait::latin1ToString("&#124;"), Trait::latin1ToChar(sep));

                    typename MdBlock<Trait>::Data fragment;
                    fragment.push_back({*it, lineData.second});
                    MdBlock<Trait> block = {fragment, 0};

                    std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);

                    RawHtmlBlock<Trait> html;

                    parseFormattedTextLinksImages(block, p, doc, linksToParse, workingPath, fileName,
                        collectRefLinks, false, html, false);

                    if (!p->isEmpty()) {
                        if (p->items().at(0)->type() == ItemType::Paragraph) {
                            const auto pp = std::static_pointer_cast<Paragraph<Trait>>(p->items().at(0));

                            for (auto it = pp->items().cbegin(), last = pp->items().cend(); it != last; ++it) {
                                c->appendItem((*it));
                            }
                        } else if (p->items().at(0)->type() == ItemType::RawHtml) {
                            c->appendItem(p->items().at(0));
                        }
                    } else if (html.m_html.get()) {
                        c->appendItem(html.m_html);
                    }
                }

                tr->appendCell(c);
            }

            if (!tr->isEmpty())
                table->appendRow(tr);

            return true;
        };

        {
            auto fmt = fr.m_data.at(1).first;

            auto columns = fmt.split(typename Trait::InternalString(Trait::latin1ToChar(sep)));

            for (auto it = columns.begin(), last = columns.end(); it != last; ++it) {
                *it = it->simplified();

                if (!it->isEmpty()) {
                    typename Table<Trait>::Alignment a = Table<Trait>::AlignLeft;

                    if (it->asString().endsWith(Trait::latin1ToString(":")) &&
                        it->asString().startsWith(Trait::latin1ToString(":"))) {
                        a = Table<Trait>::AlignCenter;
                    } else if (it->asString().endsWith(Trait::latin1ToString(":"))) {
                        a = Table<Trait>::AlignRight;
                    }

                    table->setColumnAlignment(table->columnsCount(), a);
                }
            }
        }

        fr.m_data.erase(fr.m_data.cbegin() + 1);

        long long int r = 0;

        for (const auto &line : std::as_const(fr.m_data)) {
            if (!parseTableRow(line)) {
                break;
            }

            ++r;
        }

        fr.m_data.erase(fr.m_data.cbegin(), fr.m_data.cbegin() + r);

        if (!table->isEmpty() && !collectRefLinks) {
            parent->appendItem(table);
        }
    }
}

template<class Trait>
inline bool
isH(const typename Trait::String &s,
    const typename Trait::Char &c)
{
    long long int p = skipSpaces<Trait>(0, s);

    if (p > 3) {
        return false;
    }

    const auto start = p;

    for (; p < s.size(); ++p) {
        if (s[p] != c) {
            break;
        }
    }

    if (p - start < 1) {
        return false;
    }

    for (; p < s.size(); ++p) {
        if (!s[p].isSpace()) {
            return false;
        }
    }

    return true;
}

template<class Trait>
inline bool
isH1(const typename Trait::String &s)
{
    return isH<Trait>(s, Trait::latin1ToChar('='));
}

template<class Trait>
inline bool
isH2(const typename Trait::String &s)
{
    return isH<Trait>(s, Trait::latin1ToChar('-'));
}

template<class Trait>
inline std::pair<long long int, long long int>
prevPosition(const MdBlock<Trait> &fr,
             long long int pos,
             long long int line)
{
    if (pos > 0) {
        return {pos - 1, line};
    }

    for (long long int i = 0; i < static_cast<long long int>(fr.m_data.size()); ++i) {
        if (fr.m_data.at(i).second.m_lineNumber == line) {
            if (i > 0) {
                return {fr.m_data.at(i - 1).first.virginPos(fr.m_data.at(i - 1).first.length() - 1),
                    line - 1};
            }
        }
    }

    return {pos, line};
}

template<class Trait>
inline std::pair<long long int, long long int>
nextPosition(const MdBlock<Trait> &fr,
             long long int pos,
             long long int line)
{
    for (long long int i = 0; i < static_cast<long long int>(fr.m_data.size()); ++i) {
        if (fr.m_data.at(i).second.m_lineNumber == line) {
            if (fr.m_data.at(i).first.virginPos(fr.m_data.at(i).first.length() - 1) >= pos + 1) {
                return {pos + 1, line};
            } else if (i + 1 < static_cast<long long int>(fr.m_data.size())) {
                return {fr.m_data.at(i + 1).first.virginPos(0), fr.m_data.at(i + 1).second.m_lineNumber};
            } else {
                return {pos, line};
            }
        }
    }

    return {pos, line};
}

template<class Trait>
inline void
Parser<Trait>::parseParagraph(MdBlock<Trait> &fr,
                              std::shared_ptr<Block<Trait>> parent,
                              std::shared_ptr<Document<Trait>> doc,
                              typename Trait::StringList &linksToParse,
                              const typename Trait::String &workingPath,
                              const typename Trait::String &fileName,
                              bool collectRefLinks,
                              RawHtmlBlock<Trait> &html)
{
    parseFormattedTextLinksImages(fr, parent, doc, linksToParse, workingPath, fileName,
        collectRefLinks, false, html, false);
}

template<class Trait>
struct UnprotectedDocsMethods {
    static bool
    isFreeTag(std::shared_ptr<RawHtml<Trait>> html)
    {
        return html->isFreeTag();
    }

    static void
    setFreeTag(std::shared_ptr<RawHtml<Trait>> html, bool on)
    {
        html->setFreeTag(on);
    }
};

template<class Trait>
inline typename Parser<Trait>::Delims
Parser<Trait>::collectDelimiters(const typename MdBlock<Trait>::Data &fr)
{
    Delims d;

    for (long long int line = 0; line < (long long int)fr.size(); ++line) {
        const typename Trait::String &str = fr.at(line).first.asString();
        const auto p = skipSpaces<Trait>(0, str);
        const auto withoutSpaces = str.sliced(p);

        if (isHorizontalLine<Trait>(withoutSpaces) && p < 4) {
            d.push_back({Delimiter::HorizontalLine, line, 0, str.length(), false, false, false});
        } else if (isH1<Trait>(withoutSpaces) && p < 4) {
            d.push_back({Delimiter::H1, line, 0, str.length(), false, false, false});
        } else if (isH2<Trait>(withoutSpaces) && p < 4) {
            d.push_back({Delimiter::H2, line, 0, str.length(), false, false, false});
        } else {
            bool backslash = false;
            bool space = true;
            bool word = false;

            for (long long int i = p; i < str.size(); ++i) {
                bool now = false;

                if (str[i] == Trait::latin1ToChar('\\') && !backslash) {
                    backslash = true;
                    now = true;
                } else if (str[i].isSpace() && !backslash) {
                    space = true;
                    now = true;
                } else {
                    // * or _
                    if ((str[i] == Trait::latin1ToChar('_') || str[i] == Trait::latin1ToChar('*')) && !backslash) {
                        typename Trait::String style;

                        const bool punctBefore = (i > 0 ? str[i - 1].isPunct() || str[i - 1].isSymbol() : true);
                        const bool uWhitespaceBefore = (i > 0 ? Trait::isUnicodeWhitespace(str[i - 1]) : true);
                        const bool uWhitespaceOrPunctBefore = uWhitespaceBefore || punctBefore;
                        const bool alNumBefore = (i > 0 ? str[i - 1].isLetterOrNumber() : false);

                        const auto ch = str[i];

                        while (i < str.length() && str[i] == ch) {
                            style.push_back(str[i]);
                            ++i;
                        }

                        typename Delimiter::DelimiterType dt = Delimiter::Unknown;

                        if (ch == Trait::latin1ToChar('*')) {
                            dt = Delimiter::Emphasis1;
                        } else {
                            dt = Delimiter::Emphasis2;
                        }

                        const bool punctAfter = (i < str.length() ? str[i].isPunct() || str[i].isSymbol() : true);
                        const bool uWhitespaceAfter = (i < str.length() ? Trait::isUnicodeWhitespace(str[i]) : true);
                        const bool alNumAfter = (i < str.length() ? str[i].isLetterOrNumber() : false);
                        const bool leftFlanking = !uWhitespaceAfter && (!punctAfter || (punctAfter && uWhitespaceOrPunctBefore))
                            && !(ch == Trait::latin1ToChar('_') && alNumBefore && alNumAfter);
                        const bool rightFlanking = !uWhitespaceBefore && (!punctBefore || (punctBefore && (uWhitespaceAfter || punctAfter)))
                            && !(ch == Trait::latin1ToChar('_') && alNumBefore && alNumAfter);

                        if (leftFlanking || rightFlanking) {
                            const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                            for (auto j = 0; j < style.length(); ++j) {
                                d.push_back({dt, line, i - style.length() + j, 1, space, spaceAfter,
                                    word, false, leftFlanking, rightFlanking});
                            }

                            word = false;
                        } else {
                            word = true;
                        }

                        --i;
                    }
                    // ~
                    else if (str[i] == Trait::latin1ToChar('~') && !backslash) {
                        typename Trait::String style;

                        const bool punctBefore = (i > 0 ? str[i - 1].isPunct() || str[i - 1].isSymbol() : true);
                        const bool uWhitespaceBefore = (i > 0 ? Trait::isUnicodeWhitespace(str[i - 1]) : true);
                        const bool uWhitespaceOrPunctBefore = uWhitespaceBefore || punctBefore;

                        while (i < str.length() && str[i] == Trait::latin1ToChar('~')) {
                            style.push_back(str[i]);
                            ++i;
                        }

                        if (style.length() <= 2) {
                            const bool punctAfter = (i < str.length() ? str[i].isPunct() || str[i].isSymbol() : true);
                            const bool uWhitespaceAfter = (i < str.length() ? Trait::isUnicodeWhitespace(str[i]) : true);
                            const bool leftFlanking = !uWhitespaceAfter && (!punctAfter || (punctAfter && uWhitespaceOrPunctBefore));
                            const bool rightFlanking = !uWhitespaceBefore && (!punctBefore || (punctBefore && (uWhitespaceAfter || punctAfter)));

                            if (leftFlanking || rightFlanking) {
                                const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                                d.push_back({Delimiter::Strikethrough,
                                             line,
                                             i - style.length(),
                                             style.length(),
                                             space,
                                             spaceAfter,
                                             word,
                                             false,
                                             leftFlanking,
                                             rightFlanking});

                                word = false;
                            } else {
                                word = true;
                            }
                        } else {
                            word = true;
                        }

                        --i;
                    }
                    // [
                    else if (str[i] == Trait::latin1ToChar('[') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::SquareBracketsOpen, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // !
                    else if (str[i] == Trait::latin1ToChar('!') && !backslash) {
                        if (i + 1 < str.length()) {
                            if (str[i + 1] == Trait::latin1ToChar('[')) {
                                const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                                d.push_back({Delimiter::ImageOpen, line, i, 2, space, spaceAfter, word, false});

                                ++i;

                                word = false;
                            } else {
                                word = true;
                            }
                        } else {
                            word = true;
                        }
                    }
                    // (
                    else if (str[i] == Trait::latin1ToChar('(') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::ParenthesesOpen, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // ]
                    else if (str[i] == Trait::latin1ToChar(']') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::SquareBracketsClose, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // )
                    else if (str[i] == Trait::latin1ToChar(')') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::ParenthesesClose, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // <
                    else if (str[i] == Trait::latin1ToChar('<') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::Less, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // >
                    else if (str[i] == Trait::latin1ToChar('>') && !backslash) {
                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::Greater, line, i, 1, space, spaceAfter, word, false});

                        word = false;
                    }
                    // `
                    else if (str[i] == Trait::latin1ToChar('`')) {
                        typename Trait::String code;

                        while (i < str.length() && str[i] == Trait::latin1ToChar('`')) {
                            code.push_back(str[i]);
                            ++i;
                        }

                        if (backslash) {
                            if (i - code.length() - 2 >= 0) {
                                if (str[i - code.length() - 2].isSpace())
                                    space = true;
                            } else {
                                space = true;
                            }
                        }

                        const bool spaceAfter = (i < str.length() ? str[i].isSpace() : true);

                        d.push_back({Delimiter::InlineCode,
                                     line,
                                     i - code.length() - (backslash ? 1 : 0),
                                     code.length() + (backslash ? 1 : 0),
                                     space,
                                     spaceAfter,
                                     word,
                                     backslash});

                        word = false;

                        --i;
                    }
                    // $
                    else if (str[i] == Trait::latin1ToChar('$')) {
                        typename Trait::String m;

                        while (i < str.length() && str[i] == Trait::latin1ToChar('$')) {
                            m.push_back(str[i]);
                            ++i;
                        }

                        if (m.length() <= 2 && !backslash) {
                            d.push_back({Delimiter::Math, line, i - m.length(), m.length(),
                                false, false, false, false});
                        }

                        word = false;

                        --i;
                    } else {
                        word = true;
                    }
                }

                if (!now) {
                    backslash = false;
                    space = false;
                }
            }
        }
    }

    return d;
}

template<class Trait>
inline bool
isLineBreak(const typename Trait::String &s)
{
    return (s.endsWith(Trait::latin1ToString("  ")) || s.endsWith(Trait::latin1ToString("\\")));
}

template<class Trait>
inline long long int
lineBreakLength(const typename Trait::String &s)
{
    return (s.endsWith(Trait::latin1ToString("  ")) ? 2 : 1);
}

template<class Trait>
inline typename Trait::String
removeLineBreak(const typename Trait::String &s)
{
    if (s.endsWith(Trait::latin1ToString("\\"))) {
        return s.sliced(0, s.size() - 1);
    } else {
        return s;
    }
}

template<class Trait>
inline void
initLastItemWithOpts(TextParsingOpts<Trait> &po,
                     std::shared_ptr<ItemWithOpts<Trait>> item)
{
    item->openStyles() = po.m_openStyles;
    po.m_openStyles.clear();
    po.m_lastItemWithStyle = item;
}

template<class Trait>
inline void
makeTextObject(const typename Trait::String &text,
               bool spaceBefore,
               bool spaceAfter,
               TextParsingOpts<Trait> &po,
               long long int startPos,
               long long int startLine,
               long long int endPos,
               long long int endLine)
{
    auto s = removeBackslashes<typename Trait::String, Trait>(replaceEntity<Trait>(text));

    if (!s.isEmpty()) {
        spaceBefore = spaceBefore || s[0].isSpace();
        spaceAfter = spaceAfter || s[s.size() - 1].isSpace();
    }

    s = s.simplified();

    if (!s.isEmpty()) {
        po.m_rawTextData.push_back({text, startPos, startLine, spaceBefore, spaceAfter});

        if (endPos < 0 && endLine - 1 >= 0) {
            endPos = po.m_fr.m_data.at(endLine - 1).first.length() - 1;
            --endLine;
        }

        std::shared_ptr<Text<Trait>> t(new Text<Trait>);
        t->setText(s);
        t->setOpts(po.m_opts);
        t->setSpaceBefore(spaceBefore);
        t->setSpaceAfter(spaceAfter);
        t->setStartColumn(po.m_fr.m_data.at(startLine).first.virginPos(startPos));
        t->setStartLine(po.m_fr.m_data.at(startLine).second.m_lineNumber);
        t->setEndColumn(po.m_fr.m_data.at(endLine).first.virginPos(endPos));
        t->setEndLine(po.m_fr.m_data.at(endLine).second.m_lineNumber);

        initLastItemWithOpts<Trait>(po, t);

        po.m_parent->setEndColumn(po.m_fr.m_data.at(endLine).first.virginPos(endPos));
        po.m_parent->setEndLine(po.m_fr.m_data.at(endLine).second.m_lineNumber);

        po.m_wasRefLink = false;
        po.m_firstInParagraph = false;
        po.m_parent->appendItem(t);

        po.m_lastText = t;
    } else {
        po.m_pos = startPos;
    }
}

template<class Trait>
inline void
makeTextObjectWithLineBreak(const typename Trait::String &text,
                            bool spaceBefore,
                            bool spaceAfter,
                            TextParsingOpts<Trait> &po,
                            long long int startPos,
                            long long int startLine,
                            long long int endPos,
                            long long int endLine)
{
    MD_UNUSED(spaceAfter)

    makeTextObject(text, spaceBefore, true, po, startPos, startLine, endPos, endLine);

    std::shared_ptr<LineBreak<Trait>> hr(new LineBreak<Trait>);
    hr->setText(po.m_fr.m_data.at(endLine).first.asString().sliced(endPos + 1));
    hr->setSpaceAfter(true);
    hr->setSpaceBefore(po.m_fr.m_data.at(endLine).first.asString()[endPos].isSpace());
    hr->setStartColumn(po.m_fr.m_data.at(endLine).first.virginPos(endPos + 1));
    hr->setStartLine(po.m_fr.m_data.at(endLine).second.m_lineNumber);
    hr->setEndColumn(po.m_fr.m_data.at(endLine).first.virginPos(po.m_fr.m_data.at(endLine).first.length() - 1));
    hr->setEndLine(po.m_fr.m_data.at(endLine).second.m_lineNumber);
    po.m_parent->setEndColumn(hr->endColumn());
    po.m_parent->setEndLine(hr->endLine());
    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;
    po.m_parent->appendItem(hr);
}

template<class Trait>
inline void
checkForTableInParagraph(TextParsingOpts<Trait> &po,
                         long long int lastLine)
{
    if (!po.m_opts) {
        long long int i = po.m_pos > 0 ? po.m_line + 1 : po.m_line;

        for (; i <= lastLine; ++i) {
            const auto h = isTableHeader<Trait>(po.m_fr.m_data[i].first.asString());
            const auto c = i + 1 < static_cast<long long int>(po.m_fr.m_data.size()) ?
                                    isTableAlignment<Trait>(po.m_fr.m_data[i + 1].first.asString()) : 0;

            if (h && c && c == h) {
                po.m_detected = TextParsingOpts<Trait>::Detected::Table;
                po.m_startTableLine = i;
                po.m_columnsCount = c;
                po.m_lastTextLine = i - 1;
                po.m_lastTextPos = po.m_fr.m_data[po.m_lastTextLine].first.length();

                return;
            }
        }
    }

    po.m_lastTextLine = po.m_fr.m_data.size() - 1;
    po.m_lastTextPos = po.m_fr.m_data.back().first.length();
}

template<class Trait>
inline void
makeText(
    // Inclusive. Don't pass lastLine > actual line position with 0 lastPos. Pass as is,
    // i.e. if line length is 18 and you need whole line then pass lastLine = index of line,
    //	and lastPos = 18, or you may crash here if you will pass lastLine = index of line + 1
    // and lastPos = 0...
    long long int lastLine,
    // Not inclusive
    long long int lastPos,
    TextParsingOpts<Trait> &po)
{
    if (po.m_line > lastLine) {
        return;
    } else if (po.m_line == lastLine && po.m_pos >= lastPos) {
        return;
    }

    typename Trait::String text;

    const auto isLastChar = po.m_pos >= po.m_fr.m_data.at(po.m_line).first.length();
    long long int startPos = (isLastChar ? 0 : po.m_pos);
    long long int startLine = (isLastChar ? po.m_line + 1 : po.m_line);

    bool spaceBefore = (po.m_pos > 0 && po.m_pos < po.m_fr.m_data.at(po.m_line).first.length() ?
                            po.m_fr.m_data.at(po.m_line).first[po.m_pos - 1].isSpace() ||
                                po.m_fr.m_data.at(po.m_line).first[po.m_pos].isSpace() :
                            true) || po.m_isSpaceBefore;

    po.m_isSpaceBefore = false;

    bool lineBreak =
        (!po.m_ignoreLineBreak && po.m_line != (long long int)(po.m_fr.m_data.size() - 1) &&
            (po.m_line == lastLine ? (lastPos == po.m_fr.m_data.at(po.m_line).first.length() &&
                isLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString())) :
            isLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString())));

    // makeTOWLB
    auto makeTOWLB = [&]() {
        if (po.m_line != (long long int)(po.m_fr.m_data.size() - 1)) {
            const auto &line = po.m_fr.m_data.at(po.m_line).first.asString();

            makeTextObjectWithLineBreak(text, spaceBefore, true, po, startPos, startLine,
                line.length() - lineBreakLength<Trait>(line) - 1, po.m_line);

            startPos = 0;
            startLine = po.m_line + 1;

            text.clear();

            spaceBefore = true;
        }
    }; // makeTOWLB

    if (lineBreak) {
        text.push_back(removeLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString()).sliced(po.m_pos));

        makeTOWLB();
    } else {
        const auto s =
            po.m_fr.m_data.at(po.m_line).first.asString().sliced(po.m_pos,
                (po.m_line == lastLine ? lastPos - po.m_pos :
                    po.m_fr.m_data.at(po.m_line).first.length() - po.m_pos));
        text.push_back(s);

        po.m_pos = (po.m_line == lastLine ? lastPos : po.m_fr.m_data.at(po.m_line).first.length());

        bool isSpaceAfter = po.m_pos > 0 ? po.m_fr.m_data.at(po.m_line).first[po.m_pos - 1].isSpace() ||
            po.m_pos == po.m_fr.m_data.at(po.m_line).first.length() : true;
        isSpaceAfter = !isSpaceAfter && po.m_pos < po.m_fr.m_data.at(po.m_line).first.length() ?
            po.m_fr.m_data.at(po.m_line).first[po.m_pos].isSpace() : isSpaceAfter;

        makeTextObject(text,
                       spaceBefore,
                       isSpaceAfter,
                       po,
                       startPos,
                       startLine,
                       po.m_line == lastLine ? lastPos - 1 : po.m_fr.m_data.at(po.m_line).first.length() - 1,
                       po.m_line);

        text.clear();
    }

    if (po.m_line != lastLine) {
        ++po.m_line;
        startPos = 0;
        startLine = po.m_line;

        for (; po.m_line < lastLine; ++po.m_line) {
            lineBreak = (!po.m_ignoreLineBreak && po.m_line != (long long int)(po.m_fr.m_data.size() - 1) &&
                isLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString()));

            const auto s = (lineBreak ? removeLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString()) :
                po.m_fr.m_data.at(po.m_line).first.asString());
            text.push_back(s);

            if (lineBreak) {
                makeTOWLB();
            } else {
                makeTextObject(text, true, true, po, 0, po.m_line,
                    po.m_fr.m_data.at(po.m_line).first.length() - 1, po.m_line);
            }

            text.clear();
        }

        lineBreak = (!po.m_ignoreLineBreak && po.m_line != (long long int)(po.m_fr.m_data.size() - 1) &&
            lastPos == po.m_fr.m_data.at(po.m_line).first.length() &&
            isLineBreak<Trait>(po.m_fr.m_data.at(po.m_line).first.asString()));

        auto s = po.m_fr.m_data.at(po.m_line).first.asString().sliced(0, lastPos);

        po.m_pos = lastPos;

        bool isSpaceAfter = po.m_pos > 0 ? po.m_fr.m_data.at(po.m_line).first[po.m_pos - 1].isSpace() ||
            po.m_pos == po.m_fr.m_data.at(po.m_line).first.length() : true;
        isSpaceAfter = !isSpaceAfter && po.m_pos < po.m_fr.m_data.at(po.m_line).first.length() ?
            po.m_fr.m_data.at(po.m_line).first[po.m_pos].isSpace() : isSpaceAfter;

        if (!lineBreak) {
            text.push_back(s);

            makeTextObject(text, true, isSpaceAfter, po, 0, lastLine, lastPos - 1, lastLine);
        } else {
            s = removeLineBreak<Trait>(s);
            text.push_back(s);

            makeTOWLB();
        }
    }
}

template<class Trait>
inline void
skipSpacesInHtml(long long int &l,
                 long long int &p,
                 const typename MdBlock<Trait>::Data &fr)
{
    while (l < (long long int)fr.size()) {
        while (p < fr[l].first.length()) {
            if (!fr[l].first[p].isSpace()) {
                return;
            }

            ++p;
        }

        p = 0;
        ++l;
    }
}

template<class Trait>
inline std::pair<bool, bool>
readUnquotedHtmlAttrValue(long long int &l,
                          long long int &p,
                          const typename MdBlock<Trait>::Data &fr)
{
    static const typename Trait::String notAllowed = Trait::latin1ToString("\"`=<'");

    const auto start = p;

    for (; p < fr[l].first.length(); ++p) {
        if (fr[l].first[p].isSpace()) {
            break;
        } else if (notAllowed.contains(fr[l].first[p])) {
            return {false, false};
        } else if (fr[l].first[p] == Trait::latin1ToChar('>')) {
            return {p - start > 0, p - start > 0};
        }
    }

    return {p - start > 0, p - start > 0};
}

template<class Trait>
inline std::pair<bool, bool>
readHtmlAttrValue(long long int &l,
                  long long int &p,
                  const typename MdBlock<Trait>::Data &fr)
{
    if (p < fr[l].first.length() && fr[l].first[p] != Trait::latin1ToChar('"') &&
        fr[l].first[p] != Trait::latin1ToChar('\'')) {
        return readUnquotedHtmlAttrValue<Trait>(l, p, fr);
    }

    const auto s = fr[l].first[p];

    ++p;

    if (p >= fr[l].first.length()) {
        return {false, false};
    }

    for (; l < (long long int)fr.size(); ++l) {
        bool doBreak = false;

        for (; p < fr[l].first.length(); ++p) {
            const auto ch = fr[l].first[p];

            if (ch == s) {
                doBreak = true;

                break;
            }
        }

        if (doBreak) {
            break;
        }

        p = 0;
    }

    if (l >= (long long int)fr.size()) {
        return {false, false};
    }

    if (p >= fr[l].first.length()) {
        return {false, false};
    }

    if (fr[l].first[p] != s) {
        return {false, false};
    }

    ++p;

    return {true, true};
}

template<class Trait>
inline std::pair<bool, bool>
readHtmlAttr(long long int &l,
             long long int &p,
             const typename MdBlock<Trait>::Data &fr,
             bool checkForSpace)
{
    long long int tl = l, tp = p;

    skipSpacesInHtml<Trait>(l, p, fr);

    if (l >= (long long int)fr.size()) {
        return {false, false};
    }

    // /
    if (p < fr[l].first.length() && fr[l].first[p] == Trait::latin1ToChar('/')) {
        return {false, true};
    }

    // >
    if (p < fr[l].first.length() && fr[l].first[p] == Trait::latin1ToChar('>')) {
        return {false, true};
    }

    if (checkForSpace) {
        if (tl == l && tp == p) {
            return {false, false};
        }
    }

    const auto start = p;

    for (; p < fr[l].first.length(); ++p) {
        const auto ch = fr[l].first[p];

        if (ch.isSpace() || ch == Trait::latin1ToChar('>') || ch == Trait::latin1ToChar('=')) {
            break;
        }
    }

    const typename Trait::String name = fr[l].first.asString().sliced(start, p - start).toLower();

    if (!name.startsWith(Trait::latin1ToString("_")) && !name.startsWith(Trait::latin1ToString(":")) &&
        !name.isEmpty() && !(name[0].unicode() >= 97 && name[0].unicode() <= 122)) {
        return {false, false};
    }

    static const typename Trait::String allowedInName =
        Trait::latin1ToString("abcdefghijklmnopqrstuvwxyz0123456789_.:-");

    for (long long int i = 1; i < name.length(); ++i) {
        if (!allowedInName.contains(name[i])) {
            return {false, false};
        }
    }

    // >
    if (p < fr[l].first.length() && fr[l].first[p] == Trait::latin1ToChar('>')) {
        return {false, true};
    }

    tl = l;
    tp = p;

    skipSpacesInHtml<Trait>(l, p, fr);

    if (l >= (long long int)fr.size()) {
        return {false, false};
    }

    // =
    if (p < fr[l].first.length()) {
        if (fr[l].first[p] != Trait::latin1ToChar('=')) {
            l = tl;
            p = tp;

            return {true, true};
        } else {
            ++p;
        }
    } else {
        return {true, false};
    }

    skipSpacesInHtml<Trait>(l, p, fr);

    if (l >= (long long int)fr.size()) {
        return {false, false};
    }

    return readHtmlAttrValue<Trait>(l, p, fr);
}

template<class Trait>
inline std::tuple<bool, long long int, long long int, bool, typename Trait::String>
isHtmlTag(long long int line, long long int pos, TextParsingOpts<Trait> &po, int rule);

template<class Trait>
inline bool
isOnlyHtmlTagsAfterOrClosedRule1(long long int line,
                                 long long int pos,
                                 TextParsingOpts<Trait> &po,
                                 int rule)
{
    static const std::set<typename Trait::String> s_rule1Finish = {Trait::latin1ToString("/pre"),
                                                                   Trait::latin1ToString("/script"),
                                                                   Trait::latin1ToString("/style"),
                                                                   Trait::latin1ToString("/textarea")};

    auto p = skipSpaces<Trait>(pos, po.m_fr.m_data[line].first.asString());

    while (p < po.m_fr.m_data[line].first.length()) {
        bool ok = false;

        long long int l;
        typename Trait::String tag;

        std::tie(ok, l, p, std::ignore, tag) = isHtmlTag(line, p, po, rule);

        ++p;

        if (rule != 1) {
            if (!ok) {
                return false;
            }

            if (l > line) {
                return true;
            }
        } else {
            if (s_rule1Finish.find(tag.toLower()) != s_rule1Finish.cend() && l == line) {
                return true;
            }

            if (l > line) {
                return false;
            }
        }

        p = skipSpaces<Trait>(p, po.m_fr.m_data[line].first.asString());
    }

    if (p >= po.m_fr.m_data[line].first.length()) {
        return true;
    }

    return false;
}

template<class Trait>
inline bool
isSetextHeadingBetween(const TextParsingOpts<Trait> &po,
                       long long int startLine,
                       long long int endLine)
{
    for (; startLine <= endLine; ++startLine) {
        const auto pos = skipSpaces<Trait>(0, po.m_fr.m_data.at(startLine).first.asString());
        const auto line = po.m_fr.m_data.at(startLine).first.asString().sliced(pos);

        if ((isH1<Trait>(line) || isH2<Trait>(line)) && pos < 4) {
            return true;
        }
    }

    return false;
}

template<class Trait>
inline std::tuple<bool, long long int, long long int, bool, typename Trait::String>
isHtmlTag(long long int line,
          long long int pos,
          TextParsingOpts<Trait> &po,
          int rule)
{
    if (po.m_fr.m_data[line].first[pos] != Trait::latin1ToChar('<')) {
        return {false, line, pos, false, {}};
    }

    typename Trait::String tag;

    long long int l = line;
    long long int p = pos + 1;
    bool first = false;

    {
        const auto tmp = skipSpaces<Trait>(0, po.m_fr.m_data[l].first.asString());
        first = (tmp == pos);
    }

    if (p >= po.m_fr.m_data[l].first.length()) {
        return {false, line, pos, first, tag};
    }

    bool closing = false;

    if (po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('/')) {
        closing = true;

        tag.push_back(Trait::latin1ToChar('/'));

        ++p;
    }

    const auto start = p;

    // tag
    for (; p < po.m_fr.m_data[l].first.length(); ++p) {
        const auto ch = po.m_fr.m_data[l].first[p];

        if (ch.isSpace() || ch == Trait::latin1ToChar('>') || ch == Trait::latin1ToChar('/')) {
            break;
        }
    }

    tag.push_back(po.m_fr.m_data[l].first.asString().sliced(start, p - start));

    if (p < po.m_fr.m_data[l].first.length() && po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('/')) {
        if (p + 1 < po.m_fr.m_data[l].first.length() &&
            po.m_fr.m_data[l].first[p + 1] == Trait::latin1ToChar('>')) {
            long long int tmp = 0;

            if (rule == 7) {
                tmp = skipSpaces<Trait>(p + 2, po.m_fr.m_data[l].first.asString());
            }

            bool onLine = (first && (rule == 7 ? tmp == po.m_fr.m_data[l].first.length() :
                isOnlyHtmlTagsAfterOrClosedRule1(l, p + 2, po, rule == 1)));

            if (!isSetextHeadingBetween(po, line, l)) {
                return {true, l, p + 1, onLine, tag};
            } else {
                return {false, line, pos, first, tag};
            }
        } else {
            return {false, line, pos, first, tag};
        }
    }

    if (p < po.m_fr.m_data[l].first.length() && po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('>')) {
        long long int tmp = 0;

        if (rule == 7) {
            tmp = skipSpaces<Trait>(p + 1, po.m_fr.m_data[l].first.asString());
        }

        bool onLine = (first && (rule == 7 ? tmp == po.m_fr.m_data[l].first.length() :
            isOnlyHtmlTagsAfterOrClosedRule1(l, p + 1, po, rule == 1)));

        if (!isSetextHeadingBetween(po, line, l)) {
            return {true, l, p, onLine, tag};
        } else {
            return {false, line, pos, first, tag};
        }
    }

    skipSpacesInHtml<Trait>(l, p, po.m_fr.m_data);

    if (l >= (long long int)po.m_fr.m_data.size()) {
        return {false, line, pos, first, tag};
    }

    if (po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('>')) {
        long long int tmp = 0;

        if (rule == 7) {
            tmp = skipSpaces<Trait>(p + 1, po.m_fr.m_data[l].first.asString());
        }

        bool onLine = (first && (rule == 7 ? tmp == po.m_fr.m_data[l].first.length() :
            isOnlyHtmlTagsAfterOrClosedRule1(l, p + 1, po, rule == 1)));

        if (!isSetextHeadingBetween(po, line, l)) {
            return {true, l, p, onLine, tag};
        } else {
            return {false, line, pos, first, tag};
        }
    }

    bool attr = true;
    bool firstAttr = true;

    while (attr) {
        bool ok = false;

        std::tie(attr, ok) = readHtmlAttr<Trait>(l, p, po.m_fr.m_data, !firstAttr);

        firstAttr = false;

        if (closing && attr) {
            return {false, line, pos, first, tag};
        }

        if (!ok) {
            return {false, line, pos, first, tag};
        }
    }

    if (po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('/')) {
        ++p;
    } else {
        skipSpacesInHtml<Trait>(l, p, po.m_fr.m_data);

        if (l >= (long long int)po.m_fr.m_data.size()) {
            return {false, line, pos, first, tag};
        }
    }

    if (po.m_fr.m_data[l].first[p] == Trait::latin1ToChar('>')) {
        long long int tmp = 0;

        if (rule == 7) {
            tmp = skipSpaces<Trait>(p + 1, po.m_fr.m_data[l].first.asString());
        }

        bool onLine = (first && (rule == 7 ? tmp == po.m_fr.m_data[l].first.length() :
            isOnlyHtmlTagsAfterOrClosedRule1(l, p + 1, po, rule == 1)));

        if (!isSetextHeadingBetween(po, line, l)) {
            return {true, l, p, onLine, tag};
        } else {
            return {false, line, pos, first, tag};
        }
    }

    return {false, line, pos, first, {}};
}

template<class Trait>
inline std::pair<typename Trait::String, bool>
Parser<Trait>::readHtmlTag(typename Delims::const_iterator it,
                           TextParsingOpts<Trait> &po)
{
    long long int i = it->m_pos + 1;
    const auto start = i;

    if (start >= po.m_fr.m_data[it->m_line].first.length()) {
        return {{}, false};
    }

    for (; i < po.m_fr.m_data[it->m_line].first.length(); ++i) {
        const auto ch = po.m_fr.m_data[it->m_line].first[i];

        if (ch.isSpace() || ch == Trait::latin1ToChar('>')) {
            break;
        }
    }

    return {po.m_fr.m_data[it->m_line].first.asString().sliced(start, i - start),
            i < po.m_fr.m_data[it->m_line].first.length() ?
                po.m_fr.m_data[it->m_line].first[i] == Trait::latin1ToChar('>') : false};
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::findIt(typename Delims::const_iterator it,
                      typename Delims::const_iterator last,
                      TextParsingOpts<Trait> &po)
{
    auto ret = it;

    for (; it != last; ++it) {
        if ((it->m_line == po.m_line && it->m_pos < po.m_pos) || it->m_line < po.m_line) {
            ret = it;
        } else {
            break;
        }
    }

    return ret;
}

template<class Trait>
inline void
eatRawHtml(long long int line,
           long long int pos,
           long long int toLine,
           long long int toPos,
           TextParsingOpts<Trait> &po,
           bool finish,
           int htmlRule,
           bool onLine,
           bool continueEating = false)
{
    if (line <= toLine) {
        typename Trait::String h = po.m_html.m_html->text();

        if (!h.isEmpty() && !continueEating) {
            for (long long int i = 0; i < po.m_fr.m_emptyLinesBefore; ++i) {
                h.push_back(Trait::latin1ToChar('\n'));
            }
        }

        const auto first = po.m_fr.m_data[line].first.asString().sliced(
            pos,
            (line == toLine ? (toPos >= 0 ? toPos - pos : po.m_fr.m_data[line].first.length() - pos) :
                po.m_fr.m_data[line].first.length() - pos));

        if (!h.isEmpty() && !first.isEmpty()) {
            h.push_back(Trait::latin1ToChar('\n'));
        }

        if (!first.isEmpty()) {
            h.push_back(first);
        }

        ++line;

        for (; line < toLine; ++line) {
            h.push_back(Trait::latin1ToChar('\n'));
            h.push_back(po.m_fr.m_data[line].first.asString());
        }

        if (line == toLine && toPos != 0) {
            h.push_back(Trait::latin1ToChar('\n'));
            h.push_back(po.m_fr.m_data[line].first.asString().sliced(0, toPos > 0 ?
                toPos : po.m_fr.m_data[line].first.length()));
        }

        auto endColumn = toPos;
        auto endLine = toLine;

        if (endColumn == 0 && endLine > 0) {
            --endLine;
            endColumn = po.m_fr.m_data.at(endLine).first.length();
        }

        po.m_html.m_html->setEndColumn(po.m_fr.m_data.at(endLine).first.virginPos(endColumn >= 0 ?
            endColumn - 1 : po.m_fr.m_data.at(endLine).first.length() - 1));
        po.m_html.m_html->setEndLine(po.m_fr.m_data.at(endLine).second.m_lineNumber);

        po.m_line = (toPos >= 0 ? toLine : toLine + 1);
        po.m_pos = (toPos >= 0 ? toPos : 0);

        if (po.m_line + 1 < static_cast<long long int>(po.m_fr.m_data.size()) &&
            po.m_pos >= po.m_fr.m_data.at(po.m_line).first.length()) {
            ++po.m_line;
            po.m_pos = 0;
        }

        po.m_html.m_html->setText(h);
    }

    UnprotectedDocsMethods<Trait>::setFreeTag(po.m_html.m_html, onLine);

    if (finish) {
        if (po.m_html.m_onLine || htmlRule == 7 || po.m_line < (long long int)po.m_fr.m_data.size()) {
            if (!po.m_collectRefLinks) {
                po.m_parent->appendItem(po.m_html.m_html);
                po.m_parent->setEndColumn(po.m_html.m_html->endColumn());
                po.m_parent->setEndLine(po.m_html.m_html->endLine());
                initLastItemWithOpts<Trait>(po, po.m_html.m_html);
                po.m_html.m_html->setOpts(po.m_opts);
                po.m_isSpaceBefore = false;
                po.m_lastText = nullptr;
            } else {
                po.m_tmpHtml = po.m_html.m_html;
            }

            resetHtmlTag(po.m_html);
        }
    } else {
        po.m_html.m_continueHtml = true;
    }
}

template<class Trait>
inline bool
Parser<Trait>::isNewBlockIn(MdBlock<Trait> &fr,
                            long long int startLine,
                            long long int endLine)
{
    for (auto i = startLine + 1; i <= endLine; ++i) {
        const auto type = whatIsTheLine(fr.m_data[i].first);

        switch (type) {
        case Parser<Trait>::BlockType::Footnote:
        case Parser<Trait>::BlockType::FensedCodeInList:
        case Parser<Trait>::BlockType::SomethingInList:
        case Parser<Trait>::BlockType::List:
        case Parser<Trait>::BlockType::ListWithFirstEmptyLine:
        case Parser<Trait>::BlockType::Code:
        case Parser<Trait>::BlockType::Blockquote:
        case Parser<Trait>::BlockType::Heading:
        case Parser<Trait>::BlockType::EmptyLine:
            return true;

        default:
            break;
        }

        const auto ns = skipSpaces<Trait>(0, fr.m_data[i].first.asString());

        if (ns < 4) {
            const auto s = fr.m_data[i].first.asString().sliced(ns);

            if (isHorizontalLine<Trait>(s) || isH1<Trait>(s) || isH2<Trait>(s)) {
                return true;
            }
        }
    }

    return false;
}

template<class Trait>
inline void
Parser<Trait>::finishRule1HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po,
                                  bool skipFirst)
{
    static const std::set<typename Trait::String> s_finish = {Trait::latin1ToString("/pre"),
                                                              Trait::latin1ToString("/script"),
                                                              Trait::latin1ToString("/style"),
                                                              Trait::latin1ToString("/textarea")};

    if (it != last) {
        bool ok = false;
        long long int l = -1, p = -1;

        if (po.m_html.m_html->text().isEmpty() && it->m_type == Delimiter::Less && skipFirst) {
            std::tie(ok, l, p, po.m_html.m_onLine, std::ignore) =
                isHtmlTag(it->m_line, it->m_pos, po, 1);
        }

        if (po.m_html.m_onLine) {
            for (it = (skipFirst && it != last ? std::next(it) : it); it != last; ++it) {
                if (it->m_type == Delimiter::Less) {
                    typename Trait::String tag;
                    bool closed = false;

                    std::tie(tag, closed) = readHtmlTag(it, po);

                    if (closed) {
                        if (s_finish.find(tag.toLower()) != s_finish.cend()) {
                            eatRawHtml(po.m_line, po.m_pos, it->m_line, -1, po,
                                true, 1, po.m_html.m_onLine);

                            return;
                        }
                    }
                }
            }
        } else if (ok && !isNewBlockIn(po.m_fr, it->m_line, l)) {
            eatRawHtml(po.m_line, po.m_pos, l, p + 1, po, true, 1, false);

            return;
        } else {
            resetHtmlTag(po.m_html);

            return;
        }
    }

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 1, po.m_html.m_onLine);
    } else {
        resetHtmlTag(po.m_html);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishRule2HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    if (it != last) {
        const auto start = it;

        MdLineData::CommentData commentData = {2, true};
        bool onLine = po.m_html.m_onLine;

        if (po.m_html.m_html->text().isEmpty() && it->m_type == Delimiter::Less) {
            long long int i = po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos);

            commentData = po.m_fr.m_data[it->m_line].second.m_htmlCommentData[i];

            onLine = (it->m_pos == skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString()));
            po.m_html.m_onLine = onLine;
        }

        if (commentData.first != -1 && commentData.second) {
            for (; it != last; ++it) {
                if (it->m_type == Delimiter::Greater) {
                    auto p = it->m_pos;

                    bool doContinue = false;

                    for (char i = 0; i < commentData.first; ++i) {
                        if (!(p > 0 && po.m_fr.m_data[it->m_line].first[p - 1] == Trait::latin1ToChar('-'))) {
                            doContinue = true;

                            break;
                        }

                        --p;
                    }

                    if (doContinue) {
                        continue;
                    }

                    if (onLine || !isNewBlockIn(po.m_fr, start->m_line, it->m_line)) {
                        eatRawHtml(po.m_line, po.m_pos, it->m_line,
                            onLine ? po.m_fr.m_data[it->m_line].first.length() : it->m_pos + 1,
                            po, true, 2, onLine);
                    } else {
                        resetHtmlTag(po.m_html);
                    }

                    return;
                }
            }
        }
    }

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 2, po.m_html.m_onLine);
    } else {
        resetHtmlTag(po.m_html);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishRule3HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    bool onLine = po.m_html.m_onLine;

    if (it != last) {
        const auto start = it;

        if (po.m_html.m_html->text().isEmpty() && it->m_type == Delimiter::Less) {
            onLine = (it->m_pos == skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString()));
            po.m_html.m_onLine = onLine;
        }

        for (; it != last; ++it) {
            if (it->m_type == Delimiter::Greater) {
                if (it->m_pos > 0 && po.m_fr.m_data[it->m_line].first[it->m_pos - 1] == Trait::latin1ToChar('?')) {
                    long long int i = it->m_pos + 1;

                    for (; i < po.m_fr.m_data[it->m_line].first.length(); ++i) {
                        if (po.m_fr.m_data[it->m_line].first[i] == Trait::latin1ToChar('<')) {
                            break;
                        }
                    }

                    if (onLine || !isNewBlockIn(po.m_fr, start->m_line, it->m_line)) {
                        eatRawHtml(po.m_line, po.m_pos, it->m_line, i, po, true, 3, onLine);
                    } else {
                        resetHtmlTag(po.m_html);
                    }

                    return;
                }
            }
        }
    }

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 3, onLine);
    } else {
        resetHtmlTag(po.m_html);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishRule4HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    if (it != last) {
        const auto start = it;

        bool onLine = po.m_html.m_onLine;

        if (po.m_html.m_html->text().isEmpty() && it->m_type == Delimiter::Less) {
            onLine = (it->m_pos == skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString()));
            po.m_html.m_onLine = onLine;
        }

        for (; it != last; ++it) {
            if (it->m_type == Delimiter::Greater) {
                long long int i = it->m_pos + 1;

                for (; i < po.m_fr.m_data[it->m_line].first.length(); ++i) {
                    if (po.m_fr.m_data[it->m_line].first[i] == Trait::latin1ToChar('<')) {
                        break;
                    }
                }

                if (onLine || !isNewBlockIn(po.m_fr, start->m_line, it->m_line)) {
                    eatRawHtml(po.m_line, po.m_pos, it->m_line, i, po, true, 4, onLine);
                } else {
                    resetHtmlTag(po.m_html);
                }

                return;
            }
        }
    }

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 4, true);
    } else {
        resetHtmlTag(po.m_html);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishRule5HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    if (it != last) {
        const auto start = it;

        bool onLine = po.m_html.m_onLine;

        if (po.m_html.m_html->text().isEmpty() && it->m_type == Delimiter::Less) {
            onLine = (it->m_pos == skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString()));
            po.m_html.m_onLine = onLine;
        }

        for (; it != last; ++it) {
            if (it->m_type == Delimiter::Greater) {
                if (it->m_pos > 1 && po.m_fr.m_data[it->m_line].first[it->m_pos - 1] == Trait::latin1ToChar(']') &&
                    po.m_fr.m_data[it->m_line].first[it->m_pos - 2] == Trait::latin1ToChar(']')) {
                    long long int i = it->m_pos + 1;

                    for (; i < po.m_fr.m_data[it->m_line].first.length(); ++i) {
                        if (po.m_fr.m_data[it->m_line].first[i] == Trait::latin1ToChar('<')) {
                            break;
                        }
                    }

                    if (onLine || !isNewBlockIn(po.m_fr, start->m_line, it->m_line)) {
                        eatRawHtml(po.m_line, po.m_pos, it->m_line, i, po, true, 5, onLine);
                    } else {
                        resetHtmlTag(po.m_html);
                    }

                    return;
                }
            }
        }
    }

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 5, true);
    } else {
        resetHtmlTag(po.m_html);
    }
}

template<class Trait>
inline void
Parser<Trait>::finishRule6HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    po.m_html.m_onLine = (it != last ?
        it->m_pos == skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString()) : true);

    if (po.m_html.m_onLine) {
        eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po,
            false, 6, po.m_html.m_onLine);
    } else {
        const auto nit = std::find_if(std::next(it), last, [](const auto &d) {
            return (d.m_type == Delimiter::Greater);
        });

        if (nit != last && !isNewBlockIn(po.m_fr, it->m_line, nit->m_line)) {
            eatRawHtml(po.m_line, po.m_pos, nit->m_line, nit->m_pos + nit->m_len, po,
                true, 6, false);
        }
    }

    if (po.m_fr.m_emptyLineAfter && po.m_html.m_html) {
        po.m_html.m_continueHtml = false;
    }
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::finishRawHtmlTag(typename Delims::const_iterator it,
                                typename Delims::const_iterator last,
                                TextParsingOpts<Trait> &po,
                                bool skipFirst)
{
    po.m_detected = TextParsingOpts<Trait>::Detected::HTML;

    switch (po.m_html.m_htmlBlockType) {
    case 1:
        finishRule1HtmlTag(it, last, po, skipFirst);
        break;

    case 2:
        finishRule2HtmlTag(it, last, po);
        break;

    case 3:
        finishRule3HtmlTag(it, last, po);
        break;

    case 4:
        finishRule4HtmlTag(it, last, po);
        break;

    case 5:
        finishRule5HtmlTag(it, last, po);
        break;

    case 6:
        finishRule6HtmlTag(it, last, po);
        break;

    case 7:
        return finishRule7HtmlTag(it, last, po);

    default:
        po.m_detected = TextParsingOpts<Trait>::Detected::Nothing;
        break;
    }

    return findIt(it, last, po);
}

template<class Trait>
inline int
Parser<Trait>::htmlTagRule(typename Delims::const_iterator it,
                           typename Delims::const_iterator last,
                           TextParsingOpts<Trait> &po)
{
    MD_UNUSED(last)

    typename Trait::String tag;

    std::tie(tag, std::ignore) = readHtmlTag(it, po);

    if (tag.startsWith(Trait::latin1ToString("![CDATA["))) {
        return 5;
    }

    tag = tag.toLower();

    static const typename Trait::String s_validHtmlTagLetters =
        Trait::latin1ToString("abcdefghijklmnopqrstuvwxyz0123456789-");

    bool closing = false;

    if (tag.startsWith(Trait::latin1ToString("/"))) {
        tag.remove(0, 1);
        closing = true;
    }

    if (tag.endsWith(Trait::latin1ToString("/"))) {
        tag.remove(tag.size() - 1, 1);
    }

    if (tag.isEmpty()) {
        return -1;
    }

    if (!tag.startsWith(Trait::latin1ToString("!")) &&
        !tag.startsWith(Trait::latin1ToString("?")) &&
        !(tag[0].unicode() >= 97 && tag[0].unicode() <= 122)) {
        return -1;
    }

    static const std::set<typename Trait::String> s_rule1 = {Trait::latin1ToString("pre"),
                                                             Trait::latin1ToString("script"),
                                                             Trait::latin1ToString("style"),
                                                             Trait::latin1ToString("textarea")};

    if (!closing && s_rule1.find(tag) != s_rule1.cend()) {
        return 1;
    } else if (tag.startsWith(Trait::latin1ToString("!--"))) {
        return 2;
    } else if (tag.startsWith(Trait::latin1ToString("?"))) {
        return 3;
    } else if (tag.startsWith(Trait::latin1ToString("!")) && tag.size() > 1 &&
               ((tag[1].unicode() >= 65 && tag[1].unicode() <= 90) ||
                    (tag[1].unicode() >= 97 && tag[1].unicode() <= 122))) {
        return 4;
    } else {
        static const std::set<typename Trait::String> s_rule6 = {
            Trait::latin1ToString("address"),  Trait::latin1ToString("article"),    Trait::latin1ToString("aside"),    Trait::latin1ToString("base"),
            Trait::latin1ToString("basefont"), Trait::latin1ToString("blockquote"), Trait::latin1ToString("body"),     Trait::latin1ToString("caption"),
            Trait::latin1ToString("center"),   Trait::latin1ToString("col"),        Trait::latin1ToString("colgroup"), Trait::latin1ToString("dd"),
            Trait::latin1ToString("details"),  Trait::latin1ToString("dialog"),     Trait::latin1ToString("dir"),      Trait::latin1ToString("div"),
            Trait::latin1ToString("dl"),       Trait::latin1ToString("dt"),         Trait::latin1ToString("fieldset"), Trait::latin1ToString("figcaption"),
            Trait::latin1ToString("figure"),   Trait::latin1ToString("footer"),     Trait::latin1ToString("form"),     Trait::latin1ToString("frame"),
            Trait::latin1ToString("frameset"), Trait::latin1ToString("h1"),         Trait::latin1ToString("h2"),       Trait::latin1ToString("h3"),
            Trait::latin1ToString("h4"),       Trait::latin1ToString("h5"),         Trait::latin1ToString("h6"),       Trait::latin1ToString("head"),
            Trait::latin1ToString("header"),   Trait::latin1ToString("hr"),         Trait::latin1ToString("html"),     Trait::latin1ToString("iframe"),
            Trait::latin1ToString("legend"),   Trait::latin1ToString("li"),         Trait::latin1ToString("link"),     Trait::latin1ToString("main"),
            Trait::latin1ToString("menu"),     Trait::latin1ToString("menuitem"),   Trait::latin1ToString("nav"),      Trait::latin1ToString("noframes"),
            Trait::latin1ToString("ol"),       Trait::latin1ToString("optgroup"),   Trait::latin1ToString("option"),   Trait::latin1ToString("p"),
            Trait::latin1ToString("param"),    Trait::latin1ToString("section"),    Trait::latin1ToString("search"),   Trait::latin1ToString("summary"),
            Trait::latin1ToString("table"),    Trait::latin1ToString("tbody"),      Trait::latin1ToString("td"),       Trait::latin1ToString("tfoot"),
            Trait::latin1ToString("th"),       Trait::latin1ToString("thead"),      Trait::latin1ToString("title"),    Trait::latin1ToString("tr"),
            Trait::latin1ToString("track"),    Trait::latin1ToString("ul")};

        for (long long int i = 1; i < tag.size(); ++i) {
            if (!s_validHtmlTagLetters.contains(tag[i])) {
                return -1;
            }
        }

        if (s_rule6.find(tag) != s_rule6.cend()) {
            return 6;
        } else {
            bool tag = false;

            std::tie(tag, std::ignore, std::ignore, std::ignore, std::ignore) =
                isHtmlTag(it->m_line, it->m_pos, po, 7);

            if (tag) {
                return 7;
            }
        }
    }

    return -1;
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForRawHtml(typename Delims::const_iterator it,
                               typename Delims::const_iterator last,
                               TextParsingOpts<Trait> &po)
{
    const auto rule = htmlTagRule(it, last, po);

    if (rule == -1) {
        resetHtmlTag(po.m_html);

        po.m_firstInParagraph = false;

        return it;
    }

    po.m_html.m_htmlBlockType = rule;
    po.m_html.m_html.reset(new RawHtml<Trait>);
    po.m_html.m_html->setStartColumn(po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos));
    po.m_html.m_html->setStartLine(po.m_fr.m_data.at(it->m_line).second.m_lineNumber);

    return finishRawHtmlTag(it, last, po, true);
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::finishRule7HtmlTag(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    if (it != last) {
        const auto start = it;
        long long int l = -1, p = -1;
        bool onLine = false;
        bool ok = false;

        std::tie(ok, l, p, onLine, std::ignore) = isHtmlTag(it->m_line, it->m_pos, po, 7);

        onLine = onLine && it->m_line == 0 && l == start->m_line;

        if (ok) {
            eatRawHtml(po.m_line, po.m_pos, l, ++p, po, !onLine, 7, onLine);

            po.m_html.m_onLine = onLine;

            it = findIt(it, last, po);

            if (onLine) {
                for (; it != last; ++it) {
                    if (it->m_type == Delimiter::Less) {
                        const auto rule = htmlTagRule(it, last, po);

                        if (rule != -1 && rule != 7) {
                            eatRawHtml(po.m_line, po.m_pos, it->m_line, it->m_pos, po, true, 7, onLine, true);

                            return std::prev(it);
                        }
                    }
                }

                eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, false, 7, onLine, true);

                return std::prev(last);
            } else {
                return it;
            }
        } else {
            return it;
        }
    } else {
        if (po.m_html.m_onLine) {
            eatRawHtml(po.m_line, po.m_pos, po.m_fr.m_data.size() - 1, -1, po, true, 7, true);

            return last;
        } else {
            resetHtmlTag(po.m_html);
        }
    }

    return it;
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForMath(typename Delims::const_iterator it,
                            typename Delims::const_iterator last,
                            TextParsingOpts<Trait> &po)
{
    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    const auto end = std::find_if(std::next(it), last, [&](const auto &d) {
        return (d.m_type == Delimiter::Math && d.m_len == it->m_len);
    });

    if (end != last && end->m_line <= po.m_lastTextLine) {
        typename Trait::String math;

        if (it->m_line == end->m_line) {
            math = po.m_fr.m_data[it->m_line].first.asString().sliced(
                it->m_pos + it->m_len, end->m_pos - (it->m_pos + it->m_len));
        } else {
            math = po.m_fr.m_data[it->m_line].first.asString().sliced(it->m_pos + it->m_len);

            for (long long int i = it->m_line + 1; i < end->m_line; ++i) {
                math.push_back(Trait::latin1ToChar('\n'));
                math.push_back(po.m_fr.m_data[i].first.asString());
            }

            math.push_back(Trait::latin1ToChar('\n'));
            math.push_back(po.m_fr.m_data[end->m_line].first.asString().sliced(0, end->m_pos));
        }

        if (!po.m_collectRefLinks) {
            std::shared_ptr<Math<Trait>> m(new Math<Trait>);

            auto startLine = po.m_fr.m_data.at(it->m_line).second.m_lineNumber;
            auto startColumn = po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len);

            if (it->m_pos + it->m_len >= po.m_fr.m_data.at(it->m_line).first.length()) {
                std::tie(startColumn, startLine) = nextPosition(po.m_fr, startColumn, startLine);
            }

            auto endColumn = po.m_fr.m_data.at(end->m_line).first.virginPos(end->m_pos);
            auto endLine = po.m_fr.m_data.at(end->m_line).second.m_lineNumber;

            if (endColumn == 0) {
                std::tie(endColumn, endLine) = prevPosition(po.m_fr, endColumn, endLine);
            } else {
                --endColumn;
            }

            m->setStartColumn(startColumn);
            m->setStartLine(startLine);
            m->setEndColumn(endColumn);
            m->setEndLine(endLine);
            m->setInline(it->m_len == 1);
            m->setStartDelim({po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos),
                              po.m_fr.m_data[it->m_line].second.m_lineNumber,
                              po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos + it->m_len - 1),
                              po.m_fr.m_data[it->m_line].second.m_lineNumber});
            m->setEndDelim({po.m_fr.m_data[end->m_line].first.virginPos(end->m_pos),
                            po.m_fr.m_data[end->m_line].second.m_lineNumber,
                            po.m_fr.m_data[end->m_line].first.virginPos(end->m_pos + end->m_len - 1),
                            po.m_fr.m_data[end->m_line].second.m_lineNumber});
            m->setFensedCode(false);

            initLastItemWithOpts<Trait>(po, m);

            if (math.startsWith(Trait::latin1ToString("`")) &&
                math.endsWith(Trait::latin1ToString("`")) &&
                !math.endsWith(Trait::latin1ToString("\\`")) &&
                math.length() > 1) {
                math = math.sliced(1, math.length() - 2);
            }

            m->setExpr(math);

            po.m_parent->appendItem(m);

            po.m_pos = end->m_pos + end->m_len;
            po.m_line = end->m_line;
            po.m_isSpaceBefore = false;
            po.m_lastText = nullptr;
        }

        return end;
    }

    return it;
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForAutolinkHtml(typename Delims::const_iterator it,
                                    typename Delims::const_iterator last,
                                    TextParsingOpts<Trait> &po,
                                    bool updatePos)
{
    const auto nit = std::find_if(std::next(it), last, [](const auto &d) {
        return (d.m_type == Delimiter::Greater);
    });

    if (nit != last) {
        if (nit->m_line == it->m_line) {
            const auto url = po.m_fr.m_data.at(it->m_line).first.asString().sliced(
                it->m_pos + 1, nit->m_pos - it->m_pos - 1);

            bool isUrl = true;

            for (long long int i = 0; i < url.size(); ++i) {
                if (url[i].isSpace()) {
                    isUrl = false;

                    break;
                }
            }

            if (isUrl) {
                if (!isValidUrl<Trait>(url) && !isEmail<Trait>(url)) {
                    isUrl = false;
                }
            }

            if (isUrl) {
                if (!po.m_collectRefLinks) {
                    std::shared_ptr<Link<Trait>> lnk(new Link<Trait>);
                    lnk->setStartColumn(po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos));
                    lnk->setStartLine(po.m_fr.m_data.at(it->m_line).second.m_lineNumber);
                    lnk->setEndColumn(po.m_fr.m_data.at(nit->m_line).first.virginPos(nit->m_pos + nit->m_len - 1));
                    lnk->setEndLine(po.m_fr.m_data.at(nit->m_line).second.m_lineNumber);
                    lnk->setUrl(url.simplified());
                    lnk->setOpts(po.m_opts);
                    lnk->setTextPos({po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos + 1),
                                     po.m_fr.m_data[it->m_line].second.m_lineNumber,
                                     po.m_fr.m_data[nit->m_line].first.virginPos(nit->m_pos - 1),
                                     po.m_fr.m_data[nit->m_line].second.m_lineNumber});
                    lnk->setUrlPos(lnk->textPos());
                    po.m_parent->appendItem(lnk);
                }

                po.m_wasRefLink = false;
                po.m_firstInParagraph = false;
                po.m_isSpaceBefore = false;
                po.m_lastText = nullptr;

                if (updatePos) {
                    po.m_pos = nit->m_pos + nit->m_len;
                    po.m_line = nit->m_line;
                }

                return nit;
            } else {
                return checkForRawHtml(it, last, po);
            }
        } else {
            return checkForRawHtml(it, last, po);
        }
    } else {
        return checkForRawHtml(it, last, po);
    }
}

template<class Trait>
inline void
Parser<Trait>::makeInlineCode(long long int startLine,
                              long long int startPos,
                              long long int lastLine,
                              long long int lastPos,
                              TextParsingOpts<Trait> &po,
                              typename Delims::const_iterator startDelimIt,
                              typename Delims::const_iterator endDelimIt)
{
    typename Trait::String c;

    for (; po.m_line <= lastLine; ++po.m_line) {
        c.push_back(po.m_fr.m_data.at(po.m_line).first.asString().sliced(
            po.m_pos, (po.m_line == lastLine ? lastPos - po.m_pos :
                po.m_fr.m_data.at(po.m_line).first.length() - po.m_pos)));

        if (po.m_line < lastLine) {
            c.push_back(Trait::latin1ToChar(' '));
        }

        po.m_pos = 0;
    }

    po.m_line = lastLine;

    if (c[0] == Trait::latin1ToChar(' ') && c[c.size() - 1] == Trait::latin1ToChar(' ') &&
        skipSpaces<Trait>(0, c) < c.size()) {
        c.remove(0, 1);
        c.remove(c.size() - 1, 1);
        ++startPos;
        --lastPos;
    }

    if (!c.isEmpty()) {
        auto code = std::make_shared<Code<Trait>>(c, false, true);

        code->setStartColumn(po.m_fr.m_data.at(startLine).first.virginPos(startPos));
        code->setStartLine(po.m_fr.m_data.at(startLine).second.m_lineNumber);
        code->setEndColumn(po.m_fr.m_data.at(lastLine).first.virginPos(lastPos - 1));
        code->setEndLine(po.m_fr.m_data.at(lastLine).second.m_lineNumber);
        code->setStartDelim({po.m_fr.m_data.at(startDelimIt->m_line).first.virginPos(
                                startDelimIt->m_pos + (startDelimIt->m_backslashed ? 1 : 0)),
                             po.m_fr.m_data.at(startDelimIt->m_line).second.m_lineNumber,
                             po.m_fr.m_data.at(startDelimIt->m_line).first.virginPos(
                                startDelimIt->m_pos + (startDelimIt->m_backslashed ? 1 : 0)) +
                                startDelimIt->m_len - 1 - (startDelimIt->m_backslashed ? 1 : 0),
                             po.m_fr.m_data.at(startDelimIt->m_line).second.m_lineNumber});
        code->setEndDelim(
            {po.m_fr.m_data.at(endDelimIt->m_line).first.virginPos(
                endDelimIt->m_pos + (endDelimIt->m_backslashed ? 1 : 0)),
             po.m_fr.m_data.at(endDelimIt->m_line).second.m_lineNumber,
             po.m_fr.m_data.at(endDelimIt->m_line).first.virginPos(
                endDelimIt->m_pos + (endDelimIt->m_backslashed ? 1 : 0) +
                    endDelimIt->m_len - 1 - (endDelimIt->m_backslashed ? 1 : 0)),
             po.m_fr.m_data.at(endDelimIt->m_line).second.m_lineNumber});
        code->setOpts(po.m_opts);

        initLastItemWithOpts<Trait>(po, code);

        po.m_parent->appendItem(code);
    }

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;
    po.m_isSpaceBefore = false;
    po.m_lastText = nullptr;
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForInlineCode(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po)
{
    const auto len = it->m_len;
    const auto start = it;

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    ++it;

    for (; it != last; ++it) {
        if (it->m_line <= po.m_lastTextLine) {
            const auto p = skipSpaces<Trait>(0, po.m_fr.m_data.at(it->m_line).first.asString());
            const auto withoutSpaces = po.m_fr.m_data.at(it->m_line).first.asString().sliced(p);

            if ((it->m_type == Delimiter::HorizontalLine && withoutSpaces[0] == Trait::latin1ToChar('-')) ||
                it->m_type == Delimiter::H1 || it->m_type == Delimiter::H2) {
                break;
            } else if (it->m_type == Delimiter::InlineCode && (it->m_len - (it->m_backslashed ? 1 : 0)) == len) {
                if (!po.m_collectRefLinks) {
                    makeText(start->m_line, start->m_pos, po);

                    po.m_pos = start->m_pos + start->m_len;

                    makeInlineCode(start->m_line, start->m_pos + start->m_len, it->m_line,
                        it->m_pos + (it->m_backslashed ? 1 : 0), po, start, it);

                    po.m_line = it->m_line;
                    po.m_pos = it->m_pos + it->m_len;
                }

                return it;
            }
        } else {
            break;
        }
    }

    if (!po.m_collectRefLinks) {
        makeText(start->m_line, start->m_pos + start->m_len, po);
    }

    return start;
}

template<class Trait>
inline std::pair<typename MdBlock<Trait>::Data, typename Parser<Trait>::Delims::const_iterator>
Parser<Trait>::readTextBetweenSquareBrackets(typename Delims::const_iterator start,
                                             typename Delims::const_iterator it,
                                             typename Delims::const_iterator last,
                                             TextParsingOpts<Trait> &po,
                                             bool doNotCreateTextOnFail,
                                             WithPosition *pos)
{
    if (it != last && it->m_line <= po.m_lastTextLine) {
        if (start->m_line == it->m_line) {
            const auto p = start->m_pos + start->m_len;
            const auto n = it->m_pos - p;

            if (pos) {
                long long int startPos, startLine, endPos, endLine;
                std::tie(startPos, startLine) = nextPosition(po.m_fr,
                                                             po.m_fr.m_data[start->m_line].first.virginPos(
                                                                start->m_pos + start->m_len - 1),
                                                             po.m_fr.m_data[start->m_line].second.m_lineNumber);
                std::tie(endPos, endLine) =
                    prevPosition(po.m_fr, po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos),
                        po.m_fr.m_data[it->m_line].second.m_lineNumber);

                *pos = {startPos, startLine, endPos, endLine};
            }

            return {{{po.m_fr.m_data.at(start->m_line).first.sliced(p, n).simplified(),
                    {po.m_fr.m_data.at(start->m_line).second.m_lineNumber}}}, it};
        } else {
            if (it->m_line - start->m_line < 3) {
                typename MdBlock<Trait>::Data res;
                res.push_back({po.m_fr.m_data.at(start->m_line).first.sliced(
                    start->m_pos + start->m_len).simplified(), po.m_fr.m_data.at(start->m_line).second});

                long long int i = start->m_line + 1;

                for (; i <= it->m_line; ++i) {
                    if (i == it->m_line) {
                        res.push_back({po.m_fr.m_data.at(i).first.sliced(0, it->m_pos).simplified(),
                            po.m_fr.m_data.at(i).second});
                    } else {
                        res.push_back({po.m_fr.m_data.at(i).first.simplified(), po.m_fr.m_data.at(i).second});
                    }
                }

                if (pos) {
                    long long int startPos, startLine, endPos, endLine;
                    std::tie(startPos, startLine) = nextPosition(po.m_fr,
                                                                 po.m_fr.m_data[start->m_line].first.virginPos(
                                                                    start->m_pos + start->m_len - 1),
                                                                 po.m_fr.m_data[start->m_line].second.m_lineNumber);
                    std::tie(endPos, endLine) =
                        prevPosition(po.m_fr, po.m_fr.m_data[it->m_line].first.virginPos(it->m_pos),
                            po.m_fr.m_data[it->m_line].second.m_lineNumber);

                    *pos = {startPos, startLine, endPos, endLine};
                }

                return {res, it};
            } else {
                if (!po.m_collectRefLinks && !doNotCreateTextOnFail) {
                    makeText(start->m_line, start->m_pos + start->m_len, po);
                }

                return {{}, start};
            }
        }
    } else {
        if (!po.m_collectRefLinks && !doNotCreateTextOnFail) {
            makeText(start->m_line, start->m_pos + start->m_len, po);
        }

        return {{}, start};
    }
}

template<class Trait>
inline std::pair<typename MdBlock<Trait>::Data, typename Parser<Trait>::Delims::const_iterator>
Parser<Trait>::checkForLinkText(typename Delims::const_iterator it,
                                typename Delims::const_iterator last,
                                TextParsingOpts<Trait> &po,
                                WithPosition *pos)
{
    const auto start = it;

    long long int brackets = 0;

    const bool collectRefLinks = po.m_collectRefLinks;
    po.m_collectRefLinks = true;
    long long int l = po.m_line, p = po.m_pos;

    for (it = std::next(it); it != last; ++it) {
        bool quit = false;

        switch (it->m_type) {
        case Delimiter::SquareBracketsClose: {
            if (!brackets)
                quit = true;
            else
                --brackets;
        } break;

        case Delimiter::SquareBracketsOpen:
        case Delimiter::ImageOpen:
            ++brackets;
            break;

        case Delimiter::InlineCode:
            it = checkForInlineCode(it, last, po);
            break;

        case Delimiter::Less:
            it = checkForAutolinkHtml(it, last, po, false);
            break;

        default:
            break;
        }

        if (quit) {
            break;
        }
    }

    const auto r = readTextBetweenSquareBrackets(start, it, last, po, false, pos);

    po.m_collectRefLinks = collectRefLinks;
    resetHtmlTag(po.m_html);
    po.m_line = l;
    po.m_pos = p;

    return r;
}

template<class Trait>
inline std::pair<typename MdBlock<Trait>::Data, typename Parser<Trait>::Delims::const_iterator>
Parser<Trait>::checkForLinkLabel(typename Delims::const_iterator it,
                                 typename Delims::const_iterator last,
                                 TextParsingOpts<Trait> &po,
                                 WithPosition *pos)
{
    const auto start = it;

    for (it = std::next(it); it != last; ++it) {
        bool quit = false;

        switch (it->m_type) {
        case Delimiter::SquareBracketsClose: {
            quit = true;
        } break;

        case Delimiter::SquareBracketsOpen:
        case Delimiter::ImageOpen: {
            it = last;
            quit = true;
        } break;

        default:
            break;
        }

        if (quit)
            break;
    }

    return readTextBetweenSquareBrackets(start, it, last, po, true, pos);
}

template<class Trait>
inline typename Trait::String
Parser<Trait>::toSingleLine(const typename MdBlock<Trait>::Data &d)
{
    typename Trait::String res;
    bool first = true;

    for (const auto &s : d) {
        if (!first) {
            res.push_back(Trait::latin1ToChar(' '));
        }
        res.push_back(s.first.asString());
        first = false;
    }

    return res;
}

template<class Trait>
inline std::shared_ptr<Link<Trait>>
Parser<Trait>::makeLink(const typename Trait::String &url,
                        const typename MdBlock<Trait>::Data &text,
                        TextParsingOpts<Trait> &po,
                        bool doNotCreateTextOnFail,
                        long long int startLine,
                        long long int startPos,
                        long long int lastLine,
                        long long int lastPos,
                        const WithPosition &textPos,
                        const WithPosition &urlPos)
{
    MD_UNUSED(doNotCreateTextOnFail)

    typename Trait::String u = (url.startsWith(Trait::latin1ToString("#")) ?
        url : removeBackslashes<typename Trait::String, Trait>(replaceEntity<Trait>(url)));

    if (!u.isEmpty()) {
        if (!u.startsWith(Trait::latin1ToString("#"))) {
            const auto checkForFile = [&](typename Trait::String &url,
                                          const typename Trait::String &ref = {}) -> bool {
                if (Trait::fileExists(url)) {
                    url = Trait::absoluteFilePath(url);

                    if (!po.m_collectRefLinks) {
                        po.m_linksToParse.push_back(url);
                    }

                    if (!ref.isEmpty()) {
                        url = ref + Trait::latin1ToString("/") + url;
                    }

                    return true;
                } else if (Trait::fileExists(url, po.m_workingPath)) {
                    url = Trait::absoluteFilePath(po.m_workingPath + Trait::latin1ToString("/") + url);

                    if (!po.m_collectRefLinks) {
                        po.m_linksToParse.push_back(url);
                    }

                    if (!ref.isEmpty()) {
                        url = ref + Trait::latin1ToString("/") + url;
                    }

                    return true;
                } else {
                    return false;
                }
            };

            if (!checkForFile(u) && u.contains(Trait::latin1ToChar('#'))) {
                const auto i = u.indexOf(Trait::latin1ToChar('#'));
                const auto ref = u.sliced(i);
                u = u.sliced(0, i);

                if (!checkForFile(u, ref)) {
                    u = u + ref;
                }
            }
        } else
            u = u + (po.m_workingPath.isEmpty() ? typename Trait::String() :
                Trait::latin1ToString("/") + po.m_workingPath) + Trait::latin1ToString("/") +
                po.m_fileName;
    }

    std::shared_ptr<Link<Trait>> link(new Link<Trait>);
    link->setUrl(u);
    link->setOpts(po.m_opts);
    link->setTextPos(textPos);
    link->setUrlPos(urlPos);

    MdBlock<Trait> block = {text, 0};

    std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);

    RawHtmlBlock<Trait> html;

    parseFormattedTextLinksImages(block,
                                  std::static_pointer_cast<Block<Trait>>(p),
                                  po.m_doc,
                                  po.m_linksToParse,
                                  po.m_workingPath,
                                  po.m_fileName,
                                  po.m_collectRefLinks,
                                  true,
                                  html,
                                  true);

    if (!p->isEmpty()) {
        std::shared_ptr<Image<Trait>> img;

        if (p->items().size() == 1 && p->items().at(0)->type() == ItemType::Paragraph) {
            const auto ip = std::static_pointer_cast<Paragraph<Trait>>(p->items().at(0));

            for (auto it = ip->items().cbegin(), last = ip->items().cend(); it != last; ++it) {
                switch ((*it)->type()) {
                case ItemType::Link:
                    return {};

                case ItemType::Image: {
                    img = std::static_pointer_cast<Image<Trait>>(*it);
                } break;

                default:
                    break;
                }
            }

            if (img.get()) {
                link->setImg(img);
            }

            link->setP(ip);
        }
    }

    if (html.m_html.get()) {
        link->p()->appendItem(html.m_html);
    }

    link->setText(toSingleLine(text).simplified());
    link->setStartColumn(po.m_fr.m_data.at(startLine).first.virginPos(startPos));
    link->setStartLine(po.m_fr.m_data.at(startLine).second.m_lineNumber);
    link->setEndColumn(po.m_fr.m_data.at(lastLine).first.virginPos(lastPos - 1));
    link->setEndLine(po.m_fr.m_data.at(lastLine).second.m_lineNumber);

    initLastItemWithOpts<Trait>(po, link);

    po.m_isSpaceBefore = false;
    po.m_lastText = nullptr;

    return link;
}

template<class Trait>
inline bool
Parser<Trait>::createShortcutLink(const typename MdBlock<Trait>::Data &text,
                                  TextParsingOpts<Trait> &po,
                                  long long int startLine,
                                  long long int startPos,
                                  long long int lastLineForText,
                                  long long int lastPosForText,
                                  typename Delims::const_iterator lastIt,
                                  const typename MdBlock<Trait>::Data &linkText,
                                  bool doNotCreateTextOnFail,
                                  const WithPosition &textPos,
                                  const WithPosition &linkTextPos)
{
    const auto u = Trait::latin1ToString("#") + toSingleLine(text).simplified().toCaseFolded().toUpper();
    const auto url = u + Trait::latin1ToString("/") + (po.m_workingPath.isEmpty() ?
        typename Trait::String() : po.m_workingPath + Trait::latin1ToString("/")) + po.m_fileName;

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    if (po.m_doc->labeledLinks().find(url) != po.m_doc->labeledLinks().cend()) {
        if (!po.m_collectRefLinks) {
            const auto isLinkTextEmpty = toSingleLine(linkText).simplified().isEmpty();

            const auto link = makeLink(u,
                                       removeBackslashes<Trait>(isLinkTextEmpty ? text : linkText),
                                       po,
                                       doNotCreateTextOnFail,
                                       startLine,
                                       startPos,
                                       lastIt->m_line,
                                       lastIt->m_pos + lastIt->m_len,
                                       (isLinkTextEmpty ? textPos : linkTextPos),
                                       textPos);

            if (link.get()) {
                po.m_linksToParse.push_back(url);
                po.m_parent->appendItem(link);

                po.m_line = lastIt->m_line;
                po.m_pos = lastIt->m_pos + lastIt->m_len;
            } else {
                if (!po.m_collectRefLinks && !doNotCreateTextOnFail) {
                    makeText(lastLineForText, lastPosForText, po);
                }

                return false;
            }
        }

        return true;
    } else if (!po.m_collectRefLinks && !doNotCreateTextOnFail) {
        makeText(lastLineForText, lastPosForText, po);
    }

    return false;
}

template<class Trait>
inline std::shared_ptr<Image<Trait>>
Parser<Trait>::makeImage(const typename Trait::String &url,
                         const typename MdBlock<Trait>::Data &text,
                         TextParsingOpts<Trait> &po,
                         bool doNotCreateTextOnFail,
                         long long int startLine,
                         long long int startPos,
                         long long int lastLine,
                         long long int lastPos,
                         const WithPosition &textPos,
                         const WithPosition &urlPos)
{
    MD_UNUSED(doNotCreateTextOnFail)

    std::shared_ptr<Image<Trait>> img(new Image<Trait>);

    typename Trait::String u = (url.startsWith(Trait::latin1ToString("#")) ? url :
        removeBackslashes<typename Trait::String, Trait>(replaceEntity<Trait>(url)));

    if (Trait::fileExists(u)) {
        img->setUrl(u);
    } else if (Trait::fileExists(u, po.m_workingPath)) {
        img->setUrl(po.m_workingPath + Trait::latin1ToString("/") + u);
    } else {
        img->setUrl(u);
    }

    MdBlock<Trait> block = {text, 0};

    std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);

    RawHtmlBlock<Trait> html;

    parseFormattedTextLinksImages(block,
                                  std::static_pointer_cast<Block<Trait>>(p),
                                  po.m_doc,
                                  po.m_linksToParse,
                                  po.m_workingPath,
                                  po.m_fileName,
                                  po.m_collectRefLinks,
                                  true,
                                  html,
                                  true);

    if (!p->isEmpty()) {
        if (p->items().size() == 1 && p->items().at(0)->type() == ItemType::Paragraph) {
            img->setP(std::static_pointer_cast<Paragraph<Trait>>(p->items().at(0)));
        }
    }

    img->setText(toSingleLine(removeBackslashes<Trait>(text)).simplified());
    img->setStartColumn(po.m_fr.m_data.at(startLine).first.virginPos(startPos));
    img->setStartLine(po.m_fr.m_data.at(startLine).second.m_lineNumber);
    img->setEndColumn(po.m_fr.m_data.at(lastLine).first.virginPos(lastPos - 1));
    img->setEndLine(po.m_fr.m_data.at(lastLine).second.m_lineNumber);
    img->setTextPos(textPos);
    img->setUrlPos(urlPos);

    initLastItemWithOpts<Trait>(po, img);

    po.m_isSpaceBefore = false;
    po.m_lastText = nullptr;

    return img;
}

template<class Trait>
inline bool
Parser<Trait>::createShortcutImage(const typename MdBlock<Trait>::Data &text,
                                   TextParsingOpts<Trait> &po,
                                   long long int startLine,
                                   long long int startPos,
                                   long long int lastLineForText,
                                   long long int lastPosForText,
                                   typename Delims::const_iterator lastIt,
                                   const typename MdBlock<Trait>::Data &linkText,
                                   bool doNotCreateTextOnFail,
                                   const WithPosition &textPos,
                                   const WithPosition &linkTextPos)
{
    const auto url = Trait::latin1ToString("#") + toSingleLine(text).simplified().toCaseFolded().toUpper() +
        Trait::latin1ToString("/") + (po.m_workingPath.isEmpty() ? typename Trait::String() :
            po.m_workingPath + Trait::latin1ToString("/")) + po.m_fileName;

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    const auto iit = po.m_doc->labeledLinks().find(url);

    if (iit != po.m_doc->labeledLinks().cend()) {
        if (!po.m_collectRefLinks) {
            const auto isLinkTextEmpty = toSingleLine(linkText).simplified().isEmpty();

            const auto img = makeImage(iit->second->url(),
                                       (isLinkTextEmpty ? text : linkText),
                                       po,
                                       doNotCreateTextOnFail,
                                       startLine,
                                       startPos,
                                       lastIt->m_line,
                                       lastIt->m_pos + lastIt->m_len,
                                       (isLinkTextEmpty ? textPos : linkTextPos),
                                       textPos);

            po.m_parent->appendItem(img);

            po.m_line = lastIt->m_line;
            po.m_pos = lastIt->m_pos + lastIt->m_len;
        }

        return true;
    } else if (!po.m_collectRefLinks && !doNotCreateTextOnFail) {
        makeText(lastLineForText, lastPosForText, po);
    }

    return false;
}

template<class Trait>
inline void
skipSpacesUpTo1Line(long long int &line,
                    long long int &pos,
                    const typename MdBlock<Trait>::Data &fr)
{
    pos = skipSpaces<Trait>(pos, fr.at(line).first.asString());

    if (pos == fr.at(line).first.length() && line + 1 < (long long int)fr.size()) {
        ++line;
        pos = skipSpaces<Trait>(0, fr.at(line).first.asString());
    }
}

template<class Trait>
inline std::tuple<long long int, long long int, bool, typename Trait::String, long long int>
readLinkDestination(long long int line,
                    long long int pos,
                    const TextParsingOpts<Trait> &po,
                    WithPosition *urlPos = nullptr)
{
    skipSpacesUpTo1Line<Trait>(line, pos, po.m_fr.m_data);

    const auto destLine = line;
    const auto &s = po.m_fr.m_data.at(line).first.asString();
    bool backslash = false;

    if (pos < s.length() && line <= po.m_lastTextLine) {
        if (s[pos] == Trait::latin1ToChar('<')) {
            ++pos;

            if (urlPos) {
                urlPos->setStartColumn(po.m_fr.m_data[line].first.virginPos(pos));
                urlPos->setStartLine(po.m_fr.m_data[line].second.m_lineNumber);
            }

            const auto start = pos;

            while (pos < s.size()) {
                bool now = false;

                if (s[pos] == Trait::latin1ToChar('\\') && !backslash) {
                    backslash = true;
                    now = true;
                } else if (!backslash && s[pos] == Trait::latin1ToChar('<')) {
                    return {line, pos, false, {}, destLine};
                } else if (!backslash && s[pos] == Trait::latin1ToChar('>')) {
                    break;
                }

                if (!now) {
                    backslash = false;
                }

                ++pos;
            }

            if (pos < s.size() && s[pos] == Trait::latin1ToChar('>')) {
                if (urlPos) {
                    urlPos->setEndColumn(po.m_fr.m_data[line].first.virginPos(pos - 1));
                    urlPos->setEndLine(po.m_fr.m_data[line].second.m_lineNumber);
                }

                ++pos;

                return {line, pos, true, s.sliced(start, pos - start - 1), destLine};
            } else {
                return {line, pos, false, {}, destLine};
            }
        } else {
            long long int pc = 0;

            const auto start = pos;

            if (urlPos) {
                urlPos->setStartColumn(po.m_fr.m_data[line].first.virginPos(pos));
                urlPos->setStartLine(po.m_fr.m_data[line].second.m_lineNumber);
            }

            while (pos < s.size()) {
                bool now = false;

                if (s[pos] == Trait::latin1ToChar('\\') && !backslash) {
                    backslash = true;
                    now = true;
                } else if (!backslash && s[pos] == Trait::latin1ToChar(' ')) {
                    if (!pc) {
                        if (urlPos) {
                            urlPos->setEndColumn(po.m_fr.m_data[line].first.virginPos(pos - 1));
                            urlPos->setEndLine(po.m_fr.m_data[line].second.m_lineNumber);
                        }

                        return {line, pos, true, s.sliced(start, pos - start), destLine};
                    } else {
                        return {line, pos, false, {}, destLine};
                    }
                } else if (!backslash && s[pos] == Trait::latin1ToChar('(')) {
                    ++pc;
                } else if (!backslash && s[pos] == Trait::latin1ToChar(')')) {
                    if (!pc) {
                        if (urlPos) {
                            urlPos->setEndColumn(po.m_fr.m_data[line].first.virginPos(pos - 1));
                            urlPos->setEndLine(po.m_fr.m_data[line].second.m_lineNumber);
                        }

                        return {line, pos, true, s.sliced(start, pos - start), destLine};
                    } else {
                        --pc;
                    }
                }

                if (!now) {
                    backslash = false;
                }

                ++pos;
            }

            if (urlPos) {
                urlPos->setEndColumn(po.m_fr.m_data[line].first.virginPos(pos - 1));
                urlPos->setEndLine(po.m_fr.m_data[line].second.m_lineNumber);
            }

            return {line, pos, true, s.sliced(start, pos - start), destLine};
        }
    } else {
        return {line, pos, false, {}, destLine};
    }
}

template<class Trait>
inline std::tuple<long long int, long long int, bool, typename Trait::String, long long int>
readLinkTitle(long long int line,
              long long int pos,
              const TextParsingOpts<Trait> &po)
{
    const auto space = (pos < po.m_fr.m_data.at(line).first.length() ?
        po.m_fr.m_data.at(line).first[pos].isSpace() : true);

    const auto firstLine = line;

    skipSpacesUpTo1Line<Trait>(line, pos, po.m_fr.m_data);

    if (pos >= po.m_fr.m_data.at(line).first.length()) {
        return {line, pos, true, {}, firstLine};
    }

    const auto sc = po.m_fr.m_data.at(line).first[pos];

    if (sc != Trait::latin1ToChar('"') && sc != Trait::latin1ToChar('\'') &&
        sc != Trait::latin1ToChar('(') && sc != Trait::latin1ToChar(')')) {
        return {line, pos, (firstLine != line && line <= po.m_lastTextLine), {}, firstLine};
    } else if (!space && sc != Trait::latin1ToChar(')')) {
        return {line, pos, false, {}, firstLine};
    }

    if (sc == Trait::latin1ToChar(')')) {
        return {line, pos, line <= po.m_lastTextLine, {}, firstLine};
    }

    const auto startLine = line;

    bool backslash = false;

    ++pos;

    skipSpacesUpTo1Line<Trait>(line, pos, po.m_fr.m_data);

    typename Trait::String title;

    while (line < (long long int)po.m_fr.m_data.size() && pos < po.m_fr.m_data.at(line).first.length()) {
        bool now = false;

        if (po.m_fr.m_data.at(line).first[pos] == Trait::latin1ToChar('\\') && !backslash) {
            backslash = true;
            now = true;
        } else if (sc == Trait::latin1ToChar('(') &&
            po.m_fr.m_data.at(line).first[pos] == Trait::latin1ToChar(')') && !backslash) {
            ++pos;
            return {line, pos, line <= po.m_lastTextLine, title, startLine};
        } else if (sc == Trait::latin1ToChar('(') &&
            po.m_fr.m_data.at(line).first[pos] == Trait::latin1ToChar('(') && !backslash) {
            return {line, pos, false, {}, startLine};
        } else if (sc != Trait::latin1ToChar('(') && po.m_fr.m_data.at(line).first[pos] == sc && !backslash) {
            ++pos;
            return {line, pos, line <= po.m_lastTextLine, title, startLine};
        } else {
            title.push_back(po.m_fr.m_data.at(line).first[pos]);
        }

        if (!now) {
            backslash = false;
        }

        ++pos;

        if (pos == po.m_fr.m_data.at(line).first.length()) {
            skipSpacesUpTo1Line<Trait>(line, pos, po.m_fr.m_data);
        }
    }

    return {line, pos, false, {}, startLine};
}

template<class Trait>
inline std::tuple<typename Trait::String, typename Trait::String, typename Parser<Trait>::Delims::const_iterator, bool>
Parser<Trait>::checkForInlineLink(typename Delims::const_iterator it,
                                  typename Delims::const_iterator last,
                                  TextParsingOpts<Trait> &po,
                                  WithPosition *urlPos)
{
    long long int p = it->m_pos + it->m_len;
    long long int l = it->m_line;
    bool ok = false;
    typename Trait::String dest, title;
    long long int destStartLine = 0;

    std::tie(l, p, ok, dest, destStartLine) = readLinkDestination<Trait>(l, p, po, urlPos);

    if (!ok) {
        return {{}, {}, it, false};
    }

    long long int s = 0;

    std::tie(l, p, ok, title, s) = readLinkTitle<Trait>(l, p, po);

    skipSpacesUpTo1Line<Trait>(l, p, po.m_fr.m_data);

    if (!ok || (l >= (long long int)po.m_fr.m_data.size() || p >= po.m_fr.m_data.at(l).first.length() ||
        po.m_fr.m_data.at(l).first[p] != Trait::latin1ToChar(')'))) {
        return {{}, {}, it, false};
    }

    for (; it != last; ++it) {
        if (it->m_line == l && it->m_pos == p) {
            return {dest, title, it, true};
        }
    }

    return {{}, {}, it, false};
}

template<class Trait>
inline std::tuple<typename Trait::String, typename Trait::String, typename Parser<Trait>::Delims::const_iterator, bool>
Parser<Trait>::checkForRefLink(typename Delims::const_iterator it,
                               typename Delims::const_iterator last,
                               TextParsingOpts<Trait> &po,
                               WithPosition *urlPos)
{
    long long int p = it->m_pos + it->m_len + 1;
    long long int l = it->m_line;
    bool ok = false;
    typename Trait::String dest, title;
    long long int destStartLine = 0;

    std::tie(l, p, ok, dest, destStartLine) = readLinkDestination<Trait>(l, p, po, urlPos);

    if (!ok) {
        return {{}, {}, it, false};
    }

    long long int titleStartLine = 0;

    std::tie(l, p, ok, title, titleStartLine) = readLinkTitle<Trait>(l, p, po);

    if (!ok) {
        return {{}, {}, it, false};
    }

    if (!title.isEmpty()) {
        p = skipSpaces<Trait>(p, po.m_fr.m_data.at(l).first.asString());

        if (titleStartLine == destStartLine && p < po.m_fr.m_data.at(l).first.length()) {
            return {{}, {}, it, false};
        } else if (titleStartLine != destStartLine && p < po.m_fr.m_data.at(l).first.length()) {
            l = destStartLine;
            p = po.m_fr.m_data.at(l).first.length();
            title.clear();
        }
    }

    for (; it != last; ++it) {
        if (it->m_line > l || (it->m_line == l && it->m_pos >= p)) {
            break;
        }
    }

    po.m_line = l;
    po.m_pos = p;

    return {dest, title, std::prev(it), true};
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForImage(typename Delims::const_iterator it,
                             typename Delims::const_iterator last,
                             TextParsingOpts<Trait> &po)
{
    const auto start = it;

    typename MdBlock<Trait>::Data text;

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    WithPosition textPos;
    std::tie(text, it) = checkForLinkText(it, last, po, &textPos);

    if (it != start) {
        if (it->m_pos + it->m_len < po.m_fr.m_data.at(it->m_line).first.length()) {
            // Inline -> (
            if (po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len] == Trait::latin1ToChar('(')) {
                typename Trait::String url, title;
                typename Delims::const_iterator iit;
                bool ok;

                WithPosition urlPos;
                std::tie(url, title, iit, ok) = checkForInlineLink(std::next(it), last, po, &urlPos);

                if (ok) {
                    if (!po.m_collectRefLinks) {
                        po.m_parent->appendItem(
                            makeImage(url, text, po, false, start->m_line, start->m_pos,
                                iit->m_line, iit->m_pos + iit->m_len, textPos, urlPos));
                    }

                    po.m_line = iit->m_line;
                    po.m_pos = iit->m_pos + iit->m_len;

                    return iit;
                } else if (createShortcutImage(text, po, start->m_line, start->m_pos, start->m_line,
                    start->m_pos + start->m_len, it, {}, false, textPos, {})) {
                    return it;
                }
            }
            // Reference -> [
            else if (po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len] == Trait::latin1ToChar('[')) {
                typename MdBlock<Trait>::Data label;
                typename Delims::const_iterator lit;

                WithPosition labelPos;
                std::tie(label, lit) = checkForLinkLabel(std::next(it), last, po, &labelPos);

                if (lit != std::next(it)) {
                    const auto isLabelEmpty = toSingleLine(label).simplified().isEmpty();

                    if (!isLabelEmpty
                        && createShortcutImage(label,
                                               po,
                                               start->m_line,
                                               start->m_pos,
                                               start->m_line,
                                               start->m_pos + start->m_len,
                                               lit,
                                               text,
                                               true,
                                               labelPos,
                                               textPos)) {
                        return lit;
                    } else if (isLabelEmpty
                               && createShortcutImage(text,
                                                      po,
                                                      start->m_line,
                                                      start->m_pos,
                                                      start->m_line,
                                                      start->m_pos + start->m_len,
                                                      lit,
                                                      {},
                                                      false,
                                                      textPos,
                                                      {})) {
                        return lit;
                    }
                } else if (createShortcutImage(text, po, start->m_line, start->m_pos, start->m_line,
                    start->m_pos + start->m_len, it, {}, false, textPos, {})) {
                    return it;
                }
            } else {
                return checkShortcut(start, last, po, &Parser<Trait>::createShortcutImage);
            }
        } else {
            return checkShortcut(start, last, po, &Parser<Trait>::createShortcutImage);
        }
    }

    return start;
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForLink(typename Delims::const_iterator it,
                            typename Delims::const_iterator last,
                            TextParsingOpts<Trait> &po)
{
    const auto start = it;

    typename MdBlock<Trait>::Data text;

    const auto wasRefLink = po.m_wasRefLink;
    const auto firstInParagraph = po.m_firstInParagraph;
    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    const auto ns = skipSpaces<Trait>(0, po.m_fr.m_data.at(po.m_line).first.asString());

    WithPosition textPos;
    std::tie(text, it) = checkForLinkText(it, last, po, &textPos);

    if (it != start) {
        // Footnote reference.
        if (text.front().first.asString().startsWith(Trait::latin1ToString("^")) &&
            text.front().first.asString().simplified().length() > 1 && text.size() == 1 &&
            start->m_line == it->m_line) {
            if (!po.m_collectRefLinks) {
                std::shared_ptr<FootnoteRef<Trait>> fnr(new FootnoteRef<Trait>(
                    Trait::latin1ToString("#") + toSingleLine(text).simplified().toCaseFolded().toUpper() +
                    Trait::latin1ToString("/") + (po.m_workingPath.isEmpty() ? typename Trait::String() :
                        po.m_workingPath + Trait::latin1ToString("/")) + po.m_fileName));
                fnr->setStartColumn(po.m_fr.m_data.at(start->m_line).first.virginPos(start->m_pos));
                fnr->setStartLine(po.m_fr.m_data.at(start->m_line).second.m_lineNumber);
                fnr->setEndColumn(po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                fnr->setEndLine(po.m_fr.m_data.at(it->m_line).second.m_lineNumber);
                fnr->setIdPos(textPos);

                typename Trait::String fnrText = Trait::latin1ToString("[");
                bool firstFnrText = true;

                for (const auto &t : text) {
                    if (!firstFnrText) {
                        fnrText.push_back(Trait::latin1ToString("\n"));
                    }

                    firstFnrText = false;

                    fnrText.push_back(t.first.asString());
                }

                fnrText.push_back(Trait::latin1ToString("]"));

                fnr->setText(fnrText);
                fnr->setSpaceBefore(start->m_pos > 0 ?
                    po.m_fr.m_data.at(start->m_line).first[start->m_pos - 1].isSpace() : true);
                fnr->setSpaceAfter(it->m_pos + it->m_len < po.m_fr.m_data.at(it->m_line).first.length() ?
                    po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len].isSpace() : true);

                po.m_parent->appendItem(fnr);

                initLastItemWithOpts<Trait>(po, fnr);
            }

            po.m_line = it->m_line;
            po.m_pos = it->m_pos + it->m_len;

            return it;
        } else if (it->m_pos + it->m_len < po.m_fr.m_data.at(it->m_line).first.length()) {
            // Reference definition -> :
            if (po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len] == Trait::latin1ToChar(':')) {
                // Reference definitions allowed only at start of paragraph.
                if ((po.m_line == 0 || wasRefLink || firstInParagraph) && ns < 4 && start->m_pos == ns) {
                    typename Trait::String url, title;
                    typename Delims::const_iterator iit;
                    bool ok;

                    WithPosition labelPos;

                    std::tie(text, it) = checkForLinkLabel(start, last, po, &labelPos);

                    if (it != start && !toSingleLine(text).simplified().isEmpty()) {
                        WithPosition urlPos;
                        std::tie(url, title, iit, ok) = checkForRefLink(it, last, po, &urlPos);

                        if (ok) {
                            const auto label = Trait::latin1ToString("#") +
                                toSingleLine(text).simplified().toCaseFolded().toUpper() +
                                Trait::latin1ToString("/") +
                                (po.m_workingPath.isEmpty() ? typename Trait::String() :
                                    po.m_workingPath + Trait::latin1ToString("/")) + po.m_fileName;

                            std::shared_ptr<Link<Trait>> link(new Link<Trait>);
                            link->setStartColumn(po.m_fr.m_data.at(start->m_line).first.virginPos(
                                start->m_pos));
                            link->setStartLine(po.m_fr.m_data.at(start->m_line).second.m_lineNumber);

                            const auto endPos = prevPosition(po.m_fr,
                                po.m_fr.m_data.at(po.m_line).first.virginPos(po.m_pos),
                                po.m_fr.m_data.at(po.m_line).second.m_lineNumber);

                            link->setEndColumn(endPos.first);
                            link->setEndLine(endPos.second);

                            link->setTextPos(labelPos);
                            link->setUrlPos(urlPos);

                            url = removeBackslashes<typename Trait::String, Trait>(
								replaceEntity<Trait>(url));

                            if (!url.isEmpty()) {
                                if (Trait::fileExists(url)) {
                                    url = Trait::absoluteFilePath(url);
                                } else if (Trait::fileExists(url, po.m_workingPath)) {
                                    url = Trait::absoluteFilePath(
                                        (po.m_workingPath.isEmpty() ? typename Trait::String() :
                                            po.m_workingPath + Trait::latin1ToString("/")) + url);
                                }
                            }

                            link->setUrl(url);

                            po.m_wasRefLink = true;

                            if (po.m_doc->labeledLinks().find(label) == po.m_doc->labeledLinks().cend()) {
                                po.m_doc->insertLabeledLink(label, link);
                            }

                            return iit;
                        } else {
                            return checkShortcut(start, last, po, &Parser<Trait>::createShortcutLink);
                        }
                    } else {
                        return start;
                    }
                } else {
                    return checkShortcut(start, last, po, &Parser<Trait>::createShortcutLink);
                }
            }
            // Inline -> (
            else if (po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len] == Trait::latin1ToChar('(')) {
                typename Trait::String url, title;
                typename Delims::const_iterator iit;
                bool ok;

                WithPosition urlPos;
                std::tie(url, title, iit, ok) = checkForInlineLink(std::next(it), last, po, &urlPos);

                if (ok) {
                    const auto link = makeLink(url,
                                               removeBackslashes<Trait>(text),
                                               po,
                                               false,
                                               start->m_line,
                                               start->m_pos,
                                               iit->m_line,
                                               iit->m_pos + iit->m_len,
                                               textPos,
                                               urlPos);

                    if (link.get()) {
                        if (!po.m_collectRefLinks) {
                            po.m_parent->appendItem(link);
                        }

                        po.m_line = iit->m_line;
                        po.m_pos = iit->m_pos + iit->m_len;

                        return iit;
                    } else {
                        return start;
                    }
                } else if (createShortcutLink(text, po, start->m_line, start->m_pos, start->m_line,
                    start->m_pos + start->m_len, it, {}, false, textPos, {})) {
                    return it;
                }
            }
            // Reference -> [
            else if (po.m_fr.m_data.at(it->m_line).first[it->m_pos + it->m_len] == Trait::latin1ToChar('[')) {
                typename MdBlock<Trait>::Data label;
                typename Delims::const_iterator lit;

                WithPosition labelPos;
                std::tie(label, lit) = checkForLinkLabel(std::next(it), last, po, &labelPos);

                const auto isLabelEmpty = toSingleLine(label).simplified().isEmpty();

                if (lit != std::next(it)) {
                    if (!isLabelEmpty
                        && createShortcutLink(label,
                                              po,
                                              start->m_line,
                                              start->m_pos,
                                              start->m_line,
                                              start->m_pos + start->m_len,
                                              lit,
                                              text,
                                              true,
                                              labelPos,
                                              textPos)) {
                        return lit;
                    } else if (isLabelEmpty
                               && createShortcutLink(text,
                                                     po,
                                                     start->m_line,
                                                     start->m_pos,
                                                     start->m_line,
                                                     start->m_pos + start->m_len,
                                                     lit,
                                                     {},
                                                     false,
                                                     textPos,
                                                     {})) {
                        return lit;
                    }
                } else if (createShortcutLink(text, po, start->m_line, start->m_pos, start->m_line,
                    start->m_pos + start->m_len, it, {}, false, textPos, {})) {
                    return it;
                }
            } else {
                return checkShortcut(start, last, po, &Parser<Trait>::createShortcutLink);
            }
        } else {
            return checkShortcut(start, last, po, &Parser<Trait>::createShortcutLink);
        }
    }

    return start;
}

inline bool
isClosingStyle(const std::vector<std::pair<Style, long long int>> &styles,
               Style s)
{
    const auto it = std::find_if(styles.cbegin(), styles.cend(), [&](const auto &p) {
        return (p.first == s);
    });

    return it != styles.cend();
}

inline void
closeStyle(std::vector<std::pair<Style, long long int>> &styles,
           Style s)
{
    const auto it = std::find_if(styles.crbegin(), styles.crend(), [&](const auto &p) {
        return (p.first == s);
    });

    if (it != styles.crend()) {
        styles.erase(it.base() - 1);
    }
}

inline void
applyStyles(int &opts,
            const std::vector<std::pair<Style, long long int>> &styles)
{
    opts = 0;

    for (const auto &s : styles) {
        switch (s.first) {
        case Style::Strikethrough:
            opts |= StrikethroughText;
            break;

        case Style::Italic1:
        case Style::Italic2:
            opts |= ItalicText;
            break;

        case Style::Bold1:
        case Style::Bold2:
            opts |= BoldText;
            break;

        default:
            break;
        }
    }
}

inline void
appendPossibleDelimiter(std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> &vars,
                        long long int len,
                        int type,
                        bool leftAndRight)
{
    for (auto &v : vars) {
        v.push_back({{len, leftAndRight}, type});
    }
}

inline std::vector<std::pair<std::pair<long long int, bool>, int>>
longestSequenceWithMoreOpeningsAtStart(const std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> &vars)
{
    size_t max = 0;

    for (const auto &s : vars) {
        if (s.size() > max) {
            max = s.size();
        }
    }

    std::vector<std::pair<std::pair<long long int, bool>, int>> ret;

    size_t maxOp = 0;

    for (const auto &s : vars) {
        if (s.size() == max) {
            size_t op = 0;

            for (const auto &v : s) {
                if (v.first.first > 0) {
                    ++op;
                } else {
                    break;
                }
            }

            if (op > maxOp) {
                maxOp = op;
                ret = s;
            }
        }
    }

    return ret;
}

inline void
collectDelimiterVariants(std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> &vars,
                         long long int itLength,
                         int type,
                         bool leftFlanking,
                         bool rightFlanking)
{
    {
        auto vars1 = vars;
        auto vars2 = vars;

        vars.clear();

        if (leftFlanking) {
            appendPossibleDelimiter(vars1, itLength, type, leftFlanking && rightFlanking);
            std::copy(vars1.cbegin(), vars1.cend(), std::back_inserter(vars));
        }

        if (rightFlanking) {
            appendPossibleDelimiter(vars2, -itLength, type, leftFlanking && rightFlanking);
            std::copy(vars2.cbegin(), vars2.cend(), std::back_inserter(vars));
        }
    }
}

template<class Trait>
inline void
Parser<Trait>::createStyles(std::vector<std::pair<Style, long long int>> &s,
                            long long int l,
                            typename Delimiter::DelimiterType t,
                            long long int &count)
{
    if (t != Delimiter::Strikethrough) {
        if (l % 2 == 1) {
            s.push_back({t == Delimiter::Emphasis1 ? Style::Italic1 : Style::Italic2, 1});
            ++count;
        }

        if (l >= 2) {
            for (long long int i = 0; i < l / 2; ++i) {
                s.push_back({t == Delimiter::Emphasis1 ? Style::Bold1 : Style::Bold2, 2});
                count += 2;
            }
        }
    } else {
        s.push_back({Style::Strikethrough, l});
        ++count;
    }
}

template<class Trait>
inline bool
Parser<Trait>::isSequence(typename Delims::const_iterator it,
                          long long int itLine,
                          long long int itPos,
                          typename Delimiter::DelimiterType t)
{
    return (itLine == it->m_line && itPos + it->m_len == it->m_pos && it->m_type == t);
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::readSequence(typename Delims::const_iterator it,
                            typename Delims::const_iterator last,
                            long long int &line,
                            long long int &pos,
                            long long int &len,
                            typename Delims::const_iterator &current)
{
    line = it->m_line;
    pos = it->m_pos;
    len = it->m_len;
    current = it;
    const auto t = it->m_type;

    it = std::next(it);

    while (it != last && isSequence(it, line, pos, t)) {
        current = it;

        pos += it->m_len;
        len += it->m_len;

        ++it;
    }

    return std::prev(it);
}

template<class Trait>
inline int
Parser<Trait>::emphasisToInt(typename Delimiter::DelimiterType t)
{
    switch (t) {
    case Delimiter::Strikethrough:
        return 0;

    case Delimiter::Emphasis1:
        return 1;

    case Delimiter::Emphasis2:
        return 2;

    default:
        return -1;
    }
}

template<class Trait>
inline std::pair<bool, size_t>
Parser<Trait>::checkEmphasisSequence(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
                                     size_t idx)
{
    static const auto strikeType = emphasisToInt(Delimiter::Strikethrough);

    if (s[idx].second == strikeType) {
        if (s[idx].first.first > 0) {
            const auto len = s[idx].first.first;

            const auto it = std::find_if(s.cbegin() + idx + 1, s.cend(), [len](const auto &p) {
                if (p.first.first == -len && p.second == strikeType) {
                    return true;
                } else {
                    return false;
                }
            });

            if (it != s.cend()) {
                return {true, std::distance(s.cbegin(), it)};
            } else {
                return {false, 0};
            }
        } else {
            return {false, 0};
        }
    }

    std::vector<std::pair<std::pair<long long int, bool>, int>> st;

    size_t i = 0;

    for (; i <= idx; ++i) {
        st.push_back(s[i]);
    }

    for (; i < s.size(); ++i) {
        if (s[i].first.first < 0) {
            if (checkStack(st, s[i], idx)) {
                return {true, i};
            } else if (st.size() <= idx) {
                return {false, 0};
            }
        } else {
            st.push_back(s[i]);
        }
    }

    return {false, 0};
}

template<class Trait>
inline std::vector<std::pair<std::pair<long long int, bool>, int>>
Parser<Trait>::fixSequence(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s)
{
    std::vector<std::pair<std::pair<long long int, bool>, int>> tmp;
    std::map<int, long long int> length;

    for (const auto &p : s) {
        if (p.first.first < 0 && length[p.second] + p.first.first < 0) {
            tmp.push_back({{-length[p.second], p.first.second}, p.second});

            length[p.second] = 0;
        } else {
            tmp.push_back(p);

            length[p.second] += p.first.first;
        }
    }

    return tmp;
}

template<class Trait>
inline std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>>
Parser<Trait>::closedSequences(const std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> &vars,
                               size_t idx)
{
    std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> tmp;

    const auto longest = longestSequenceWithMoreOpeningsAtStart(vars);

    for (const auto &v : vars) {
        if (longest.size() == v.size()) {
            bool closed = false;
            std::tie(closed, std::ignore) = checkEmphasisSequence(v, idx);

            if (closed) {
                tmp.push_back(fixSequence(v));
            }
        }
    }

    return tmp;
}

template<class Trait>
inline std::vector<std::pair<Style, long long int>>
Parser<Trait>::createStyles(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
                            size_t i,
                            typename Delimiter::DelimiterType t,
                            long long int &count)
{
    std::vector<std::pair<Style, long long int>> styles;

    const size_t idx = i;
    long long int len = s[i].first.first;

    size_t closeIdx = 0;
    std::tie(std::ignore, closeIdx) = checkEmphasisSequence(s, i);

    for (i = closeIdx;; --i) {
        if (s[i].second == s[idx].second && s[i].first.first < 0) {
            auto l = std::abs(s[i].first.first);

            createStyles(styles, std::min(l, len), t, count);

            len -= std::min(l, len);

            if (!len) {
                break;
            }
        }

        if (i == 0) {
            break;
        }
    }

    return styles;
}

template<class Trait>
inline bool
isSkipAllEmphasis(const std::vector<std::pair<std::pair<long long int, bool>, int>> &s,
                  size_t idx)
{
    if (s[idx].first.second) {
        for (size_t i = idx + 1; i < s.size(); ++i) {
            if (s[i].second == s[idx].second && s[i].first.first < 0) {
                return ((s[idx].first.first - s[i].first.first) % 3 == 0 &&
                    !(s[idx].first.first % 3 == 0 && s[i].first.first % 3 == 0));
            }
        }
    }

    return false;
}

template<class Trait>
inline std::tuple<bool, std::vector<std::pair<Style, long long int>>, long long int, long long int>
Parser<Trait>::isStyleClosed(typename Delims::const_iterator it,
                             typename Delims::const_iterator last,
                             TextParsingOpts<Trait> &po)
{
    const auto open = it;
    auto current = it;

    std::vector<std::vector<std::pair<std::pair<long long int, bool>, int>>> vars, closed;
    vars.push_back({});

    long long int itLine = open->m_line, itPos = open->m_pos, itLength = open->m_len;

    const long long int line = po.m_line, pos = po.m_pos;
    const bool collectRefLinks = po.m_collectRefLinks;

    po.m_collectRefLinks = true;

    bool first = true;

    std::for_each(po.m_styles.cbegin(), po.m_styles.cend(), [&vars](const auto &p) {
        if (p.first == Style::Strikethrough) {
            vars.front().push_back({{p.second, false}, 0});
        }
    });

    {
        {
            const auto c1 = std::count_if(po.m_styles.cbegin(), po.m_styles.cend(), [&](const auto &p) {
                return (p.first == Style::Italic1);
            });

            if (c1) {
                vars.front().push_back({{c1, false}, 1});
            }

            const auto c2 = std::count_if(po.m_styles.cbegin(),
                                          po.m_styles.cend(),
                                          [&](const auto &p) {
                                              return (p.first == Style::Bold1);
                                          }) * 2;

            if (c2) {
                vars.front().push_back({{c2, false}, 1});
            }
        }

        {
            const auto c1 = std::count_if(po.m_styles.cbegin(), po.m_styles.cend(), [&](const auto &p) {
                return (p.first == Style::Italic2);
            });

            if (c1) {
                vars.front().push_back({{c1, false}, 2});
            }

            const auto c2 = std::count_if(po.m_styles.cbegin(),
                                          po.m_styles.cend(),
                                          [&](const auto &p) {
                                              return (p.first == Style::Bold2);
                                          }) * 2;

            if (c2) {
                vars.front().push_back({{c2, false}, 2});
            }
        }
    }

    const auto idx = vars.front().size();

    for (; it != last; ++it) {
        if (it->m_line <= po.m_lastTextLine) {
            switch (it->m_type) {
            case Delimiter::SquareBracketsOpen:
                it = checkForLink(it, last, po);
                break;

            case Delimiter::ImageOpen:
                it = checkForImage(it, last, po);
                break;

            case Delimiter::Less:
                it = checkForAutolinkHtml(it, last, po, false);
                break;

            case Delimiter::Strikethrough:
            case Delimiter::Emphasis1:
            case Delimiter::Emphasis2: {
                it = readSequence(it, last, itLine, itPos, itLength, current);

                if (first) {
                    vars.front().push_back({{itLength, it->m_leftFlanking && it->m_rightFlanking},
                        emphasisToInt(open->m_type)});
                    first = false;
                } else {
                    collectDelimiterVariants(vars, itLength, emphasisToInt(it->m_type),
                        it->m_leftFlanking, it->m_rightFlanking);
                }
            } break;

            case Delimiter::InlineCode:
                it = checkForInlineCode(it, last, po);
                break;

            default:
                break;
            }
        } else {
            break;
        }
    }

    po.m_line = line;
    po.m_pos = pos;
    po.m_collectRefLinks = collectRefLinks;

    closed = closedSequences(vars, idx);

    if (!closed.empty()) {
        long long int itCount = 0;

        return {true, createStyles(longestSequenceWithMoreOpeningsAtStart(closed), idx,
            open->m_type, itCount), vars.front().at(idx).first.first, itCount};
    } else {
        return {false, {{Style::Unknown, 0}}, isSkipAllEmphasis<Trait>(vars.front(), idx) ?
            vars.front().at(idx).first.first : open->m_len, 1};
    }
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::incrementIterator(typename Delims::const_iterator it,
                                 typename Delims::const_iterator last,
                                 long long int count)
{
    const auto len = std::distance(it, last);

    if (count < len) {
        return it + count;
    } else {
        return it + (len - 1);
    }
}

template<class Trait>
inline void
appendCloseStyle(TextParsingOpts<Trait> &po,
                 const StyleDelim &s)
{
    if (po.m_lastItemWithStyle) {
        po.m_lastItemWithStyle->closeStyles().push_back(s);
    }
}

template<class Trait>
inline typename Parser<Trait>::Delims::const_iterator
Parser<Trait>::checkForStyle(typename Delims::const_iterator first,
                             typename Delims::const_iterator it,
                             typename Delims::const_iterator last,
                             TextParsingOpts<Trait> &po)
{
    long long int count = 1;

    po.m_wasRefLink = false;
    po.m_firstInParagraph = false;

    if (it->m_rightFlanking) {
        long long int line = it->m_line, pos = it->m_pos + it->m_len, ppos = it->m_pos;
        const auto t = it->m_type;
        long long int len = it->m_len;

        for (auto j = std::next(it); j != last; ++j) {
            if (j->m_line == line && pos == j->m_pos && j->m_type == t) {
                len += j->m_len;
                pos = j->m_pos + j->m_len;
                ++count;
            } else {
                break;
            }
        }

        if (it != first) {
            for (auto j = std::prev(it);; --j) {
                if (j->m_line == line && ppos - j->m_len == j->m_pos && j->m_type == t) {
                    len += j->m_len;
                    ppos = j->m_pos;
                    ++count;
                } else {
                    break;
                }

                if (j == first) {
                    break;
                }
            }
        }

        long long int opened = 0;

        for (auto it = po.m_styles.crbegin(), last = po.m_styles.crend(); it != last; ++it) {
            bool doBreak = false;

            switch (t) {
            case Delimiter::Emphasis1: {
                if (it->first == Style::Italic1 || it->first == Style::Bold1) {
                    opened = it->second;
                    doBreak = true;
                }
            } break;

            case Delimiter::Emphasis2: {
                if (it->first == Style::Italic2 || it->first == Style::Bold2) {
                    opened = it->second;
                    doBreak = true;
                }
            } break;

            case Delimiter::Strikethrough: {
                if (it->first == Style::Strikethrough) {
                    opened = it->second;
                    doBreak = true;
                }
            } break;

            default:
                break;
            }

            if (doBreak)
                break;
        }

        const bool sumMult3 = (it->m_leftFlanking ? ((opened + len) % 3 == 0) : false);

        if (count && opened && (!sumMult3 || (count % 3 == 0 && opened % 3 == 0))) {
            if (count > opened) {
                count = opened;
            }

            auto pos = po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos);
            const auto line = po.m_fr.m_data.at(it->m_line).second.m_lineNumber;

            if (it->m_type == Delimiter::Strikethrough) {
                const auto len = it->m_len;

                for (auto i = 0; i < count; ++i) {
                    closeStyle(po.m_styles, Style::Strikethrough);
                    appendCloseStyle(po, {StrikethroughText, pos, line, pos + len - 1, line});
                    pos += len;
                }
            } else {
                if (count % 2 == 1) {
                    const auto st = (it->m_type == Delimiter::Emphasis1 ? Style::Italic1 : Style::Italic2);

                    closeStyle(po.m_styles, st);
                    appendCloseStyle(po, {ItalicText, pos, line, pos, line});
                    ++pos;
                }

                if (count >= 2) {
                    const auto st = (it->m_type == Delimiter::Emphasis1 ? Style::Bold1 : Style::Bold2);

                    for (auto i = 0; i < count / 2; ++i) {
                        closeStyle(po.m_styles, st);
                        appendCloseStyle(po, {BoldText, pos, line, pos + 1, line});
                        pos += 2;
                    }
                }
            }

            applyStyles(po.m_opts, po.m_styles);

            const auto j = incrementIterator(it, last, count - 1);

            po.m_pos = j->m_pos + j->m_len;
            po.m_line = j->m_line;

            if (po.m_lastText) {
                po.m_lastText->setSpaceAfter(po.m_lastText->isSpaceAfter() ||
                    (po.m_pos < po.m_fr.m_data[po.m_line].first.length() ?
                        po.m_fr.m_data[po.m_line].first[po.m_pos].isSpace() : true));
            }

            return j;
        }
    }

    count = 1;

    if (it->m_leftFlanking) {
        switch (it->m_type) {
        case Delimiter::Strikethrough:
        case Delimiter::Emphasis1:
        case Delimiter::Emphasis2: {
            bool closed = false;
            std::vector<std::pair<Style, long long int>> styles;
            long long int len = 0;

            std::tie(closed, styles, len, count) = isStyleClosed(it, last, po);

            if (closed) {
                auto pos = po.m_fr.m_data.at(it->m_line).first.virginPos(it->m_pos);
                const auto line = po.m_fr.m_data.at(it->m_line).second.m_lineNumber;

                for (const auto &p : styles) {
                    po.m_styles.push_back({p.first, p.second});

                    if (!po.m_collectRefLinks) {
                        po.m_openStyles.push_back({styleToTextOption(p.first), pos, line,
                            pos + p.second - 1, line});
                    }

                    pos += p.second;
                }

                po.m_pos = it->m_pos + len;
                po.m_line = it->m_line;

                po.m_isSpaceBefore = (it->m_pos > 0 ?
                    po.m_fr.m_data[it->m_line].first[it->m_pos - 1].isSpace() : true) ||
                        po.m_isSpaceBefore;

                applyStyles(po.m_opts, po.m_styles);
            } else if (!po.m_collectRefLinks) {
                makeText(it->m_line, it->m_pos + len, po);
            }
        } break;

        default: {
            if (!po.m_collectRefLinks) {
                makeText(it->m_line, it->m_pos + it->m_len, po);
            }
        } break;
        }
    }

    if (!count) {
        count = 1;
    }

    resetHtmlTag(po.m_html);

    return incrementIterator(it, last, count - 1);
}

template<class Trait>
inline std::shared_ptr<Text<Trait>>
concatenateText(typename Block<Trait>::Items::const_iterator it,
                typename Block<Trait>::Items::const_iterator last)
{
    std::shared_ptr<Text<Trait>> t(new Text<Trait>);
    t->setOpts(std::static_pointer_cast<Text<Trait>>(*it)->opts());
    t->setSpaceBefore(std::static_pointer_cast<Text<Trait>>(*it)->isSpaceBefore());
    t->setStartColumn((*it)->startColumn());
    t->setStartLine((*it)->startLine());

    typename ItemWithOpts<Trait>::Styles close;

    typename Trait::String data;

    for (; it != last; ++it) {
        const auto tt = std::static_pointer_cast<Text<Trait>>(*it);

        if (tt->isSpaceBefore()) {
            data.push_back(Trait::latin1ToChar(' '));
        }

        data.push_back(tt->text());

        if (tt->isSpaceAfter()) {
            data.push_back(Trait::latin1ToChar(' '));
        }

        if (!tt->openStyles().empty()) {
            std::copy(tt->openStyles().cbegin(), tt->openStyles().cend(),
                std::back_inserter(t->openStyles()));
        }

        if (!tt->closeStyles().empty()) {
            std::copy(tt->closeStyles().cbegin(), tt->closeStyles().cend(),
                std::back_inserter(close));
        }
    }

    it = std::prev(it);

    t->setText(data.simplified());
    t->setSpaceAfter(std::static_pointer_cast<Text<Trait>>(*it)->isSpaceAfter());
    t->setEndColumn((*it)->endColumn());
    t->setEndLine((*it)->endLine());
    t->closeStyles() = close;

    return t;
}

inline bool
isSemiOptimization(OptimizeParagraphType t)
{
    switch (t) {
    case OptimizeParagraphType::Semi:
    case OptimizeParagraphType::SemiWithoutRawData:
        return true;

    default:
        return false;
    }
}

inline bool
isWithoutRawDataOptimization(OptimizeParagraphType t)
{
    switch (t) {
    case OptimizeParagraphType::FullWithoutRawData:
    case OptimizeParagraphType::SemiWithoutRawData:
        return true;

    default:
        return false;
    }
}

template<class Trait>
inline std::shared_ptr<Paragraph<Trait>>
optimizeParagraph(std::shared_ptr<Paragraph<Trait>> &p,
                  TextParsingOpts<Trait> &po,
                  OptimizeParagraphType type = OptimizeParagraphType::Full)
{
    std::shared_ptr<Paragraph<Trait>> np(new Paragraph<Trait>);
    np->setStartColumn(p->startColumn());
    np->setStartLine(p->startLine());
    np->setEndColumn(p->endColumn());
    np->setEndLine(p->endLine());

    int opts = TextWithoutFormat;
    auto start = p->items().cend();
    long long int line = -1;
    long long int auxStart = 0, auxIt = 0;
    bool finished = false;

    for (auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it) {
        if ((*it)->type() == ItemType::Text) {
            const auto t = std::static_pointer_cast<Text<Trait>>(*it);

            if (start == last) {
                start = it;
                opts = t->opts();
                line = t->endLine();
                finished = (isSemiOptimization(type) && !t->closeStyles().empty());
            } else {
                if (opts != t->opts() || t->startLine() != line || finished ||
                    (!t->openStyles().empty() && isSemiOptimization(type))) {
                    if (!isWithoutRawDataOptimization(type)) {
                        po.concatenateAuxText(auxStart, auxIt);
                        auxIt = auxIt - (auxIt - auxStart) + 1;
                        auxStart = auxIt;
                    }

                    np->appendItem(concatenateText<Trait>(start, it));
                    start = it;
                    opts = t->opts();
                    line = t->endLine();
                }

                finished = (isSemiOptimization(type) && !t->closeStyles().empty());
            }

            if (!isWithoutRawDataOptimization(type))
                ++auxIt;
        } else {
            finished = false;

            if (start != last) {
                if (!isWithoutRawDataOptimization(type)) {
                    po.concatenateAuxText(auxStart, auxIt);
                    auxIt = auxIt - (auxIt - auxStart) + 1;
                    auxStart = auxIt;
                }

                np->appendItem(concatenateText<Trait>(start, it));
                start = last;
                opts = TextWithoutFormat;
                line = (*it)->endLine();
            }

            np->appendItem((*it));
        }
    }

    if (start != p->items().cend()) {
        np->appendItem(concatenateText<Trait>(start, p->items().cend()));

        if (!isWithoutRawDataOptimization(type)) {
            po.concatenateAuxText(auxStart, po.m_rawTextData.size());
        }
    }

    p = np;

    return p;
}

template<class Trait>
inline void
Parser<Trait>::parseTableInParagraph(TextParsingOpts<Trait> &po,
                                     std::shared_ptr<Paragraph<Trait>> parent,
                                     std::shared_ptr<Document<Trait>> doc,
                                     typename Trait::StringList &linksToParse,
                                     const typename Trait::String &workingPath,
                                     const typename Trait::String &fileName,
                                     bool collectRefLinks)
{
    MdBlock<Trait> fr;
    std::copy(po.m_fr.m_data.cbegin() + po.m_startTableLine, po.m_fr.m_data.cend(),
        std::back_inserter(fr.m_data));
    fr.m_emptyLineAfter = po.m_fr.m_emptyLineAfter;

    parseTable(fr, parent, doc, linksToParse, workingPath, fileName, collectRefLinks,
        po.m_columnsCount);

    po.m_line = po.m_fr.m_data.size() - fr.m_data.size();
    po.m_pos = 0;

    if (!fr.m_data.empty()) {
        po.m_detected = TextParsingOpts<Trait>::Detected::Code;
    }
}

inline void
normalizePos(long long int &pos,
             long long int &line,
             long long int length,
             long long int linesCount)
{
    if (pos != 0 && line < linesCount && pos == length) {
        pos = 0;
        ++line;
    }
}

template<class Trait>
inline bool
Parser<Trait>::isListOrQuoteAfterHtml(TextParsingOpts<Trait> &po)
{
    if (po.m_detected == TextParsingOpts<Trait>::Detected::HTML &&
        ((!po.m_parent->items().empty() &&
            po.m_parent->items().back()->type() == ItemType::RawHtml) || po.m_tmpHtml.get())) {
        auto html = (po.m_tmpHtml.get() ? po.m_tmpHtml :
            std::static_pointer_cast<RawHtml<Trait>>(po.m_parent->items().back()));

        bool dontClearDetection = false;

        long long int line = po.m_line;
        long long int pos = po.m_pos;

        normalizePos(pos, line, line < static_cast<long long int>(po.m_fr.m_data.size()) ?
                                    po.m_fr.m_data[line].first.length() : 0, po.m_fr.m_data.size());

        if (pos == 0) {
            if (line < static_cast<long long int>(po.m_fr.m_data.size())) {
                const auto type = whatIsTheLine(po.m_fr.m_data[line].first);

                switch (type) {
                case Parser<Trait>::BlockType::List: {
                    int num = 0;

                    if (isOrderedList<Trait>(po.m_fr.m_data[line].first.asString(), &num)) {
                        if (num == 1)
                            return true;
                    } else {
                        return true;
                    }
                } break;

                case Parser<Trait>::BlockType::Blockquote:
                    return true;

                case Parser<Trait>::BlockType::ListWithFirstEmptyLine: {
                    if (UnprotectedDocsMethods<Trait>::isFreeTag(html)) {
                        return true;
                    }
                } break;

                case Parser<Trait>::BlockType::EmptyLine:
                    dontClearDetection = true;
                    break;

                default:
                    break;
                }
            }
        }

        if (!dontClearDetection) {
            po.m_detected = TextParsingOpts<Trait>::Detected::Nothing;
        }
    }

    po.m_tmpHtml.reset();

    return false;
}

template<class Trait>
inline std::shared_ptr<Paragraph<Trait>>
makeParagraph(typename Block<Trait>::Items::const_iterator first,
              typename Block<Trait>::Items::const_iterator last)
{
    auto p = std::make_shared<Paragraph<Trait>>();

    p->setStartColumn((*first)->startColumn());
    p->setStartLine((*first)->startLine());

    for (; first != last; ++first) {
        p->appendItem(*first);
        p->setEndColumn((*first)->endColumn());
        p->setEndLine((*first)->endLine());
    }

    return p;
}

template<class Trait>
inline std::shared_ptr<Paragraph<Trait>>
splitParagraphsAndFreeHtml(std::shared_ptr<Block<Trait>> parent,
                           std::shared_ptr<Paragraph<Trait>> p,
                           TextParsingOpts<Trait> &po,
                           bool collectRefLinks,
                           bool fullyOptimizeParagraphs = true)
{
    auto first = p->items().cbegin();
    auto it = first;
    auto last = p->items().cend();

    for (; it != last; ++it) {
        if (first == last) {
            first = it;
        }

        if ((*it)->type() == ItemType::RawHtml &&
            UnprotectedDocsMethods<Trait>::isFreeTag(std::static_pointer_cast<RawHtml<Trait>>(*it))) {
            auto p = makeParagraph<Trait>(first, it);

            if (!collectRefLinks) {
                if (!p->isEmpty()) {
                    parent->appendItem(optimizeParagraph<Trait>(p, po,
                                                                fullyOptimizeParagraphs ?
                                                                    OptimizeParagraphType::FullWithoutRawData :
                                                                    OptimizeParagraphType::SemiWithoutRawData));
                }

                parent->appendItem(*it);
            }

            first = last;
        }
    }

    if (first != last) {
        if (first != p->items().cbegin()) {
            const auto c = std::count_if(first, last, [](const auto &i) {
                return (i->type() == MD::ItemType::Text);
            });
            po.m_rawTextData.erase(po.m_rawTextData.cbegin(), po.m_rawTextData.cbegin() +
                (po.m_rawTextData.size() - c));

            return makeParagraph<Trait>(first, last);
        } else {
            return p;
        }
    } else {
        po.m_rawTextData.clear();

        return std::make_shared<Paragraph<Trait>>();
    }
}

template<class Trait>
inline void
makeHeading(std::shared_ptr<Block<Trait>> parent,
            std::shared_ptr<Document<Trait>> doc,
            std::shared_ptr<Paragraph<Trait>> p,
            long long int lastColumn,
            long long int lastLine,
            int level,
            const typename Trait::String &workingPath,
            const typename Trait::String &fileName,
            bool collectRefLinks,
            const WithPosition &delim,
            TextParsingOpts<Trait> &po)
{
    if (!collectRefLinks) {
        if (p->items().back()->type() == ItemType::LineBreak) {
            auto lb = std::static_pointer_cast<LineBreak<Trait>>(p->items().back());

            p = makeParagraph<Trait>(p->items().cbegin(), std::prev(p->items().cend()));

            if (p->items().back()->type() == ItemType::Text) {
                auto lt = std::static_pointer_cast<Text<Trait>>(p->items().back());
                lt->setText(typename Trait::String(lt->text() + (lb->isSpaceBefore() ?
                    Trait::latin1ToString(" ") : typename Trait::String()) + lb->text()).simplified());
                lt->setEndColumn(lt->endColumn() + lb->text().length());
                po.m_rawTextData.back().m_str += (lb->isSpaceBefore() ?
                    Trait::latin1ToString(" ") : typename Trait::String()) + lb->text();
            } else {
                auto t = std::make_shared<Text<Trait>>();
                t->setText(lb->text());
                t->setSpaceBefore(lb->isSpaceBefore());
                t->setSpaceAfter(lb->isSpaceAfter());
                t->setStartColumn(lb->startColumn());
                t->setStartLine(lb->startLine());
                t->setEndColumn(lb->endColumn());
                t->setEndLine(lb->endLine());

                p->appendItem(t);

                const auto pos = localPosFromVirgin(po.m_fr, lb->startColumn(), lb->startLine());

                po.m_rawTextData.push_back({lb->text(), pos.first, pos.second,
                    lb->isSpaceBefore(), true});
            }
        }

        std::pair<typename Trait::String, WithPosition> label;

        if (p->items().back()->type() == ItemType::Text) {
            auto t = std::static_pointer_cast<Text<Trait>>(p->items().back());

            if (t->opts() == TextWithoutFormat) {
                auto text = po.m_rawTextData.back();
                typename Trait::InternalString tmp(text.m_str);
                label = findAndRemoveHeaderLabel<Trait>(tmp);
                const auto ns = (label.second.startColumn() != -1 ?
                    skipSpaces<Trait>(label.second.startColumn(), text.m_str) : tmp.length());
                label.second.setStartColumn(t->startColumn() + label.second.startColumn());
                label.second.setEndColumn(t->startColumn() + label.second.endColumn());
                label.second.setStartLine(t->startLine());
                label.second.setEndLine(t->endLine());

                if (!label.first.isEmpty() && ns >= tmp.length()) {
                    label.first = label.first.sliced(1, label.first.length() - 2);

                    if (tmp.asString().simplified().isEmpty()) {
                        p->removeItemAt(p->items().size() - 1);

                        if (!p->items().empty()) {
                            const auto last = std::static_pointer_cast<WithPosition>(p->items().back());
                            p->setEndColumn(last->endColumn());
                            p->setEndLine(last->endLine());
                        }
                    } else {
                        auto s = replaceEntity<Trait>(tmp.asString().simplified());
                        s = removeBackslashes<typename Trait::String, Trait>(s);
                        t->setText(s);
                        t->setEndColumn(label.second.startColumn() - 1);
                        t->setSpaceAfter(true);
                        p->setEndColumn(t->endColumn());
                    }
                } else {
                    label.first.clear();
                }
            }
        }

        std::shared_ptr<Heading<Trait>> h(new Heading<Trait>);
        h->setStartColumn(p->startColumn());
        h->setStartLine(p->startLine());
        h->setEndColumn(lastColumn);
        h->setEndLine(lastLine);
        h->setLevel(level);

        if (!p->items().empty()) {
            h->setText(p);
        }

        h->setDelims({delim});

        if (label.first.isEmpty() && !p->items().empty()) {
            label.first = Trait::latin1ToString("#") + paragraphToLabel(p.get());
        } else {
            h->setLabelPos(label.second);
        }

        if (!label.first.isEmpty()) {
            label.first += Trait::latin1ToString("/") + (!workingPath.isEmpty() ?
                workingPath + Trait::latin1ToString("/") : typename Trait::String()) + fileName;

            h->setLabel(label.first);

            doc->insertLabeledHeading(label.first, h);
        }

        parent->appendItem(h);
    }
}

template<class Trait>
inline long long int
textAtIdx(std::shared_ptr<Paragraph<Trait>> p,
          size_t idx)
{
    size_t i = 0;

    for (auto it = p->items().cbegin(), last = p->items().cend(); it != last; ++it) {
        if ((*it)->type() == ItemType::Text) {
            if (i == idx) {
                return std::distance(p->items().cbegin(), it);
            }

            ++i;
        }
    }

    return -1;
}

template<class Trait>
inline void
checkForTextPlugins(std::shared_ptr<Paragraph<Trait>> p,
                    TextParsingOpts<Trait> &po,
                    const TextPluginsMap<Trait> &textPlugins,
                    bool inLink)
{
    for (const auto &plugin : textPlugins) {
        if (inLink && !std::get<bool>(plugin.second)) {
            continue;
        }

        std::get<TextPluginFunc<Trait>>(plugin.second)(p, po,
            std::get<typename Trait::StringList>(plugin.second));
    }
}

template<class Trait>
inline void
makeHorLine(const typename MdBlock<Trait>::Line &line,
            std::shared_ptr<Block<Trait>> parent)
{
    std::shared_ptr<Item<Trait>> hr(new HorizontalLine<Trait>);
    hr->setStartColumn(line.first.virginPos(skipSpaces<Trait>(0, line.first.asString())));
    hr->setStartLine(line.second.m_lineNumber);
    hr->setEndColumn(line.first.virginPos(line.first.length() - 1));
    hr->setEndLine(line.second.m_lineNumber);
    parent->appendItem(hr);
}

template<class Trait>
inline void
Parser<Trait>::parseFormattedTextLinksImages(MdBlock<Trait> &fr,
                                             std::shared_ptr<Block<Trait>> parent,
                                             std::shared_ptr<Document<Trait>> doc,
                                             typename Trait::StringList &linksToParse,
                                             const typename Trait::String &workingPath,
                                             const typename Trait::String &fileName,
                                             bool collectRefLinks,
                                             bool ignoreLineBreak,
                                             RawHtmlBlock<Trait> &html,
                                             bool inLink)

{
    if (fr.m_data.empty()) {
        return;
    }

    std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);
    p->setStartColumn(fr.m_data.at(0).first.virginPos(0));
    p->setStartLine(fr.m_data.at(0).second.m_lineNumber);
    std::shared_ptr<Paragraph<Trait>> pt(new Paragraph<Trait>);

    const auto delims = collectDelimiters(fr.m_data);

    TextParsingOpts<Trait> po = {fr, p, nullptr, doc, linksToParse, workingPath, fileName,
        collectRefLinks, ignoreLineBreak, html, m_textPlugins};

    if (!delims.empty()) {
        for (auto it = delims.cbegin(), last = delims.cend(); it != last; ++it) {
            if (html.m_html.get() && html.m_continueHtml) {
                it = finishRawHtmlTag(it, last, po, false);
            } else {
                if (isListOrQuoteAfterHtml(po)) {
                    break;
                }

                if (po.m_line > po.m_lastTextLine) {
                    checkForTableInParagraph(po, fr.m_data.size() - 1);
                }

                if (po.shouldStopParsing() && po.m_lastTextLine < it->m_line) {
                    break;
                } else if (!collectRefLinks) {
                    makeText(po.m_lastTextLine < it->m_line ? po.m_lastTextLine : it->m_line,
                        po.m_lastTextLine < it->m_line ? po.m_lastTextPos : it->m_pos, po);
                } else {
                    const auto prevLine = po.m_line;

                    po.m_line = (po.m_lastTextLine < it->m_line ? po.m_lastTextLine : it->m_line);
                    po.m_pos = (po.m_lastTextLine < it->m_line ? po.m_lastTextPos : it->m_pos);

                    if (po.m_line > prevLine) {
                        po.m_firstInParagraph = false;
                    } else if (po.m_pos > skipSpaces<Trait>(0, po.m_fr.m_data[po.m_line].first.asString())) {
                        po.m_firstInParagraph = false;
                    }
                }

                switch (it->m_type) {
                case Delimiter::SquareBracketsOpen: {
                    it = checkForLink(it, last, po);
                    p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                    p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                } break;

                case Delimiter::ImageOpen: {
                    it = checkForImage(it, last, po);
                    p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                    p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                } break;

                case Delimiter::Less: {
                    it = checkForAutolinkHtml(it, last, po, true);

                    if (!html.m_html.get()) {
                        p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                        p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                    }
                } break;

                case Delimiter::Strikethrough:
                case Delimiter::Emphasis1:
                case Delimiter::Emphasis2: {
                    if (!collectRefLinks) {
                        it = checkForStyle(delims.cbegin(), it, last, po);
                        p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                        p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                    }
                } break;

                case Delimiter::Math: {
                    it = checkForMath(it, last, po);
                    p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                    p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                } break;

                case Delimiter::InlineCode: {
                    if (!it->m_backslashed) {
                        it = checkForInlineCode(it, last, po);
                        p->setEndColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos + it->m_len - 1));
                        p->setEndLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                    }
                } break;

                case Delimiter::HorizontalLine: {
                    po.m_wasRefLink = false;
                    po.m_firstInParagraph = false;

                    const auto pos = skipSpaces<Trait>(0, po.m_fr.m_data[it->m_line].first.asString());
                    const auto withoutSpaces = po.m_fr.m_data[it->m_line].first.asString().sliced(pos);

                    auto h2 = isH2<Trait>(withoutSpaces);

                    if (!p->isEmpty()) {
                        optimizeParagraph<Trait>(p, po, OptimizeParagraphType::Semi);

                        checkForTextPlugins<Trait>(p, po, m_textPlugins, inLink);

                        if (it->m_line - 1 >= 0) {
                            p->setEndColumn(fr.m_data.at(it->m_line - 1).first.virginPos(
                                fr.m_data.at(it->m_line - 1).first.length() - 1));
                            p->setEndLine(fr.m_data.at(it->m_line - 1).second.m_lineNumber);
                        }

                        p = splitParagraphsAndFreeHtml(parent, p, po, collectRefLinks, m_fullyOptimizeParagraphs);

                        if (!p->isEmpty()) {
                            if (!collectRefLinks) {
                                if (!h2 || (p->items().size() == 1 &&
                                    p->items().front()->type() == ItemType::LineBreak)) {
                                    parent->appendItem(p);

                                    h2 = false;
                                } else {
                                    makeHeading(parent,
                                                doc,
                                                optimizeParagraph<Trait>(p, po, defaultParagraphOptimization()),
                                                fr.m_data[it->m_line].first.virginPos(it->m_pos + it->m_len - 1),
                                                fr.m_data[it->m_line].second.m_lineNumber,
                                                2,
                                                workingPath,
                                                fileName,
                                                collectRefLinks,
                                                {po.m_fr.m_data[it->m_line].first.virginPos(pos),
                                                 fr.m_data[it->m_line].second.m_lineNumber,
                                                 po.m_fr.m_data[it->m_line].first.virginPos(
                                                    lastNonSpacePos<Trait>(po.m_fr.m_data[it->m_line].first.asString())),
                                                 fr.m_data[it->m_line].second.m_lineNumber},
                                                po);

                                    po.m_checkLineOnNewType = true;
                                }
                            }
                        } else {
                            h2 = false;
                        }
                    } else {
                        h2 = false;
                    }

                    p.reset(new Paragraph<Trait>);
                    po.m_rawTextData.clear();

                    if (it->m_line + 1 < static_cast<long long int>(fr.m_data.size())) {
                        p->setStartColumn(fr.m_data.at(it->m_line + 1).first.virginPos(0));
                        p->setStartLine(fr.m_data.at(it->m_line + 1).second.m_lineNumber);
                    }

                    po.m_parent = p;
                    po.m_line = it->m_line;
                    po.m_pos = it->m_pos + it->m_len;

                    if (!h2 && !collectRefLinks) {
                        makeHorLine<Trait>(fr.m_data[it->m_line], parent);
                    }
                } break;

                case Delimiter::H1:
                case Delimiter::H2: {
                    po.m_wasRefLink = false;
                    po.m_firstInParagraph = false;

                    optimizeParagraph<Trait>(p, po, OptimizeParagraphType::Semi);

                    checkForTextPlugins<Trait>(p, po, m_textPlugins, inLink);

                    if (it->m_line - 1 >= 0) {
                        p->setEndColumn(fr.m_data.at(it->m_line - 1).first.virginPos(
                            fr.m_data.at(it->m_line - 1).first.length() - 1));
                        p->setEndLine(fr.m_data.at(it->m_line - 1).second.m_lineNumber);
                    }

                    p = splitParagraphsAndFreeHtml(parent, p, po, collectRefLinks,
                        m_fullyOptimizeParagraphs);

                    if (!p->isEmpty() && !((p->items().size() == 1 &&
                        p->items().front()->type() == ItemType::LineBreak))) {
                        makeHeading(parent,
                                    doc,
                                    optimizeParagraph<Trait>(p, po, defaultParagraphOptimization()),
                                    fr.m_data[it->m_line].first.virginPos(it->m_pos + it->m_len - 1),
                                    fr.m_data[it->m_line].second.m_lineNumber,
                                    it->m_type == Delimiter::H1 ? 1 : 2,
                                    workingPath,
                                    fileName,
                                    collectRefLinks,
                                    {po.m_fr.m_data[it->m_line].first.virginPos(skipSpaces<Trait>(
                                        0, po.m_fr.m_data[it->m_line].first.asString())),
                                     fr.m_data[it->m_line].second.m_lineNumber,
                                     po.m_fr.m_data[it->m_line].first.virginPos(lastNonSpacePos<Trait>(
                                        po.m_fr.m_data[it->m_line].first.asString())),
                                     fr.m_data[it->m_line].second.m_lineNumber},
                                    po);

                        po.m_checkLineOnNewType = true;

                        p.reset(new Paragraph<Trait>);
                        po.m_rawTextData.clear();

                        if (it->m_line + 1 < static_cast<long long int>(fr.m_data.size())) {
                            p->setStartColumn(fr.m_data.at(it->m_line + 1).first.virginPos(0));
                            p->setStartLine(fr.m_data.at(it->m_line + 1).second.m_lineNumber);
                        }

                        po.m_line = it->m_line;
                        po.m_pos = it->m_pos + it->m_len;
                    } else if (p->startColumn() == -1) {
                        p->setStartColumn(fr.m_data.at(it->m_line).first.virginPos(it->m_pos));
                        p->setStartLine(fr.m_data.at(it->m_line).second.m_lineNumber);
                    }

                    po.m_parent = p;
                } break;

                default: {
                    if (!po.shouldStopParsing()) {
                        po.m_wasRefLink = false;
                        po.m_firstInParagraph = false;

                        if (!collectRefLinks) {
                            makeText(it->m_line, it->m_pos + it->m_len, po);
                        } else {
                            po.m_line = it->m_line;
                            po.m_pos = it->m_pos + it->m_len;
                        }
                    }
                } break;
                }

                if (po.shouldStopParsing()) {
                    break;
                }

                if (po.m_checkLineOnNewType) {
                    if (po.m_line + 1 < static_cast<long long int>(po.m_fr.m_data.size())) {
                        const auto type = Parser<Trait>::whatIsTheLine(po.m_fr.m_data[po.m_line + 1].first);

                        if (type == Parser<Trait>::BlockType::CodeIndentedBySpaces) {
                            po.m_detected = TextParsingOpts<Trait>::Detected::Code;

                            break;
                        }
                    }

                    po.m_checkLineOnNewType = false;
                }
            }
        }
    } else {
        if (html.m_html.get() && html.m_continueHtml) {
            finishRawHtmlTag(delims.cend(), delims.cend(), po, false);
        }
    }

    if (po.m_lastTextLine == -1) {
        checkForTableInParagraph(po, po.m_fr.m_data.size() - 1);
    }

    if (po.m_detected == TextParsingOpts<Trait>::Detected::Table) {
        if (!collectRefLinks) {
            makeText(po.m_lastTextLine, po.m_lastTextPos, po);
        }

        parseTableInParagraph(po, pt, doc, linksToParse, workingPath, fileName, collectRefLinks);
    }

    while (po.m_detected == TextParsingOpts<Trait>::Detected::HTML &&
           po.m_line < static_cast<long long int>(po.m_fr.m_data.size())) {
        if (!isListOrQuoteAfterHtml(po)) {
            if (!collectRefLinks) {
                makeText(po.m_line, po.m_fr.m_data[po.m_line].first.length(), po);
            }

            po.m_pos = 0;
            ++po.m_line;
        } else {
            break;
        }
    }

    if (po.m_detected == TextParsingOpts<Trait>::Detected::Nothing &&
        po.m_line <= static_cast<long long int>(po.m_fr.m_data.size() - 1)) {
        if (!collectRefLinks) {
            makeText(po.m_fr.m_data.size() - 1, po.m_fr.m_data.back().first.length(), po);
        }
    }

    if (!p->isEmpty()) {
        optimizeParagraph<Trait>(p, po, OptimizeParagraphType::Semi);

        checkForTextPlugins<Trait>(p, po, m_textPlugins, inLink);

        p = splitParagraphsAndFreeHtml(parent, p, po, collectRefLinks, m_fullyOptimizeParagraphs);

        if (!p->isEmpty() && !collectRefLinks) {
            parent->appendItem(optimizeParagraph<Trait>(p, po, defaultParagraphOptimization()));
        }

        po.m_rawTextData.clear();
    }

    if (!pt->isEmpty() && !collectRefLinks) {
        parent->appendItem(pt->items().front());
    }

    normalizePos(po.m_pos, po.m_line, po.m_line < static_cast<long long int>(po.m_fr.m_data.size()) ?
                                    po.m_fr.m_data[po.m_line].first.length() : 0, po.m_fr.m_data.size());

    if (po.m_detected != TextParsingOpts<Trait>::Detected::Nothing &&
        po.m_line < static_cast<long long int>(po.m_fr.m_data.size())) {
        typename MdBlock<Trait>::Data tmp;
        std::copy(fr.m_data.cbegin() + po.m_line, fr.m_data.cend(), std::back_inserter(tmp));

        StringListStream<Trait> stream(tmp);

        Parser<Trait>::parse(stream, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
    }
}

template<class Trait>
inline void
Parser<Trait>::parseFootnote(MdBlock<Trait> &fr,
                             std::shared_ptr<Block<Trait>>,
                             std::shared_ptr<Document<Trait>> doc,
                             typename Trait::StringList &linksToParse,
                             const typename Trait::String &workingPath,
                             const typename Trait::String &fileName,
                             bool collectRefLinks)
{
    {
        const auto it = (std::find_if(fr.m_data.rbegin(), fr.m_data.rend(), [](const auto &s) {
                            return !s.first.simplified().isEmpty();
                        })).base();

        if (it != fr.m_data.end()) {
            fr.m_data.erase(it, fr.m_data.end());
        }
    }

    if (!fr.m_data.empty()) {
        std::shared_ptr<Footnote<Trait>> f(new Footnote<Trait>);
        f->setStartColumn(fr.m_data.front().first.virginPos(0));
        f->setStartLine(fr.m_data.front().second.m_lineNumber);
        f->setEndColumn(fr.m_data.back().first.virginPos(fr.m_data.back().first.length() - 1));
        f->setEndLine(fr.m_data.back().second.m_lineNumber);

        const auto delims = collectDelimiters(fr.m_data);

        RawHtmlBlock<Trait> html;

        TextParsingOpts<Trait> po = {fr, f, nullptr, doc, linksToParse, workingPath, fileName,
            collectRefLinks, false, html, m_textPlugins};
        po.m_lastTextLine = fr.m_data.size();
        po.m_lastTextPos = fr.m_data.back().first.length();

        if (!delims.empty() && delims.cbegin()->m_type == Delimiter::SquareBracketsOpen &&
            !delims.cbegin()->m_isWordBefore) {
            typename MdBlock<Trait>::Data id;
            typename Delims::const_iterator it = delims.cend();

            po.m_line = delims.cbegin()->m_line;
            po.m_pos = delims.cbegin()->m_pos;

            std::tie(id, it) = checkForLinkText(delims.cbegin(), delims.cend(), po);

            if (!toSingleLine(id).simplified().isEmpty() &&
                id.front().first.asString().startsWith(Trait::latin1ToString("^")) &&
                it != delims.cend() &&
                fr.m_data.at(it->m_line).first.length() > it->m_pos + 2 &&
                fr.m_data.at(it->m_line).first[it->m_pos + 1] == Trait::latin1ToChar(':') &&
                fr.m_data.at(it->m_line).first[it->m_pos + 2].isSpace()) {
                f->setIdPos({fr.m_data[delims.cbegin()->m_line].first.virginPos(delims.cbegin()->m_pos),
                             fr.m_data[delims.cbegin()->m_line].second.m_lineNumber,
                             fr.m_data.at(it->m_line).first.virginPos(it->m_pos + 1),
                             fr.m_data.at(it->m_line).second.m_lineNumber});

                {
                    typename MdBlock<Trait>::Data tmp;
                    std::copy(fr.m_data.cbegin() + it->m_line, fr.m_data.cend(),
                        std::back_inserter(tmp));
                    fr.m_data = tmp;
                }

                fr.m_data.front().first = fr.m_data.front().first.sliced(it->m_pos + 3);

                for (auto it = fr.m_data.begin(), last = fr.m_data.end(); it != last; ++it) {
                    if (it->first.asString().startsWith(Trait::latin1ToString("    "))) {
                        it->first = it->first.sliced(4);
                    }
                }

                StringListStream<Trait> stream(fr.m_data);

                parse(stream, f, doc, linksToParse, workingPath, fileName, collectRefLinks);

                if (!f->isEmpty()) {
                    doc->insertFootnote(Trait::latin1ToString("#") + toSingleLine(id).simplified() +
                        Trait::latin1ToString("/") + (!workingPath.isEmpty() ?
                            workingPath + Trait::latin1ToString("/") : typename Trait::String()) + fileName,
                        f);
                }
            }
        }
    }
}

template<class Trait>
inline void
Parser<Trait>::parseBlockquote(MdBlock<Trait> &fr,
                               std::shared_ptr<Block<Trait>> parent,
                               std::shared_ptr<Document<Trait>> doc,
                               typename Trait::StringList &linksToParse,
                               const typename Trait::String &workingPath,
                               const typename Trait::String &fileName,
                               bool collectRefLinks,
                               RawHtmlBlock<Trait> &)
{
    const long long int pos = fr.m_data.front().first.asString().indexOf(Trait::latin1ToChar('>'));
    long long int extra = 0;

    if (pos > -1) {
        typename Blockquote<Trait>::Delims delims;

        long long int i = 0, j = 0;

        BlockType bt = BlockType::EmptyLine;

        for (auto it = fr.m_data.begin(), last = fr.m_data.end(); it != last; ++it, ++i) {
            const auto ns = skipSpaces<Trait>(0, it->first.asString());
            const auto gt = (ns < it->first.length() ? (it->first[ns] == Trait::latin1ToChar('>') ? ns : -1) : -1);

            if (gt > -1) {
                const auto dp = it->first.virginPos(gt);
                delims.push_back({dp, it->second.m_lineNumber, dp, it->second.m_lineNumber});

                if (it == fr.m_data.begin()) {
                    extra = gt + (it->first.length() > gt + 1 ?
                        (it->first[gt + 1] == Trait::latin1ToChar(' ') ? 1 : 0) : 0) + 1;
                }

                it->first = it->first.sliced(gt + (it->first.length() > gt + 1 ?
                    (it->first[gt + 1] == Trait::latin1ToChar(' ') ? 1 : 0) : 0) + 1);

                bt = whatIsTheLine(it->first);
            }
            // Process lazyness...
            else {
                if (ns < 4 && isHorizontalLine<Trait>(it->first.asString().sliced(ns))) {
                    break;
                }

                const auto tmpBt = whatIsTheLine(it->first);

                if (isListType(tmpBt)) {
                    break;
                }

                if (bt == BlockType::Text) {
                    if (isH1<Trait>(it->first.asString())) {
                        const auto p = it->first.asString().indexOf(Trait::latin1ToChar('='));

                        it->first.insert(p, Trait::latin1ToChar('\\'));

                        continue;
                    } else if (isH2<Trait>(it->first.asString())) {
                        const auto p = it->first.asString().indexOf(Trait::latin1ToChar('-'));

                        it->first.insert(p, Trait::latin1ToChar('\\'));

                        continue;
                    }
                }

                if ((bt == BlockType::Text || bt == BlockType::Blockquote || bt == BlockType::List)
                    && (tmpBt == BlockType::Text || tmpBt == BlockType::CodeIndentedBySpaces)) {
                    continue;
                } else {
                    break;
                }
            }
        }

        typename MdBlock<Trait>::Data tmp;

        for (; j < i; ++j) {
            tmp.push_back(fr.m_data.at(j));
        }

        StringListStream<Trait> stream(tmp);

        std::shared_ptr<Blockquote<Trait>> bq(new Blockquote<Trait>);
        bq->setStartColumn(fr.m_data.at(0).first.virginPos(0) - extra);
        bq->setStartLine(fr.m_data.at(0).second.m_lineNumber);
        bq->setEndColumn(fr.m_data.at(j - 1).first.virginPos(fr.m_data.at(j - 1).first.length() - 1));
        bq->setEndLine(fr.m_data.at(j - 1).second.m_lineNumber);
        bq->delims() = delims;

        parse(stream, bq, doc, linksToParse, workingPath, fileName, collectRefLinks);

        if (!collectRefLinks) {
            parent->appendItem(bq);
        }

        if (i < (long long int)fr.m_data.size()) {
            tmp.clear();

            std::copy(fr.m_data.cbegin() + i, fr.m_data.cend(), std::back_inserter(tmp));

            StringListStream<Trait> stream(tmp);

            parse(stream, parent, doc, linksToParse, workingPath, fileName, collectRefLinks);
        }
    }
}

template<class Trait>
inline bool
isListItemAndNotNested(const typename Trait::String &s,
                       long long int indent)
{
    long long int p = skipSpaces<Trait>(0, s);

    if (p >= indent || p == s.size()) {
        return false;
    }

    bool space = false;

    if (p + 1 >= s.size()) {
        space = true;
    } else {
        space = s[p + 1].isSpace();
    }

    if (p < 4) {
        if (s[p] == Trait::latin1ToChar('*') && space) {
            return true;
        } else if (s[p] == Trait::latin1ToChar('-') && space) {
            return true;
        } else if (s[p] == Trait::latin1ToChar('+') && space) {
            return true;
        } else {
            return isOrderedList<Trait>(s);
        }
    } else
        return false;
}

template<class Trait>
inline std::pair<long long int, long long int>
calculateIndent(const typename Trait::String &s,
                long long int p)
{
    return {0, skipSpaces<Trait>(p, s)};
}

template<class Trait>
inline std::tuple<bool, long long int, typename Trait::Char, bool>
listItemData(const typename Trait::String &s,
             bool wasText)
{
    long long int p = skipSpaces<Trait>(0, s);

    if (p == s.size()) {
        return {false, 0, typename Trait::Char(), false};
    }

    bool space = false;

    if (p + 1 >= s.size()) {
        space = true;
    } else {
        space = s[p + 1].isSpace();
    }

    if (p < 4) {
        if (s[p] == Trait::latin1ToChar('*') && space) {
            return {true, p + 2, Trait::latin1ToChar('*'),
                p + 2 < s.size() ? !s.sliced(p + 2).simplified().isEmpty() : false};
        } else if (s[p] == Trait::latin1ToChar('-')) {
            if (isH2<Trait>(s) && wasText) {
                return {false, p + 2, Trait::latin1ToChar('-'), false};
            } else if (space) {
                return {true, p + 2, Trait::latin1ToChar('-'),
                    p + 2 < s.size() ? !s.sliced(p + 2).simplified().isEmpty() : false};
            }
        } else if (s[p] == Trait::latin1ToChar('+') && space) {
            return {true, p + 2, Trait::latin1ToChar('+'),
                p + 2 < s.size() ? !s.sliced(p + 2).simplified().isEmpty() : false};
        } else {
            int d = 0, l = 0;
            typename Trait::Char c;

            if (isOrderedList<Trait>(s, &d, &l, &c)) {
                return {true, p + l + 2, c,
                    p + l + 2 < s.size() ? !s.sliced(p + l + 2).simplified().isEmpty() : false};
            } else {
                return {false, 0, typename Trait::Char(), false};
            }
        }
    }

    return {false, 0, typename Trait::Char(), false};
}

template<class Trait>
inline void
setLastPos(std::shared_ptr<Item<Trait>> item,
           long long int pos,
           long long int line)
{
    item->setEndColumn(pos);
    item->setEndLine(line);
}

template<class Trait>
inline void
updateLastPosInList(const RawHtmlBlock<Trait> &html)
{
    if (html.m_parent != html.m_topParent) {
        const auto it = html.m_toAdjustLastPos.find(html.m_parent);

        if (it != html.m_toAdjustLastPos.end()) {
            for (auto &i : it->second) {
                i.first->setEndColumn(html.m_html->endColumn());
                i.first->setEndLine(html.m_html->endLine());
            }
        }
    }
}

template<class Trait>
inline long long int
Parser<Trait>::parseList(MdBlock<Trait> &fr,
                         std::shared_ptr<Block<Trait>> parent,
                         std::shared_ptr<Document<Trait>> doc,
                         typename Trait::StringList &linksToParse,
                         const typename Trait::String &workingPath,
                         const typename Trait::String &fileName,
                         bool collectRefLinks,
                         RawHtmlBlock<Trait> &html)
{
    bool resetTopParent = false;
    long long int line = -1;

    if (!html.m_topParent) {
        html.m_topParent = parent;
        resetTopParent = true;
    }

    const auto p = skipSpaces<Trait>(0, fr.m_data.front().first.asString());

    if (p != fr.m_data.front().first.length()) {
        std::shared_ptr<List<Trait>> list(new List<Trait>);

        typename MdBlock<Trait>::Data listItem;
        auto it = fr.m_data.begin();
        listItem.push_back(*it);
        list->setStartColumn(it->first.virginPos(p));
        list->setStartLine(it->second.m_lineNumber);
        ++it;

        long long int indent = 0;
        typename Trait::Char marker;

        std::tie(std::ignore, indent, marker, std::ignore) =
            listItemData<Trait>(listItem.front().first.asString(), false);

        html.m_blocks.push_back({list, list->startColumn() + indent});

        if (!collectRefLinks) {
            html.m_toAdjustLastPos.insert({list, html.m_blocks});
        }

        bool updateIndent = false;

        auto addListMakeNew = [&]() {
            if (!list->isEmpty() && !collectRefLinks) {
                parent->appendItem(list);
            }

            html.m_blocks.pop_back();

            list.reset(new List<Trait>);

            html.m_blocks.push_back({list, indent});

            if (!collectRefLinks) {
                html.m_toAdjustLastPos.insert({list, html.m_blocks});
            }
        };

        auto processLastHtml = [&](std::shared_ptr<ListItem<Trait>> resItem) {
            if (html.m_html && resItem) {
                auto htmlParent = (resItem->startLine() == html.m_html->startLine() ||
                    html.m_html->startColumn() >= resItem->startColumn() + indent ?
                        resItem : html.findParent(html.m_html->startColumn()));

                if (!htmlParent) {
                    htmlParent = html.m_topParent;
                }

                if (htmlParent != resItem) {
                    addListMakeNew();
                }

                if (!collectRefLinks) {
                    htmlParent->appendItem(html.m_html);
                    updateLastPosInList<Trait>(html);
                }

                resetHtmlTag<Trait>(html);
            }
        };

        auto processListItem = [&]() {
            MdBlock<Trait> block = {listItem, 0};

            std::shared_ptr<ListItem<Trait>> resItem;

            line = parseListItem(block, list, doc, linksToParse, workingPath, fileName,
                collectRefLinks, html, &resItem);
            listItem.clear();

            processLastHtml(resItem);
        };

        for (auto last = fr.m_data.end(); it != last; ++it) {
            if (updateIndent) {
                std::tie(std::ignore, indent, marker, std::ignore) =
                    listItemData<Trait>(it->first.asString(), false);

                if (!collectRefLinks) {
                    html.m_blocks.back().second = indent;
                }

                updateIndent = false;
            }

            const auto ns = skipSpaces<Trait>(0, it->first.asString());

            if (isH1<Trait>(it->first.asString().sliced(ns)) && ns < indent && !listItem.empty()) {
                const auto p = it->first.asString().indexOf(Trait::latin1ToChar('='));

                it->first.insert(p, Trait::latin1ToChar('\\'));
            } else if (isHorizontalLine<Trait>(it->first.asString().sliced(ns)) &&
                ns < indent && !listItem.empty()) {
                updateIndent = true;

                processListItem();

                if (!list->isEmpty()) {
                    addListMakeNew();
                }

                if (!collectRefLinks) {
                    makeHorLine<Trait>(*it, parent);
                }

                continue;
            } else if (isListItemAndNotNested<Trait>(it->first.asString(), indent) &&
                !listItem.empty()) {
                typename Trait::Char tmpMarker;
                std::tie(std::ignore, indent, tmpMarker, std::ignore) =
                    listItemData<Trait>(it->first.asString(), false);

                processListItem();

                if (tmpMarker != marker) {
                    if (!list->isEmpty()) {
                        addListMakeNew();
                    }

                    marker = tmpMarker;
                }
            }

            if (line > 0) {
                break;
            }

            listItem.push_back(*it);

            if (list->startColumn() == -1) {
                list->setStartColumn(
                    it->first.virginPos(std::min(it->first.length() ?
                        it->first.length() - 1 : 0, skipSpaces<Trait>(0, it->first.asString()))));
                list->setStartLine(it->second.m_lineNumber);

                if (!collectRefLinks) {
                    html.m_blocks.back().second += list->startColumn();
                }
            }
        }

        if (!listItem.empty()) {
            MdBlock<Trait> block = {listItem, 0};
            line = parseListItem(block, list, doc, linksToParse, workingPath, fileName,
                collectRefLinks, html);
        }

        if (!list->isEmpty() && !collectRefLinks) {
            parent->appendItem(list);
        }

        html.m_blocks.pop_back();
    }

    if (resetTopParent) {
        html.m_topParent.reset();
    }

    return line;
}

template<class Trait>
inline long long int
Parser<Trait>::parseListItem(MdBlock<Trait> &fr,
                             std::shared_ptr<Block<Trait>> parent,
                             std::shared_ptr<Document<Trait>> doc,
                             typename Trait::StringList &linksToParse,
                             const typename Trait::String &workingPath,
                             const typename Trait::String &fileName,
                             bool collectRefLinks,
                             RawHtmlBlock<Trait> &html,
                             std::shared_ptr<ListItem<Trait>> *resItem)
{
    {
        const auto it = (std::find_if(fr.m_data.rbegin(), fr.m_data.rend(), [](const auto &s) {
                            return !s.first.simplified().isEmpty();
                        })).base();

        if (it != fr.m_data.end()) {
            fr.m_data.erase(it, fr.m_data.end());
        }
    }

    const auto p = skipSpaces<Trait>(0, fr.m_data.front().first.asString());

    std::shared_ptr<ListItem<Trait>> item(new ListItem<Trait>);

    item->setStartColumn(fr.m_data.front().first.virginPos(p));
    item->setStartLine(fr.m_data.front().second.m_lineNumber);

    int i = 0, len = 0;

    if (isOrderedList<Trait>(fr.m_data.front().first.asString(), &i, &len)) {
        item->setListType(ListItem<Trait>::Ordered);
        item->setStartNumber(i);
        item->setDelim({item->startColumn(), item->startLine(), item->startColumn() + len, item->startLine()});
    } else {
        item->setListType(ListItem<Trait>::Unordered);
        item->setDelim({item->startColumn(), item->startLine(), item->startColumn(), item->startLine()});
    }

    if (item->listType() == ListItem<Trait>::Ordered) {
        item->setOrderedListPreState(i == 1 ? ListItem<Trait>::Start : ListItem<Trait>::Continue);
    }

    typename MdBlock<Trait>::Data data;

    auto it = fr.m_data.begin();
    ++it;

    int pos = 1;

    long long int indent = 0;
    bool wasText = false;

    std::tie(std::ignore, indent, std::ignore, wasText) =
        listItemData<Trait>(fr.m_data.front().first.asString(), wasText);

    html.m_blocks.push_back({item, item->startColumn() + indent});

    if (!collectRefLinks) {
        html.m_toAdjustLastPos.insert({item, html.m_blocks});
    }

    const auto firstNonSpacePos = calculateIndent<Trait>(
        fr.m_data.front().first.asString(), indent).second;

    if (firstNonSpacePos - indent < 4) {
        indent = firstNonSpacePos;
    }

    if (indent < fr.m_data.front().first.length()) {
        data.push_back({fr.m_data.front().first.right(fr.m_data.front().first.length() - indent),
            fr.m_data.front().second});
    }

    bool taskList = false;
    bool checked = false;

    if (!data.empty()) {
        auto p = skipSpaces<Trait>(0, data.front().first.asString());

        if (p < data.front().first.length()) {
            if (data.front().first[p] == Trait::latin1ToChar('[')) {
                const auto startTaskDelimPos = data.front().first.virginPos(p);

                ++p;

                if (p < data.front().first.length()) {
                    if (data.front().first[p] == Trait::latin1ToChar(' ') ||
                        data.front().first[p].toLower() == Trait::latin1ToChar('x')) {
                        if (data.front().first[p].toLower() == Trait::latin1ToChar('x')) {
                            checked = true;
                        }

                        ++p;

                        if (p < data.front().first.length()) {
                            if (data.front().first[p] == Trait::latin1ToChar(']')) {
                                item->setTaskDelim({startTaskDelimPos, item->startLine(), data.front().first.virginPos(p), item->startLine()});

                                taskList = true;

                                data[0].first = data[0].first.sliced(p + 1);
                            }
                        }
                    }
                }
            }
        }
    }

    if (taskList) {
        item->setTaskList();
        item->setChecked(checked);
    }

    bool fensedCode = false;
    typename Trait::String startOfCode;
    bool wasEmptyLine = false;

    std::vector<std::pair<RawHtmlBlock<Trait>, long long int>> htmlToAdd;
    long long int line = -1;

    auto parseStream = [&] (StringListStream<Trait> &stream)
    {
        const auto tmpHtml = html;
        html = parse(stream, item, doc, linksToParse, workingPath, fileName, collectRefLinks, false, true);
        html.m_topParent = tmpHtml.m_topParent;
        html.m_blocks = tmpHtml.m_blocks;
        html.m_toAdjustLastPos = tmpHtml.m_toAdjustLastPos;
    };

    for (auto last = fr.m_data.end(); it != last; ++it, ++pos) {
        if (!fensedCode) {
            fensedCode = isCodeFences<Trait>(it->first.asString().startsWith(
                typename Trait::String(indent, Trait::latin1ToChar(' '))) ?
                    it->first.asString().sliced(indent) : it->first.asString());

            if (fensedCode) {
                startOfCode = startSequence<Trait>(it->first.asString());
            }
        } else if (fensedCode &&
                   isCodeFences<Trait>(it->first.asString().startsWith(
                        typename Trait::String(indent, Trait::latin1ToChar(' '))) ?
                            it->first.asString().sliced(indent) : it->first.asString(),
                        true) && startSequence<Trait>(it->first.asString()).contains(startOfCode)) {
            fensedCode = false;
        }

        if (!fensedCode) {
            long long int newIndent = 0;
            bool ok = false;

            std::tie(ok, newIndent, std::ignore, wasText) = listItemData<Trait>(
                it->first.asString().startsWith(typename Trait::String(indent, Trait::latin1ToChar(' '))) ?
                    it->first.asString().sliced(indent) : it->first.asString(),
                wasText);

            if (ok) {
                StringListStream<Trait> stream(data);

                parseStream(stream);

                data.clear();

                if (html.m_html.get()) {
                    html.m_parent = html.findParent(html.m_html->startColumn());

                    if (!html.m_parent) {
                        html.m_parent = html.m_topParent;
                    }

                    if (html.m_continueHtml) {
                        MdBlock<Trait> tmp;
                        tmp.m_emptyLineAfter = fr.m_emptyLineAfter;
                        std::copy(it, last, std::back_inserter(tmp.m_data));

                        parseText(tmp, html.m_parent, doc, linksToParse, workingPath, fileName,
                            collectRefLinks, html);

                        break;
                    }

                    htmlToAdd.push_back({html, html.m_parent->items().size()});
                    updateLastPosInList<Trait>(html);
                    resetHtmlTag<Trait>(html);
                }

                if (!htmlToAdd.empty() && htmlToAdd.back().first.m_parent == html.m_topParent) {
                    line = it->second.m_lineNumber;

                    break;
                } else {
                    typename MdBlock<Trait>::Data nestedList;
                    nestedList.push_back(*it);
                    ++it;

                    wasEmptyLine = false;

                    for (; it != last; ++it) {
                        const auto ns = skipSpaces<Trait>(0, it->first.asString());
                        std::tie(ok, std::ignore, std::ignore, wasText) =
                            listItemData<Trait>((ns >= indent ? it->first.asString().sliced(indent) :
                                it->first.asString()), wasText);

                        if (ok) {
                            wasEmptyLine = false;
                        }

                        if (ok || ns >= indent + newIndent || ns == it->first.length() || !wasEmptyLine) {
                            nestedList.push_back(*it);
                        } else {
                            break;
                        }

                        wasEmptyLine = (ns == it->first.length());

                        wasText = (wasEmptyLine ? false : wasText);
                    }

                    for (auto it = nestedList.begin(), last = nestedList.end(); it != last; ++it) {
                        it->first = it->first.sliced(std::min(skipSpaces<Trait>(
                            0, it->first.asString()), indent));
                    }

                    while (!nestedList.empty() &&
                        nestedList.back().first.asString().simplified().isEmpty()) {
                        nestedList.pop_back();
                    }

                    MdBlock<Trait> block = {nestedList, 0};

                    line = parseList(block, item, doc, linksToParse, workingPath, fileName,
                        collectRefLinks, html);

                    if (line >= 0) {
                        break;
                    }

                    for (; it != last; ++it) {
                        if (it->first.asString().startsWith(typename Trait::String(
                            indent, Trait::latin1ToChar(' ')))) {
                            it->first = it->first.sliced(indent);
                        }

                        data.push_back(*it);
                    }

                    break;
                }
            } else {
                if (it->first.asString().startsWith(typename Trait::String(
                    indent, Trait::latin1ToChar(' ')))) {
                    it->first = it->first.sliced(indent);
                }

                data.push_back(*it);

                wasEmptyLine = (skipSpaces<Trait>(0, it->first.asString()) == it->first.length());

                wasText = !wasEmptyLine;
            }
        } else {
            if (it->first.asString().startsWith(typename Trait::String(
                indent, Trait::latin1ToChar(' ')))) {
                it->first = it->first.sliced(indent);
            }

            data.push_back(*it);
        }
    }

    if (!data.empty()) {
        StringListStream<Trait> stream(data);

        parseStream(stream);

        if (html.m_html) {
            html.m_parent = html.findParent(html.m_html->startColumn());

            if (!html.m_parent) {
                html.m_parent = html.m_topParent;
            }
        }
    }

    if (!collectRefLinks) {
        parent->appendItem(item);

        long long int i = 0;

        for (auto &h : htmlToAdd) {
            if (h.first.m_parent != h.first.m_topParent) {
                h.first.m_parent->insertItem(h.second + i, h.first.m_html);

                ++i;

                updateLastPosInList(h.first);
            } else {
                html = h.first;

                break;
            }
        }

        long long int htmlStartColumn = -1;
        long long int htmlStartLine = -1;

        if (html.m_html) {
            std::tie(htmlStartColumn, htmlStartLine) =
                localPosFromVirgin<Trait>(fr, html.m_html->startColumn(), html.m_html->startLine());
        }

        long long int localLine = (html.m_html ? htmlStartLine : fr.m_data.size() - 1);

        if (html.m_html) {
            if (skipSpaces<Trait>(0, fr.m_data[localLine].first.asString()) >= htmlStartColumn) {
                --localLine;
            }
        }

        const auto lastLine = fr.m_data[localLine].second.m_lineNumber;

        const auto lastColumn = fr.m_data[localLine].first.virginPos(
            fr.m_data[localLine].first.length() ? fr.m_data[localLine].first.length() - 1 : 0);

        item->setEndColumn(lastColumn);
        item->setEndLine(lastLine);
        parent->setEndColumn(lastColumn);
        parent->setEndLine(lastLine);
    }

    if (resItem) {
        *resItem = item;
    }

    html.m_blocks.pop_back();

    return line;
}

template<class Trait>
inline void
Parser<Trait>::parseCode(MdBlock<Trait> &fr,
                         std::shared_ptr<Block<Trait>> parent,
                         bool collectRefLinks)
{
    if (!collectRefLinks) {
        const auto indent = skipSpaces<Trait>(0, fr.m_data.front().first.asString());

        if (indent != fr.m_data.front().first.length()) {
            WithPosition startDelim, endDelim, syntaxPos;
            typename Trait::String syntax;
            isStartOfCode<Trait>(fr.m_data.front().first.asString(), &syntax, &startDelim, &syntaxPos);
            syntax = replaceEntity<Trait>(syntax);
            startDelim.setStartLine(fr.m_data.front().second.m_lineNumber);
            startDelim.setEndLine(startDelim.startLine());
            startDelim.setStartColumn(fr.m_data.front().first.virginPos(startDelim.startColumn()));
            startDelim.setEndColumn(fr.m_data.front().first.virginPos(startDelim.endColumn()));

            if (syntaxPos.startColumn() != -1) {
                syntaxPos.setStartLine(startDelim.startLine());
                syntaxPos.setEndLine(startDelim.startLine());
                syntaxPos.setStartColumn(fr.m_data.front().first.virginPos(syntaxPos.startColumn()));
                syntaxPos.setEndColumn(fr.m_data.front().first.virginPos(syntaxPos.endColumn()));
            }

            const long long int startPos = fr.m_data.front().first.virginPos(indent);
            const long long int emptyColumn = fr.m_data.front().first.virginPos(fr.m_data.front().first.length());
            const long long int startLine = fr.m_data.front().second.m_lineNumber;
            const long long int endPos = fr.m_data.back().first.virginPos(fr.m_data.back().first.length() - 1);
            const long long int endLine = fr.m_data.back().second.m_lineNumber;

            fr.m_data.erase(fr.m_data.cbegin());

            {
                const auto it = std::prev(fr.m_data.cend());

                if (it->second.m_lineNumber > -1) {
                    endDelim.setStartColumn(it->first.virginPos(skipSpaces<Trait>(0, it->first.asString())));
                    endDelim.setStartLine(it->second.m_lineNumber);
                    endDelim.setEndLine(endDelim.startLine());
                    endDelim.setEndColumn(it->first.virginPos(it->first.length() - 1));
                }

                fr.m_data.erase(it);
            }

            if (syntax.toLower() == Trait::latin1ToString("math")) {
                typename Trait::String math;
                bool first = true;

                for (const auto &l : std::as_const(fr.m_data)) {
                    if (!first) {
                        math.push_back(Trait::latin1ToChar('\n'));
                    }

                    math.push_back(l.first.virginString());

                    first = false;
                }

                if (!collectRefLinks) {
                    std::shared_ptr<Paragraph<Trait>> p(new Paragraph<Trait>);
                    p->setStartColumn(startPos);
                    p->setStartLine(startLine);
                    p->setEndColumn(endPos);
                    p->setEndLine(endLine);

                    std::shared_ptr<Math<Trait>> m(new Math<Trait>);

                    if (!fr.m_data.empty()) {
                        m->setStartColumn(fr.m_data.front().first.virginPos(0));
                        m->setStartLine(fr.m_data.front().second.m_lineNumber);
                        m->setEndColumn(fr.m_data.back().first.virginPos(fr.m_data.back().first.length() - 1));
                        m->setEndLine(fr.m_data.back().second.m_lineNumber);
                    } else {
                        m->setStartColumn(emptyColumn);
                        m->setStartLine(startLine);
                        m->setEndColumn(emptyColumn);
                        m->setEndLine(startLine);
                    }

                    m->setInline(false);
                    m->setExpr(math);
                    m->setStartDelim(startDelim);
                    m->setEndDelim(endDelim);
                    m->setSyntaxPos(syntaxPos);
                    m->setFensedCode(true);
                    p->appendItem(m);

                    parent->appendItem(p);
                }
            } else {
                parseCodeIndentedBySpaces(fr, parent, collectRefLinks, indent, syntax, emptyColumn,
                    startLine, true, startDelim, endDelim, syntaxPos);
            }
        }
    }
}

template<class Trait>
inline void
Parser<Trait>::parseCodeIndentedBySpaces(MdBlock<Trait> &fr,
                                         std::shared_ptr<Block<Trait>> parent,
                                         bool collectRefLinks,
                                         int indent,
                                         const typename Trait::String &syntax,
                                         long long int emptyColumn,
                                         long long int startLine,
                                         bool fensedCode,
                                         const WithPosition &startDelim,
                                         const WithPosition &endDelim,
                                         const WithPosition &syntaxPos)
{
    if (!collectRefLinks) {
        typename Trait::String code;
        long long int startPos = 0;
        bool first = true;

        for (const auto &l : std::as_const(fr.m_data)) {
            const auto ns = skipSpaces<Trait>(0, l.first.asString());
            if (first) {
                startPos = ns;
            }
            first = false;

            code.push_back((indent > 0 ? l.first.virginString(ns < indent ? ns : indent) +
                    typename Trait::String(Trait::latin1ToChar('\n')) :
                typename Trait::String(l.first.virginString()) +
                    typename Trait::String(Trait::latin1ToChar('\n'))));
        }

        if (!code.isEmpty()) {
            code.remove(code.length() - 1, 1);
        }

        std::shared_ptr<Code<Trait>> codeItem(new Code<Trait>(code, fensedCode, false));
        codeItem->setSyntax(syntax);
        codeItem->setStartDelim(startDelim);
        codeItem->setEndDelim(endDelim);
        codeItem->setSyntaxPos(syntaxPos);

        if (!fr.m_data.empty()) {
            codeItem->setStartColumn(fr.m_data.front().first.virginPos(startPos));
            codeItem->setStartLine(fr.m_data.front().second.m_lineNumber);
            codeItem->setEndColumn(fr.m_data.back().first.virginPos(fr.m_data.back().first.length() - 1));
            codeItem->setEndLine(fr.m_data.back().second.m_lineNumber);
        } else {
            codeItem->setStartColumn(emptyColumn);
            codeItem->setStartLine(startLine);
            codeItem->setEndColumn(emptyColumn);
            codeItem->setEndLine(startLine);
        }

        if (fensedCode) {
            parent->appendItem(codeItem);
        } else if (!parent->items().empty() && parent->items().back()->type() == ItemType::Code) {
            auto c = std::static_pointer_cast<Code<Trait>>(parent->items().back());

            if (!c->isFensedCode()) {
                auto line = c->endLine();
                auto text = c->text();

                for (; line < codeItem->startLine(); ++line) {
                    text.push_back(Trait::latin1ToString("\n"));
                }

                text.push_back(codeItem->text());
                c->setText(text);
                c->setEndColumn(codeItem->endColumn());
                c->setEndLine(codeItem->endLine());
            } else {
                parent->appendItem(codeItem);
            }
        } else {
            parent->appendItem(codeItem);
        }
    }
}

} /* namespace MD */

#endif // MD4QT_MD_PARSER_HPP_INCLUDED

/*
    SPDX-FileCopyrightText: 2022-2025 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

#ifndef MD4QT_MD_TRAITS_HPP_INCLUDED
#define MD4QT_MD_TRAITS_HPP_INCLUDED

#ifdef MD4QT_QT_SUPPORT

// C++ include.
#include <map>
#include <memory>
#include <variant>

// Qt include.
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QVector>

#endif // MD4QT_QT_SUPPORT

namespace MD
{

/*!
 * \namespace MD::impl
 * \inmodule md4qt
 * \inheaderfile md4qt/traits.h
 *
 * \brief Namespace for some implemetation details, shouldn't be needed by a developers.
 *
 * Namespace with some implementation details, in most cases shouldn't be needed by developers.
 */
namespace impl
{

/*!
 * \inheaderfile md4qt/traits.h
 *
 * Returns a length of a string defined by string variant.
 *
 * \a v String variant.
 */
template<class StringVariant, class String, class StringView>
inline long long int length(const StringVariant &v)
{
    if (std::holds_alternative<String>(v)) {
        return std::get<String>(v).length();
    } else {
        return std::get<StringView>(v).length();
    }
}

/*!
 * \inheaderfile md4qt/traits.h
 *
 * Returns a character at a given position.
 *
 * \a v String variant.
 *
 * \a position Position of a character.
 */
template<class StringVariant, class String, class StringView, class Char>
inline Char getChar(const StringVariant &v, long long int position)
{
    if (std::holds_alternative<String>(v)) {
        return std::get<String>(v).at(position);
    } else {
        return std::get<StringView>(v).at(position);
    }
}

/*!
 * \inheaderfile md4qt/traits.h
 *
 * Returns a sliced string variant.
 *
 * \a v String variant.
 *
 * \a pos Start position.
 *
 * \a n Length.
 */
template<class StringVariant, class String, class StringView>
inline StringVariant slice(const StringVariant &v, long long int pos, long long int n = -1)
{
    if(n == -1) {
        n = length<StringVariant, String, StringView>(v) - pos;
    }

    if (std::holds_alternative<String>(v)) {
        return std::get<String>(v).sliced(pos, n);
    } else {
        return std::get<StringView>(v).sliced(pos, n);
    }
}

} /* namespace impl */

/*!
 * \class MD::InternalStringT
 * \inmodule md4qt
 * \inheaderfile md4qt/traits.h
 *
 * \brief Internal string, used to get virgin (original) string from transformed string.
 *
 * Actually this is a wrapper around string implemented to have ability to get virgin string/substring
 * after implemented modifications. In other words - if from string "abc" remove letter "b" and then
 * ask for virgin substring starting at position 0 and length 2, this class will give you virgin
 * "abc" string.
 */
template<class String, class StringView, class Char, class Latin1Char>
class InternalStringT
{
public:
    /*!
     * Default constructor.
     */
    InternalStringT()
    {
    }

    /*!
     * Constructor from string view.
     */
    InternalStringT(StringView s)
        : m_virginStr(s)
    {
        if (!s.isEmpty()) {
            m_str.push_back(s);
            m_pos.push_back(0);
        }
    }

    /*!
     * Returns full virgin string.
     */
    StringView fullVirginString() const
    {
        return m_virginStr;
    }

    /*!
     * Returns a length of this string.
     */
    long long int length() const
    {
        return (m_str.empty() ? 0 : m_pos.back() + impl::length<StringVariant, String, StringView>(m_str.back()));
    }

    /*!
     * Returns virgin sub-string with position and length in the transformed string.
     *
     * \a pos Position.
     *
     * \a len Length.
     */
    String virginSubString(long long int pos = 0, long long int len = -1) const
    {
        if (pos < 0) {
            pos = 0;
        }

        if (pos + len > length() || len < 0) {
            len = length() - pos;
        }

        if (len == 0) {
            return (isEmpty() ? m_virginStr : String());
        }

        auto virginStartPos = virginPos(pos);
        String startStr, endStr;

        if (m_virginStr[virginStartPos] == Latin1Char('\t')) {
            const auto spaces = countOfSpacesForTab(virginStartPos);

            for (long long int i = 1; i < spaces; ++i) {
                if (virginPos(pos + i) != virginStartPos) {
                    startStr = String(i, Latin1Char(' '));
                    ++virginStartPos;
                    break;
                }
            }
        }

        auto virginEndPos = virginPos(pos + len - 1, true);

        if (m_virginStr[virginEndPos] == Latin1Char('\t')) {
            const auto spaces = countOfSpacesForTab(virginEndPos);

            for (long long int i = 1; i < spaces; ++i) {
                if (virginPos(pos + len - 1 - i) != virginEndPos) {
                    endStr = String(i, Latin1Char(' '));
                    --virginEndPos;
                    break;
                }
            }
        }

        return startStr + m_virginStr.sliced(virginStartPos, virginEndPos - virginStartPos + 1) + endStr;
    }

    /*!
     * Returns virgin position from transformed.
     *
     * \a pos Transformed position.
     *
     * \a end If true will be return last virgin position before transformation.
     *        For example if in virgin string 2 characters were replaced with 1,
     *        we will receive position of second character if \a end is true.
     */
    long long int virginPos(long long int pos,
                            bool end = false) const
    {
        for (auto it = m_changedPos.crbegin(), last = m_changedPos.crend(); it != last; ++it) {
            pos = virginPosImpl(pos, *it, end);
        }

        return pos;
    }

    /*!
     * Returns character at a given \a position position.
     *
     * \a position Position.
     */
    Char operator[](long long int position) const
    {
        const auto it = std::prev(std::upper_bound(m_pos.cbegin(), m_pos.cend(), position));

        if (it != m_pos.cend()) {
            return impl::getChar<StringVariant, String, StringView, Char>(
                        m_str[std::distance(m_pos.cbegin(), it)], position - (*it));
        } else {
            return {};
        }
    }

    /*!
     * Replace substring.
     *
     * \a pos Position.
     *
     * \a size Length.
     *
     * \a with Value to insert.
     */
    InternalStringT &replaceOne(long long int pos, long long int size, const String &with)
    {
        const auto oldLength = length();

        auto it = std::prev(std::upper_bound(m_pos.begin(), m_pos.end(), pos));

        if (size > length() - pos) {
            size = length() - pos;
        }

        const auto replacedSize = size;

        if (it != m_pos.end()) {
            const auto start = std::distance(m_pos.begin(), it);
            auto old = m_str[start];
            auto idx = start;
            auto end = -1;

            if (pos > *it) {
                m_str[idx] = impl::slice<StringVariant, String, StringView>(old, 0, pos - (*it));
                ++idx;
            }

            old = impl::slice<StringVariant, String, StringView>(old, pos - (*it));

            while (size > 0) {
                const auto len = impl::length<StringVariant, String, StringView>(old);

                if (len < size) {
                    size -= len;

                    if (end != -1) {
                        ++end;
                    } else {
                        end = idx;
                    }
                } else if (len == size) {
                    old = {};
                    break;
                } else if (len > size) {
                    old = impl::slice<StringVariant, String, StringView>(old, size);
                    break;
                }

                if (end < static_cast<long long int>(m_str.size())) {
                    old = m_str[end];
                } else {
                    old = {};
                    break;
                }
            }

            if (end > idx && idx < static_cast<long long int>(m_pos.size())) {
                m_pos.erase(m_pos.begin() + idx, m_pos.begin() + end + 1);
                m_str.erase(m_str.begin() + idx, m_str.begin() + end + 1);
            }

            bool removed = false;

            if (!with.isEmpty()) {
                if (start != idx) {
                    m_str.insert(m_str.begin() + idx, with);
                    m_pos.insert(m_pos.begin() + idx, pos);
                } else {
                    m_str[idx] = with;
                    m_pos[idx] = pos;
                }

                ++idx;
            } else {
                if (start == idx) {
                    m_pos.erase(m_pos.begin() + idx);
                    m_str.erase(m_str.begin() + idx);

                    removed = true;
                }
            }

            auto updatedPos = pos + with.length();

            if (impl::length<StringVariant, String, StringView>(old)) {
                m_pos.insert(m_pos.begin() + idx, updatedPos);
                m_str.insert(m_str.begin() + idx, old);

                ++idx;
            } else {
                updatedPos = (removed ? (idx - 1 >= 0 ? m_pos[idx - 1] : 0) : pos);
            }

            for (; idx < static_cast<long long int>(m_pos.size()); ++idx) {
                updatedPos += (idx - 1 >= 0 ? impl::length<StringVariant, String, StringView>(m_str[idx - 1]) : 0);
                m_pos[idx] = updatedPos;
            }
        }

        if (with.length() != replacedSize) {
            m_changedPos.push_back({{0, oldLength}, {}});
            m_changedPos.back().second.push_back({pos, replacedSize, with.size()});
        }

        return *this;
    }

    /*!
     * Find string.
     *
     * \a what What to find.
     *
     * \a from Start position.
     */
    long long int indexOf(const String &what, long long int from = 0)
    {
        if (from < 0 || from >= length()) {
            return -1;
        }

        if (what.isEmpty()) {
            return 0;
        }

        for (long long int i = from; i <= length() - what.length(); ++i) {
            if (this->operator [](i) == what[0]) {
                bool match = true;
                for (long long int j = 1; j < what.length(); ++j) {
                    if (this->operator [](i + j) != what[j]) {
                        match = false;
                        break;
                    }
                }

                if (match) {
                    return i;
                }
            }
        }

        return -1;
    }

    /*!
     * Replace string.
     *
     * \a what What to replace.
     *
     * \a with Value to insert.
     */
    InternalStringT &replace(const String &what, const String &with)
    {
        long long int pos = 0;

        while ((pos = indexOf(what, pos)) != -1) {
            replaceOne(pos, what.length(), with);
            pos += with.length();
        }

        return *this;
    }

    /*!
     * Remove sub-string.
     *
     * \a pos Position.
     *
     * \a size Length.
     */
    InternalStringT &remove(long long int pos, long long int size)
    {
        return replaceOne(pos, size, {});
    }

    /*!
     * Returns whether this string empty?
     */
    bool isEmpty() const
    {
        return m_str.empty();
    }

    /*!
     * Returns simplified string.
     */
    InternalStringT simplified() const
    {
        if (isEmpty()) {
            return *this;
        }

        const auto len = length();

        InternalStringT result = *this;
        result.m_str.clear();
        result.m_pos.clear();

        long long int i = 0;
        bool init = false;
        bool first = true;
        long long int spaces = 0;
        long long int pos = 0;

        while (true) {
            long long int tmp = i;

            while (i < length() && this->operator [](i).isSpace()) {
                ++i;
            }

            spaces = i - tmp;

            if (i != tmp) {
                if (!init) {
                    result.m_changedPos.push_back({{0, len}, {}});
                    init = true;
                }

                if (i - tmp > 1 || first) {
                    result.m_changedPos.back().second.push_back({tmp, i - tmp, (first ? 0 : 1)});
                }
            }

            first = false;

            String word;

            while (i != len && !this->operator [](i).isSpace()) {
                word.push_back(this->operator [](i));
                ++i;
            }

            if (!word.isEmpty()) {
                result.m_str.push_back(word);
                result.m_pos.push_back(pos);
                pos += word.length();
            }

            if (i == len) {
                break;
            }

            result.m_str.push_back(String(Latin1Char(' ')));
            result.m_pos.push_back(pos);
            ++pos;
        }

        if (!result.isEmpty() && result[result.length() - 1] == Latin1Char(' ')) {
            result.remove(result.length() - 1, 1);

            if (spaces > 1) {
                result.m_changedPos.back().second.back().m_len = 0;
            } else if (spaces == 1) {
                result.m_changedPos.back().second.push_back({length() - spaces, spaces, 0});
            }
        }

        return result;
    }

    /*!
     * Split string.
     *
     * \a sep Separator.
     */
    std::vector<InternalStringT> split(const InternalStringT &sep) const
    {
        std::vector<InternalStringT> result;
        const auto len = m_str.length();

        if (sep.isEmpty()) {
            for (long long int i = 0; i < m_str.length(); ++i) {
                auto is = *this;
                is.m_str = m_str[i];
                is.m_changedPos.push_back({{i, len}, {}});

                result.push_back(is);
            }

            return result;
        }

        long long int pos = 0;
        long long int fpos = 0;

        while ((fpos = m_str.indexOf(sep.asString(), pos)) != -1 && fpos < length()) {
            if (fpos - pos > 0) {
                auto is = *this;
                is.m_str = m_str.sliced(pos, fpos - pos);
                is.m_changedPos.push_back({{pos, len}, {}});

                result.push_back(is);
            }

            pos = fpos + sep.length();
        }

        if (pos < m_str.length()) {
            auto is = *this;
            is.m_str = m_str.sliced(pos, m_str.length() - pos);
            is.m_changedPos.push_back({{pos, len}, {}});

            result.push_back(is);
        }

        return result;
    }

    /*!
     * Returns sliced sub-string.
     *
     * \a pos Position.
     *
     * \a len Length.
     */
    InternalStringT sliced(long long int pos, long long int len = -1) const
    {
        InternalStringT tmp = *this;
        const auto oldLen = m_str.length();
        tmp.m_str = tmp.m_str.sliced(pos, (len == -1 ? tmp.m_str.length() - pos : len));
        tmp.m_changedPos.push_back({{pos, oldLen}, {}});
        if (len != -1 && len < length() - pos) {
            tmp.m_changedPos.back().second.push_back({pos + len, length() - pos - len, 0});
        }

        return tmp;
    }

    /*!
     * Returns a substring that contains the \a n rightmost characters of the string.
     *
     * \a n Count of characters.
     */
    InternalStringT right(long long int n) const
    {
        InternalStringT tmp = *this;
        const auto len = m_str.length();
        tmp.m_str = tmp.m_str.right(n);
        tmp.m_changedPos.push_back({{length() - n, len}, {}});

        return tmp;
    }

    /*!
     * Insert one character.
     *
     * \a pos Position.
     *
     * \a ch Character.
     */
    InternalStringT &insert(long long int pos, Char ch)
    {
        return insert(pos, String(1, ch));
    }

    /*!
     * Insert string.
     *
     * \a pos Position.
     *
     * \a s String.
     */
    InternalStringT &insert(long long int pos, const String &s)
    {
        const auto len = m_str.length();
        const auto ilen = s.length();

        m_str.insert(pos, s);

        m_changedPos.push_back({{0, len}, {}});
        m_changedPos.back().second.push_back({pos, 1, ilen + 1});

        return *this;
    }

private:
    /*!
     * \typealias MD::InternalStringT::StringVariant
     * \inmodule md4qt
     * \inheaderfile md4qt/traits.h
     *
     * \brief Holder of actual string - variant with string or string view.
     */
    using StringVariant = std::variant<String, StringView>;

    /*!
     * Transformed string.
     */
    std::vector<StringVariant> m_str;
    /*!
     * Positions of sub-parts of transformed string.
     */
    std::vector<long long int> m_pos;
    /*!
     * Virgin (original) string.
     */
    StringView m_virginStr;

    /*!
     * Auxiliary struct to store information about transformation.
     */
    struct ChangedPos {
        long long int m_pos = -1;
        long long int m_oldLen = -1;
        long long int m_len = -1;
    };

    /*!
     * Auxiliary struct to store information about transformation.
     */
    struct LengthAndStartPos {
        long long int m_firstPos = 0;
        long long int m_length = 0;
    };

    /*!
     * Information about transformations.
     */
    std::vector<std::pair<LengthAndStartPos, std::vector<ChangedPos>>> m_changedPos;

private:
    long long int virginPosImpl(long long int pos,
                                const std::pair<LengthAndStartPos, std::vector<ChangedPos>> &changed,
                                bool end) const
    {
        for (const auto &c : changed.second) {
            const auto startPos = c.m_pos;
            const auto endPos = startPos + c.m_len - 1;

            if (pos >= startPos && pos <= endPos) {
                const auto oldEndPos = startPos + c.m_oldLen - 1;

                if (pos > oldEndPos || end) {
                    return oldEndPos + changed.first.m_firstPos;
                } else {
                    return pos + changed.first.m_firstPos;
                }
            } else if (pos > endPos) {
                pos += c.m_oldLen - c.m_len;
            } else {
                break;
            }
        }

        pos += changed.first.m_firstPos;

        return (pos > changed.first.m_length ? changed.first.m_length : pos);
    }

    long long int countOfSpacesForTab(long long int virginPos) const
    {
        long long int p = 0;

        for (const auto &v : std::as_const(m_changedPos)) {
            p += v.first.m_firstPos;

            if (virginPos < p) {
                break;
            }

            for (const auto &c : std::as_const(v.second)) {
                if (c.m_pos + p == virginPos) {
                    return c.m_len;
                }

                virginPos += (virginPos > c.m_pos ? c.m_len - c.m_oldLen : 0);
            }
        }

        return -1;
    }
}; // class InternalString

/*!
 * \inheaderfile md4qt/traits.h
 *
 * Comparison of MD::InternalStringT with other string.
 *
 * \a str First string to compare.
 *
 * \a other Other string to compare.
 */
template<class String, class StringView, class Char, class Latin1Char, class T>
bool operator == (const InternalStringT<String, StringView, Char, Latin1Char> &str, const T &other)
{
    const auto length = str.length();

    if (length != other.length() ) {
        return false;
    }

    for (long long int i = 0; i < length; ++i) {
        if (str[i] != other[i]) {
            return false;
        }
    }

    return true;
}

#ifdef MD4QT_QT_SUPPORT

//
// QStringTrait
//

/*!
 * \class MD::QStringTrait
 * \inmodule md4qt
 * \inheaderfile md4qt/traits.h
 *
 * \brief Trait to use this library with QString.
 *
 * Trait for \c {md4qt} library to work with QString as string.
 */
struct QStringTrait {
    template<class T>
    using Vector = QVector<T>;

    template<class T, class U>
    using Map = std::map<T, U>;

    using String = QString;

    using StringView = QStringView;

    using Char = QChar;

    using InternalString = InternalStringT<String, StringView, Char, QLatin1Char>;

    using InternalStringList = std::vector<InternalString>;

    using TextStream = QTextStream;

    using StringList = QStringList;

    using Url = QUrl;

    /*!
     * Returns whether Unicode whitespace?
     *
     * \a ch Character to check.
     */
    static bool isUnicodeWhitespace(const QChar &ch)
    {
        const auto c = ch.unicode();

        if (ch.category() == QChar::Separator_Space) {
            return true;
        } else if (c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D) {
            return true;
        } else {
            return false;
        }
    }

    /*!
     * Convert UTF-16 into trait's string.
     *
     * \a u16 String.
     */
    static String utf16ToString(const char16_t *u16)
    {
        return QString::fromUtf16(u16);
    }

    /*!
     * Convert Latin1 into trait's string.
     *
     * \a latin1 String.
     */
    static String latin1ToString(const char *latin1)
    {
        return QLatin1String(latin1);
    }

    /*!
     * Convert Latin1 char into trait's char.
     *
     * \a latin1 Character.
     */
    static Char latin1ToChar(char latin1)
    {
        return QLatin1Char(latin1);
    }

    /*!
     * Convert UTF8 into trait's string.
     *
     * \a utf8 UTF-8 string.
     */
    static String utf8ToString(const char *utf8)
    {
        return QString::fromUtf8(utf8, -1);
    }

    /*!
     * Returns whether file exist.
     *
     * \a fileName File name.
     *
     * \a workingPath Working path.
     */
    static bool fileExists(const String &fileName, const String &workingPath)
    {
        return QFileInfo::exists((workingPath.isEmpty() ?
                                    QString() : workingPath + latin1ToString("/")) + fileName);
    }

    /*!
     * Returns whether file exist.
     *
     * \a fileName File name.
     */
    static bool fileExists(const String &fileName)
    {
        return QFileInfo::exists(fileName);
    }

    /*!
     * Returns absolute file path.
     *
     * \a path Path.
     */
    static String absoluteFilePath(const String &path)
    {
        return QFileInfo(path).absoluteFilePath();
    }

    /*!
     * Add UCS4 to string.
     *
     * \a str String.
     *
     * \a ch Character to append.
     */
    static void appendUcs4(String &str, char32_t ch)
    {
        str += QChar::fromUcs4(ch);
    }

    /*!
     * Search for last occurrence of string.
     *
     * \a where String for checking.
     *
     * \a what What to look for?
     *
     * \a from Start position.
     */
    static long long int lastIndexOf(const String &where, const String &what, long long int from)
    {
        if (from < 0) {
            return -1;
        } else {
            return where.lastIndexOf(what, from);
        }
    }
}; // struct QStringTrait

#endif // MD4QT_QT_SUPPORT

} /* namespace MD */

#endif // MD4QT_MD_TRAITS_HPP_INCLUDED

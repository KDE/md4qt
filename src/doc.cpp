/*
    SPDX-FileCopyrightText: 2026 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// md4qt include.
#include "doc.h"
#include "constants.h"
#include "text_stream.h"

namespace MD
{

//
// WithPosition
//

WithPosition::WithPosition() = default;

WithPosition::~WithPosition() = default;

WithPosition::WithPosition(qsizetype startColumn,
                           qsizetype startLine,
                           qsizetype endColumn,
                           qsizetype endLine)
    : m_startColumn(startColumn)
    , m_startLine(startLine)
    , m_endColumn(endColumn)
    , m_endLine(endLine)
{
}

void WithPosition::applyPositions(const WithPosition &other)
{
    if (this != &other) {
        *this = other;
    }
}

qsizetype WithPosition::startColumn() const
{
    return m_startColumn;
}

qsizetype WithPosition::startLine() const
{
    return m_startLine;
}

qsizetype WithPosition::endColumn() const
{
    return m_endColumn;
}

qsizetype WithPosition::endLine() const
{
    return m_endLine;
}

void WithPosition::setStartColumn(qsizetype c)
{
    m_startColumn = c;
}

void WithPosition::setStartLine(qsizetype l)
{
    m_startLine = l;
}

void WithPosition::setEndColumn(qsizetype c)
{
    m_endColumn = c;
}

void WithPosition::setEndLine(qsizetype l)
{
    m_endLine = l;
}

bool WithPosition::isNullPositions() const
{
    return (m_startColumn == -1 || m_startLine == -1 || m_endColumn == -1 || m_endLine == -1);
}

bool operator==(const WithPosition &l,
                const WithPosition &r)
{
    return (l.startColumn() == r.startColumn()
            && l.startLine() == r.startLine()
            && l.endColumn() == r.endColumn()
            && l.endLine() == r.endLine());
}

//
// Item
//

Item::Item() = default;

Item::~Item() = default;

void Item::writeStartOfLine(QTextStream &) const
{
}

QTextStream &operator<<(QTextStream &stream,
                        const Item &item)
{
    SerialiseHelper helper;

    item.write(stream, &helper);

    return stream;
}

//
// SerialiseHelper
//

void SerialiseHelper::push(const Item *item)
{
    m_items.push(item);
}

void SerialiseHelper::pop()
{
    m_items.pop();
}

void SerialiseHelper::startLine(QTextStream &stream)
{
    for (const auto &item : std::as_const(m_items)) {
        item->writeStartOfLine(stream);
    }
}

bool SerialiseHelper::isFirst() const
{
    return m_first;
}

void SerialiseHelper::setFirst(bool on)
{
    m_first = on;
}

//
// StyleDelim
//

StyleDelim::StyleDelim(int s,
                       qsizetype startColumn,
                       qsizetype startLine,
                       qsizetype endColumn,
                       qsizetype endLine,
                       EmphasisSymbol symbol)
    : WithPosition(startColumn,
                   startLine,
                   endColumn,
                   endLine)
    , m_style(s)
    , m_symbol(symbol)
{
}

StyleDelim::~StyleDelim() = default;

int StyleDelim::style() const
{
    return m_style;
}

void StyleDelim::setStyle(int t)
{
    m_style = t;
}

EmphasisSymbol StyleDelim::symbol() const
{
    return m_symbol;
}

void StyleDelim::setSymbol(EmphasisSymbol s)
{
    m_symbol = s;
}

void StyleDelim::write(QTextStream &stream) const
{
    static const QString s_2Underline = QStringLiteral("__");
    static const QString s_2Asterisk = QStringLiteral("**");
    static const QString s_1Underline = QStringLiteral("_");
    static const QString s_1Asterisk = QStringLiteral("*");
    static const QString s_2Tilde = QStringLiteral("~~");
    static const QString s_1Tilde = QStringLiteral("~");

    switch (m_style) {
    case BoldText: {
        switch (symbol()) {
        case EmphasisSymbol::Underline: {
            stream << s_2Underline;
        } break;

        case EmphasisSymbol::Asterisk:
        default: {
            stream << s_2Asterisk;
        } break;
        }
    } break;

    case ItalicText: {
        switch (symbol()) {
        case EmphasisSymbol::Underline: {
            stream << s_1Underline;
        } break;

        case EmphasisSymbol::Asterisk:
        default: {
            stream << s_1Asterisk;
        } break;
        }
    } break;

    case StrikethroughText: {
        if (endColumn() - startColumn() > 0) {
            stream << s_2Tilde;
        } else {
            stream << s_1Tilde;
        }
    } break;

    default:
        break;
    }
}

bool operator==(const StyleDelim &l,
                const StyleDelim &r)
{
    return (static_cast<WithPosition>(l) == static_cast<WithPosition>(r) && l.style() == r.style());
}

//
// ItemWithOpts
//

ItemWithOpts::ItemWithOpts() = default;

ItemWithOpts::~ItemWithOpts() = default;

void ItemWithOpts::applyItemWithOpts(const ItemWithOpts &other)
{
    if (this != &other) {
        WithPosition::applyPositions(other);
        m_opts = other.m_opts;
        m_openStyles = other.m_openStyles;
        m_closeStyles = other.m_closeStyles;
    }
}

int ItemWithOpts::opts() const
{
    return m_opts;
}

void ItemWithOpts::setOpts(int o)
{
    m_opts = o;
}

const ItemWithOpts::Styles &ItemWithOpts::openStyles() const
{
    return m_openStyles;
}

void ItemWithOpts::setOpenStyles(const Styles &s)
{
    m_openStyles = s;
}

void ItemWithOpts::appendOpenStyle(const StyleDelim &s)
{
    m_openStyles.append(s);
}

void ItemWithOpts::appendOpenStyles(const Styles &s)
{
    m_openStyles.append(s);
}

const ItemWithOpts::Styles &ItemWithOpts::closeStyles() const
{
    return m_closeStyles;
}

void ItemWithOpts::setCloseStyles(const Styles &s)
{
    m_closeStyles = s;
}

void ItemWithOpts::appendCloseStyle(const StyleDelim &s)
{
    m_closeStyles.append(s);
}

void ItemWithOpts::appendCloseStyles(const Styles &s)
{
    m_closeStyles.append(s);
}

void ItemWithOpts::writeOpenStyles(QTextStream &stream) const
{
    for (const auto &s : openStyles()) {
        s.write(stream);
    }
}

void ItemWithOpts::writeCloseStyles(QTextStream &stream) const
{
    for (const auto &s : closeStyles()) {
        s.write(stream);
    }
}

//
// PageBreak
//

PageBreak::PageBreak() = default;

PageBreak::~PageBreak() = default;

ItemType PageBreak::type() const
{
    return ItemType::PageBreak;
}

Item::SharedPointer PageBreak::clone(Document *doc) const
{
    Q_UNUSED(doc)

    return QSharedPointer<PageBreak>::create();
}

void PageBreak::write(QTextStream &,
                      SerialiseHelper *) const
{
}

//
// HorizontalLine
//

HorizontalLine::HorizontalLine(HorizontalLineSymbol s)
    : m_symbol(s)
{
}

HorizontalLine::~HorizontalLine() = default;

ItemType HorizontalLine::type() const
{
    return ItemType::HorizontalLine;
}

Item::SharedPointer HorizontalLine::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto h = QSharedPointer<HorizontalLine>::create();
    h->applyPositions(*this);
    h->setSymbol(symbol());

    return h;
}

HorizontalLineSymbol HorizontalLine::symbol() const
{
    return m_symbol;
}

void HorizontalLine::setSymbol(HorizontalLineSymbol s)
{
    m_symbol = s;
}

void HorizontalLine::write(QTextStream &stream,
                           SerialiseHelper *) const
{
    static const QString s_asteriskLine = QStringLiteral("***");
    static const QString s_underlineLine = QStringLiteral("___");
    static const QString s_dashLine = QStringLiteral("---");

    switch (symbol()) {
    case HorizontalLineSymbol::Underline: {
        stream << s_underlineLine;
    } break;

    case HorizontalLineSymbol::Dash: {
        stream << s_dashLine;
    } break;

    case HorizontalLineSymbol::Asterisk:
    default: {
        stream << s_asteriskLine;
    } break;
    }
}

//
// Anchor
//

Anchor::Anchor(const QString &l)
    : m_label(l)
{
}

Anchor::~Anchor() = default;

Item::SharedPointer Anchor::clone(Document *doc) const
{
    Q_UNUSED(doc)

    return QSharedPointer<Anchor>::create(m_label);
}

ItemType Anchor::type() const
{
    return ItemType::Anchor;
}

const QString &Anchor::label() const
{
    return m_label;
}

void Anchor::setLabel(const QString &l)
{
    m_label = l;
}

void Anchor::write(QTextStream &,
                   SerialiseHelper *) const
{
}

//
// RawHtml
//

RawHtml::RawHtml() = default;

RawHtml::~RawHtml() = default;

Item::SharedPointer RawHtml::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto h = QSharedPointer<RawHtml>::create();
    h->applyItemWithOpts(*this);
    h->setText(m_text);

    return h;
}

ItemType RawHtml::type() const
{
    return ItemType::RawHtml;
}

const QString &RawHtml::text() const
{
    return m_text;
}

void RawHtml::setText(const QString &t)
{
    m_text = t;
}

void writeMultiline(const QString &text,
                    QTextStream &stream,
                    SerialiseHelper *helper)
{
    QString data = text;

    QTextStream tmp(&data);
    TextStream s(tmp);
    bool firstLine = true;

    while (!s.atEnd()) {
        if (!firstLine) {
            stream << s_newLineChar;
            helper->startLine(stream);
        }

        auto line = s.readLine();
        stream << line.view();
        firstLine = false;
    }
}

void RawHtml::write(QTextStream &stream,
                    SerialiseHelper *helper) const
{
    writeOpenStyles(stream);

    writeMultiline(text(), stream, helper);

    writeCloseStyles(stream);
}

//
// Text
//

Text::Text() = default;

Text::~Text() = default;

void Text::applyText(const Text &t)
{
    if (this != &t) {
        ItemWithOpts::applyItemWithOpts(t);
        setText(t.text());
        setMarkdownContent(t.markdownContent());
    }
}

Item::SharedPointer Text::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto t = QSharedPointer<Text>::create();
    t->applyText(*this);

    return t;
}

ItemType Text::type() const
{
    return ItemType::Text;
}

const QString &Text::text() const
{
    return m_text;
}

void Text::setText(const QString &t)
{
    m_text = t;
}

const QString &Text::markdownContent() const
{
    return m_markdown;
}

void Text::setMarkdownContent(const QString &v)
{
    m_markdown = v;
}

void Text::write(QTextStream &stream,
                 SerialiseHelper *helper) const
{
    writeOpenStyles(stream);

    QString data = (markdownContent().isEmpty() ? text() : markdownContent());

    for (qsizetype i = 0; i < data.size(); ++i) {
        if (!data[i].isSpace()) {
            if (data[i] == s_minusChar || data[i] == s_equalSignChar || data[i] == s_verticalLineChar) {
                data.insert(i, s_reverseSolidusChar);
            }

            if (helper->isFirst()) {
                data.remove(0, i);
            }

            break;
        }
    }

    stream << data;

    writeCloseStyles(stream);
}

//
// LineBreak
//

LineBreak::LineBreak() = default;

LineBreak::~LineBreak() = default;

Item::SharedPointer LineBreak::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto b = QSharedPointer<LineBreak>::create();
    b->applyText(*this);
    b->setSymbol(symbol());

    return b;
}

ItemType LineBreak::type() const
{
    return ItemType::LineBreak;
}

LineBreakType LineBreak::symbol() const
{
    return m_symbol;
}

void LineBreak::setSymbol(LineBreakType s)
{
    m_symbol = s;
}

void LineBreak::write(QTextStream &stream,
                      SerialiseHelper *) const
{
    static const QString s_spaces = QStringLiteral("  ");
    static const QString s_backslash = QStringLiteral("\\");

    switch (symbol()) {
    case LineBreakType::Backslash: {
        stream << s_backslash;
    } break;

    case LineBreakType::Spaces:
    default: {
        stream << s_spaces;
    } break;
    }
}

//
// Block
//

Block::Block() = default;

Block::~Block() = default;

void Block::applyBlock(const Block &other,
                       Document *doc)
{
    if (this != &other) {
        WithPosition::applyPositions(other);

        m_items.clear();

        for (const auto &i : other.items()) {
            appendItem(i->clone(doc));
        }
    }
}

const Block::Items &Block::items() const
{
    return m_items;
}

void Block::setItems(const Items &i)
{
    m_items = i;
}

void Block::insertItem(qsizetype idx,
                       Item::SharedPointer i)
{
    m_items.insert(m_items.cbegin() + idx, i);
}

void Block::appendItem(Item::SharedPointer i)
{
    m_items.push_back(i);
}

void Block::removeItemAt(qsizetype idx)
{
    if (idx >= 0 && idx < static_cast<qsizetype>(m_items.size())) {
        m_items.erase(m_items.cbegin() + idx);
    }
}

Item::SharedPointer Block::getItemAt(qsizetype idx) const
{
    return m_items.at(idx);
}

bool Block::isEmpty() const
{
    return m_items.empty();
}

void Block::write(QTextStream &stream,
                  SerialiseHelper *helper) const
{
    bool firstItem = true;

    for (const auto &item : items()) {
        if (item->type() != MD::ItemType::Anchor) {
            if (!firstItem) {
                stream << s_newLineChar;
                helper->startLine(stream);
                stream << s_newLineChar;
                helper->startLine(stream);
            }

            item->write(stream, helper);

            firstItem = false;
        }
    }
}

//
// Paragraph
//

Paragraph::Paragraph() = default;

Paragraph::~Paragraph() = default;

Item::SharedPointer Paragraph::clone(Document *doc) const
{
    auto p = QSharedPointer<Paragraph>::create();
    p->applyBlock(*this, doc);

    return p;
}

ItemType Paragraph::type() const
{
    return ItemType::Paragraph;
}

void Paragraph::write(QTextStream &stream,
                      SerialiseHelper *helper) const
{
    qsizetype lineNumber = -1;

    helper->setFirst(true);

    for (const auto &item : items()) {
        if (lineNumber == -1) {
            lineNumber = item->startLine();
        }

        if (item->startLine() != lineNumber) {
            stream << s_newLineChar;

            helper->startLine(stream);
        }

        item->write(stream, helper);

        lineNumber = item->endLine();

        helper->setFirst(false);
    }
}

//
// Heading
//

Heading::Heading()
    : m_text(new Paragraph)
    , m_type(HeadingType::Unknown)
{
}

Heading::~Heading() = default;

Item::SharedPointer Heading::clone(Document *doc) const
{
    auto h = QSharedPointer<Heading>::create();
    h->setHeadingType(headingType());
    h->applyPositions(*this);
    h->setText(m_text->clone(doc).staticCast<Paragraph>());
    h->setLevel(m_level);
    h->setLabel(m_label);
    h->setDelims(m_delims);
    h->setLabelPos(m_labelPos);
    h->setLabelVariants(m_labelVariants);
    h->setOriginalLabel(originalLabel());

    if (doc && isLabeled()) {
        for (const auto &label : std::as_const(m_labelVariants)) {
            doc->insertLabeledHeading(label, h);
        }
    }

    return h;
}

ItemType Heading::type() const
{
    return ItemType::Heading;
}

Heading::ParagraphSharedPointer Heading::text() const
{
    return m_text;
}

void Heading::setText(ParagraphSharedPointer t)
{
    m_text = t;
}

int Heading::level() const
{
    return m_level;
}

void Heading::setLevel(int l)
{
    m_level = l;
}

bool Heading::isLabeled() const
{
    return m_label.size() > 0;
}

const QString &Heading::label() const
{
    return m_label;
}

void Heading::setLabel(const QString &l)
{
    m_label = l;
}

const QString &Heading::originalLabel() const
{
    return m_originalLabel;
}

void Heading::setOriginalLabel(const QString &l)
{
    m_originalLabel = l;
}

const Heading::Delims &Heading::delims() const
{
    return m_delims;
}

void Heading::setDelims(const Delims &d)
{
    m_delims = d;
}

const WithPosition &Heading::labelPos() const
{
    return m_labelPos;
}

void Heading::setLabelPos(const WithPosition &p)
{
    m_labelPos = p;
}

const Heading::LabelsVector &Heading::labelVariants() const
{
    return m_labelVariants;
}

void Heading::setLabelVariants(const LabelsVector &vars)
{
    m_labelVariants = vars;
}

void Heading::appendLabelVariant(const QString &v)
{
    m_labelVariants.append(v);
}

HeadingType Heading::headingType() const
{
    return m_type;
}

void Heading::setHeadingType(HeadingType t)
{
    m_type = t;
}

void Heading::write(QTextStream &stream,
                    SerialiseHelper *helper) const
{
    if (headingType() == HeadingType::ATX || headingType() == HeadingType::Unknown) {
        stream << QString(level(), s_numberSignChar) << s_spaceChar;
    }

    text()->write(stream, helper);

    if (headingType() == HeadingType::Setext) {
        stream << s_newLineChar;
        helper->startLine(stream);

        if (level() == 1) {
            stream << s_equalSignChar;
        } else {
            stream << s_minusChar;
        }
    } else if (!originalLabel().isEmpty()) {
        stream << s_leftCurlyBracketChar << originalLabel() << s_rightCurlyBracketChar;
    }
}

//
// Blockquote
//

Blockquote::Blockquote() = default;

Blockquote::~Blockquote() = default;

Item::SharedPointer Blockquote::clone(Document *doc) const
{
    auto b = QSharedPointer<Blockquote>::create();
    b->applyBlock(*this, doc);
    b->setDelims(m_delims);

    return b;
}

ItemType Blockquote::type() const
{
    return ItemType::Blockquote;
}

const Blockquote::Delims &Blockquote::delims() const
{
    return m_delims;
}

void Blockquote::setDelims(const Delims &d)
{
    m_delims = d;
}

void Blockquote::appendDelim(const WithPosition &p)
{
    m_delims.append(p);
}

void Blockquote::write(QTextStream &stream,
                       SerialiseHelper *helper) const
{
    helper->push(this);

    writeStartOfLine(stream);

    Block::write(stream, helper);

    helper->pop();
}

void Blockquote::writeStartOfLine(QTextStream &stream) const
{
    stream << s_greaterSignChar << s_spaceChar;
}

//
// ListItem
//

ListItem::ListItem() = default;

ListItem::~ListItem() = default;

Item::SharedPointer ListItem::clone(Document *doc) const
{
    auto l = QSharedPointer<ListItem>::create();
    l->setSymbol(symbol());
    l->applyBlock(*this, doc);
    l->setListType(m_listType);
    l->setOrderedListPreState(m_orderedListState);
    l->setStartNumber(m_startNumber);
    l->setTaskList(m_isTaskList);
    l->setChecked(m_isChecked);
    l->setDelim(m_delim);
    l->setTaskDelim(m_taskDelim);

    return l;
}

ItemType ListItem::type() const
{
    return ItemType::ListItem;
}

ListItem::ListType ListItem::listType() const
{
    return m_listType;
}

void ListItem::setListType(ListType t)
{
    m_listType = t;
}

ListItem::OrderedListPreState ListItem::orderedListPreState() const
{
    return m_orderedListState;
}

void ListItem::setOrderedListPreState(OrderedListPreState s)
{
    m_orderedListState = s;
}

int ListItem::startNumber() const
{
    return m_startNumber;
}

void ListItem::setStartNumber(int n)
{
    m_startNumber = n;
}

bool ListItem::isTaskList() const
{
    return m_isTaskList;
}

void ListItem::setTaskList(bool on)
{
    m_isTaskList = on;
}

bool ListItem::isChecked() const
{
    return m_isChecked;
}

void ListItem::setChecked(bool on)
{
    m_isChecked = on;
}

const WithPosition &ListItem::delim() const
{
    return m_delim;
}

void ListItem::setDelim(const WithPosition &d)
{
    m_delim = d;
}

const WithPosition &ListItem::taskDelim() const
{
    return m_taskDelim;
}

void ListItem::setTaskDelim(const WithPosition &d)
{
    m_taskDelim = d;
}

ListItemSymbol ListItem::symbol() const
{
    return m_symbol;
}

void ListItem::setSymbol(ListItemSymbol s)
{
    m_symbol = s;
}

void ListItem::write(QTextStream &stream,
                     SerialiseHelper *helper) const
{
    helper->push(this);

    m_offset = 0;

    switch (symbol()) {
    case ListItemSymbol::Minus: {
        stream << s_minusChar;
        ++m_offset;
    } break;

    case ListItemSymbol::Plus: {
        stream << s_plusSignChar;
        ++m_offset;
    } break;

    case ListItemSymbol::Dot: {
        const auto number = QString::number(startNumber());
        stream << number << s_dotChar;
        m_offset += number.length() + 1;
    } break;

    case ListItemSymbol::Bracket: {
        const auto number = QString::number(startNumber());
        stream << number << s_rightParenthesisChar;
        m_offset += number.length() + 1;
    } break;

    case ListItemSymbol::Asterisk:
    default: {
        stream << s_asteriskChar;
        ++m_offset;
    } break;
    }

    stream << s_spaceChar;

    ++m_offset;

    if (isTaskList()) {
        stream << s_leftSquareBracketChar;

        if (isChecked()) {
            stream << s_xChar;
        } else {
            stream << s_spaceChar;
        }

        stream << s_rightSquareBracketChar << s_spaceChar;
    }

    Block::write(stream, helper);

    helper->pop();
}

void ListItem::writeStartOfLine(QTextStream &stream) const
{
    stream << QString(m_offset, s_spaceChar);
}

//
// List
//

List::List() = default;

List::~List() = default;

Item::SharedPointer List::clone(Document *doc) const
{
    auto l = QSharedPointer<List>::create();
    l->applyBlock(*this, doc);

    return l;
}

ItemType List::type() const
{
    return ItemType::List;
}

//
// LinkBase
//

LinkBase::LinkBase()
    : m_p(new Paragraph)
{
}

LinkBase::~LinkBase() = default;

void LinkBase::applyLinkBase(const LinkBase &other,
                             Document *doc)
{
    if (this != &other) {
        ItemWithOpts::applyItemWithOpts(other);
        setUrl(other.url());
        setTitle(other.title());
        setText(other.text());
        setP(other.p()->clone(doc).staticCast<Paragraph>());
        setTextPos(other.textPos());
        setUrlPos(other.urlPos());
        setMarkdownContent(other.markdownContent());
    }
}

const QString &LinkBase::url() const
{
    return m_url;
}

void LinkBase::setUrl(const QString &u)
{
    m_url = u;
}

const QString &LinkBase::title() const
{
    return m_title;
}

void LinkBase::setTitle(const QString &t)
{
    m_title = t;
}

const QString &LinkBase::text() const
{
    return m_text;
}

void LinkBase::setText(const QString &t)
{
    m_text = t;
}

bool LinkBase::isEmpty() const
{
    return m_url.size() <= 0;
}

LinkBase::ParagraphSharedPointer LinkBase::p() const
{
    return m_p;
}

void LinkBase::setP(ParagraphSharedPointer v)
{
    m_p = v;
}

const WithPosition &LinkBase::textPos() const
{
    return m_textPos;
}

void LinkBase::setTextPos(const WithPosition &pos)
{
    m_textPos = pos;
}

const WithPosition &LinkBase::urlPos() const
{
    return m_urlPos;
}

void LinkBase::setUrlPos(const WithPosition &pos)
{
    m_urlPos = pos;
}

const QString &LinkBase::markdownContent() const
{
    return m_markdownContent;
}

void LinkBase::setMarkdownContent(const QString &md)
{
    m_markdownContent = md;
}

void LinkBase::write(QTextStream &stream,
                     SerialiseHelper *) const
{
    writeOpenStyles(stream);
    stream << markdownContent();
    writeCloseStyles(stream);
}

//
// Image
//

Image::Image() = default;

Image::~Image() = default;

Item::SharedPointer Image::clone(Document *doc) const
{
    auto i = QSharedPointer<Image>::create();
    i->applyLinkBase(*this, doc);

    return i;
}

ItemType Image::type() const
{
    return ItemType::Image;
}

//
// Link
//

Link::Link()
    : LinkBase()
    , m_img(new Image)
{
}

Link::~Link() = default;

Item::SharedPointer Link::clone(Document *doc) const
{
    auto l = QSharedPointer<Link>::create();
    l->applyLinkBase(*this, doc);
    l->setImg(m_img->clone(doc).staticCast<Image>());

    return l;
}

ItemType Link::type() const
{
    return ItemType::Link;
}

Link::ImageSharedPointer Link::img() const
{
    return m_img;
}

void Link::setImg(ImageSharedPointer i)
{
    m_img = i;
}

//
// Code
//

Code::Code(const QString &t,
           bool fensedCode,
           bool inl)
    : ItemWithOpts()
    , m_text(t)
    , m_inlined(inl)
    , m_fensed(fensedCode)
{
}

Code::~Code() = default;

void Code::applyCode(const Code &other)
{
    if (this != &other) {
        ItemWithOpts::applyItemWithOpts(other);
        setText(other.text());
        setInline(other.isInline());
        setSyntax(other.syntax());
        setSyntaxPos(other.syntaxPos());
        setStartDelim(other.startDelim());
        setEndDelim(other.endDelim());
        setFensedCode(other.isFensedCode());
        setFensedCodeSymbol(other.fensedCodeSymbol());
        setMarkdownContent(other.markdownContent());
    }
}

Item::SharedPointer Code::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto c = QSharedPointer<Code>::create(m_text, m_fensed, m_inlined);
    c->applyCode(*this);

    return c;
}

ItemType Code::type() const
{
    return ItemType::Code;
}

const QString &Code::text() const
{
    return m_text;
}

void Code::setText(const QString &t)
{
    m_text = t;
}

bool Code::isInline() const
{
    return m_inlined;
}

void Code::setInline(bool on)
{
    m_inlined = on;
}

const QString &Code::syntax() const
{
    return m_syntax;
}

void Code::setSyntax(const QString &s)
{
    m_syntax = s;
}

const WithPosition &Code::syntaxPos() const
{
    return m_syntaxPos;
}

void Code::setSyntaxPos(const WithPosition &p)
{
    m_syntaxPos = p;
}

const WithPosition &Code::startDelim() const
{
    return m_startDelim;
}

void Code::setStartDelim(const WithPosition &d)
{
    m_startDelim = d;
}

const WithPosition &Code::endDelim() const
{
    return m_endDelim;
}

void Code::setEndDelim(const WithPosition &d)
{
    m_endDelim = d;
}

bool Code::isFensedCode() const
{
    return m_fensed;
}

void Code::setFensedCode(bool on)
{
    m_fensed = on;
}

const QString &Code::markdownContent() const
{
    return m_markdowm;
}

void Code::setMarkdownContent(const QString &v)
{
    m_markdowm = v;
}

FensedCodeSymbol Code::fensedCodeSymbol() const
{
    return m_fensedSymbol;
}

void Code::setFensedCodeSymbol(FensedCodeSymbol s)
{
    m_fensedSymbol = s;
}

void Code::write(QTextStream &stream,
                 SerialiseHelper *helper) const
{
    if (isInline()) {
        writeOpenStyles(stream);

        const QString opener(startDelim().endColumn() - startDelim().startColumn() + 1, s_graveAccentChar);

        stream << opener;

        stream << (markdownContent().isEmpty() ? text() : markdownContent());

        stream << opener;

        writeCloseStyles(stream);
    } else {
        helper->push(this);

        qsizetype delimCount = (isFensedCode() ? startDelim().endColumn() - startDelim().startColumn() + 1 : 3);
        const QChar symbol = (fensedCodeSymbol() == FensedCodeSymbol::Tilde ? s_tildeChar : s_graveAccentChar);

        if (!isFensedCode()) {
            while (text().indexOf(QString(delimCount, symbol)) != -1) {
                ++delimCount;
            }
        }

        const QString opener(delimCount, symbol);

        stream << opener << syntax() << s_newLineChar;
        helper->startLine(stream);

        if (!text().isEmpty()) {
            writeMultiline(text(), stream, helper);

            stream << s_newLineChar;

            helper->startLine(stream);
        }

        stream << opener;

        helper->pop();
    }
}

//
// Math
//

Math::Math()
    : Code({},
           false,
           true)
{
}

Math::~Math() = default;

Item::SharedPointer Math::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto m = QSharedPointer<Math>::create();
    m->applyCode(*this);

    return m;
}

ItemType Math::type() const
{
    return ItemType::Math;
}

const QString &Math::expr() const
{
    return Code::text();
}

void Math::setExpr(const QString &e)
{
    Code::setText(e);
}

void Math::write(QTextStream &stream,
                 SerialiseHelper *helper) const
{
    const auto writeDelims = [&]() {
        if (this->isInline()) {
            stream << s_dollarSignChar;
        } else {
            stream << QString(2, s_dollarSignChar);
        }
    };

    writeOpenStyles(stream);

    writeDelims();

    writeMultiline(expr(), stream, helper);

    if (expr().isEmpty()) {
        stream << s_spaceChar;
    }

    writeDelims();

    writeCloseStyles(stream);
}

//
// TableCell
//

TableCell::TableCell() = default;

TableCell::~TableCell() = default;

Item::SharedPointer TableCell::clone(Document *doc) const
{
    auto c = QSharedPointer<TableCell>::create();
    c->applyBlock(*this, doc);

    return c;
}

ItemType TableCell::type() const
{
    return ItemType::TableCell;
}

void TableCell::write(QTextStream &stream,
                      SerialiseHelper *helper) const
{
    for (const auto &item : items()) {
        item->write(stream, helper);
    }
}

//
// TableRow
//

TableRow::TableRow() = default;

TableRow::~TableRow() = default;

Item::SharedPointer TableRow::clone(Document *doc) const
{
    auto t = QSharedPointer<TableRow>::create();
    t->applyPositions(*this);

    for (const auto &c : cells()) {
        t->appendCell(c->clone(doc).staticCast<TableCell>());
    }

    return t;
}

ItemType TableRow::type() const
{
    return ItemType::TableRow;
}

const TableRow::Cells &TableRow::cells() const
{
    return m_cells;
}

void TableRow::setCells(const Cells &c)
{
    m_cells = c;
}

void TableRow::appendCell(TableCellSharedPointer c)
{
    m_cells.push_back(c);
}

bool TableRow::isEmpty() const
{
    return m_cells.empty();
}

void TableRow::write(QTextStream &stream,
                     SerialiseHelper *helper) const
{
    if (cells().isEmpty()) {
        return;
    }

    stream << s_verticalLineChar << s_spaceChar;

    bool first = true;

    for (const auto &cell : cells()) {
        if (!first) {
            stream << s_spaceChar;
        }

        cell->write(stream, helper);

        stream << s_spaceChar << s_verticalLineChar;

        first = false;
    }
}

//
// Table
//

Table::Table() = default;

Table::~Table() = default;

Item::SharedPointer Table::clone(Document *doc) const
{
    auto t = QSharedPointer<Table>::create();
    t->applyPositions(*this);

    for (const auto &r : rows()) {
        t->appendRow(r->clone(doc).staticCast<TableRow>());
    }

    for (int i = 0; i < columnsCount(); ++i) {
        t->setColumnAlignment(i, columnAlignment(i));
    }

    return t;
}

ItemType Table::type() const
{
    return ItemType::Table;
}

const Table::Rows &Table::rows() const
{
    return m_rows;
}

void Table::setRows(Rows &r)
{
    m_rows = r;
}

void Table::appendRow(TableRowSharedPointer r)
{
    m_rows.push_back(r);
}

Table::Alignment Table::columnAlignment(int idx) const
{
    return m_aligns.at(idx);
}

void Table::setColumnAlignment(int idx,
                               Alignment a)
{
    if (idx + 1 > columnsCount()) {
        m_aligns.push_back(a);
    } else {
        m_aligns[idx] = a;
    }
}

int Table::columnsCount() const
{
    return m_aligns.size();
}

bool Table::isEmpty() const
{
    return (m_aligns.empty() || m_rows.empty());
}

void writeAlignment(Table::Alignment a,
                    QTextStream &stream)
{
    static const QString s_left = QStringLiteral(":---");
    static const QString s_center = QStringLiteral(":---:");
    static const QString s_right = QStringLiteral("---:");

    switch (a) {
    case Table::AlignLeft: {
        stream << s_left;
    } break;

    case Table::AlignRight: {
        stream << s_right;
    } break;

    case Table::AlignCenter:
    default: {
        stream << s_center;
    } break;
    }
}

void Table::write(QTextStream &stream,
                  SerialiseHelper *helper) const
{
    if (rows().isEmpty()) {
        return;
    }

    bool first = true;

    for (const auto &row : rows()) {
        if (!first) {
            stream << s_newLineChar;

            helper->startLine(stream);
        }

        row->write(stream, helper);

        if (first) {
            stream << s_newLineChar;

            helper->startLine(stream);

            for (int i = 0; i < columnsCount(); ++i) {
                if (i == 0) {
                    stream << s_verticalLineChar;
                }

                stream << s_spaceChar;

                writeAlignment(columnAlignment(i), stream);

                stream << s_spaceChar << s_verticalLineChar;
            }

            first = false;
        }
    }
}

//
// FootnoteRef
//

FootnoteRef::FootnoteRef(const QString &i)
    : m_id(i)
{
}

FootnoteRef::~FootnoteRef() = default;

Item::SharedPointer FootnoteRef::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto f = QSharedPointer<FootnoteRef>::create(m_id);
    f->applyText(*this);
    f->setIdPos(m_idPos);
    f->setMarkdownContent(markdownContent());

    return f;
}

ItemType FootnoteRef::type() const
{
    return ItemType::FootnoteRef;
}

const QString &FootnoteRef::id() const
{
    return m_id;
}

void FootnoteRef::setId(const QString &i)
{
    m_id = i;
}

const WithPosition &FootnoteRef::idPos() const
{
    return m_idPos;
}

void FootnoteRef::setIdPos(const WithPosition &pos)
{
    m_idPos = pos;
}

const QString &FootnoteRef::markdownContent() const
{
    return m_markdownContent;
}

void FootnoteRef::setMarkdownContent(const QString &md)
{
    m_markdownContent = md;
}

void FootnoteRef::write(QTextStream &stream,
                        SerialiseHelper *) const
{
    writeOpenStyles(stream);

    stream << markdownContent();

    writeCloseStyles(stream);
}

//
// Footnote
//

Footnote::Footnote() = default;

Footnote::~Footnote() = default;

Item::SharedPointer Footnote::clone(Document *doc) const
{
    auto f = QSharedPointer<Footnote>::create();
    f->applyBlock(*this, doc);
    f->setIdPos(m_idPos);

    return f;
}

ItemType Footnote::type() const
{
    return ItemType::Footnote;
}

const WithPosition &Footnote::idPos() const
{
    return m_idPos;
}

void Footnote::setIdPos(const WithPosition &pos)
{
    m_idPos = pos;
}

void Footnote::write(QTextStream &stream,
                     SerialiseHelper *helper) const
{
    helper->push(this);

    bool firstItem = true;
    m_offset = 0;

    for (const auto &item : items()) {
        if (!firstItem) {
            m_offset = 4;
            stream << s_newLineChar;
            helper->startLine(stream);
            stream << s_newLineChar;
            helper->startLine(stream);
        }

        item->write(stream, helper);

        firstItem = false;
    }

    helper->pop();
}

void Footnote::writeStartOfLine(QTextStream &stream) const
{
    stream << QString(m_offset, s_spaceChar);
}

//
// Document
//

Document::Document() = default;

Document::~Document() = default;

ItemType Document::type() const
{
    return ItemType::Document;
}

Item::SharedPointer Document::clone(Document *doc) const
{
    Q_UNUSED(doc)

    auto d = QSharedPointer<Document>::create();
    d->applyBlock(*this, d.get());

    for (auto it = m_footnotes.cbegin(), last = m_footnotes.cend(); it != last; ++it) {
        d->insertFootnote(it.key(),
                          it.value().m_originalLabel,
                          it.value().m_footnote->clone(d.get()).staticCast<Footnote>());
    }

    for (auto it = m_labeledLinks.cbegin(), last = m_labeledLinks.cend(); it != last; ++it) {
        d->insertLabeledLink(it.key(), it.value()->clone(d.get()).staticCast<Link>());
    }

    d->setAuxLabelsMap(auxLabelsMap());

    return d;
}

const Document::Footnotes &Document::footnotesMap() const
{
    return m_footnotes;
}

void Document::setFootnotesMap(const Footnotes &f)
{
    m_footnotes = f;
}

void Document::insertFootnote(const QString &id,
                              const QString &originalLabel,
                              FootnoteSharedPointer fn)
{
    m_footnotes.insert(id, FootnoteWithLabel{fn, originalLabel});
}

const Document::LabeledLinks &Document::labeledLinks() const
{
    return m_labeledLinks;
}

void Document::setLabeledLinks(const LabeledLinks &l)
{
    m_labeledLinks = l;
}

void Document::insertLabeledLink(const QString &label,
                                 LinkSharedPointer lnk)
{
    m_labeledLinks.insert(label, lnk);
}

const Document::LabeledHeadings &Document::labeledHeadings() const
{
    return m_labeledHeadings;
}

void Document::setLabeledHeadings(const LabeledHeadings &h)
{
    m_labeledHeadings = h;
}

void Document::insertLabeledHeading(const QString &label,
                                    HeadingSharedPointer h)
{
    m_labeledHeadings.insert(label, h);
}

const Document::AuxLabelsMap &Document::auxLabelsMap() const
{
    return m_auxLabelsMap;
}

void Document::setAuxLabelsMap(const AuxLabelsMap &m)
{
    m_auxLabelsMap = m;
}

void Document::insertAuxLabel(const QString &label,
                              const QString &path)
{
    if (!m_auxLabelsMap.contains(label)) {
        m_auxLabelsMap.insert(label, {});
    }

    m_auxLabelsMap[label].insert(path, 0);
}

qsizetype Document::getAuxLabelCounter(const QString &label,
                                       const QString &path)
{
    const auto it = m_auxLabelsMap.constFind(label);

    if (it != m_auxLabelsMap.constEnd()) {
        const auto cit = it->constFind(path);

        if (cit != it->constEnd()) {
            return cit.value();
        }
    }

    return 0;
}

void Document::incrementAuxLabelCounter(const QString &label,
                                        const QString &path)
{
    ++m_auxLabelsMap[label][path];
}

void Document::write(QTextStream &stream,
                     SerialiseHelper *helper) const
{
    Block::write(stream, helper);

    bool first = true;

    if (!footnotesMap().isEmpty()
        && (items().count() > 1 || (items().count() == 1 && items().at(0)->type() != MD::ItemType::Anchor))) {
        stream << s_newLineChar << s_newLineChar;
    }

    for (auto it = footnotesMap().cbegin(), last = footnotesMap().cend(); it != last; ++it) {
        if (!first) {
            stream << s_newLineChar << s_newLineChar;
        }

        stream
            << s_leftSquareBracketChar
            << it.value().m_originalLabel
            << s_rightSquareBracketChar
            << s_colonChar
            << s_spaceChar;

        it.value().m_footnote->write(stream, helper);

        first = false;
    }

    if (!labeledLinks().isEmpty()) {
        stream << s_newLineChar << s_newLineChar;
    }

    for (auto it = labeledLinks().cbegin(), last = labeledLinks().cend(); it != last; ++it) {
        it.value()->write(stream, helper);

        stream << s_newLineChar;
    }

    if (labeledLinks().isEmpty()) {
        stream << s_newLineChar;
    }
}

} /* namespace MD */

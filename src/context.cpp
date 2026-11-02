/*
    SPDX-FileCopyrightText: 2026 Igor Mironchik <igor.mironchik@gmail.com>
    SPDX-License-Identifier: MIT
*/

// md4qt include.
#include "context.h"

namespace MD
{

Context::Context(Context *prnt)
    : m_parent(prnt)
{
}

Context::~Context() = default;

void Context::applyParentContext(Context &ctx,
                                 bool updateNestingLevel)
{
    setIndentColumn(ctx.lastChildIndent());

    if (updateNestingLevel) {
        setNestingLevel(ctx.children().isEmpty() ? ctx.nestingLevel() : ctx.children().back().nestingLevel());
    }

    m_parent = &ctx;
}

void Context::updateParentContextForAllChildren()
{
    for (auto it = m_children.begin(), last = m_children.end(); it != last; ++it) {
        it->applyParentContext(*this, false);
        it->updateParentContextForAllChildren();
    }
}

} /* namespace MD */

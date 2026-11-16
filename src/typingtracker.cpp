// SPDX-License-Identifier: GPL-3.0-or-later
#include "typingtracker.h"

TypingTracker::TypingTracker(Composition *composition)
    : m_composition(composition)
{
}

bool TypingTracker::keyPressed(const Key &key, qint64 timeMs, bool shuangpinActive)
{
    if (key.kind == Key::Kind::Modifier) {
        m_modifiersDown.insert(key.code);
        return false;
    }
    if (!shuangpinActive) {
        m_composition->reset();
        return false;
    }
    const bool schemeKey = key.kind == Key::Kind::Character && m_composition->scheme().isKey(key.character);
    if (schemeKey && m_modifiersDown.isEmpty()) {
        m_composition->key(key.character, timeMs);
        return true;
    }
    if (key.kind == Key::Kind::Backspace)
        m_composition->backspace(timeMs);
    else
        m_composition->reset();
    return false;
}

void TypingTracker::keyReleased(const Key &key)
{
    if (key.kind == Key::Kind::Modifier)
        m_modifiersDown.remove(key.code);
}

void TypingTracker::inputMethodLeft()
{
    m_composition->reset();
}

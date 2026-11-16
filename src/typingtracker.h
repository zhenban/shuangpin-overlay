// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "composition.h"
#include "key.h"

#include <QSet>

/// Turns raw key presses into edits of a Composition.
///
/// Letters typed on their own extend it, Backspace shortens it and every
/// other key ends it, as does a letter typed with a modifier held (a
/// shortcut, or a capital the input method passes straight through).
/// Nothing is tracked while the Shuangpin input method is not the active
/// one.
class TypingTracker
{
public:
    explicit TypingTracker(Composition *composition);

    /// @p timeMs is any monotonic clock. Returns true if the key is one the
    /// scheme uses, typed in a way that makes it part of the composition.
    bool keyPressed(const Key &key, qint64 timeMs, bool shuangpinActive);
    void keyReleased(const Key &key);

    /// The input method is no longer the Shuangpin one; what was being
    /// typed has been committed or dropped by it.
    void inputMethodLeft();

private:
    Composition *m_composition;
    QSet<int> m_modifiersDown;
};

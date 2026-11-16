// SPDX-License-Identifier: GPL-3.0-or-later
#include "visibilitypolicy.h"

#include <QtMinMax>

namespace {

// In "only when stuck" mode, how long the chart stays after the syllable.
constexpr int kStuckLingerMs = 1500;
// Half a syllable is waiting: the typist is probably reading the chart, so
// it stays this many times longer, and at least a minute.
constexpr int kPendingFactor = 6;
constexpr int kPendingMinimumMs = 60000;

} // namespace

bool VisibilityPolicy::shouldBeShown(const Facts &facts)
{
    // Without the keyboard nothing could bring the chart back, so only the
    // input method decides.
    if (!facts.keyboardAvailable)
        return facts.shuangpinActive;
    if (!facts.shown)
        return false; // only a key press shows it again
    if (facts.held)
        return true;
    if (!facts.shuangpinActive)
        return false;
    if (facts.pending)
        return facts.idleMs <= qMax(kPendingMinimumMs, facts.idleLimitMs * kPendingFactor);
    if (facts.onlyWhenStuck)
        return facts.idleMs <= kStuckLingerMs;
    return facts.idleLimitMs <= 0 || facts.idleMs <= facts.idleLimitMs;
}

bool VisibilityPolicy::showOnKey(const Facts &facts)
{
    return facts.shuangpinActive && !facts.onlyWhenStuck;
}

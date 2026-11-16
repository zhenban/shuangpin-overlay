// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QtTypes>

/// Decides when the chart should be on screen. Pure rules, no clock and no
/// window: the caller supplies the facts and applies the answer.
namespace VisibilityPolicy {

struct Facts {
    bool shown = true;
    bool keyboardAvailable = true; ///< can key presses be seen at all
    bool shuangpinActive = true;   ///< is the Shuangpin input method the active one
    bool pending = false;          ///< a syllable is half typed
    bool held = false;             ///< pointer over the chart, menu open or being dragged
    bool onlyWhenStuck = false;    ///< the user's "only appear when I get stuck" setting
    qint64 idleMs = 0;             ///< time since the last key press
    int idleLimitMs = 0;           ///< the user's idle limit; 0 means never hide
};

/// Pause after an initial before the chart appears in "only when stuck" mode.
inline constexpr int stuckDelayMs = 800;

/// Whether the chart should be shown, checked periodically.
bool shouldBeShown(const Facts &facts);

/// Whether a scheme key that was just typed should bring the chart back
/// immediately. In "only when stuck" mode it never does; the chart waits
/// for stuckDelayMs of hesitation instead.
bool showOnKey(const Facts &facts);

} // namespace VisibilityPolicy

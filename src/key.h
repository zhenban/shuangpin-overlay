// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QChar>

/// A key of the keyboard, reduced to what matters for following a
/// composition.
struct Key {
    enum class Kind {
        Character, ///< a letter or ';' (see character)
        Backspace,
        Modifier,  ///< Shift, Ctrl, Alt or Meta, either side
        Other,     ///< anything else: it ends the composition
    };

    Kind kind = Kind::Other;
    QChar character; ///< lower case; set for Kind::Character only
    int code = 0;    ///< tells one physical key from another

    static Key letter(QChar character) { return {Kind::Character, character, character.unicode()}; }
};

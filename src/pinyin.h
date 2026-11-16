// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>

/// Which Mandarin syllables exist. Finals use ü where pinyin writes it after
/// n and l ("nü", "lüe"); after j, q, x and y the plain "u" forms are used,
/// matching how the syllables are spelled.
namespace Pinyin {

/// All initials, including the two-letter ones. The empty string is the
/// zero initial ("an", "er", ...).
const QStringList &initials();

/// Finals that can follow the given initial. Unknown initials give an
/// empty list.
const QStringList &finalsFor(const QString &initial);

bool isSyllable(const QString &initial, const QString &final);

} // namespace Pinyin

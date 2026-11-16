// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "composition.h"
#include "scheme.h"

#include <QColor>
#include <QHash>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>

#include <optional>

class QPainter;
class QPalette;

/// Colours of the chart. The panel is always dark; the accent comes from
/// the desktop.
struct ChartTheme {
    QColor panel{24, 24, 27, 238};
    QColor key{255, 255, 255, 20};
    QColor letter{0xa1, 0xa1, 0xaa};
    QColor text{0xf4, 0xf4, 0xf5};
    QColor note{0xa1, 0xa1, 0xaa};
    QColor error{0xf8, 0x71, 0x71};
    QColor pressed{0xf2, 0xb8, 0x4b};   ///< fill of a key being pressed
    QColor onPressed{0x18, 0x18, 0x1b}; ///< text on such a key
    QColor accent{0xf2, 0xb8, 0x4b};    ///< small accent text on the panel

    /// Takes the accent from the palette's highlight colour.
    static ChartTheme fromPalette(const QPalette &palette);
};

/// What the chart should show at one moment.
struct ChartState {
    enum class Footer { Default, Accent, Error };

    /// Keys being pressed or just released, with a strength from 0 to 1.
    QHash<QChar, qreal> lit;
    /// While a syllable is half typed: the keys that can finish it and what
    /// each would give. Every other key is dimmed.
    std::optional<QMap<QChar, QStringList>> hint;
    /// Bottom line; empty shows the scheme's name and summary.
    QString footer;
    Footer footerStyle = Footer::Default;

    /// The state that shows @p composition: its readout in the footer and,
    /// if @p hints is set and a syllable is half typed, the keys that can
    /// finish it. No key is lit.
    static ChartState of(const Composition &composition, bool hints);
};

/// Draws a scheme as a keyboard.
namespace ChartPainter {

QSize size(const Scheme &scheme, qreal scale);
void paint(QPainter &painter, const Scheme &scheme, const ChartState &state, const ChartTheme &theme, qreal scale);

} // namespace ChartPainter

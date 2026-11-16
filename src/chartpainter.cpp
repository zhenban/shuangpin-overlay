// SPDX-License-Identifier: GPL-3.0-or-later
#include "chartpainter.h"

#include "keyboardlayout.h"

#include <QFont>
#include <QPainter>
#include <QPalette>

namespace {

// Sizes at scale 1, in logical pixels.
constexpr qreal kKey = 62;
constexpr qreal kGap = 6;
constexpr qreal kPadding = 14;
constexpr qreal kFooter = 30;
constexpr qreal kPanelRadius = 14;
constexpr qreal kKeyRadius = 8;
constexpr int kDimAlpha = 55;
constexpr int kDimKeyAlpha = 8;

// Each row starts a little further right, as on a real keyboard.
constexpr qreal kRowIndent[KeyboardLayout::rowCount] = {0.0, 0.35, 0.85};

/// The rows to draw: ';' only appears for the schemes that use it.
QStringList rows(const Scheme &scheme)
{
    QStringList result;
    for (const char *row : KeyboardLayout::rows) {
        QString keys = QString::fromLatin1(row);
        if (!scheme.usesSemicolon())
            keys.remove(u';');
        result << keys;
    }
    return result;
}

QColor mix(const QColor &from, const QColor &to, qreal amount)
{
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * amount,
                            from.greenF() + (to.greenF() - from.greenF()) * amount,
                            from.blueF() + (to.blueF() - from.blueF()) * amount,
                            from.alphaF() + (to.alphaF() - from.alphaF()) * amount);
}

QFont font(qreal pixels, bool bold = true)
{
    QFont f;
    f.setPixelSize(qMax(1, int(pixels)));
    f.setBold(bold);
    return f;
}

} // namespace

ChartTheme ChartTheme::fromPalette(const QPalette &palette)
{
    ChartTheme theme;
    const QColor highlight = palette.color(QPalette::Active, QPalette::Highlight);
    theme.pressed = highlight;
    theme.onPressed = highlight.lightnessF() > 0.55 ? QColor(0x18, 0x18, 0x1b) : QColor(Qt::white);
    // Small text needs to stay readable on the dark panel.
    theme.accent = highlight;
    if (theme.accent.lightnessF() < 0.65)
        theme.accent.setHslF(highlight.hslHueF(), highlight.hslSaturationF(), 0.65);
    return theme;
}

ChartState ChartState::of(const Composition &composition, bool hints)
{
    ChartState state;
    if (hints && composition.isPending())
        state.hint = composition.scheme().continuationLabels(composition.pendingKey());
    if (!composition.isEmpty()) {
        const Composition::Readout readout = composition.readout();
        state.footer = readout.text;
        state.footerStyle = readout.valid ? Footer::Accent : Footer::Error;
    }
    return state;
}

QSize ChartPainter::size(const Scheme &scheme, qreal scale)
{
    // The widest row decides: the middle one, when the scheme uses ';'.
    qreal columns = 0;
    const QStringList keyRows = rows(scheme);
    for (int row = 0; row < keyRows.size(); ++row)
        columns = qMax(columns, kRowIndent[row] + keyRows.at(row).size());
    const qreal width = kPadding * 2 + (kKey + kGap) * columns - kGap;
    const qreal height = kPadding * 2 + kKey * 3 + kGap * 2 + kFooter;
    return QSizeF(width * scale, height * scale).toSize();
}

void ChartPainter::paint(QPainter &painter, const Scheme &scheme, const ChartState &state,
                         const ChartTheme &theme, qreal scale)
{
    const qreal s = scale;
    const qreal key = kKey * s;
    const qreal step = (kKey + kGap) * s;
    const qreal padding = kPadding * s;
    const QSize total = size(scheme, scale);

    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.panel);
    painter.drawRoundedRect(QRectF(QPointF(0, 0), total), kPanelRadius * s, kPanelRadius * s);

    const QFont small = font(13 * s);
    const bool hinting = state.hint.has_value();

    const QStringList keyRows = rows(scheme);
    for (int row = 0; row < keyRows.size(); ++row) {
        const QString &letters = keyRows.at(row);
        for (int column = 0; column < letters.size(); ++column) {
            const QChar letter = letters.at(column);
            const qreal x = padding + kRowIndent[row] * step + column * step;
            const qreal y = padding + row * step;
            const qreal lit = state.lit.value(letter, 0.0);
            const bool allowed = hinting && state.hint->contains(letter);
            const bool dim = hinting && !allowed && lit < 0.05;

            auto ink = [&](const QColor &colour, bool dimmed) {
                QColor c = mix(colour, theme.onPressed, lit);
                if (dimmed)
                    c.setAlpha(kDimAlpha);
                return c;
            };

            QColor fill = mix(theme.key, theme.pressed, lit);
            if (dim)
                fill.setAlpha(kDimKeyAlpha);
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawRoundedRect(QRectF(x, y, key, key), kKeyRadius * s, kKeyRadius * s);

            const QRectF top(x + 7 * s, y + 4 * s, key - 14 * s, 18 * s);
            painter.setFont(small);
            painter.setPen(ink(theme.letter, dim));
            painter.drawText(top, Qt::AlignLeft | Qt::AlignVCenter, letter.toUpper());
            const QStringList initials = scheme.initialLabels(letter);
            if (!initials.isEmpty()) {
                // Initials say nothing while a final is expected.
                painter.setPen(ink(theme.accent, hinting && lit < 0.05));
                painter.drawText(top, Qt::AlignRight | Qt::AlignVCenter, initials.join(u' '));
            }

            const QStringList labels = allowed ? state.hint->value(letter) : scheme.finalLabels(letter);
            painter.setPen(ink(theme.text, dim));
            if (labels.size() == 1) {
                // Whole syllables shown as hints can be long.
                painter.setFont(font((labels.first().size() > 4 ? 14 : 17) * s));
                painter.drawText(QRectF(x, y + 20 * s, key, key - 24 * s), Qt::AlignCenter, labels.first());
            } else if (!labels.isEmpty()) {
                const int lines = qMin(3, int(labels.size()));
                const qreal lineHeight = (lines == 2 ? 18 : 13) * s;
                painter.setFont(font((lines == 2 ? 15 : 11) * s));
                for (int line = 0; line < lines; ++line) {
                    painter.drawText(QRectF(x, y + 21 * s + lineHeight * line, key, lineHeight),
                                     Qt::AlignCenter, labels.at(line));
                }
            }
        }
    }

    QString footer = state.footer;
    QColor footerColour = theme.note;
    if (footer.isEmpty()) {
        footer = scheme.displayName();
        if (!scheme.summary().isEmpty())
            footer += QStringLiteral("    ") + scheme.summary();
    } else if (state.footerStyle == ChartState::Footer::Accent) {
        footerColour = theme.accent;
    } else if (state.footerStyle == ChartState::Footer::Error) {
        footerColour = theme.error;
    }
    painter.setFont(font(13 * s, false));
    painter.setPen(footerColour);
    painter.drawText(QRectF(padding, total.height() - padding - 22 * s, total.width() - 2 * padding, 24 * s),
                     Qt::AlignCenter, footer);
}

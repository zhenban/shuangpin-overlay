// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QChar>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QTimer>

/// Which keys of the chart are lit: fully while held down, then fading out
/// for a moment after release so that a quick tap is still seen.
class KeyHighlights : public QObject
{
    Q_OBJECT
public:
    explicit KeyHighlights(QObject *parent = nullptr);

    void press(QChar key);
    void release(QChar key);

    /// Strength from 0 to 1 for every key that is lit right now.
    QHash<QChar, qreal> strengths() const;

Q_SIGNALS:
    void changed();

private:
    void fade();

    QElapsedTimer m_clock;
    QSet<QChar> m_down;
    QHash<QChar, qint64> m_releasedAt;
    QTimer m_fadeTimer;
};

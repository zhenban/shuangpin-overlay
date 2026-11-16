// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "scheme.h"

#include <QHash>
#include <QObject>
#include <QString>

/// A key that was deleted and replaced by a different one.
struct Slip {
    bool initial = false; ///< first key of a syllable, otherwise the second
    QString meant;        ///< the initial or final the typist wanted
    QChar meantKey;
    QChar hitKey;
    QString hit;          ///< what the wrong key stood for, if anything

    bool operator==(const Slip &) const = default;
};

/// Follows what is being typed, as far as it can be guessed from key
/// presses alone: letters extend the composition, Backspace shortens it and
/// anything else ends it. The input method does not tell us, so a click on
/// a candidate or a change of focus can leave this one key out of step
/// until the next reset.
///
/// Nothing typed is kept beyond the current composition.
class Composition : public QObject
{
    Q_OBJECT
public:
    explicit Composition(QObject *parent = nullptr);

    void setScheme(const Scheme &scheme);
    const Scheme &scheme() const { return m_scheme; }

    /// A key of the scheme was pressed. @p timeMs is any monotonic clock.
    void key(QChar key, qint64 timeMs);
    void backspace(qint64 timeMs);
    void reset();

    QString keys() const { return m_keys; }
    bool isEmpty() const { return m_keys.isEmpty(); }
    /// True when the first key of a syllable is waiting for its second.
    bool isPending() const { return m_keys.size() % 2 == 1; }
    QChar pendingKey() const { return isPending() ? m_keys.back() : QChar(); }

    struct Readout {
        QString text;       ///< e.g. "zhong  guo  sh + ?"
        bool valid = true;  ///< false if some pair shown is not a syllable
    };
    /// The last syllables of the composition spelled out. Pairs that are
    /// not syllables are written as typed and followed by a cross.
    Readout readout(int maxSyllables = 6) const;

Q_SIGNALS:
    void changed();
    /// A syllable was completed; @p ms is the pause between its two keys.
    void finalTimed(const QString &final, int ms);
    void slipFound(const Slip &slip);

private:
    void checkSlip(QChar key);

    Scheme m_scheme;
    QString m_keys;
    QHash<int, QChar> m_deleted; // position -> key removed by Backspace
    int m_backspaceRun = 0;      // Backspaces since the last letter
    int m_lastRun = 0;           // length of the run being typed over
    qint64 m_pendingSince = 0;
};

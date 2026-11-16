// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "composition.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

/// Practice figures, kept per scheme: how long each final takes to recall
/// and which keys get mixed up. Only these aggregates are stored; the text
/// that was typed never is.
class PracticeStats : public QObject
{
    Q_OBJECT
public:
    struct SlowFinal {
        QString final;
        int averageMs = 0;
    };
    struct FrequentSlip {
        Slip slip;
        int count = 0;
    };

    /// @p filePath is where the figures are saved; empty keeps them in
    /// memory only.
    explicit PracticeStats(const QString &filePath = QString(), QObject *parent = nullptr);

    ~PracticeStats() override;

    static QString defaultFilePath();

    void setScheme(const QString &schemeId);

    void addFinalTime(const QString &final, int ms);
    void addSlip(const Slip &slip);
    void clear();

    /// Finals clearly slower than the typist's usual pace, slowest first.
    /// Empty until enough has been typed to tell.
    QList<SlowFinal> slowest(int count = 6) const;
    /// Slips seen at least twice, most frequent first.
    QList<FrequentSlip> frequentSlips(int count = 8) const;

private:
    struct Timing {
        int samples = 0;
        double averageMs = 0;
    };
    struct SchemeData {
        QHash<QString, Timing> finals;
        QHash<QString, int> slips; // encoded Slip -> count
    };

    void load();
    void save();
    void saveSoon();

    QString m_filePath;
    QTimer m_saveTimer; // typing changes the figures constantly; write rarely
    QString m_scheme;
    QHash<QString, SchemeData> m_data;
};

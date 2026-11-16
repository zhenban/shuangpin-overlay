// SPDX-License-Identifier: GPL-3.0-or-later
#include "practicestats.h"

#include <QTemporaryDir>
#include <QTest>

class TestPracticeStats : public QObject
{
    Q_OBJECT

    static Slip slip(const char *meant, char meantKey, char hitKey)
    {
        Slip s;
        s.meant = QString::fromLatin1(meant);
        s.meantKey = QLatin1Char(meantKey);
        s.hitKey = QLatin1Char(hitKey);
        return s;
    }

private Q_SLOTS:
    void needsEnoughTypingBeforeJudging()
    {
        PracticeStats stats;
        stats.setScheme(QStringLiteral("xiaohe"));
        for (int i = 0; i < 5; ++i)
            stats.addFinalTime(QStringLiteral("uang"), 2000);
        QVERIFY(stats.slowest().isEmpty());
    }

    void namesTheSlowFinals()
    {
        PracticeStats stats;
        stats.setScheme(QStringLiteral("xiaohe"));
        const QStringList quick = {QStringLiteral("a"), QStringLiteral("o"),  QStringLiteral("e"),  QStringLiteral("i"),
                                   QStringLiteral("u"), QStringLiteral("ai"), QStringLiteral("ei"), QStringLiteral("ao")};
        for (int i = 0; i < 6; ++i) {
            for (const QString &final : quick)
                stats.addFinalTime(final, 300);
            stats.addFinalTime(QStringLiteral("uang"), 1500);
            stats.addFinalTime(QStringLiteral("iong"), 900);
        }
        const QList<PracticeStats::SlowFinal> slow = stats.slowest();
        QCOMPARE(slow.size(), 2);
        QCOMPARE(slow.at(0).final, QStringLiteral("uang"));
        QCOMPARE(slow.at(0).averageMs, 1500);
        QCOMPARE(slow.at(1).final, QStringLiteral("iong"));
    }

    void countsRepeatedSlips()
    {
        PracticeStats stats;
        stats.setScheme(QStringLiteral("xiaohe"));
        stats.addSlip(slip("uang", 'l', 'k'));
        stats.addSlip(slip("ong", 's', 'd'));
        QVERIFY(stats.frequentSlips().isEmpty()); // once can be an accident
        stats.addSlip(slip("uang", 'l', 'k'));
        const QList<PracticeStats::FrequentSlip> slips = stats.frequentSlips();
        QCOMPARE(slips.size(), 1);
        QCOMPARE(slips.at(0).count, 2);
        QCOMPARE(slips.at(0).slip, slip("uang", 'l', 'k'));
    }

    void keepsSchemesApartAndSurvivesARestart()
    {
        QTemporaryDir directory;
        const QString file = directory.filePath(QStringLiteral("nested/stats.json"));
        {
            PracticeStats stats(file);
            stats.setScheme(QStringLiteral("xiaohe"));
            stats.addSlip(slip("uang", 'l', 'k'));
            stats.addSlip(slip("uang", 'l', 'k'));
            stats.setScheme(QStringLiteral("ziranma"));
            QVERIFY(stats.frequentSlips().isEmpty());
        }
        PracticeStats reloaded(file);
        reloaded.setScheme(QStringLiteral("xiaohe"));
        QCOMPARE(reloaded.frequentSlips().size(), 1);
        reloaded.clear();
        QVERIFY(reloaded.frequentSlips().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestPracticeStats)
#include "test_practicestats.moc"

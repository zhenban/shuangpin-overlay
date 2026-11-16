// SPDX-License-Identifier: GPL-3.0-or-later
#include "testutil.h"

#include "composition.h"

#include <QSignalSpy>

class TestComposition : public QObject
{
    Q_OBJECT

    static void type(Composition &composition, const char *keys, qint64 &clock, int gap = 100)
    {
        for (const char *key = keys; *key; ++key) {
            clock += gap;
            if (*key == '<')
                composition.backspace(clock);
            else if (*key == ' ')
                composition.reset();
            else
                composition.key(QLatin1Char(*key), clock);
        }
    }

private Q_SLOTS:
    void init()
    {
        qRegisterMetaType<Slip>();
        m_composition.setScheme(shippedScheme("xiaohe"));
        m_clock = 0;
    }

    void readout()
    {
        type(m_composition, "vsgou", m_clock);
        QVERIFY(m_composition.isPending());
        QCOMPARE(m_composition.pendingKey(), QChar(u'u'));
        QCOMPARE(m_composition.readout().text, QStringLiteral("zhong  guo  sh + ?"));
        QVERIFY(m_composition.readout().valid);

        type(m_composition, "l", m_clock);
        QVERIFY(!m_composition.isPending());
        QCOMPARE(m_composition.readout().text, QStringLiteral("zhong  guo  shuang"));

        type(m_composition, "<", m_clock);
        QCOMPARE(m_composition.readout().text, QStringLiteral("zhong  guo  sh + ?"));

        type(m_composition, "q", m_clock);
        QCOMPARE(m_composition.readout().text, QStringLiteral("zhong  guo  uq✗"));
        QVERIFY(!m_composition.readout().valid);

        type(m_composition, " ", m_clock);
        QVERIFY(m_composition.isEmpty());
        QCOMPARE(m_composition.readout().text, QString());
    }

    void readoutKeepsTheLastSyllables()
    {
        type(m_composition, "nihcnihcnihcnihc", m_clock);
        QCOMPARE(m_composition.readout(3).text, QStringLiteral("… hao  ni  hao"));
    }

    void backspaceOnEmptyDoesNothing()
    {
        QSignalSpy changed(&m_composition, &Composition::changed);
        type(m_composition, "<<", m_clock);
        QVERIFY(m_composition.isEmpty());
        QCOMPARE(changed.count(), 0);
    }

    void timesTheFinal()
    {
        QSignalSpy timed(&m_composition, &Composition::finalTimed);
        m_composition.key(u'u', 1000);
        m_composition.key(u'l', 1700);
        QCOMPARE(timed.count(), 1);
        QCOMPARE(timed.at(0).at(0).toString(), QStringLiteral("uang"));
        QCOMPARE(timed.at(0).at(1).toInt(), 700);

        // Not a syllable, a syllable without an initial, and a long pause
        // are all left out.
        m_composition.reset();
        m_composition.key(u's', 2000);
        m_composition.key(u'q', 2100);
        m_composition.key(u'a', 2200);
        m_composition.key(u'h', 2300);
        m_composition.key(u'g', 3000);
        m_composition.key(u'o', 60000);
        QCOMPARE(timed.count(), 1);
    }

    void findsASlipInAFinal()
    {
        QSignalSpy slips(&m_composition, &Composition::slipFound);
        type(m_composition, "uk<l", m_clock); // shuai, corrected to shuang
        QCOMPARE(slips.count(), 1);
        const Slip slip = slips.at(0).at(0).value<Slip>();
        QVERIFY(!slip.initial);
        QCOMPARE(slip.meant, QStringLiteral("uang"));
        QCOMPARE(slip.meantKey, QChar(u'l'));
        QCOMPARE(slip.hitKey, QChar(u'k'));
        QCOMPARE(slip.hit, QStringLiteral("uai"));
    }

    void findsASlipInAnInitial()
    {
        QSignalSpy slips(&m_composition, &Composition::slipFound);
        type(m_composition, "zs<<vs", m_clock); // zong, corrected to zhong
        QCOMPARE(slips.count(), 1);
        const Slip slip = slips.at(0).at(0).value<Slip>();
        QVERIFY(slip.initial);
        QCOMPARE(slip.meant, QStringLiteral("zh"));
        QCOMPARE(slip.meantKey, QChar(u'v'));
        QCOMPARE(slip.hitKey, QChar(u'z'));
        QCOMPARE(slip.hit, QStringLiteral("z"));
    }

    void ignoresWhatIsNotASlip()
    {
        QSignalSpy slips(&m_composition, &Composition::slipFound);
        type(m_composition, "uk<k ", m_clock);       // the same key again
        type(m_composition, "nihc<<<<wo ", m_clock); // a change of mind
        type(m_composition, "uk<q ", m_clock);       // replaced by something that is no syllable
        type(m_composition, "uk l", m_clock);        // a reset in between
        QCOMPARE(slips.count(), 0);
    }

private:
    Composition m_composition;
    qint64 m_clock = 0;
};

QTEST_GUILESS_MAIN(TestComposition)
#include "test_composition.moc"

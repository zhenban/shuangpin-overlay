// SPDX-License-Identifier: GPL-3.0-or-later
#include "visibilitypolicy.h"

#include <QTest>

using VisibilityPolicy::Facts;
using VisibilityPolicy::shouldBeShown;
using VisibilityPolicy::showOnKey;

class TestVisibilityPolicy : public QObject
{
    Q_OBJECT

    /// Shown, Shuangpin active, hiding after ten idle seconds.
    static Facts typing()
    {
        Facts facts;
        facts.idleLimitMs = 10000;
        return facts;
    }

private Q_SLOTS:
    void hidesWhenTheKeyboardIsIdle()
    {
        Facts facts = typing();
        facts.idleMs = 9000;
        QVERIFY(shouldBeShown(facts));
        facts.idleMs = 11000;
        QVERIFY(!shouldBeShown(facts));

        facts.idleLimitMs = 0; // "never"
        facts.idleMs = 3600000;
        QVERIFY(shouldBeShown(facts));
    }

    void hidesInAnotherInputMethod()
    {
        Facts facts = typing();
        facts.shuangpinActive = false;
        QVERIFY(!shouldBeShown(facts));
    }

    void waitsMuchLongerForHalfASyllable()
    {
        Facts facts = typing();
        facts.pending = true;
        facts.idleMs = 59000;
        QVERIFY(shouldBeShown(facts));
        facts.idleMs = 61000;
        QVERIFY(!shouldBeShown(facts));

        facts.idleLimitMs = 30000; // six times the limit when that is longer
        facts.idleMs = 170000;
        QVERIFY(shouldBeShown(facts));
        facts.idleMs = 190000;
        QVERIFY(!shouldBeShown(facts));
    }

    void staysWhileHeld()
    {
        Facts facts = typing();
        facts.held = true;
        facts.idleMs = 3600000;
        facts.shuangpinActive = false;
        QVERIFY(shouldBeShown(facts));
    }

    void staysHiddenUntilAKeyBringsItBack()
    {
        Facts facts = typing();
        facts.shown = false;
        QVERIFY(!shouldBeShown(facts));
        facts.held = true; // the pointer over an invisible chart must not show it
        QVERIFY(!shouldBeShown(facts));
        QVERIFY(showOnKey(facts));

        facts.shuangpinActive = false;
        QVERIFY(!showOnKey(facts));
    }

    void onlyWhenStuck()
    {
        Facts facts = typing();
        facts.onlyWhenStuck = true;
        facts.idleMs = 1000;
        QVERIFY(shouldBeShown(facts));
        facts.idleMs = 2000; // lingers briefly after the syllable, then goes
        QVERIFY(!shouldBeShown(facts));
        facts.pending = true; // but waits while the typist is stuck
        QVERIFY(shouldBeShown(facts));

        facts.shown = false;
        QVERIFY(!showOnKey(facts)); // typing alone does not bring it back
    }

    void followsTheInputMethodWithoutAKeyboard()
    {
        Facts facts = typing();
        facts.keyboardAvailable = false;
        facts.idleMs = 3600000;
        QVERIFY(shouldBeShown(facts));
        facts.shuangpinActive = false;
        QVERIFY(!shouldBeShown(facts));
        facts.shown = false;
        facts.shuangpinActive = true;
        QVERIFY(shouldBeShown(facts)); // nothing else could show it again
    }
};

QTEST_GUILESS_MAIN(TestVisibilityPolicy)
#include "test_visibilitypolicy.moc"

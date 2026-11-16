// SPDX-License-Identifier: GPL-3.0-or-later
#include "testutil.h"

#include "typingtracker.h"

class TestTypingTracker : public QObject
{
    Q_OBJECT

    static Key key(Key::Kind kind, int code = 0) { return {kind, QChar(), code}; }

    bool press(const Key &key, bool active = true) { return m_tracker.keyPressed(key, ++m_clock, active); }
    void type(const char *letters)
    {
        for (const char *letter = letters; *letter; ++letter)
            press(Key::letter(QLatin1Char(*letter)));
    }

private Q_SLOTS:
    void init()
    {
        m_composition.setScheme(shippedScheme("xiaohe"));
        m_tracker = TypingTracker(&m_composition);
    }

    void lettersExtendTheComposition()
    {
        QVERIFY(press(Key::letter(u'v')));
        QVERIFY(press(Key::letter(u's')));
        QCOMPARE(m_composition.keys(), QStringLiteral("vs"));
    }

    void backspaceShortensIt()
    {
        type("vsg");
        QVERIFY(!press(key(Key::Kind::Backspace)));
        QCOMPARE(m_composition.keys(), QStringLiteral("vs"));
    }

    void otherKeysEndIt()
    {
        type("vs");
        QVERIFY(!press(key(Key::Kind::Other)));
        QVERIFY(m_composition.isEmpty());
    }

    void keysTheSchemeDoesNotUseEndIt()
    {
        type("vs");
        QVERIFY(!press(Key::letter(u';'))); // Xiaohe has no use for ';'
        QVERIFY(m_composition.isEmpty());

        m_composition.setScheme(shippedScheme("microsoft"));
        type("y");
        QVERIFY(press(Key::letter(u';')));
        QCOMPARE(m_composition.readout().text, QStringLiteral("ying"));
    }

    void aLetterWithAModifierIsAShortcut()
    {
        type("vs");
        const Key control = key(Key::Kind::Modifier, 29);
        QVERIFY(!press(control));
        QCOMPARE(m_composition.keys(), QStringLiteral("vs")); // the modifier alone changes nothing
        QVERIFY(!press(Key::letter(u'c')));
        QVERIFY(m_composition.isEmpty());

        m_tracker.keyReleased(control);
        QVERIFY(press(Key::letter(u'c')));
        QCOMPARE(m_composition.keys(), QStringLiteral("c"));
    }

    void modifiersAreTrackedPerKey()
    {
        const Key leftShift = key(Key::Kind::Modifier, 42);
        const Key rightShift = key(Key::Kind::Modifier, 54);
        press(leftShift);
        press(rightShift);
        m_tracker.keyReleased(leftShift);
        QVERIFY(!press(Key::letter(u'a'))); // the right one is still down
        m_tracker.keyReleased(rightShift);
        QVERIFY(press(Key::letter(u'a')));
    }

    void nothingIsTrackedInAnotherInputMethod()
    {
        type("vs");
        QVERIFY(!press(Key::letter(u'g'), false));
        QVERIFY(m_composition.isEmpty());

        type("g");
        m_tracker.inputMethodLeft();
        QVERIFY(m_composition.isEmpty());
    }

private:
    Composition m_composition;
    TypingTracker m_tracker{&m_composition};
    qint64 m_clock = 0;
};

QTEST_GUILESS_MAIN(TestTypingTracker)
#include "test_typingtracker.moc"

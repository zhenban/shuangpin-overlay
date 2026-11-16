// SPDX-License-Identifier: GPL-3.0-or-later
#include "testutil.h"

#include "pinyin.h"
#include "schemelibrary.h"

class TestScheme : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pairs_data()
    {
        QTest::addColumn<QString>("scheme");
        QTest::addColumn<QString>("keys");
        QTest::addColumn<QStringList>("expected");

        auto row = [](const char *scheme, const char *keys, const QStringList &expected) {
            QTest::addRow("%s %s", scheme, keys) << QString::fromLatin1(scheme) << QString::fromLatin1(keys) << expected;
        };
        // Xiaohe
        row("xiaohe", "ul", {QStringLiteral("shuang")});
        row("xiaohe", "vs", {QStringLiteral("zhong")});
        row("xiaohe", "go", {QStringLiteral("guo")});
        row("xiaohe", "bo", {QStringLiteral("bo")});
        row("xiaohe", "xs", {QStringLiteral("xiong")});
        row("xiaohe", "ju", {QStringLiteral("ju")});
        row("xiaohe", "nv", {QStringLiteral("nü")});
        row("xiaohe", "lt", {QStringLiteral("lüe")});
        row("xiaohe", "yt", {QStringLiteral("yue")});
        row("xiaohe", "sq", {});
        // Xiaohe types syllables without an initial by their first letter.
        row("xiaohe", "aa", {QStringLiteral("a")});
        row("xiaohe", "ai", {QStringLiteral("ai")});
        row("xiaohe", "ah", {QStringLiteral("ang")});
        row("xiaohe", "eg", {QStringLiteral("eng")});
        row("xiaohe", "er", {QStringLiteral("er")});
        row("xiaohe", "ou", {QStringLiteral("ou")});
        // Microsoft uses the O key instead, and ';' for ing.
        row("microsoft", "oh", {QStringLiteral("ang")});
        row("microsoft", "or", {QStringLiteral("er")});
        row("microsoft", "y;", {QStringLiteral("ying")});
        row("microsoft", "ny", {QStringLiteral("nü")});
        row("microsoft", "lv", {QStringLiteral("lüe")});
        row("microsoft", "jt", {QStringLiteral("jue")});
        // Ziranma accepts both ways.
        row("ziranma", "oh", {QStringLiteral("ang")});
        row("ziranma", "ah", {QStringLiteral("ang")});
        row("ziranma", "aa", {QStringLiteral("a")});
        // Where one key carries two initials, a pair can mean two syllables.
        row("abc", "ah", {QStringLiteral("zhang")});
        row("ziguang", "ar", {QStringLiteral("chan")});
    }

    void pairs()
    {
        QFETCH(QString, scheme);
        QFETCH(QString, keys);
        QFETCH(QStringList, expected);
        const Scheme s = shippedScheme(scheme.toLatin1().constData());
        QCOMPARE(texts(s.resolve(keys.at(0), keys.at(1))), expected);
    }

    void everyShippedSchemeIsComplete_data()
    {
        QTest::addColumn<QString>("id");
        for (const char *id : {"abc", "guobiao", "microsoft", "pinyinjiajia", "xiaohe", "zhongwenzhixing",
                               "ziguang", "ziranma"})
            QTest::addRow("%s", id) << QString::fromLatin1(id);
    }

    /// Every syllable of the language must be reachable with two keys.
    void everyShippedSchemeIsComplete()
    {
        QFETCH(QString, id);
        const Scheme scheme = shippedScheme(id.toLatin1().constData());
        QVERIFY(!scheme.displayName().isEmpty());
        QVERIFY(!scheme.fcitxProfile().isEmpty());

        QSet<QString> reachable;
        const QString keys = QStringLiteral("abcdefghijklmnopqrstuvwxyz;");
        for (const QChar first : keys) {
            const auto continuations = scheme.continuations(first);
            for (const QList<Syllable> &syllables : continuations) {
                for (const Syllable &syllable : syllables)
                    reachable.insert(syllable.text());
            }
        }
        for (const QString &initial : Pinyin::initials()) {
            for (const QString &final : Pinyin::finalsFor(initial)) {
                // Xiaohe has no key for "er"; it is typed as written.
                QVERIFY2(reachable.contains(initial + final), qUtf8Printable(initial + final));
            }
        }
    }

    void continuationsNarrowTheChoice()
    {
        const Scheme xiaohe = shippedScheme("xiaohe");
        const auto afterSh = xiaohe.continuations(u'u');
        QCOMPARE(texts(afterSh.value(u'k')), QStringList{QStringLiteral("shuai")});
        QCOMPARE(texts(afterSh.value(u'l')), QStringList{QStringLiteral("shuang")});
        QVERIFY(!afterSh.contains(u's')); // no "shong"
        QVERIFY(!afterSh.contains(u'q')); // no "shiu"

        const auto afterA = xiaohe.continuations(u'a');
        QString keys;
        for (auto it = afterA.constBegin(); it != afterA.constEnd(); ++it)
            keys += it.key();
        QCOMPARE(keys, QStringLiteral("ahino"));
    }

    void continuationLabels()
    {
        const Scheme xiaohe = shippedScheme("xiaohe");
        // With an initial the key shows the final; without, the syllable.
        QCOMPARE(xiaohe.continuationLabels(u'u').value(u'l'), QStringList{QStringLiteral("uang")});
        QCOMPARE(xiaohe.continuationLabels(u'a').value(u'h'), QStringList{QStringLiteral("ang")});
        QCOMPARE(xiaohe.continuationLabels(u'a').value(u'i'), QStringList{QStringLiteral("ai")});
    }

    void library()
    {
        const SchemeLibrary library = SchemeLibrary::load();
        QCOMPARE(library.all().size(), 8);
        QCOMPARE(library.find(QStringLiteral("ziranma"))->fcitxProfile(), QStringLiteral("Ziranma"));
        QCOMPARE(library.findByFcitxProfile(QStringLiteral("MS"))->id(), QStringLiteral("microsoft"));
        QVERIFY(!library.find(QStringLiteral("nope")));
        QVERIFY(!library.findByFcitxProfile(QString()));
        QVERIFY(!library.findByFcitxProfile(QStringLiteral("Custom")));
        QCOMPARE(library.fallback()->id(), SchemeLibrary::defaultId());

        // A scheme added under an existing id replaces it.
        SchemeLibrary changed = library;
        changed.add(*Scheme::fromJson(R"({"id": "xiaohe", "finals": {"ai": "q"}})"));
        QCOMPARE(changed.all().size(), 8);
        QCOMPARE(changed.find(QStringLiteral("xiaohe"))->keyForFinal(QStringLiteral("ai")), QChar(u'q'));
        QVERIFY(!SchemeLibrary().fallback());
    }

    void labels()
    {
        const Scheme xiaohe = shippedScheme("xiaohe");
        QCOMPARE(xiaohe.initialLabels(u'v'), QStringList{QStringLiteral("zh")});
        QCOMPARE(xiaohe.finalLabels(u'k'), (QStringList{QStringLiteral("ing"), QStringLiteral("uai")}));
        QCOMPARE(xiaohe.finalLabels(u't'), QStringList{QStringLiteral("ue")});
        QCOMPARE(xiaohe.firstKeyMeanings(u'u'), QStringList{QStringLiteral("sh")});
        QVERIFY(!xiaohe.usesSemicolon());
        QVERIFY(shippedScheme("microsoft").usesSemicolon());
        QCOMPARE(xiaohe.summary(), QStringLiteral("zh=v  ch=i  sh=u"));
    }

    void rejectsBrokenFiles()
    {
        QString error;
        QVERIFY(!Scheme::fromJson("not json", &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!Scheme::fromJson(R"({"finals": {"ai": "d"}})", &error));
        QVERIFY(!Scheme::fromJson(R"({"id": "x"})", &error));
        QVERIFY(!Scheme::fromJson(R"({"id": "x", "finals": {"ai": "dd"}})", &error));
        QVERIFY(Scheme::fromJson(R"({"id": "x", "finals": {"ai": "d"}})", &error));
    }
};

QTEST_GUILESS_MAIN(TestScheme)
#include "test_scheme.moc"

// SPDX-License-Identifier: GPL-3.0-or-later
#include "scheme.h"

#include "pinyin.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>

#include <algorithm>

namespace {

const QString kZeroMark = QStringLiteral("∅");

// Order in which finals are listed on a key.
const QStringList &finalOrder()
{
    static const QStringList order = QStringLiteral(
        "a o e i u ai ei ao ou an en ang eng ong er ia ie iao iu ian in iang ing iong "
        "ua uo uai ui uan un uang ue üe ü").split(u' ');
    return order;
}

bool readKey(const QJsonValue &value, QChar *key)
{
    const QString s = value.toString().toLower();
    if (s.size() != 1)
        return false;
    *key = s.at(0);
    return true;
}

} // namespace

std::optional<Scheme> Scheme::fromJson(const QByteArray &json, QString *error)
{
    auto fail = [error](const QString &message) -> std::optional<Scheme> {
        if (error)
            *error = message;
        return std::nullopt;
    };

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject())
        return fail(parseError.errorString());
    const QJsonObject root = doc.object();

    Scheme scheme;
    scheme.m_id = root.value(QLatin1String("id")).toString();
    if (scheme.m_id.isEmpty())
        return fail(QStringLiteral("missing \"id\""));
    scheme.m_fcitxProfile = root.value(QLatin1String("fcitxProfile")).toString();

    const QJsonObject names = root.value(QLatin1String("name")).toObject();
    for (auto it = names.begin(); it != names.end(); ++it)
        scheme.m_names.insert(it.key(), it.value().toString());

    const QJsonObject initials = root.value(QLatin1String("initials")).toObject();
    for (auto it = initials.begin(); it != initials.end(); ++it) {
        QChar key;
        if (!readKey(it.value(), &key))
            return fail(QStringLiteral("initial \"%1\" needs a single key").arg(it.key()));
        scheme.m_initialKeys.insert(it.key(), key);
    }

    const QJsonObject finals = root.value(QLatin1String("finals")).toObject();
    if (finals.isEmpty())
        return fail(QStringLiteral("missing \"finals\""));
    for (auto it = finals.begin(); it != finals.end(); ++it) {
        QChar key;
        if (!readKey(it.value(), &key))
            return fail(QStringLiteral("final \"%1\" needs a single key").arg(it.key()));
        scheme.m_finalKeys.insert(it.key(), key);
    }

    const QJsonObject zero = root.value(QLatin1String("zeroInitial")).toObject();
    scheme.m_zeroKeys = zero.value(QLatin1String("keys")).toString().toLower();
    scheme.m_zeroByFirstLetter = zero.value(QLatin1String("byFirstLetter")).toBool();

    scheme.build();
    return scheme;
}

void Scheme::add(QChar first, QChar second, const Syllable &syllable)
{
    QList<Syllable> &list = m_table[QString(first) + second];
    if (!list.contains(syllable))
        list.append(syllable);
    for (QChar key : {first, second}) {
        if (!m_keys.contains(key))
            m_keys.append(key);
    }
}

void Scheme::build()
{
    // The five one-letter finals sit on their own keys unless a scheme says
    // otherwise.
    for (const char letter : {'a', 'o', 'e', 'i', 'u'}) {
        const QString final(QLatin1Char{letter});
        if (!m_finalKeys.contains(final))
            m_finalKeys.insert(final, QLatin1Char(letter));
    }

    auto note = [this](QChar key, const QString &meaning) {
        QStringList &list = m_firstKeyInitials[key];
        if (!list.contains(meaning))
            list.append(meaning);
    };

    // Ordinary syllables: initial key followed by final key.
    for (const QString &initial : Pinyin::initials()) {
        if (initial.isEmpty())
            continue;
        QChar first;
        if (m_initialKeys.contains(initial))
            first = m_initialKeys.value(initial);
        else if (initial.size() == 1)
            first = initial.at(0);
        else
            continue; // a scheme without a key for zh, ch or sh
        note(first, initial);
        if (initial.size() > 1)
            m_initialLabels[first].append(initial);
        for (const QString &final : Pinyin::finalsFor(initial)) {
            const auto key = m_finalKeys.constFind(final);
            if (key != m_finalKeys.constEnd())
                add(first, *key, {initial, final});
        }
    }

    // Syllables without an initial.
    const QStringList zeroFinals = Pinyin::finalsFor(QString());
    for (const QChar zeroKey : std::as_const(m_zeroKeys)) {
        for (const QString &final : zeroFinals) {
            const auto key = m_finalKeys.constFind(final);
            if (key == m_finalKeys.constEnd())
                continue;
            add(zeroKey, *key, {QString(), final});
            note(zeroKey, kZeroMark);
        }
    }
    if (m_zeroByFirstLetter) {
        // One letter: type it twice. Two letters: as written. Longer: the
        // first letter, then the final's key.
        for (const QString &final : zeroFinals) {
            const QChar first = final.at(0);
            if (final.size() == 1) {
                add(first, first, {QString(), final});
            } else if (final.size() == 2) {
                add(first, final.at(1), {QString(), final});
            } else {
                const auto key = m_finalKeys.constFind(final);
                if (key == m_finalKeys.constEnd())
                    continue;
                add(first, *key, {QString(), final});
            }
            note(first, kZeroMark);
        }
    }
    // Two-letter syllables typed as written still work wherever the pair is
    // free (this mirrors what Fcitx accepts).
    for (const QString &final : zeroFinals) {
        if (final.size() == 2 && !m_table.contains(final)) {
            add(final.at(0), final.at(1), {QString(), final});
            note(final.at(0), kZeroMark);
        }
    }

    for (auto it = m_finalKeys.constBegin(); it != m_finalKeys.constEnd(); ++it)
        m_finalLabels[it.value()].append(it.key());
    for (QStringList &labels : m_finalLabels) {
        // "ue" and "üe" are the same sound to the typist; print it once.
        if (labels.contains(QStringLiteral("ue")))
            labels.removeAll(QStringLiteral("üe"));
        std::sort(labels.begin(), labels.end(), [](const QString &a, const QString &b) {
            return finalOrder().indexOf(a) < finalOrder().indexOf(b);
        });
    }
    for (QStringList &labels : m_initialLabels)
        labels.sort();

    m_usesSemicolon = m_keys.contains(u';');
}

QString Scheme::displayName() const
{
    const QString language = QLocale().name().section(u'_', 0, 0);
    QString name = m_names.value(language);
    if (name.isEmpty())
        name = m_names.value(QStringLiteral("en"));
    return name.isEmpty() ? m_id : name;
}

QStringList Scheme::firstKeyMeanings(QChar key) const
{
    return m_firstKeyInitials.value(key);
}

QList<Syllable> Scheme::resolve(QChar first, QChar second) const
{
    return m_table.value(QString(first) + second);
}

QMap<QChar, QList<Syllable>> Scheme::continuations(QChar first) const
{
    QMap<QChar, QList<Syllable>> result;
    for (auto it = m_table.constBegin(); it != m_table.constEnd(); ++it) {
        if (it.key().at(0) == first)
            result.insert(it.key().at(1), it.value());
    }
    return result;
}

QMap<QChar, QStringList> Scheme::continuationLabels(QChar first) const
{
    QMap<QChar, QStringList> labels;
    const QMap<QChar, QList<Syllable>> next = continuations(first);
    for (auto it = next.constBegin(); it != next.constEnd(); ++it) {
        QStringList &list = labels[it.key()];
        for (const Syllable &syllable : it.value()) {
            const QString label = syllable.initial.isEmpty() ? syllable.text() : syllable.final;
            if (!list.contains(label))
                list.append(label);
        }
    }
    return labels;
}

QString Scheme::summary() const
{
    QStringList parts;
    for (const QString &initial : {QStringLiteral("zh"), QStringLiteral("ch"), QStringLiteral("sh")}) {
        if (m_initialKeys.contains(initial))
            parts << initial + u'=' + m_initialKeys.value(initial);
    }
    return parts.join(QStringLiteral("  "));
}

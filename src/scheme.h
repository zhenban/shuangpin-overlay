// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QChar>
#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>

/// One syllable a pair of keys can stand for. An empty initial is the zero
/// initial.
struct Syllable {
    QString initial;
    QString final;

    QString text() const { return initial + final; }
    bool operator==(const Syllable &) const = default;
};

/// A Shuangpin scheme: which key carries which initial and final, and how
/// syllables without an initial are typed. Keys are lower-case letters, plus
/// ';' in the schemes that use it.
class Scheme
{
public:
    /// Parses a scheme file. On failure returns nothing and, if given,
    /// fills in @p error.
    static std::optional<Scheme> fromJson(const QByteArray &json, QString *error = nullptr);

    QString id() const { return m_id; }
    /// Value of ShuangpinProfile in Fcitx5's pinyin.conf, if there is one.
    QString fcitxProfile() const { return m_fcitxProfile; }
    /// Name in the current UI language, falling back to English.
    QString displayName() const;

    bool usesSemicolon() const { return m_usesSemicolon; }
    /// True if the key can start or end a syllable in this scheme.
    bool isKey(QChar key) const { return m_keys.contains(key); }

    /// Two-letter initials (zh, ch, sh) printed on the key.
    QStringList initialLabels(QChar key) const { return m_initialLabels.value(key); }
    /// Finals printed on the key when no syllable is in progress.
    QStringList finalLabels(QChar key) const { return m_finalLabels.value(key); }
    /// What the key means as the first key of a syllable: its initials,
    /// with "∅" standing for the zero initial.
    QStringList firstKeyMeanings(QChar key) const;

    /// The key a final is typed with; null if the scheme has none.
    QChar keyForFinal(const QString &final) const { return m_finalKeys.value(final); }

    /// Every syllable the two keys can spell; empty if there is none.
    QList<Syllable> resolve(QChar first, QChar second) const;
    /// For a given first key, every second key that completes a syllable.
    QMap<QChar, QList<Syllable>> continuations(QChar first) const;

    /// The same, as the text to print on each of those keys: the final it
    /// would add, or the whole syllable when there is no initial.
    QMap<QChar, QStringList> continuationLabels(QChar first) const;

    /// One-line reminder of the scheme's special keys, e.g. "zh=v  ch=i".
    QString summary() const;

private:
    void build();
    void add(QChar first, QChar second, const Syllable &syllable);

    QString m_id;
    QString m_fcitxProfile;
    QHash<QString, QString> m_names;
    QHash<QString, QChar> m_initialKeys; // zh, ch, sh -> key
    QHash<QString, QChar> m_finalKeys;   // final -> key
    QString m_zeroKeys;
    bool m_zeroByFirstLetter = false;

    bool m_usesSemicolon = false;
    QString m_keys;
    QHash<QChar, QStringList> m_initialLabels;
    QHash<QChar, QStringList> m_finalLabels;
    QHash<QChar, QStringList> m_firstKeyInitials;
    QHash<QString, QList<Syllable>> m_table; // two keys -> syllables
};

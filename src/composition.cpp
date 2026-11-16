// SPDX-License-Identifier: GPL-3.0-or-later
#include "composition.h"

namespace {

// Pauses longer than this are not typing speed; the typist was elsewhere.
constexpr int kLongestPauseMs = 6000;
// Deleting more than one syllable usually means a change of mind, not a slip.
constexpr int kLongestSlipRun = 2;

const QString kCross = QStringLiteral("✗");

QString describe(const QList<Syllable> &syllables)
{
    QStringList texts;
    for (const Syllable &syllable : syllables)
        texts << syllable.text();
    return texts.join(u'/');
}

} // namespace

Composition::Composition(QObject *parent)
    : QObject(parent)
{
}

void Composition::setScheme(const Scheme &scheme)
{
    m_scheme = scheme;
    reset();
}

void Composition::key(QChar key, qint64 timeMs)
{
    checkSlip(key);
    m_keys.append(key);
    if (isPending()) {
        m_pendingSince = timeMs;
    } else {
        const QList<Syllable> syllables = m_scheme.resolve(m_keys.at(m_keys.size() - 2), key);
        const qint64 pause = timeMs - m_pendingSince;
        // Only time syllables with an initial: that is where a final has to
        // be recalled.
        if (syllables.size() == 1 && !syllables.first().initial.isEmpty() && pause <= kLongestPauseMs)
            Q_EMIT finalTimed(syllables.first().final, int(pause));
    }
    Q_EMIT changed();
}

void Composition::backspace(qint64 timeMs)
{
    if (m_keys.isEmpty())
        return;
    m_deleted.insert(m_keys.size() - 1, m_keys.back());
    m_keys.chop(1);
    ++m_backspaceRun;
    m_pendingSince = timeMs;
    Q_EMIT changed();
}

void Composition::reset()
{
    const bool wasEmpty = m_keys.isEmpty();
    m_keys.clear();
    m_deleted.clear();
    m_backspaceRun = 0;
    m_lastRun = 0;
    if (!wasEmpty)
        Q_EMIT changed();
}

void Composition::checkSlip(QChar key)
{
    const int position = m_keys.size();
    if (m_backspaceRun > 0) {
        m_lastRun = m_backspaceRun;
        m_backspaceRun = 0;
    }
    const auto deleted = m_deleted.constFind(position);
    if (deleted == m_deleted.constEnd())
        return;
    const QChar wrong = *deleted;
    m_deleted.erase(deleted);
    if (wrong == key || m_lastRun > kLongestSlipRun)
        return;

    Slip slip;
    slip.meantKey = key;
    slip.hitKey = wrong;
    if (position % 2 == 0) {
        slip.initial = true;
        slip.meant = m_scheme.firstKeyMeanings(key).join(u'/');
        slip.hit = m_scheme.firstKeyMeanings(wrong).join(u'/');
        if (slip.meant.isEmpty())
            return;
    } else {
        const QChar first = m_keys.back();
        const QList<Syllable> meant = m_scheme.resolve(first, key);
        if (meant.isEmpty())
            return; // the replacement is not a syllable either
        const QList<Syllable> hit = m_scheme.resolve(first, wrong);
        slip.meant = meant.first().final;
        slip.hit = hit.isEmpty() ? QString() : hit.first().final;
    }
    Q_EMIT slipFound(slip);
}

Composition::Readout Composition::readout(int maxSyllables) const
{
    Readout result;
    QStringList words;
    const int pairs = m_keys.size() / 2;
    for (int pair = qMax(0, pairs - maxSyllables); pair < pairs; ++pair) {
        const QString keys = m_keys.mid(pair * 2, 2);
        const QList<Syllable> syllables = m_scheme.resolve(keys.at(0), keys.at(1));
        if (syllables.isEmpty())
            result.valid = false;
        words << (syllables.isEmpty() ? keys + kCross : describe(syllables));
    }
    if (isPending()) {
        QString meaning = m_scheme.firstKeyMeanings(pendingKey()).join(u'/');
        if (meaning.isEmpty()) {
            result.valid = false;
            meaning = QString(pendingKey()) + kCross;
        }
        words << meaning + QStringLiteral(" + ?");
    }
    result.text = words.join(QStringLiteral("  "));
    if (pairs > maxSyllables)
        result.text.prepend(QStringLiteral("… "));
    return result;
}

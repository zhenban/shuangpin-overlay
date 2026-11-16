// SPDX-License-Identifier: GPL-3.0-or-later
#include "keyhighlights.h"

namespace {

constexpr int kFadeMs = 350;
constexpr int kFrameMs = 30;

} // namespace

KeyHighlights::KeyHighlights(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
    m_fadeTimer.setInterval(kFrameMs);
    connect(&m_fadeTimer, &QTimer::timeout, this, &KeyHighlights::fade);
}

void KeyHighlights::press(QChar key)
{
    m_down.insert(key);
    m_releasedAt.remove(key);
    Q_EMIT changed();
}

void KeyHighlights::release(QChar key)
{
    if (!m_down.remove(key))
        return;
    m_releasedAt.insert(key, m_clock.elapsed());
    m_fadeTimer.start();
    Q_EMIT changed();
}

void KeyHighlights::fade()
{
    const qint64 now = m_clock.elapsed();
    m_releasedAt.removeIf([now](const auto &entry) { return now - entry.value() >= kFadeMs; });
    if (m_releasedAt.isEmpty())
        m_fadeTimer.stop();
    Q_EMIT changed();
}

QHash<QChar, qreal> KeyHighlights::strengths() const
{
    QHash<QChar, qreal> result;
    const qint64 now = m_clock.elapsed();
    for (auto it = m_releasedAt.constBegin(); it != m_releasedAt.constEnd(); ++it)
        result.insert(it.key(), qBound(0.0, 1.0 - qreal(now - it.value()) / kFadeMs, 1.0));
    for (const QChar key : m_down)
        result.insert(key, 1.0);
    return result;
}

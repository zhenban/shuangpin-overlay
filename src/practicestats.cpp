// SPDX-License-Identifier: GPL-3.0-or-later
#include "practicestats.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>

namespace {

constexpr int kMinSamples = 5;    // per final, before it is judged
constexpr int kMinFinals = 8;     // finals judged, before any is called slow
constexpr double kSlowFactor = 1.25;
constexpr int kAveragingWindow = 30; // recent typing counts for more
constexpr int kMinSlipCount = 2;
constexpr int kSaveDelayMs = 10000;

QString encode(const Slip &slip)
{
    return QStringList{slip.initial ? QStringLiteral("i") : QStringLiteral("f"), slip.meant,
                       QString(slip.meantKey), QString(slip.hitKey), slip.hit}.join(u'|');
}

Slip decode(const QString &text)
{
    const QStringList parts = text.split(u'|');
    Slip slip;
    if (parts.size() != 5 || parts.at(2).size() != 1 || parts.at(3).size() != 1)
        return slip;
    slip.initial = parts.at(0) == QLatin1String("i");
    slip.meant = parts.at(1);
    slip.meantKey = parts.at(2).at(0);
    slip.hitKey = parts.at(3).at(0);
    slip.hit = parts.at(4);
    return slip;
}

} // namespace

PracticeStats::PracticeStats(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, &PracticeStats::save);
    load();
}

PracticeStats::~PracticeStats()
{
    if (m_saveTimer.isActive())
        save();
}

void PracticeStats::saveSoon()
{
    if (!m_saveTimer.isActive())
        m_saveTimer.start();
}

QString PracticeStats::defaultFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/stats.json");
}

void PracticeStats::setScheme(const QString &schemeId)
{
    m_scheme = schemeId;
}

void PracticeStats::addFinalTime(const QString &final, int ms)
{
    Timing &timing = m_data[m_scheme].finals[final];
    ++timing.samples;
    timing.averageMs += (ms - timing.averageMs) / qMin(timing.samples, kAveragingWindow);
    saveSoon();
}

void PracticeStats::addSlip(const Slip &slip)
{
    ++m_data[m_scheme].slips[encode(slip)];
    saveSoon();
}

void PracticeStats::clear()
{
    m_data.remove(m_scheme);
    save();
}

QList<PracticeStats::SlowFinal> PracticeStats::slowest(int count) const
{
    QList<SlowFinal> judged;
    const SchemeData data = m_data.value(m_scheme);
    for (auto it = data.finals.constBegin(); it != data.finals.constEnd(); ++it) {
        if (it->samples >= kMinSamples)
            judged.append({it.key(), int(it->averageMs)});
    }
    if (judged.size() < kMinFinals)
        return {};
    std::sort(judged.begin(), judged.end(),
              [](const SlowFinal &a, const SlowFinal &b) { return a.averageMs > b.averageMs; });
    const int median = judged.at(judged.size() / 2).averageMs;

    QList<SlowFinal> slow;
    for (const SlowFinal &entry : std::as_const(judged)) {
        if (slow.size() == count || entry.averageMs <= median * kSlowFactor)
            break;
        slow.append(entry);
    }
    return slow;
}

QList<PracticeStats::FrequentSlip> PracticeStats::frequentSlips(int count) const
{
    QList<FrequentSlip> slips;
    const SchemeData data = m_data.value(m_scheme);
    for (auto it = data.slips.constBegin(); it != data.slips.constEnd(); ++it) {
        if (it.value() >= kMinSlipCount)
            slips.append({decode(it.key()), it.value()});
    }
    std::sort(slips.begin(), slips.end(),
              [](const FrequentSlip &a, const FrequentSlip &b) { return a.count > b.count; });
    return slips.mid(0, count);
}

void PracticeStats::load()
{
    if (m_filePath.isEmpty())
        return;
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    for (auto scheme = root.begin(); scheme != root.end(); ++scheme) {
        SchemeData &data = m_data[scheme.key()];
        const QJsonObject finals = scheme->toObject().value(QLatin1String("finals")).toObject();
        for (auto it = finals.begin(); it != finals.end(); ++it) {
            const QJsonArray pair = it->toArray();
            data.finals.insert(it.key(), {pair.at(0).toInt(), pair.at(1).toDouble()});
        }
        const QJsonObject slips = scheme->toObject().value(QLatin1String("slips")).toObject();
        for (auto it = slips.begin(); it != slips.end(); ++it)
            data.slips.insert(it.key(), it->toInt());
    }
}

void PracticeStats::save()
{
    m_saveTimer.stop();
    if (m_filePath.isEmpty())
        return;
    QJsonObject root;
    for (auto scheme = m_data.constBegin(); scheme != m_data.constEnd(); ++scheme) {
        QJsonObject finals;
        for (auto it = scheme->finals.constBegin(); it != scheme->finals.constEnd(); ++it)
            finals.insert(it.key(), QJsonArray{it->samples, qRound(it->averageMs)});
        QJsonObject slips;
        for (auto it = scheme->slips.constBegin(); it != scheme->slips.constEnd(); ++it)
            slips.insert(it.key(), it.value());
        root.insert(scheme.key(), QJsonObject{{QStringLiteral("finals"), finals}, {QStringLiteral("slips"), slips}});
    }
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QSaveFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

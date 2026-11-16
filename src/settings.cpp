// SPDX-License-Identifier: GPL-3.0-or-later
#include "settings.h"

#include <QCoreApplication>
#include <QStandardPaths>

namespace {

const QString kOpacity = QStringLiteral("opacity");
const QString kScale = QStringLiteral("scale");
const QString kIdleSeconds = QStringLiteral("idleSeconds");
const QString kHints = QStringLiteral("hints");
const QString kOnlyWhenStuck = QStringLiteral("onlyWhenStuck");
const QString kOverlaySurface = QStringLiteral("overlaySurface");
const QString kScheme = QStringLiteral("scheme");
const QString kPosition = QStringLiteral("position");
const QString kInputMethodNames = QStringLiteral("inputMethodNames");

struct Range {
    qreal minimum;
    qreal maximum;
    qreal initial;
    qreal step;

    qreal clamp(qreal value) const { return qBound(minimum, value, maximum); }
};
constexpr Range kOpacityRange{0.2, 1.0, 0.92, 0.05};
constexpr Range kScaleRange{0.6, 2.5, 1.0, 0.1};

QString filePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QLatin1Char('/') + QCoreApplication::applicationName() + QStringLiteral(".conf");
}

} // namespace

Settings::Settings()
    : m_settings(filePath(), QSettings::IniFormat)
{
}

qreal Settings::opacity() const
{
    return kOpacityRange.clamp(m_settings.value(kOpacity, kOpacityRange.initial).toDouble());
}

void Settings::stepOpacity(int steps)
{
    m_settings.setValue(kOpacity, kOpacityRange.clamp(opacity() + steps * kOpacityRange.step));
}

qreal Settings::scale() const
{
    return kScaleRange.clamp(m_settings.value(kScale, kScaleRange.initial).toDouble());
}

void Settings::stepScale(int steps)
{
    m_settings.setValue(kScale, kScaleRange.clamp(scale() + steps * kScaleRange.step));
}

int Settings::idleSeconds() const
{
    return qMax(0, m_settings.value(kIdleSeconds, 10).toInt());
}

void Settings::setIdleSeconds(int seconds)
{
    m_settings.setValue(kIdleSeconds, seconds);
}

bool Settings::hintsEnabled() const
{
    return m_settings.value(kHints, true).toBool();
}

void Settings::setHintsEnabled(bool enabled)
{
    m_settings.setValue(kHints, enabled);
}

bool Settings::onlyWhenStuck() const
{
    return m_settings.value(kOnlyWhenStuck, false).toBool();
}

void Settings::setOnlyWhenStuck(bool enabled)
{
    m_settings.setValue(kOnlyWhenStuck, enabled);
}

bool Settings::overlaySurface() const
{
    return m_settings.value(kOverlaySurface, false).toBool();
}

void Settings::setOverlaySurface(bool enabled)
{
    m_settings.setValue(kOverlaySurface, enabled);
}

QString Settings::scheme() const
{
    return m_settings.value(kScheme).toString();
}

void Settings::setScheme(const QString &id)
{
    m_settings.setValue(kScheme, id);
}

std::optional<QPoint> Settings::position() const
{
    const QVariant value = m_settings.value(kPosition);
    if (!value.isValid())
        return std::nullopt;
    return value.toPoint();
}

void Settings::setPosition(const QPoint &position)
{
    m_settings.setValue(kPosition, position);
}

QStringList Settings::inputMethodNames() const
{
    return m_settings.value(kInputMethodNames, QStringList{QStringLiteral("shuangpin")}).toStringList();
}

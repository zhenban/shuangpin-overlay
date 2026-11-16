// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "scheme.h"

#include <QFile>
#include <QStringList>
#include <QTest>

/// Loads one of the schemes shipped with the application.
inline Scheme shippedScheme(const char *id)
{
    QFile file(QStringLiteral(":/schemes/%1.json").arg(QLatin1String(id)));
    if (!file.open(QIODevice::ReadOnly))
        qFatal("scheme %s is not in the resources", id);
    QString error;
    const std::optional<Scheme> scheme = Scheme::fromJson(file.readAll(), &error);
    if (!scheme)
        qFatal("scheme %s: %s", id, qUtf8Printable(error));
    return *scheme;
}

inline QStringList texts(const QList<Syllable> &syllables)
{
    QStringList result;
    for (const Syllable &syllable : syllables)
        result << syllable.text();
    result.sort();
    return result;
}

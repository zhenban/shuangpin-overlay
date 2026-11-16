// SPDX-License-Identifier: GPL-3.0-or-later
#include "schemelibrary.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <algorithm>

namespace {

template<typename Predicate>
const Scheme *findIf(const QList<Scheme> &schemes, Predicate predicate)
{
    const auto it = std::find_if(schemes.cbegin(), schemes.cend(), predicate);
    return it == schemes.cend() ? nullptr : &*it;
}

} // namespace

QString SchemeLibrary::userDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/schemes");
}

QString SchemeLibrary::defaultId()
{
    return QStringLiteral("xiaohe");
}

SchemeLibrary SchemeLibrary::load()
{
    SchemeLibrary library;
    for (const QString &directory : {QStringLiteral(":/schemes"), userDirectory()}) {
        const QDir dir(directory);
        const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
        for (const QString &name : files) {
            QFile file(dir.filePath(name));
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QString error;
            const std::optional<Scheme> scheme = Scheme::fromJson(file.readAll(), &error);
            if (scheme)
                library.add(*scheme);
            else
                qWarning("Ignoring scheme %s: %s", qUtf8Printable(file.fileName()), qUtf8Printable(error));
        }
    }
    return library;
}

void SchemeLibrary::add(const Scheme &scheme)
{
    const auto same = std::find_if(m_schemes.begin(), m_schemes.end(),
                                   [&](const Scheme &s) { return s.id() == scheme.id(); });
    if (same != m_schemes.end())
        *same = scheme;
    else
        m_schemes.append(scheme);
}

const Scheme *SchemeLibrary::find(const QString &id) const
{
    return findIf(m_schemes, [&](const Scheme &s) { return s.id() == id; });
}

const Scheme *SchemeLibrary::findByFcitxProfile(const QString &profile) const
{
    if (profile.isEmpty())
        return nullptr;
    return findIf(m_schemes, [&](const Scheme &s) { return s.fcitxProfile() == profile; });
}

const Scheme *SchemeLibrary::fallback() const
{
    if (const Scheme *scheme = find(defaultId()))
        return scheme;
    return m_schemes.isEmpty() ? nullptr : &m_schemes.constFirst();
}

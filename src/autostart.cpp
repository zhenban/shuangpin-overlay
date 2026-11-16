// SPDX-License-Identifier: GPL-3.0-or-later
#include "autostart.h"

#include "config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace {

QString entryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/autostart/" APP_ID ".desktop");
}

} // namespace

bool Autostart::isEnabled()
{
    return QFile::exists(entryPath());
}

void Autostart::setEnabled(bool enabled)
{
    const QString path = entryPath();
    if (!enabled) {
        QFile::remove(path);
        return;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    // Desktop entry strings are quoted so a path with spaces still works.
    QString exec = QCoreApplication::applicationFilePath();
    exec.replace(u'\\', QLatin1String("\\\\")).replace(u'"', QLatin1String("\\\""));
    const QString entry = QStringLiteral("[Desktop Entry]\n"
                                         "Type=Application\n"
                                         "Name=%1\n"
                                         "Icon=" APP_ID "\n"
                                         "Exec=\"%2\"\n")
                              .arg(QCoreApplication::applicationName(), exec);
    file.write(entry.toUtf8());
}

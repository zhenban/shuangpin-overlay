// SPDX-License-Identifier: GPL-3.0-or-later
#include "inputmethodwatcher.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QStandardPaths>

namespace {

const QString kService = QStringLiteral("org.fcitx.Fcitx5");
const QString kPath = QStringLiteral("/controller");
const QString kInterface = QStringLiteral("org.fcitx.Fcitx.Controller1");
const QString kMethod = QStringLiteral("CurrentInputMethod");

constexpr int kPollMs = 500;
constexpr int kBlockingTimeoutMs = 200;

// What Fcitx5 uses when pinyin.conf does not say.
const QString kDefaultProfile = QStringLiteral("Ziranma");

QString fcitxConfigDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/fcitx5");
}

QDBusMessage call()
{
    return QDBusMessage::createMethodCall(kService, kPath, kInterface, kMethod);
}

} // namespace

InputMethodWatcher::InputMethodWatcher(const QStringList &names, QObject *parent)
    : QObject(parent)
    , m_names(names)
{
    readProfile();
    // Fcitx replaces the file when it saves, so watch the directories.
    const QString config = fcitxConfigDirectory();
    QStringList watched;
    for (const QString &directory : {config, config + QStringLiteral("/conf")}) {
        if (QDir(directory).exists())
            watched << directory;
    }
    if (!watched.isEmpty()) {
        auto *watcher = new QFileSystemWatcher(watched, this);
        connect(watcher, &QFileSystemWatcher::directoryChanged, this, &InputMethodWatcher::readProfile);
    }

    connect(&m_timer, &QTimer::timeout, this, &InputMethodWatcher::poll);
    m_timer.start(kPollMs);
    refresh();
}

bool InputMethodWatcher::refresh()
{
    const QDBusMessage reply = QDBusConnection::sessionBus().call(call(), QDBus::Block, kBlockingTimeoutMs);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty())
        setActive(matches(reply.arguments().constFirst().toString()));
    else
        setActive(true);
    return m_active;
}

void InputMethodWatcher::poll()
{
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call(), kPollMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *finished) {
        const QDBusPendingReply<QString> reply = *finished;
        setActive(reply.isError() ? true : matches(reply.value()));
        finished->deleteLater();
    });
}

void InputMethodWatcher::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    Q_EMIT activeChanged(active);
}

bool InputMethodWatcher::matches(const QString &inputMethod) const
{
    for (const QString &name : m_names) {
        if (inputMethod.contains(name, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

void InputMethodWatcher::readProfile()
{
    QString profile;
    if (QDir(fcitxConfigDirectory()).exists()) {
        profile = kDefaultProfile;
        QFile file(fcitxConfigDirectory() + QStringLiteral("/conf/pinyin.conf"));
        if (file.open(QIODevice::ReadOnly)) {
            while (!file.atEnd()) {
                const QString line = QString::fromUtf8(file.readLine()).trimmed();
                if (line.startsWith(u'['))
                    break; // the option lives before the first section
                if (line.startsWith(QLatin1String("ShuangpinProfile="))) {
                    profile = line.section(u'=', 1).trimmed();
                    break;
                }
            }
        }
    }
    if (profile != m_profile) {
        m_profile = profile;
        Q_EMIT fcitxProfileChanged();
    }
}

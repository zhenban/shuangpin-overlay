// SPDX-License-Identifier: GPL-3.0-or-later
#include "keyboardmonitor.h"

#include "keyboardlayout.h"

#include <QDir>
#include <QFileSystemWatcher>
#include <QSocketNotifier>

#include <cerrno>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

const QString kInputDirectory = QStringLiteral("/dev/input");

// A new device node appears before udev has set its permissions.
constexpr int kSettleMs = 1000;
// Fallback for devices that were not readable yet at the last scan.
constexpr int kRescanMs = 10000;

bool testBit(const unsigned long *bits, int bit)
{
    constexpr int perWord = sizeof(unsigned long) * 8;
    return bits[bit / perWord] & (1UL << (bit % perWord));
}

/// True for devices with letter keys. Mice and headsets also report a few
/// keys; they are left alone.
bool isKeyboard(int fd)
{
    unsigned long bits[KEY_MAX / (sizeof(unsigned long) * 8) + 1] = {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0)
        return false;
    return testBit(bits, KEY_A) && testBit(bits, KEY_Z);
}

} // namespace

/// Maps a kernel key code (linux/input-event-codes.h) to what it means here.
Key KeyboardMonitor::classify(int code)
{
    static constexpr int rowStart[KeyboardLayout::rowCount] = {KEY_Q, KEY_A, KEY_Z};

    Key key;
    key.code = code;
    switch (code) {
    case KEY_BACKSPACE:
        key.kind = Key::Kind::Backspace;
        return key;
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
    case KEY_LEFTALT:
    case KEY_RIGHTALT:
    case KEY_LEFTMETA:
    case KEY_RIGHTMETA:
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
        key.kind = Key::Kind::Modifier;
        return key;
    default:
        break;
    }
    // The codes of each letter row are consecutive.
    for (int row = 0; row < KeyboardLayout::rowCount; ++row) {
        const int offset = code - rowStart[row];
        if (offset >= 0 && offset < int(qstrlen(KeyboardLayout::rows[row]))) {
            key.kind = Key::Kind::Character;
            key.character = QLatin1Char(KeyboardLayout::rows[row][offset]);
            return key;
        }
    }
    return key;
}

KeyboardMonitor::KeyboardMonitor(QObject *parent)
    : QObject(parent)
{
    auto *settle = new QTimer(this);
    settle->setSingleShot(true);
    settle->setInterval(kSettleMs);
    connect(settle, &QTimer::timeout, this, &KeyboardMonitor::scan);

    auto *watcher = new QFileSystemWatcher({kInputDirectory}, this);
    connect(watcher, &QFileSystemWatcher::directoryChanged, settle, qOverload<>(&QTimer::start));

    m_rescan.setInterval(kRescanMs);
    connect(&m_rescan, &QTimer::timeout, this, &KeyboardMonitor::scan);
    m_rescan.start();

    scan();
}

KeyboardMonitor::~KeyboardMonitor()
{
    const QStringList paths = m_devices.keys();
    for (const QString &path : paths)
        close(path);
}

void KeyboardMonitor::scan()
{
    const bool wasAvailable = isAvailable();
    const QDir directory(kInputDirectory);
    const QStringList names = directory.entryList({QStringLiteral("event*")}, QDir::System);
    for (const QString &name : names) {
        const QString path = directory.filePath(name);
        if (m_devices.contains(path))
            continue;
        const int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
            continue;
        if (!isKeyboard(fd)) {
            ::close(fd);
            continue;
        }
        auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, [this, path] { read(path); });
        m_devices.insert(path, {fd, notifier});
    }
    if (wasAvailable != isAvailable())
        Q_EMIT availabilityChanged();
}

void KeyboardMonitor::read(const QString &path)
{
    const int fd = m_devices.value(path).fd;
    input_event events[32];
    while (true) {
        const ssize_t bytes = ::read(fd, events, sizeof(events));
        if (bytes < 0) {
            if (errno == EINTR)
                continue;
            if (errno != EAGAIN) { // unplugged, or Bluetooth went away
                close(path);
                if (!isAvailable())
                    Q_EMIT availabilityChanged();
            }
            return;
        }
        if (bytes == 0)
            return;
        const int count = int(bytes / sizeof(input_event));
        for (int i = 0; i < count; ++i) {
            const input_event &event = events[i];
            // value 2 is auto-repeat; codes from BTN_MISC up are buttons.
            if (event.type == EV_KEY && event.code < BTN_MISC && event.value != 2)
                Q_EMIT keyEvent(classify(event.code), event.value == 1);
        }
    }
}

void KeyboardMonitor::close(const QString &path)
{
    const Device device = m_devices.take(path);
    if (device.notifier) {
        device.notifier->setEnabled(false);
        device.notifier->deleteLater();
    }
    if (device.fd >= 0)
        ::close(device.fd);
}

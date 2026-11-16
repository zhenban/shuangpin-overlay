// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "key.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

class QSocketNotifier;

/// Reports key presses from every keyboard, whichever window has focus.
///
/// Wayland gives applications no way to see keys meant for other windows,
/// so this reads the kernel's input devices directly. That needs read
/// access to /dev/input/event*, normally granted by membership of the
/// "input" group. Keys are identified by position, as on a US layout, not
/// by the characters of the active layout.
class KeyboardMonitor : public QObject
{
    Q_OBJECT
public:
    explicit KeyboardMonitor(QObject *parent = nullptr);
    ~KeyboardMonitor() override;

    /// False when no keyboard could be opened, usually for lack of
    /// permission.
    bool isAvailable() const { return !m_devices.isEmpty(); }

Q_SIGNALS:
    void keyEvent(const Key &key, bool pressed);
    void availabilityChanged();

private:
    struct Device {
        int fd = -1;
        QSocketNotifier *notifier = nullptr;
    };

    static Key classify(int code);

    void scan();
    void read(const QString &path);
    void close(const QString &path);

    QHash<QString, Device> m_devices;
    QTimer m_rescan;
};

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

/// Tells whether a Shuangpin input method is the active one, and which
/// scheme it is set to, by asking Fcitx5.
///
/// Without Fcitx5 on the session bus neither can be known; the watcher then
/// reports Shuangpin as always active and no scheme.
class InputMethodWatcher : public QObject
{
    Q_OBJECT
public:
    /// @p names: the input method counts as Shuangpin when its name
    /// contains any of these.
    explicit InputMethodWatcher(const QStringList &names, QObject *parent = nullptr);

    /// The last known answer; refreshed twice a second.
    bool isShuangpinActive() const { return m_active; }
    /// Asks now, waiting briefly for the answer.
    bool refresh();

    /// ShuangpinProfile from Fcitx5's pinyin.conf, e.g. "Xiaohe". Empty if
    /// Fcitx5 is not configured on this machine.
    QString fcitxProfile() const { return m_profile; }

Q_SIGNALS:
    void activeChanged(bool active);
    void fcitxProfileChanged();

private:
    void poll();
    void setActive(bool active);
    bool matches(const QString &inputMethod) const;
    void readProfile();

    QStringList m_names;
    bool m_active = true;
    QString m_profile;
    QTimer m_timer;
};

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QPoint>
#include <QSettings>
#include <QString>
#include <QStringList>

#include <optional>

/// The user's preferences, stored in ~/.config/shuangpin-overlay.conf (or
/// wherever XDG_CONFIG_HOME points).
class Settings
{
public:
    Settings();

    /// Opacity of the chart when shown. Kept within sensible limits.
    qreal opacity() const;
    /// Makes the chart more (positive) or less opaque by that many steps.
    void stepOpacity(int steps);

    /// Size of the chart relative to its natural size. Kept within limits.
    qreal scale() const;
    void stepScale(int steps);

    /// Seconds without a key press before the chart hides; 0 never hides.
    int idleSeconds() const;
    void setIdleSeconds(int seconds);

    /// Dim the keys that cannot follow the initial just typed.
    bool hintsEnabled() const;
    void setHintsEnabled(bool enabled);

    /// Stay hidden until the typist pauses after an initial.
    bool onlyWhenStuck() const;
    void setOnlyWhenStuck(bool enabled);

    /// Be a layer-shell overlay where the compositor allows it, rather than
    /// an ordinary window. Off by default: an overlay needs no setting up,
    /// but the window manager will not move it, so dragging is our own and
    /// has none of the desktop's smoothness or effects. Read once, when the
    /// window is created.
    bool overlaySurface() const;
    void setOverlaySurface(bool enabled);

    /// Scheme id, or empty to follow the input method's own setting.
    QString scheme() const;
    void setScheme(const QString &id);

    /// Top-left corner on the screen, when the compositor lets us place the
    /// window ourselves.
    std::optional<QPoint> position() const;
    void setPosition(const QPoint &position);

    /// The chart is shown only while the active input method's name contains
    /// one of these. Not offered in the menu; edit the file to change it.
    QStringList inputMethodNames() const;

private:
    QSettings m_settings;
};

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "chartpainter.h"
#include "composition.h"
#include "key.h"
#include "keyhighlights.h"
#include "practicestats.h"
#include "schemelibrary.h"
#include "settings.h"
#include "typingtracker.h"
#include "visibilitypolicy.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include <memory>

class InputMethodWatcher;
class KeyboardMonitor;
class WindowPlacement;

/// The floating chart. It owns the parts and connects them; each part does
/// one job:
///
///  - KeyboardMonitor and InputMethodWatcher report what happens outside
///  - TypingTracker and Composition follow what is being typed
///  - VisibilityPolicy decides when the chart is on screen
///  - WindowPlacement puts it there, ChartPainter draws it
///
/// What is left here is presentation: fading in and out, the mouse, and
/// turning the current state into a picture.
class OverlayWindow : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWindow(const SchemeLibrary &schemes, QWidget *parent = nullptr);
    ~OverlayWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void changeEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

private:
    void onKey(const Key &key, bool pressed);
    void chooseScheme();
    void applySize();
    void activity();

    bool hasFocus() const;
    VisibilityPolicy::Facts facts() const;
    void checkVisibility();
    void setShown(bool shown);
    void setClickThrough(bool on);

    ChartState chartState() const;

    Settings m_settings;
    SchemeLibrary m_schemes;
    Composition m_composition;
    TypingTracker m_tracker;
    PracticeStats m_stats;
    KeyHighlights m_highlights;
    KeyboardMonitor *m_keyboard = nullptr;
    InputMethodWatcher *m_inputMethod = nullptr;
    std::unique_ptr<WindowPlacement> m_placement;
    ChartTheme m_theme;

    QElapsedTimer m_clock;
    qint64 m_lastActivityMs = 0;
    bool m_menuOpen = false;
    bool m_activeElsewhere = false; // Shuangpin was on where the typing goes

    bool m_shown = true;
    qreal m_visible = 1.0; // 0 hidden .. 1 shown, animated
    QVariantAnimation m_fade;
    QTimer m_visibilityTimer;
    QTimer m_stuckTimer;
};

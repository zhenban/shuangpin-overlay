// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

class QMouseEvent;
class QWidget;
class Settings;

/// How the overlay is put on the screen and moved around. Which
/// implementation is used depends on what the display server offers.
class WindowPlacement
{
public:
    virtual ~WindowPlacement() = default;

    /// Picks the best placement for the widget. Call before it is shown.
    static std::unique_ptr<WindowPlacement> create(QWidget *widget, Settings *settings);

    /// The left button went down on the widget.
    virtual void beginDrag(QMouseEvent *event) = 0;
    /// The pointer moved with the button down.
    virtual void drag(QMouseEvent *event) = 0;
    virtual void endDrag() = 0;
    virtual bool isDragging() const = 0;
    /// The pointer came back onto the widget. A move done by the window
    /// manager ends without a button release; this is the first sign of it.
    virtual void pointerEntered() {}
    /// True while something else shows the chart, and the widget should
    /// draw nothing.
    virtual bool drawsElsewhere() const { return false; }

    /// The widget changed size; keep it where it belongs.
    virtual void sizeChanged() = 0;
};

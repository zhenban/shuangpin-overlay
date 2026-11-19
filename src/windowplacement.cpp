// SPDX-License-Identifier: GPL-3.0-or-later
#include "windowplacement.h"

#include "config.h"
#include "settings.h"

#include <QPixmap>
#include <QStyleHints>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPainter>
#include <QRasterWindow>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#include <functional>
#include <utility>

#ifdef HAVE_LAYER_SHELL
#include <LayerShellQt/Window>
#endif

namespace {

#ifdef HAVE_LAYER_SHELL
Q_LOGGING_CATEGORY(lcPlacement, "shuangpin.placement", QtInfoMsg)
#endif

/// An ordinary window, the default. The window manager moves it, with
/// whatever effects the desktop has, and decides whether it stays above
/// others and where it appears next time. On Wayland an application cannot
/// ask for any of that itself, so the user sets it up once in the window
/// manager (see the README).
class ManagedPlacement : public WindowPlacement
{
public:
    explicit ManagedPlacement(QWidget *widget)
        : m_widget(widget)
    {
    }

    void beginDrag(QMouseEvent *) override
    {
        if (QWindow *window = m_widget->windowHandle())
            m_moving = window->startSystemMove();
    }
    void drag(QMouseEvent *) override {}
    void endDrag() override { m_moving = false; }
    bool isDragging() const override { return m_moving; }
    void pointerEntered() override { m_moving = false; }
    void sizeChanged() override {}

private:
    QWidget *m_widget;
    bool m_moving = false; // handed to the window manager, not heard back yet
};

#ifdef HAVE_LAYER_SHELL

constexpr int kInitialBottomMargin = 80;

/// Finds out how much of the screen panels leave free. Qt cannot say on
/// Wayland, but a surface anchored to all four edges is given exactly that
/// size. The probe is invisible, takes no input, and is gone once it knows.
class AreaProbe : public QRasterWindow
{
public:
    explicit AreaProbe(std::function<void(QSize)> report)
        : m_report(std::move(report))
    {
        using LayerShellQt::Window;
        setFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
        QSurfaceFormat surfaceFormat = format();
        surfaceFormat.setAlphaBufferSize(8);
        setFormat(surfaceFormat);
        create();
        Window *layer = Window::get(this);
        layer->setScope(QStringLiteral(APP_NAME "-probe"));
        layer->setLayer(Window::LayerBackground);
        layer->setKeyboardInteractivity(Window::KeyboardInteractivityNone);
        layer->setAnchors(Window::Anchors(Window::AnchorTop | Window::AnchorBottom | Window::AnchorLeft
                                          | Window::AnchorRight));
        layer->setExclusiveZone(0); // stay clear of panels
        show();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(QRect(QPoint(), size()), Qt::transparent);
        // The first picture is drawn at the size the compositor granted.
        if (m_report)
            std::exchange(m_report, nullptr)(size());
    }

private:
    std::function<void(QSize)> m_report;
};

/// Shows the chart while it is dragged. Pointer positions arrive relative
/// to the surface under the pointer, and a layer-shell surface moves only
/// after a round trip to the compositor; dragging the chart itself mixes up
/// old and new positions. So the chart stays put, invisible, keeping the
/// pointer and a fixed frame of reference, while this surface, which takes
/// no input, shows a picture of it wherever it is dragged.
///
/// It is made once and kept, transparent between drags: the compositor
/// animates surfaces that appear and disappear.
class DragStandIn : public QWidget
{
public:
    explicit DragStandIn(QSize size)
    {
        using LayerShellQt::Window;
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFixedSize(size);
        winId(); // create the QWindow
        m_layer = Window::get(windowHandle());
        m_layer->setScope(QStringLiteral(APP_NAME "-drag"));
        m_layer->setLayer(Window::LayerTop);
        m_layer->setKeyboardInteractivity(Window::KeyboardInteractivityNone);
        m_layer->setAnchors(Window::Anchors(Window::AnchorTop | Window::AnchorLeft));
        m_layer->setExclusiveZone(0); // the same area the chart's margins count in
        show();
    }

    /// Shows @p picture at @p position at once.
    void showPicture(const QPixmap &picture, QPoint position)
    {
        m_picture = picture;
        m_layer->setMargins(QMargins(position.x(), position.y(), 0, 0));
        repaint();
    }

    /// Moves the picture; the new margins go with the next frame.
    void moveTo(QPoint position)
    {
        m_layer->setMargins(QMargins(position.x(), position.y(), 0, 0));
        update();
    }

    /// Becomes transparent at once.
    void clear()
    {
        m_picture = QPixmap();
        repaint();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(QRect(QPoint(), size()), Qt::transparent);
        if (!m_picture.isNull())
            painter.drawPixmap(0, 0, m_picture);
    }

private:
    LayerShellQt::Window *m_layer = nullptr;
    QPixmap m_picture;
};

/// A layer-shell surface: above ordinary windows, clear of panels, never
/// focused, and placed by us, so its position can be remembered.
class LayerShellPlacement : public WindowPlacement
{
public:
    LayerShellPlacement(QWidget *widget, Settings *settings, LayerShellQt::Window *layer)
        : m_widget(widget)
        , m_settings(settings)
        , m_layer(layer)
        , m_area(widget->screen()->geometry().size())
    {
        using LayerShellQt::Window;
        m_layer->setScope(QStringLiteral(APP_NAME));
        m_layer->setLayer(Window::LayerTop);
        m_layer->setKeyboardInteractivity(Window::KeyboardInteractivityNone);
        m_layer->setAnchors(Window::Anchors(Window::AnchorTop | Window::AnchorLeft));
        m_layer->setExclusiveZone(0); // margins count from the area panels leave free
        place();

        // Made now, so that the compositor's animation for a new surface
        // plays on a transparent pixel and not on the first drag.
        m_standIn = std::make_unique<DragStandIn>(widget->size());

        measureArea();
        QObject::connect(widget->screen(), &QScreen::geometryChanged, widget, [this] { measureArea(); });
    }

    void beginDrag(QMouseEvent *event) override
    {
        m_dragging = true;
        m_dragOrigin = event->position();
        m_dragTarget = m_position;
    }

    void drag(QMouseEvent *event) override
    {
        if (!m_dragging)
            return;
        // The chart does not move during the drag, so every position is
        // relative to where it began.
        const QPoint delta = (event->position() - m_dragOrigin).toPoint();
        if (!m_moved && delta.manhattanLength() < QGuiApplication::styleHints()->startDragDistance())
            return; // a click, not a drag
        m_dragTarget = bounded(m_position + delta);
        if (!m_moved) {
            // Swap the chart for its picture: both at once, so that neither
            // a gap nor both of them show.
            m_moved = true;
            m_standIn->showPicture(m_widget->grab(), m_dragTarget);
            m_standingIn = true;
            m_widget->repaint();
        } else {
            m_standIn->moveTo(m_dragTarget);
        }
    }

    void endDrag() override
    {
        if (!m_dragging)
            return;
        m_dragging = false;
        if (m_moved) {
            // And back, the chart now in the new place.
            m_moved = false;
            m_standingIn = false;
            moveTo(m_dragTarget);
            m_widget->repaint();
            m_standIn->clear();
        }
        m_settings->setPosition(m_position);
    }

    bool isDragging() const override { return m_dragging; }
    bool drawsElsewhere() const override { return m_standingIn; }

    void sizeChanged() override
    {
        m_standIn->setFixedSize(m_widget->size());
        moveTo(m_position);
    }

private:
    /// Where the user left it, or bottom centre the first time.
    void place()
    {
        const QPoint initial((m_area.width() - m_widget->width()) / 2,
                             m_area.height() - m_widget->height() - kInitialBottomMargin);
        moveTo(m_settings->position().value_or(initial));
    }

    void measureArea()
    {
        m_probe.reset(new AreaProbe([this](QSize area) {
            qCDebug(lcPlacement) << "area free of panels:" << area;
            m_area = area;
            place();
            m_probe.release()->deleteLater(); // we are inside its paint event
        }));
    }

    /// The nearest position that keeps all of the chart inside the free area.
    QPoint bounded(QPoint position) const
    {
        return QPoint(qBound(0, position.x(), qMax(0, m_area.width() - m_widget->width())),
                      qBound(0, position.y(), qMax(0, m_area.height() - m_widget->height())));
    }

    /// Moves the surface.
    void moveTo(QPoint position)
    {
        position = bounded(position);
        m_position = position;
        m_layer->setMargins(QMargins(position.x(), position.y(), 0, 0));
        m_widget->update(); // the new margins travel with the next frame
    }

    QWidget *m_widget;
    Settings *m_settings;
    LayerShellQt::Window *m_layer;
    QSize m_area; // what panels leave free; margins are measured inside it
    std::unique_ptr<AreaProbe> m_probe;
    QPoint m_position;
    bool m_dragging = false;
    QPointF m_dragOrigin;
    QPoint m_dragTarget;
    bool m_moved = false; // this press has become a drag
    std::unique_ptr<DragStandIn> m_standIn;
    bool m_standingIn = false; // the stand-in is on screen; the chart draws nothing
};

/// The layer-shell handle for the widget, or null where the protocol is not
/// available or not wanted.
LayerShellQt::Window *layerShellWindow(QWidget *widget)
{
    if (QGuiApplication::platformName() != QLatin1String("wayland"))
        return nullptr;
    if (qEnvironmentVariableIsSet("SHUANGPIN_OVERLAY_NO_LAYER_SHELL"))
        return nullptr;
    // GNOME's compositor does not implement the protocol.
    if (qgetenv("XDG_CURRENT_DESKTOP").toUpper().contains("GNOME"))
        return nullptr;
    widget->winId(); // create the QWindow
    return LayerShellQt::Window::get(widget->windowHandle());
}

#endif // HAVE_LAYER_SHELL

} // namespace

std::unique_ptr<WindowPlacement> WindowPlacement::create(QWidget *widget, Settings *settings)
{
#ifdef HAVE_LAYER_SHELL
    LayerShellQt::Window *layer = settings->overlaySurface() ? layerShellWindow(widget) : nullptr;
    if (layer)
        return std::make_unique<LayerShellPlacement>(widget, settings, layer);
#else
    Q_UNUSED(settings)
#endif
    return std::make_unique<ManagedPlacement>(widget);
}

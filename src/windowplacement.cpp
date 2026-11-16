// SPDX-License-Identifier: GPL-3.0-or-later
#include "windowplacement.h"

#include "config.h"
#include "settings.h"

#include <QElapsedTimer>
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
// After the surface is told to move, pointer positions keep arriving
// relative to where it was until the compositor has acted on it. Acting on
// those would add the same distance again and again.
constexpr int kMoveSettleMs = 32;

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

        measureArea();
        QObject::connect(widget->screen(), &QScreen::geometryChanged, widget, [this] { measureArea(); });
    }

    void beginDrag(QMouseEvent *event) override
    {
        m_dragging = true;
        m_dragOrigin = event->position();
        m_sinceMove.invalidate();
    }

    void drag(QMouseEvent *event) override
    {
        if (!m_dragging)
            return;
        if (m_sinceMove.isValid() && m_sinceMove.elapsed() < kMoveSettleMs)
            return; // this position may still be relative to the old place
        // The surface follows the pointer, so the pointer's position inside
        // it differs from where the drag began by the distance still to go.
        const QPoint delta = (event->position() - m_dragOrigin).toPoint();
        if (delta.isNull())
            return;
        moveTo(m_position + delta);
        m_sinceMove.start();
    }

    void endDrag() override
    {
        if (!m_dragging)
            return;
        m_dragging = false;
        m_settings->setPosition(m_position);
    }

    bool isDragging() const override { return m_dragging; }

    void sizeChanged() override { moveTo(m_position); }

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

    /// Moves the surface, keeping all of it inside the free area.
    void moveTo(QPoint position)
    {
        position.setX(qBound(0, position.x(), qMax(0, m_area.width() - m_widget->width())));
        position.setY(qBound(0, position.y(), qMax(0, m_area.height() - m_widget->height())));
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
    QElapsedTimer m_sinceMove;
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

// SPDX-License-Identifier: GPL-3.0-or-later
#include "overlaywindow.h"

#include "inputmethodwatcher.h"
#include "keyboardmonitor.h"
#include "overlaymenu.h"
#include "windowplacement.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <QWindow>

namespace {

constexpr int kFadeInMs = 120;
constexpr int kFadeOutMs = 300;
constexpr int kVisibilityCheckMs = 500;

} // namespace

OverlayWindow::OverlayWindow(const SchemeLibrary &schemes, QWidget *parent)
    : QWidget(parent)
    , m_schemes(schemes)
    , m_tracker(&m_composition)
    , m_stats(PracticeStats::defaultFilePath())
{
    setWindowTitle(QGuiApplication::applicationDisplayName());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_theme = ChartTheme::fromPalette(palette());
    m_clock.start();

    m_inputMethod = new InputMethodWatcher(m_settings.inputMethodNames(), this);
    connect(m_inputMethod, &InputMethodWatcher::activeChanged, this, &OverlayWindow::checkVisibility);
    connect(m_inputMethod, &InputMethodWatcher::fcitxProfileChanged, this, &OverlayWindow::chooseScheme);

    m_keyboard = new KeyboardMonitor(this);
    connect(m_keyboard, &KeyboardMonitor::keyEvent, this, &OverlayWindow::onKey);
    connect(m_keyboard, &KeyboardMonitor::availabilityChanged, this, &OverlayWindow::checkVisibility);

    connect(&m_composition, &Composition::changed, this, qOverload<>(&QWidget::update));
    connect(&m_composition, &Composition::finalTimed, &m_stats, &PracticeStats::addFinalTime);
    connect(&m_composition, &Composition::slipFound, &m_stats, &PracticeStats::addSlip);
    connect(&m_highlights, &KeyHighlights::changed, this, qOverload<>(&QWidget::update));

    m_fade.setEasingCurve(QEasingCurve::InOutCubic);
    connect(&m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_visible = value.toReal();
        update();
    });
    connect(&m_fade, &QVariantAnimation::finished, this, [this] {
        if (!m_shown) {
            repaint(); // wipe the picture before clicks start passing through
            setClickThrough(true);
        }
    });

    m_stuckTimer.setSingleShot(true);
    m_stuckTimer.setInterval(VisibilityPolicy::stuckDelayMs);
    connect(&m_stuckTimer, &QTimer::timeout, this, [this] {
        if (m_settings.onlyWhenStuck() && m_composition.isPending())
            setShown(true);
    });

    connect(&m_visibilityTimer, &QTimer::timeout, this, &OverlayWindow::checkVisibility);
    m_visibilityTimer.start(kVisibilityCheckMs);

    chooseScheme();
    m_placement = WindowPlacement::create(this, &m_settings);
}

OverlayWindow::~OverlayWindow() = default;

// --- scheme -----------------------------------------------------------------

void OverlayWindow::chooseScheme()
{
    const QString wanted = m_settings.scheme();
    const Scheme *scheme = wanted.isEmpty() ? m_schemes.findByFcitxProfile(m_inputMethod->fcitxProfile())
                                            : m_schemes.find(wanted);
    if (!scheme)
        scheme = m_schemes.fallback();
    if (!scheme || scheme->id() == m_composition.scheme().id())
        return;

    m_composition.setScheme(*scheme);
    m_stats.setScheme(scheme->id());
    applySize();
    update();
}

void OverlayWindow::applySize()
{
    setFixedSize(ChartPainter::size(m_composition.scheme(), m_settings.scale()));
    if (m_placement)
        m_placement->sizeChanged();
}

// --- keyboard ---------------------------------------------------------------

void OverlayWindow::onKey(const Key &key, bool pressed)
{
    const bool onChart = key.kind == Key::Kind::Character && m_composition.scheme().isKey(key.character);
    if (!pressed) {
        m_tracker.keyReleased(key);
        if (onChart)
            m_highlights.release(key.character);
        return;
    }

    if (key.kind != Key::Kind::Modifier)
        activity();
    bool active = m_inputMethod->isShuangpinActive();
    if (!active && onChart)
        active = m_inputMethod->refresh(); // it may have been switched on a moment ago

    const bool typed = m_tracker.keyPressed(key, m_clock.elapsed(), active);
    if (onChart)
        m_highlights.press(key.character);

    if (m_settings.onlyWhenStuck() && m_composition.isPending())
        m_stuckTimer.start();
    else
        m_stuckTimer.stop();
    if (typed && VisibilityPolicy::showOnKey(facts()))
        setShown(true);
}

void OverlayWindow::activity()
{
    m_lastActivityMs = m_clock.elapsed();
}

// --- showing and hiding -----------------------------------------------------

bool OverlayWindow::hasFocus() const
{
    return QGuiApplication::focusWindow() != nullptr;
}

VisibilityPolicy::Facts OverlayWindow::facts() const
{
    VisibilityPolicy::Facts facts;
    facts.shown = m_shown;
    facts.keyboardAvailable = m_keyboard->isAvailable();
    // A click on the chart can move the keyboard focus to it, and then the
    // input method reports on the chart instead of on the window being
    // typed in. What it said before the click still stands.
    facts.shuangpinActive = hasFocus() ? m_activeElsewhere : m_inputMethod->isShuangpinActive();
    facts.pending = m_composition.isPending();
    facts.held = m_menuOpen || underMouse() || (m_placement && m_placement->isDragging());
    facts.onlyWhenStuck = m_settings.onlyWhenStuck();
    facts.idleMs = m_clock.elapsed() - m_lastActivityMs;
    facts.idleLimitMs = m_settings.idleSeconds() * 1000;
    return facts;
}

void OverlayWindow::checkVisibility()
{
    if (!hasFocus())
        m_activeElsewhere = m_inputMethod->isShuangpinActive();
    if (!m_inputMethod->isShuangpinActive())
        m_tracker.inputMethodLeft();
    setShown(VisibilityPolicy::shouldBeShown(facts()));
}

void OverlayWindow::setShown(bool shown)
{
    if (m_shown == shown)
        return;
    m_shown = shown;
    if (shown)
        setClickThrough(false);
    const qreal from = m_visible;
    const qreal target = shown ? 1.0 : 0.0;
    m_fade.stop();
    {
        // A finished animation still sits at the end of its last run, and
        // re-evaluates there whenever a key value is replaced. Keep that
        // from reaching m_visible, or the fade would start at its end.
        const QSignalBlocker blocker(&m_fade);
        m_fade.setStartValue(from);
        m_fade.setEndValue(target);
        m_fade.setDuration(qMax(1, int((shown ? kFadeInMs : kFadeOutMs) * qAbs(target - from))));
    }
    m_fade.start();
}

void OverlayWindow::setClickThrough(bool on)
{
    // The mask goes on the QWindow: a widget mask would also clip painting
    // and leave the old picture behind. With a one-pixel input region,
    // clicks reach whatever is underneath.
    if (QWindow *window = windowHandle())
        window->setMask(on ? QRegion(0, 0, 1, 1) : QRegion());
}

// --- painting ---------------------------------------------------------------

ChartState OverlayWindow::chartState() const
{
    ChartState state = ChartState::of(m_composition, m_settings.hintsEnabled());
    state.lit = m_highlights.strengths();
    return state;
}

void OverlayWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (m_visible <= 0.0 || (m_placement && m_placement->drawsElsewhere())) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), Qt::transparent);
        return;
    }
    // Draw opaque first, then blend the whole chart at once, so overlapping
    // shapes do not show through each other.
    const qreal ratio = devicePixelRatioF();
    QPixmap chart(size() * ratio);
    chart.setDevicePixelRatio(ratio);
    chart.fill(Qt::transparent);
    {
        QPainter chartPainter(&chart);
        ChartPainter::paint(chartPainter, m_composition.scheme(), chartState(), m_theme, m_settings.scale());
    }
    painter.setOpacity(m_settings.opacity() * m_visible);
    painter.drawPixmap(0, 0, chart);
}

void OverlayWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        m_theme = ChartTheme::fromPalette(palette());
        update();
    }
    QWidget::changeEvent(event);
}

// --- mouse ------------------------------------------------------------------

// Handling the chart counts as using it: the idle time starts again, so it
// does not vanish in the hand or the moment it is let go.

void OverlayWindow::mousePressEvent(QMouseEvent *event)
{
    activity();
    // Caught while fading out: it takes clicks until it is gone, so it can
    // be picked up then, and should come back.
    setShown(true);
    if (event->button() == Qt::LeftButton)
        m_placement->beginDrag(event);
}

void OverlayWindow::mouseMoveEvent(QMouseEvent *event)
{
    activity();
    m_placement->drag(event);
}

void OverlayWindow::enterEvent(QEnterEvent *event)
{
    activity(); // also the end of a move done by the window manager
    m_placement->pointerEntered();
    QWidget::enterEvent(event);
}

void OverlayWindow::leaveEvent(QEvent *event)
{
    activity(); // also the start of a move done by the window manager
    QWidget::leaveEvent(event);
}

void OverlayWindow::mouseReleaseEvent(QMouseEvent *event)
{
    activity();
    if (event->button() == Qt::LeftButton)
        m_placement->endDrag();
}

void OverlayWindow::wheelEvent(QWheelEvent *event)
{
    activity();
    const int steps = event->angleDelta().y() > 0 ? 1 : -1;
    if (event->modifiers() & Qt::ControlModifier) {
        m_settings.stepScale(steps);
        applySize();
    } else {
        m_settings.stepOpacity(steps);
    }
    update();
}

void OverlayWindow::contextMenuEvent(QContextMenuEvent *event)
{
    OverlayMenu::Context context;
    context.settings = &m_settings;
    context.stats = &m_stats;
    context.schemes = &m_schemes;
    context.currentScheme = &m_composition.scheme();
    context.keyboardAvailable = m_keyboard->isAvailable();
    context.canFollowFcitx = !m_inputMethod->fcitxProfile().isEmpty();

    OverlayMenu menu(context, this);
    connect(&menu, &OverlayMenu::schemeChoiceChanged, this, &OverlayWindow::chooseScheme);
    connect(&menu, &OverlayMenu::settingsChanged, this, [this] {
        activity(); // a changed timeout should count from now
        update();
    });

    m_menuOpen = true;
    menu.exec(event->globalPos());
    m_menuOpen = false;
    activity();
}

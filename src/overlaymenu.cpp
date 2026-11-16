// SPDX-License-Identifier: GPL-3.0-or-later
#include "overlaymenu.h"

#include "autostart.h"
#include "practicestats.h"
#include "schemelibrary.h"
#include "settings.h"

#include <QCoreApplication>

OverlayMenu::OverlayMenu(const Context &context, QWidget *parent)
    : QMenu(parent)
    , m_context(context)
{
    addNote(this, tr("Scroll: transparency · Ctrl+scroll: size"));
    if (!m_context.keyboardAvailable)
        addNote(this, tr("Cannot read the keyboard: add yourself to the “input” group"));
    addSeparator();
    addSchemes();
    addBehaviour();
    addSeparator();
    addStatistics();
    addSeparator();
    addToggle(this, tr("Start at login"), Autostart::isEnabled(), [](bool on) { Autostart::setEnabled(on); });
    addAction(tr("Quit"), qApp, &QCoreApplication::quit);
}

void OverlayMenu::addNote(QMenu *menu, const QString &text)
{
    menu->addAction(text)->setEnabled(false);
}

template<typename Handler>
QAction *OverlayMenu::addToggle(QMenu *menu, const QString &text, bool checked, Handler handler)
{
    QAction *action = menu->addAction(text);
    action->setCheckable(true);
    action->setChecked(checked);
    connect(action, &QAction::toggled, this, handler);
    return action;
}

void OverlayMenu::addSchemes()
{
    QMenu *menu = addMenu(tr("Scheme"));
    const QString chosen = m_context.settings->scheme();
    auto choose = [this](const QString &id) {
        return [this, id] {
            m_context.settings->setScheme(id);
            Q_EMIT schemeChoiceChanged();
        };
    };

    // An empty id means "whatever Fcitx5 is set to".
    addToggle(menu, tr("Follow Fcitx5"), chosen.isEmpty(), choose(QString()))->setEnabled(m_context.canFollowFcitx);
    menu->addSeparator();
    for (const Scheme &scheme : m_context.schemes->all())
        addToggle(menu, scheme.displayName(), chosen == scheme.id(), choose(scheme.id()));
}

void OverlayMenu::addBehaviour()
{
    Settings *settings = m_context.settings;
    addToggle(this, tr("Hint the finals that can follow"), settings->hintsEnabled(), [this](bool on) {
        m_context.settings->setHintsEnabled(on);
        Q_EMIT settingsChanged();
    });
    addToggle(this, tr("Only appear when I get stuck"), settings->onlyWhenStuck(), [this](bool on) {
        m_context.settings->setOnlyWhenStuck(on);
        Q_EMIT settingsChanged();
    });

    addToggle(this, tr("Float as an overlay (applies after restart)"), settings->overlaySurface(), [this](bool on) {
        m_context.settings->setOverlaySurface(on);
        Q_EMIT settingsChanged();
    });

    QMenu *idle = addMenu(tr("Hide when the keyboard is idle"));
    const QList<std::pair<int, QString>> choices = {
        {5, tr("After 5 seconds")}, {10, tr("After 10 seconds")}, {30, tr("After 30 seconds")}, {0, tr("Never")}};
    for (const auto &[seconds, label] : choices) {
        addToggle(idle, label, settings->idleSeconds() == seconds, [this, seconds] {
            m_context.settings->setIdleSeconds(seconds);
            Q_EMIT settingsChanged();
        });
    }
}

void OverlayMenu::addStatistics()
{
    const Scheme &scheme = *m_context.currentScheme;
    PracticeStats *stats = m_context.stats;

    QMenu *slow = addMenu(tr("My slowest finals"));
    const QList<PracticeStats::SlowFinal> slowest = stats->slowest();
    for (const PracticeStats::SlowFinal &entry : slowest) {
        addNote(slow, tr("%1 (%2 key): %3 s")
                          .arg(entry.final, QString(scheme.keyForFinal(entry.final).toUpper()))
                          .arg(entry.averageMs / 1000.0, 0, 'f', 1));
    }
    if (slowest.isEmpty())
        addNote(slow, tr("Not enough typing yet"));

    QMenu *slips = addMenu(tr("Keys I mix up"));
    const QList<PracticeStats::FrequentSlip> frequent = stats->frequentSlips();
    for (const PracticeStats::FrequentSlip &entry : frequent) {
        const Slip &slip = entry.slip;
        QString hit(slip.hitKey.toUpper());
        if (!slip.hit.isEmpty())
            hit += QStringLiteral(" (%1)").arg(slip.hit);
        const QString text = slip.initial ? tr("Initial %1 (%2 key), hit %3 instead: %4 times")
                                          : tr("Final %1 (%2 key), hit %3 instead: %4 times");
        addNote(slips, text.arg(slip.meant, QString(slip.meantKey.toUpper()), hit).arg(entry.count));
    }
    if (frequent.isEmpty())
        addNote(slips, tr("No repeated slips yet"));

    addAction(tr("Clear practice statistics"), stats, &PracticeStats::clear);
}

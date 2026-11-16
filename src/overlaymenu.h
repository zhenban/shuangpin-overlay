// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMenu>

class PracticeStats;
class Scheme;
class SchemeLibrary;
class Settings;

/// The overlay's context menu. It writes the user's choices to Settings
/// itself and then says what kind of thing changed.
class OverlayMenu : public QMenu
{
    Q_OBJECT
public:
    /// What the menu shows and edits. All of it must outlive the menu.
    struct Context {
        Settings *settings = nullptr;
        PracticeStats *stats = nullptr;
        const SchemeLibrary *schemes = nullptr;
        const Scheme *currentScheme = nullptr;
        bool keyboardAvailable = true;
        bool canFollowFcitx = false;
    };

    explicit OverlayMenu(const Context &context, QWidget *parent = nullptr);

Q_SIGNALS:
    /// A different scheme, or following Fcitx5, was chosen.
    void schemeChoiceChanged();
    /// Some other setting was changed.
    void settingsChanged();

private:
    void addNote(QMenu *menu, const QString &text);
    template<typename Handler>
    QAction *addToggle(QMenu *menu, const QString &text, bool checked, Handler handler);

    void addSchemes();
    void addBehaviour();
    void addStatistics();

    Context m_context;
};

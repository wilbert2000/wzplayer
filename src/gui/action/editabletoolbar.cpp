/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gui/action/editabletoolbar.h"

#include "gui/action/actionlist.h"
#include "gui/action/actionseditor.h"
#include "gui/action/toolbareditor.h"
#include "gui/action/menu/menu.h"

#include "gui/mainwindow.h"
#include "gui/desktop.h"
#include "settings/preferences.h"

#include <QMenu>
#include <QResizeEvent>
#include <QTimer>

namespace Gui {
namespace Action {


TEditableToolbar::TEditableToolbar(TMainWindow* mainwindow) :
    QToolBar(mainwindow),
    debug(logger()),
    main_window(mainwindow) {

    // Reload toolbars when entering and exiting fullscreen
    connect(main_window, &TMainWindow::didEnterFullscreenSignal,
            this, &TEditableToolbar::reload);
    connect(main_window, &TMainWindow::didExitFullscreenSignal,
            this, &TEditableToolbar::reload);
}

TEditableToolbar::~TEditableToolbar() {
}

void TEditableToolbar::addMenu(QAction* action) {

    // Create button with menu
    QToolButton* button = new QToolButton();
    button->setObjectName(action->objectName() + "_toolbutton");
    QMenu* menu = action->menu();
    button->setMenu(menu);

    // Set popupmode and default action
    if (action->objectName() == "stay_on_top_menu") {
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setDefaultAction(menu->defaultAction());
    } else if (action->objectName() == "forward_menu"
        || action->objectName() == "rewind_menu") {
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setDefaultAction(menu->defaultAction());
        // Set triggered action as default action
        connect(menu, &QMenu::triggered,
                button, &QToolButton::setDefaultAction);
        // Show menu when action disabled
        connect(action, &QAction::triggered,
                button, &QToolButton::showMenu,
                Qt::QueuedConnection);
    } else {
        // Default, use instant popup
        button->setPopupMode(QToolButton::InstantPopup);
        button->setDefaultAction(action);
    }

    addWidget(button);
}

void TEditableToolbar::setActionsFromStringList(const QStringList& acts,
                                                const TActionList& allActions) {

    clear();
    // Copy actions
    actions = acts;

    int i = 0;
    while (i < actions.count()) {
        QString action_name;
        bool ns, fs;
        TToolbarEditor::stringToAction(actions.at(i), action_name, ns, fs);
        if (action_name.isEmpty()) {
            WZERROR(QString("Failed to parse action '%1' for toolbar '%2'")
                   .arg(actions.at(i)).arg(windowTitle()));
            actions.removeAt(i);
        } else {
            if (Settings::pref->fullscreen ? fs : ns) {
                if (action_name == "separator") {
                    addSeparator();
                } else {
                    QAction* action = TToolbarEditor::findAction(action_name,
                                                                 allActions);
                    if (action) {
                        if (action_name.endsWith("_menu")) {
                            addMenu(action);
                        } else {
                            addAction(action);
                        }
                    } else {
                        WZERROR("action '" + action_name + " not found");
                        actions.removeAt(i);
                        i--;
                    }
                }
            } // if (visible)

            i++;
        }
    } // while

} // TEditableToolbar::setActionsFromStringList()

QStringList TEditableToolbar::actionsToStringList() const {
    return actions;
}

void TEditableToolbar::edit() {
    WZDEBUG(objectName());

    // Create toolbar editor dialog
    QList<QAction*> allActions = main_window->getNamedActions();
    TToolbarEditor editor(main_window, windowTitle());
    editor.setAllActions(allActions);
    editor.setActiveActions(actions);
    editor.setDefaultActions(defaultActions);
    editor.setIconSize(iconSize().width());

    // Execute
    if (editor.exec() == QDialog::Accepted) {
        // Get action names and update active actions
        QStringList new_actions = editor.saveActions();
        // Load new actions into this toolbar
        setActionsFromStringList(new_actions, allActions);
        // Update icon size
        setIconSize(QSize(editor.iconSize(), editor.iconSize()));
        // Save modified icon texts to pref
        TActionsEditor::saveSettings(Settings::pref, allActions);
        Settings::pref->sync();
    }
}

void TEditableToolbar::reload() {

    setActionsFromStringList(actions, main_window->getNamedActions());
}

} // namespace Action
} // namespace Gui

#include "moc_editabletoolbar.cpp"

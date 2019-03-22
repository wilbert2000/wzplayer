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

#include "gui/action/action.h"
#include "gui/action/actionseditor.h"
#include "gui/action/toolbareditor.h"
#include "gui/action/menu/menu.h"
#include "gui/action/menu/menuplay.h"

#include "gui/mainwindow.h"
#include "settings/preferences.h"
#include "desktop.h"
#include "images.h"

#include <QMenu>
#include <QResizeEvent>
#include <QTimer>

namespace Gui {
namespace Action {


TEditableToolbar::TEditableToolbar(QWidget* parent,
                                   const QString& name,
                                   const QString& title) :
    QToolBar(parent) {

    setObjectName(name);
    setWindowTitle(title);
    toggleViewAction()->setObjectName("toggle_" + name);
    toggleViewAction()->setIcon(Images::icon(name));

    TAction* editAct = new TAction(parent, "edit_" + name,
                                   tr("Edit %1...").arg(title.toLower()));
    connect(editAct, &TAction::triggered, this, &TEditableToolbar::edit);
    parent->addAction(editAct);

    // Reload the main toolbars when entering and exiting fullscreen
    TMainWindow* mw = qobject_cast<TMainWindow*>(parent);
    if (mw) {
        connect(mw, &TMainWindow::didEnterFullscreenSignal,
                this, &TEditableToolbar::reload);
        connect(mw, &TMainWindow::didExitFullscreenSignal,
                this, &TEditableToolbar::reload);
    }
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
    } else if (action->objectName() == "seek_forward_menu"
        || action->objectName() == "seek_rewind_menu") {
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setDefaultAction(menu->defaultAction());
        connect(static_cast<Menu::TMenuSeek*>(menu),
                &Menu::TMenuSeek::defaultActionChanged,
                button, &QToolButton::setDefaultAction);
    } else {
        // Default, use instant popup
        button->setPopupMode(QToolButton::InstantPopup);
        button->setDefaultAction(action);
    }

    addWidget(button);
}

void TEditableToolbar::setActionsFromStringList(const QStringList& acts) {
    WZTRACEOBJ("");

    clear();
    // Copy actions
    currentActions = acts;

    int i = 0;
    while (i < currentActions.count()) {
        QString actionName;
        bool ns, fs;
        TToolbarEditor::stringToAction(currentActions.at(i), actionName, ns, fs);
        if (actionName.isEmpty()) {
            WZERROROBJ(QString("Failed to parse action '%1' for toolbar '%2'")
                       .arg(currentActions.at(i)).arg(windowTitle()));
            currentActions.removeAt(i);
        } else {
            if (Settings::pref->fullscreen ? fs : ns) {
                if (actionName == "separator") {
                    addSeparator();
                } else {
                    QAction* action = findAction(actionName);
                    if (action) {
                        if (actionName.endsWith("_menu")) {
                            addMenu(action);
                        } else {
                            addAction(action);
                            QWidget* w = widgetForAction(action);
                            w->setObjectName(actionName + "_toolbutton");
                            w->setFocusPolicy(Qt::TabFocus);
                        }
                    } else {
                        WZERROROBJ("Action '" + actionName + " not found");
                        currentActions.removeAt(i);
                        i--;
                    }
                }
            } // if (visible)

            i++;
        }
    } // while

} // TEditableToolbar::setActionsFromStringList()

QStringList TEditableToolbar::actionsToStringList() const {
    return currentActions;
}

void TEditableToolbar::edit() {
    WZTRACEOBJ("");

    // Create toolbar editor dialog
    TToolbarEditor editor(parentWidget(), windowTitle());
    editor.setAllActions(TAction::allActions);
    editor.setActiveActions(currentActions);
    editor.setDefaultActions(defaultActions);
    editor.setIconSize(iconSize().width());

    // Execute
    if (editor.exec() == QDialog::Accepted) {
        // Get action names and update active actions
        QStringList newActions = editor.saveActions();
        // Load new actions into this toolbar
        setActionsFromStringList(newActions);
        // Update icon size
        setIconSize(QSize(editor.iconSize(), editor.iconSize()));
        // Save modified icon texts to pref
        TActionsEditor::saveSettings(Settings::pref);
        // Sync to disk
        Settings::pref->sync();
    }
}

void TEditableToolbar::reload() {
    setActionsFromStringList(currentActions);
}

void TEditableToolbar::saveSettings() {

    using namespace Settings;

    pref->beginGroup("actions");
    pref->setValue(objectName(), actionsToStringList());
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    pref->setValue(objectName(), iconSize());
    pref->endGroup();
}

void TEditableToolbar::loadSettings() {

    using namespace Settings;

    pref->beginGroup("actions");
    setActionsFromStringList(pref->value(objectName(), getDefaultActions())
                             .toStringList());
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    setIconSize(pref->value(objectName(), iconSize()).toSize());
    pref->endGroup();
}

} // namespace Action
} // namespace Gui

#include "moc_editabletoolbar.cpp"

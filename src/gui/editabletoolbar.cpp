/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/editabletoolbar.h"
#include <QDebug>
#include <QMenu>
#include "settings/preferences.h"
#include "gui/toolbareditor.h"
#include "gui/actionseditor.h"
#include "gui/base.h"

namespace Gui {

TEditableToolbar::TEditableToolbar(TBase* mainwindow)
	: QToolBar(mainwindow)
	, main_window(mainwindow) {

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(showPopup(const QPoint&)));

}

TEditableToolbar::~TEditableToolbar() {
}

void TEditableToolbar::setActionsFromStringList(const QStringList& acts, const TActionList& all_actions) {
	qDebug() << "Gui::TEditableToolbar::setActionsFromStringList: loading toolbar" << objectName();

	clear();
	actions = acts;

	QString action_name;
	bool ns, fs;
	int i = 0;
	while (i < actions.count()) {
		TToolbarEditor::stringToAction(actions[i], action_name, ns, fs);
		if (action_name.isEmpty()) {
			qWarning() << "Gui::TEditableToolbar::setActionsFromStringList: malformed action"
					   << actions[i] << "at pos" << i;
			actions.removeAt(i);
		} else {
			bool vis = pref->fullscreen ? fs : ns;
			if (vis) {
				if (action_name == "separator") {
					QAction* action = TToolbarEditor::newSeparator(this);
					addAction(action);
				} else {
					QAction* action = TToolbarEditor::findAction(action_name, all_actions);
					if (action) {
						addAction(action);
						// If the action is a menu change some of its properties
						if (action->objectName().endsWith("_menu")) {
							QToolButton* button = qobject_cast<QToolButton*>(widgetForAction(action));
							if (button) {
								button->setPopupMode(QToolButton::InstantPopup);
							}
						}
					} else {
						qWarning() << "Gui::TEditableToolbar::setActionsFromStringList: action"
								   << action_name << "not found";
						actions.removeAt(i);
						i--;
					}
				}
			} // if (vis)
			i++;
		}
	}
}

void TEditableToolbar::edit() {
	qDebug("Gui::TEditableToolbar::edit");

	TActionList all_actions = main_window->getAllNamedActions();
	TToolbarEditor e(main_window);
	e.setAllActions(all_actions);
	e.setActiveActions(actions);
	e.setDefaultActions(default_actions);
	e.setIconSize(iconSize().width());

	if (e.exec() == QDialog::Accepted) {
		// Get action names and update actions in all_actions
		QStringList new_actions = e.saveActions();
		// Save actions to pref
		TActionsEditor::saveToConfig(main_window, Settings::pref);
		// Load new actions
		setActionsFromStringList(new_actions, all_actions);
		resize(width(), e.iconSize());
		setIconSize(QSize(e.iconSize(), e.iconSize()));
	}
}

void TEditableToolbar::reload() {

	TActionList all_actions = main_window->getAllNamedActions();
	setActionsFromStringList(actions, all_actions);
}

void TEditableToolbar::didEnterFullscreen() {
	reload();
}

void TEditableToolbar::didExitFullscreen() {
	reload();
}

void TEditableToolbar::showPopup(const QPoint& pos) {
	qDebug("Gui::TEditableToolbar::showPopup: x: %d y: %d", pos.x(), pos.y());

	QMenu* popup = main_window->getToolbarMenu();
	if (popup) {
		popup->exec(mapToGlobal(pos));
	}
}

} // namespace Gui
#include "moc_editabletoolbar.cpp"


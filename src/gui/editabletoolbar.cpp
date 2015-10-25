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
#include "settings/preferences.h"
#include "gui/toolbareditor.h"
#include "gui/actionseditor.h"
#include "gui/base.h"

namespace Gui {

TEditableToolbar::TEditableToolbar(TBase* mainwindow)
	: QToolBar(mainwindow)
	, main_window(mainwindow) {
}

TEditableToolbar::~TEditableToolbar() {
}

void TEditableToolbar::setActionsFromStringList(const QStringList& actions, const TActionList& all_actions) {
	qDebug("Gui::TEditableToolbar::setActionsFromStringList: '%s'", objectName().toUtf8().data());

	clear();

	for (int n = 0; n < actions.count(); n++) {
		QString action_name = actions[n];
		if (action_name == "separator") {
			QAction* action = new QAction(this);
			action->setSeparator(true);
			addAction(action);
		} else {
			QAction* action = TToolbarEditor::findAction(action_name, all_actions);
			if (action) {
				addAction(action);
				if (action->objectName().endsWith("_menu")) {
					// If the action is a menu change some of its properties
					QToolButton* button = qobject_cast<QToolButton*>(widgetForAction(action));
					if (button) {
						button->setPopupMode(QToolButton::InstantPopup);
					}
				}
			} else {
				qWarning("Gui::TEditableToolbar::setActionsFromStringList: action %s not found",
						 action_name.toUtf8().data());
			}
		}
	}
}

QStringList TEditableToolbar::actionsToStringList() {
	qDebug() << "Gui::TEditableToolbar::actionsToStringList: saving" << objectName();

	TActionList action_list = actions();
	QStringList list;

	for (int n = 0; n < action_list.count(); n++) {
		QAction* action = action_list[n];
		if (action->isSeparator()) {
			list << "separator";
		} else if (action->objectName().isEmpty()) {
			qWarning("Gui::TEditableToolbar::actionsToStringList: unknown action at pos %d", n);
		} else {
			list << action->objectName();
		}
	}

	return list;
}

void TEditableToolbar::edit() {
	qDebug("Gui::TEditableToolbar::edit");

	TActionList all_actions = main_window->getAllNamedActions();
	TToolbarEditor e(main_window);
	e.setAllActions(all_actions);
	e.setActiveActions(actions());
	e.setDefaultActions(default_actions);
	e.setIconSize(iconSize().width());

	if (e.exec() == QDialog::Accepted) {
		QStringList action_names = e.saveActions(all_actions);
		setActionsFromStringList(action_names, all_actions);
		resize(width(), e.iconSize());
		setIconSize(QSize(e.iconSize(), e.iconSize()));

		// Save actions to pref
		TActionsEditor::saveToConfig(main_window, Settings::pref);
		Settings::pref->sync();
	}
}

} // namespace Gui
#include "moc_editabletoolbar.cpp"


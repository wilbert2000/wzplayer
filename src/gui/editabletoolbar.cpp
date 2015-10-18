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
#include "gui/toolbareditor.h"
#include <QAction>
#include <QMainWindow>

namespace Gui {

TEditableToolbar::TEditableToolbar(QMainWindow* parent)
	: QToolBar(parent)
	, main_window(parent) {
}

TEditableToolbar::~TEditableToolbar() {
}

TActionList TEditableToolbar::allActions() {

	if (all_actions.isEmpty()) {
		all_actions = main_window->findChildren<QAction*>();
	}
	return all_actions;
}

void TEditableToolbar::setActionsFromStringList(const QStringList& actions) {

	clear();
	TToolbarEditor::load(this, actions, allActions());
}

QStringList TEditableToolbar::actionsToStringList() {
	return TToolbarEditor::save(this);
}

void TEditableToolbar::edit() {
	qDebug("Gui::TEditableToolbar::edit");

	TToolbarEditor e(main_window);
	e.setAllActions(allActions());
	e.setActiveActions(actions());
	e.setDefaultActions(defaultActions());
	e.setIconSize(iconSize().width());

	if (e.exec() == QDialog::Accepted) {
		QStringList r = e.activeActionsToStringList();
		qDebug("Gui::TEditableToolbar::edit: list: %s", r.join(",").toUtf8().constData());
		setActionsFromStringList(r);
		resize(width(), e.iconSize());
		setIconSize(QSize(e.iconSize(), e.iconSize()));
	}
}

} // namespace Gui
#include "moc_editabletoolbar.cpp"


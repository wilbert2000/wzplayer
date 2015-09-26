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
#include "toolbareditor.h"
#include <QAction>

namespace Gui {

TEditableToolbar::TEditableToolbar(QWidget * parent) : QToolBar(parent)
{
	widget = parent;
}

TEditableToolbar::~TEditableToolbar() {
}

QList<QAction *> TEditableToolbar::allActions() {
	if (!all_actions.isEmpty()) return all_actions;

	QList<QAction *> actions;
	if (widget) {
		actions = widget->findChildren<QAction *>();
	}
	return actions;
}

void TEditableToolbar::setActionsFromStringList(QStringList action_names) {
	clear();
	ToolbarEditor::load(this, action_names, allActions());
}

QStringList TEditableToolbar::actionsToStringList() {
	return ToolbarEditor::save(this);
}

void TEditableToolbar::edit() {
	qDebug("Gui::TEditableToolbar::edit");

	ToolbarEditor e(widget);
	e.setAllActions(allActions());
	e.setActiveActions(this->actions());
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


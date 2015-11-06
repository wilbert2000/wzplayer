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

#include "gui/action.h"
#include <QWidget>

namespace Gui {

TAction::TAction (QObject* parent, const char* name, bool autoadd) 
	: QAction(parent) {

	setObjectName(name);
	if (autoadd)
		addActionToParent();
}

TAction::TAction(QKeySequence accel, QObject* parent, const char* name, bool autoadd)
	: QAction(parent) {

	setObjectName(name);
	setShortcut(accel);
	if (autoadd)
		addActionToParent();
}

TAction::~TAction() {
}

void TAction::addShortcut(QKeySequence key) {
	setShortcuts(shortcuts() << key);
}

void TAction::addActionToParent() {

	if (parent()) {
		if (parent()->inherits("QWidget")) {
			QWidget* w = static_cast<QWidget*> (parent());
			w->addAction(this);
		}
	}
}

void TAction::change(const QIcon& icon, const QString& text) {

	setIcon(icon);
	change(text);
}

void TAction::change(const QString& text) {

	setText(text);

	QString accel_text = shortcut().toString();
	if (!accel_text.isEmpty()) {
		QString s = text;
		s.replace("&", "");
		setToolTip(s + " (" + accel_text + ")");
	}
}

void TAction::update(bool check) {

	if (check != isChecked())
		trigger();
}

} // namespace Gui

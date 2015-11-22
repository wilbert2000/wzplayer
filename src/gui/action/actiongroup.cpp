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

#include "gui/action/actiongroup.h"
#include <QAction>
#include <QList>
#include <QWidget>

namespace Gui {

TActionGroupItem::TActionGroupItem(QObject* parent,
								   TActionGroup* group,
								   const QString& name,
								   const QString& text,
								   int data,
								   bool autoadd)
	: TAction(parent, name, text, "noicon", 0, autoadd) {

	setData(data);
	setCheckable(true);
	group->addAction(this);
}


TActionGroup::TActionGroup(QObject* parent, const QString& name)
	: QActionGroup(parent) {

	setObjectName(name);
	setExclusive(true);
	connect(this, SIGNAL(triggered(QAction*)),
			this, SLOT(itemTriggered(QAction*)));
}

void TActionGroup::setChecked(int ID) {
	//qDebug("Gui::TActionGroup::setChecked: ID: %d", ID);

	QList <QAction *> l = actions();
	for (int n = 0; n < l.count(); n++) {
		QAction* action = l[n];
		if (!action->isSeparator() && action->data().toInt() == ID) {
			action->setChecked(true);
			return;
		}
	}
}

void TActionGroup::clear() {

	QList <QAction*> acts = actions();
	for (int i = acts.count() - 1; i >= 0; i--) {
		QAction* action = acts[i];
		removeAction(action);
		action->deleteLater();
	}
}

void TActionGroup::itemTriggered(QAction* a) {

	int value = a->data().toInt();
	qDebug("Gui::TActionGroup::itemTriggered: '%s' ID: %d",
		   a->objectName().toUtf8().data(), value);

	emit activated(value);
}

} // namespace Gui

#include "moc_actiongroup.cpp"

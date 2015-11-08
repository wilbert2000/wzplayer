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
								   const char* name,
								   int data,
								   bool autoadd)
	: TAction(parent, name, autoadd) {

	setData(data);
	setCheckable(true);
	if (group)
		group->addAction(this);
}

TActionGroupItem::TActionGroupItem(QObject* parent,
								   TActionGroup* group,
								   const QString& text,
								   const char* name,
								   int data,
								   bool autoadd)
	: TAction(parent, name, autoadd) {

	setData(data);
	setText(text);
	setCheckable(true);
	if (group)
		group->addAction(this);
}


TActionGroup::TActionGroup(const QString& name, QObject* parent)
	: QActionGroup(parent) {

	setObjectName(name);
	setExclusive(true);
	connect(this, SIGNAL(triggered(QAction *)),
			this, SLOT(itemTriggered(QAction *)));
}

QAction* TActionGroup::setChecked(int ID) {
	//qDebug("Gui::TActionGroup::setChecked: ID: %d", ID);

	QList <QAction *> l = actions();
	int count = l.count();
	for (int n = 0; n < count; n++) {
		QAction* action = l[n];
		if ((!action->isSeparator()) && (action->data().toInt() == ID)) {
			action->setChecked(true);
			return action;
		}
	}
	return 0;
}

void TActionGroup::setCheckedSlot(int id) {
	qDebug("Gui::TActionGroup::setCheckedSlot: group %s id %d",
		   objectName().toUtf8().data(), id);

	setChecked(id);
}

int TActionGroup::checked() {
	QAction* a = checkedAction();
	if (a) 
		return a->data().toInt();
	else
		return -1;
}

void TActionGroup::uncheckAll() {

	QList <QAction *> l = actions();
	for (int n = 0; n < l.count(); n++) {
		l[n]->setChecked(false);
	}
}

void TActionGroup::setActionsEnabled(bool b) {

	QList <QAction *> l = actions();
	for (int n = 0; n < l.count(); n++) {
		l[n]->setEnabled(b);
	}
}

void TActionGroup::clear(bool remove) {
	while (actions().count() > 0) {
		QAction* a = actions()[0];
		if (a) {
			removeAction(a);
			if (remove) a->deleteLater();
		}
	}
}

void TActionGroup::itemTriggered(QAction *a) {
	qDebug("Gui::TActionGroup::itemTriggered: '%s'", a->objectName().toUtf8().data());

	int value = a->data().toInt();
	qDebug("Gui::TActionGroup::itemTriggered: ID: %d", value);

	emit activated(value);
}

void TActionGroup::addTo(QWidget *w) {
	w->addActions(actions());
}

void TActionGroup::removeFrom(QWidget *w) {

	for (int n = 0; n < actions().count(); n++) {
		w->removeAction(actions()[n]);
	}
}

} // namespace Gui

#include "moc_actiongroup.cpp"

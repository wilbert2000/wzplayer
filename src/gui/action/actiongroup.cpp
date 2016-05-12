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

#include "gui/action/actiongroup.h"
#include <QAction>
#include <QList>
#include <QWidget>
#include "log4qt/logger.h"


namespace Gui {
namespace Action {

TActionGroupItem::TActionGroupItem(QObject* parent,
                                   TActionGroup* group,
                                   const QString& name,
                                   const QString& text,
                                   int data,
                                   bool autoadd,
                                   bool icon,
                                   const QKeySequence& shortCut)
    : TAction(parent, name, text, icon ? "" : "noicon", shortCut, autoadd) {

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

QAction* TActionGroup::setChecked(int ID) {
    //logger()->debug("Gui::Action::TActionGroup::setChecked: ID: %1", ID);

	QList <QAction *> l = actions();
	for (int n = 0; n < l.count(); n++) {
		QAction* action = l[n];
		if (!action->isSeparator() && action->data().toInt() == ID) {
			action->setChecked(true);
			return action;
		}
	}

	return 0;
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
    Log4Qt::Logger::logger("Gui::Action::TActionGroup")->debug(
                "itemTriggered: '%1' ID: %2", a->objectName(), value);

	emit activated(value);
}

} // namespace Action
} // namespace Gui

#include "moc_actiongroup.cpp"

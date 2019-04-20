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
#include "wzdebug.h"

#include <QAction>
#include <QList>
#include <QWidget>

namespace Gui {
namespace Action {

TActionGroupItem::TActionGroupItem(QObject* parent,
                                   TActionGroup* group,
                                   const QString& name,
                                   const QString& text,
                                   int data,
                                   bool icon,
                                   const QKeySequence& shortCut)
    : TAction(parent, name, text, icon ? "" : "noicon", shortCut, false) {

    setData(data);
    setCheckable(true);
    group->addAction(this);
}


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Action::TActionGroup)

TActionGroup::TActionGroup(QObject* parent, const QString& name)
    : QActionGroup(parent) {

    setObjectName(name);
    connect(this, &TActionGroup::triggered, this, &TActionGroup::onTriggered);
}

QAction* TActionGroup::setChecked(int ID) {

    QList <QAction*> acts = actions();
    for (int n = 0; n < acts.count(); n++) {
        QAction* action = acts.at(n);
        if (!action->isSeparator() && action->data().toInt() == ID) {
            action->setChecked(true);
            return action;
        }
    }

    // Does not need to be an error. TWindowSizeGroup set sizes not in menu.
    WZDOBJ << "ID" << ID << "not found";
    return 0;
}

void TActionGroup::clear() {

    QList<QAction*> acts = actions();
    for (int i = acts.count() - 1; i >= 0; i--) {
        QAction* action = acts.at(i);
        removeAction(action);
        action->deleteLater();
    }
}

void TActionGroup::onTriggered(QAction* a) {

    int value = a->data().toInt();
    WZDOBJ << a->objectName() << value;
    emit triggeredID(value);
}

} // namespace Action
} // namespace Gui

#include "moc_actiongroup.cpp"

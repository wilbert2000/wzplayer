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

#ifndef GUI_ACTIONGROUP_H
#define GUI_ACTIONGROUP_H

#include "gui/action/action.h"
#include "wzdebug.h"
#include <QActionGroup>

namespace Gui {
namespace Action {

//! TActionGroup makes it easier to create exclusive menus based on items
//! with integer data.
class TActionGroup : public QActionGroup {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TActionGroup (QObject* parent, const QString& name);

    //! Remove and delete all items.
    void clear();

public slots:
    //! Looks for the item which ID is \a ID and checks it
    QAction* setChecked(int ID);

signals:
    //! Emitted when an item has been checked
    void triggeredID(int);

private slots:
    void onTriggered(QAction*);
};

//! This class makes easy to create actions for TActionGroup

class TActionGroupItem : public TAction {
public:
    TActionGroupItem(QObject* parent,
                     TActionGroup* group,
                     const QString& name,
                     const QString& text,
                     int data,
                     bool icon = false,
                     const QKeySequence& shortCut = 0);
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTIONGROUP_H

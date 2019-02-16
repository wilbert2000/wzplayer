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

#ifndef GUI_ACTION_EDITABLETOOLBAR_H
#define GUI_ACTION_EDITABLETOOLBAR_H

#include <QToolBar>
#include <QStringList>
#include "gui/action/actionlist.h"
#include "wzdebug.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TEditableToolbar : public QToolBar {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TEditableToolbar(TMainWindow* mainwindow);
    virtual ~TEditableToolbar();

    QStringList actionsToStringList() const;
    void setActionsFromStringList(const QStringList& acts,
                                  const TActionList& allActions);

    QStringList getDefaultActions() const { return defaultActions; }
    void setDefaultActions(const QStringList& actions) {
        defaultActions = actions;
    }

public slots:
    void edit();

private:
    TMainWindow* main_window;
    QStringList actions;
    QStringList defaultActions;

    void addMenu(QAction* action);

private slots:
    void reload();
}; // class TEditableToolbar

} // namespace Action
} // namesapce Gui

#endif // GUI_ACTION_EDITABLETOOLBAR_H

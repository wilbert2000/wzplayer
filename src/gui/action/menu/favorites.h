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

#ifndef GUI_ACTION_MENU_FAVORITES_H
#define GUI_ACTION_MENU_FAVORITES_H

#include "gui/action/menu/menu.h"
#include "gui/action/menu/favorite.h"
#include "log4qt/logger.h"
#include <QList>


class QAction;

namespace Gui {
namespace Action {

class TAction;

namespace Menu {

typedef QList<TFavorite> TFavoriteList;

class TFavorites : public Menu::TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TFavorites(TMainWindow* mw,
               const QString& name,
               const QString& text,
               const QString& icon,
               const QString& filename);
    virtual ~TFavorites();

    TAction* getEditAct() const { return editAct; }
    TAction* getAddAct() const { return addAct; }
    TAction* getJumpAct() const { return jumpAct; }


public slots:
    void edit();
    void addCurrentPlaying();
    void jump();

protected:
    virtual void enableActions();

    virtual void save();
    virtual void updateMenu();
    virtual void populateMenu();
    void delete_children();

    // Mark current action in the menu
    void markCurrent();

protected slots:
    void onTriggered(QAction* action);

private:
    QString _filename;
    // Current or last file clicked
    QString current_file;
    // Last item selected in the jump dialog
    int last_item;

    TAction* editAct;
    TAction* addAct;
    TAction* jumpAct;

    TFavoriteList f_list;
    QList<TFavorites*> child;

    void load();
}; // class TFavorites

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_FAVORITES_H


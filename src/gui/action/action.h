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

#ifndef GUI_ACTION_ACTION_H
#define GUI_ACTION_ACTION_H

#include <QAction>
#include <QString>
#include <QKeySequence>

namespace Gui {
namespace Action {

extern QAction* findAction(const QString& name, const QList<QAction*>& actions);
extern void updateToolTip(QAction* action);

class TAction : public QAction {
public:
    TAction(QObject* parent,
            const QString& name,
            const QString& text,
            const QString& icon = 0,
            const QKeySequence& shortCut = 0,
            bool autoAdd = true);
    virtual ~TAction();

    void addShortcut(QKeySequence key);
    void setTextAndTip(const QString& text);

private:
    //! Checks if the parent is a QWidget and adds the action to it.
    void addActionToParent();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_ACTION_H


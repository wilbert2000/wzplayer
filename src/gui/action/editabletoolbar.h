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


class TWZTimer;

namespace Gui {
namespace Action {

class TEditableToolbar : public QToolBar {
    Q_OBJECT
public:
    TEditableToolbar(QWidget* parent,
                     const QString& name,
                     const QString& title);

    QStringList actionsToStringList() const;
    void setActionsFromStringList(const QStringList& acts);

    QStringList getDefaultActions() const { return defaultActions; }
    void setDefaultActions(const QStringList& actions) {
        defaultActions = actions;
    }

    void loadSettings();
    void saveSettings();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

public slots:
    void edit();

protected:
    virtual void resizeEvent(QResizeEvent*) override;

private:
    bool isMainToolbar;
    QStringList currentActions;
    QStringList defaultActions;

    TWZTimer* fixSizeTimer;

    void addAct(QAction* action);
    void addMenu(QAction* action);
    QSize adjustSizeHint(QSize s) const;

private slots:
    void reload();
    void fixSize();
}; // class TEditableToolbar

} // namespace Action
} // namesapce Gui

#endif // GUI_ACTION_EDITABLETOOLBAR_H

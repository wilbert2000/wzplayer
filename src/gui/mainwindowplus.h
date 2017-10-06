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

#ifndef GUI_MAINWINDOWPLUS_H
#define GUI_MAINWINDOWPLUS_H

#include "gui/mainwindow.h"

#include <QSystemTrayIcon>
#include "wzdebug.h"


class QMenu;

namespace Gui {

class TMainWindowPlus : public TMainWindow {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TMainWindowPlus();
    virtual ~TMainWindowPlus();

    virtual void loadConfig();
    virtual void saveConfig();

    bool startHidden() const;

protected slots:
    virtual void closeWindow();
    virtual void quit();
    virtual void setWindowCaption(const QString& title);
    virtual void onMediaInfoChanged();

private:
    QSystemTrayIcon* tray;

    Action::TAction* quitAct;
    Action::TAction* showTrayAct;
    Action::TAction* showAllAct;

    // To save state
    bool hideMainWindowOnStartup;

    void switchToTray();
    void retranslateStrings();
    void updateShowAllAct();
    void showAll(bool b);

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason);
    void showAll();
    void toggleShowAll();
};

} // namespace Gui

#endif // GUI_MAINWINDOWPLUS_H

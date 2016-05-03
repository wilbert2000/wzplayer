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

#ifndef GUI_BASEPLUS_H
#define GUI_BASEPLUS_H

#include "gui/base.h"

#include <QSystemTrayIcon>
#include <QPoint>

class QMenu;
class QDockWidget;

namespace Gui {

class TBasePlus : public TBase {
	Q_OBJECT

public:
	TBasePlus();
	virtual ~TBasePlus();

	virtual bool startHidden();
	virtual void loadConfig();
	virtual void saveConfig();

protected:
	QSystemTrayIcon* tray;
	QMenu* context_menu;

	Action::TAction* quitAct;
	Action::TAction* showTrayAct;
	Action::TAction* showAllAct;

	// To save state
	bool mainwindow_visible;
    bool restore_playlist;

	QDockWidget* playlistdock;

    virtual void changeEvent(QEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);

protected slots:
    // Reimplemented methods
    virtual void closeWindow();
    virtual void setWindowCaption(const QString& title);
    virtual void onMediaInfoChanged();
    virtual void showPlaylist(bool v);

    // New
    virtual void trayIconActivated(QSystemTrayIcon::ActivationReason);
    virtual void toggleShowAll();
    virtual void showAll(bool b);
    virtual void showAll();
    virtual void quit();
#ifdef Q_OS_OS2
    void trayAvailable();
#endif


private:
    double restore_size_factor;
    double old_size_factor;
    bool posted_restore_size_factor;
    bool block_restore;

    void switchToTray();
	void retranslateStrings();
    void updateShowAllAct();

private slots:
    void onDockVisibilityChanged(bool visible);
    void onTopLevelChanged(bool);
    void onvideoSizeFactorChanged(double, double);
    void resizeWindowToVideoRestoreSize();
    void clearBlockRestore();
    void setWinTitle();
};

} // namespace Gui

#endif // GUI_BASEPLUS_H

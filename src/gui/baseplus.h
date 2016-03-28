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
	virtual void changeEvent(QEvent* event);
	virtual void aboutToEnterFullscreen();
	virtual void didExitFullscreen();

	void updateShowAllAct();

protected slots:
	// Reimplemented methods
	virtual void closeWindow();
	virtual void setWindowCaption(const QString& title);
	virtual void resizeWindow(int w, int h);
	virtual void updateMediaInfo();
	// New
	virtual void trayIconActivated(QSystemTrayIcon::ActivationReason);
	virtual void toggleShowAll();
	virtual void showAll(bool b);
	virtual void showAll();
	virtual void quit();
#ifdef Q_OS_OS2
	void trayAvailable();
#endif

	virtual void showPlaylist(bool b);

	void dockVisibilityChanged(bool visible);
	void onTopLevelChanged(bool);
	void stretchWindow();
	void shrinkWindow();

protected:
	QSystemTrayIcon* tray;
	QMenu* context_menu;

	Action::TAction* quitAct;
	Action::TAction* showTrayAct;
	Action::TAction* showAllAct;

	// To save state
	bool mainwindow_visible;
	QPoint playlist_pos;
	bool trayicon_playlist_was_visible;

	QDockWidget* playlistdock;
	bool fullscreen_playlist_was_visible;
	bool fullscreen_playlist_was_floating;

private:
	void switchToTray();
	void setWinTitle();
	void retranslateStrings();
};

} // namespace Gui

#endif // GUI_BASEPLUS_H

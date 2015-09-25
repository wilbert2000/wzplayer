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

#ifndef _GUI_BASEPLUS_H_
#define _GUI_BASEPLUS_H_

#include "guiconfig.h"
#include "gui/base.h"
#include "gui/widgetactions.h"
#include "gui/playlistdock.h"

#include <QSystemTrayIcon>
#include <QPoint>

class QMenu;

namespace Gui {

class TBasePlus : public TBase
{
	Q_OBJECT

public:
	TBasePlus( QWidget* parent = 0, Qt::WindowFlags flags = 0);
	~TBasePlus();

	virtual bool startHidden();
	virtual void loadConfig(const QString &group);
	virtual void saveConfig(const QString &group);

protected:
	virtual void retranslateStrings();

	void updateShowAllAct();

    virtual void aboutToEnterFullscreen();
    virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

	// Functions for other GUI's
	TTimeSliderAction * createTimeSliderAction(QWidget * parent);
	TVolumeSliderAction * createVolumeSliderAction(QWidget * parent);

protected slots:
	// Reimplemented methods
	virtual void closeWindow();
	virtual void setWindowCaption(const QString & title);
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

#if DOCK_PLAYLIST
	virtual void showTPlaylist(bool b);
	void playlistClosed();

#if !USE_DOCK_TOPLEVEL_EVENT
	void dockVisibilityChanged(bool visible);
#else
	void dockTopLevelChanged(bool floating);
#endif

	void stretchWindow();
	void shrinkWindow();
#endif


protected:
	QSystemTrayIcon * tray;
	QMenu * context_menu;

	TAction * quitAct;
	TAction * showTrayAct;
	TAction * showAllAct;

	// To save state
	bool mainwindow_visible;

	QPoint playlist_pos;
	bool trayicon_playlist_was_visible;

	//QPoint infowindow_pos;
	//bool infowindow_visible;

   int widgets_size; // To be able to restore the original size after exiting from compact mode

#if DOCK_PLAYLIST
	TPlaylistDock * playlistdock;
	bool fullscreen_playlist_was_visible;
	bool fullscreen_playlist_was_floating;
	bool compact_playlist_was_visible;
	bool ignore_playlist_events;
#endif

private:
	void switchToTray();
};

} // namespace Gui

#endif // _GUI_BASEPLUS_H_

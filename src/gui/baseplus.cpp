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

#include "gui/baseplus.h"

#include <QMenu>
#include <QDesktopWidget>
#include <QDockWidget>

#include "config.h"
#include "gui/action.h"
#include "gui/playlist.h"
#include "gui/favorites.h"
#include "gui/tvlist.h"
#include "images.h"
#include "desktop.h"


using namespace Settings;

namespace Gui {

TBasePlus::TBasePlus()
	: TBase()
	, mainwindow_visible(true)
	, trayicon_playlist_was_visible(false) {

	tray = new QSystemTrayIcon(Images::icon("logo", 22), this);
	tray->setToolTip("SMPlayer");
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			 this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

	quitAct = new TAction(QKeySequence("Ctrl+Q"), this, "quit");
	connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));
	openMenu->addAction(quitAct);

	showTrayAct = new TAction(this, "show_tray_icon");
	showTrayAct->setCheckable(true);
	connect(showTrayAct, SIGNAL(toggled(bool)),
			 tray, SLOT(setVisible(bool)));

#ifndef Q_OS_OS2
	optionsMenu->addAction(showTrayAct);
#else
	trayAvailable();
	connect(optionsMenu, SIGNAL(aboutToShow()),
			 this, SLOT(trayAvailable()));
#endif

	showAllAct = new TAction(this, "restore_hide");
	connect(showAllAct, SIGNAL(triggered()),
			 this, SLOT(toggleShowAll()));

	context_menu = new QMenu(this);
	context_menu->addAction(showAllAct);
	context_menu->addSeparator();
	context_menu->addAction(openFileAct);
	context_menu->addMenu(recentfiles_menu);
	context_menu->addAction(openDirectoryAct);
	context_menu->addAction(openDVDAct);
	context_menu->addAction(openURLAct);
	context_menu->addMenu(favorites);
#ifndef Q_OS_WIN
	context_menu->addMenu(tvlist);
	context_menu->addMenu(radiolist);
#endif
	context_menu->addSeparator();
	context_menu->addAction(playOrPauseAct);
	context_menu->addAction(stopAct);
	context_menu->addSeparator();
	context_menu->addAction(playPrevAct);
	context_menu->addAction(playNextAct);
	context_menu->addSeparator();
	context_menu->addAction(showPlaylistAct);
	context_menu->addAction(showPreferencesAct);
	context_menu->addSeparator();
	context_menu->addAction(quitAct);
	
	tray->setContextMenu(context_menu);

	// Playlistdock
	playlistdock = new QDockWidget(this);
	playlistdock->hide();
	playlistdock->setObjectName("playlistdock");
	playlistdock->setAcceptDrops(true);
	playlistdock->setWidget(playlist);
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea
								  | Qt::BottomDockWidgetArea
								  | Qt::LeftDockWidgetArea
								  | Qt::RightDockWidgetArea);
	addDockWidget(Qt::BottomDockWidgetArea, playlistdock);
	playlistdock->setFloating(true); // Floating by default

	connect(playlistdock, SIGNAL(visibilityChanged(bool)),
			this, SLOT(dockVisibilityChanged(bool)));
	connect(this, SIGNAL(openFileRequested()),
			this, SLOT(showAll()));
}

TBasePlus::~TBasePlus() {
}

bool TBasePlus::startHidden() {

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	return false;
#else
	if ((!showTrayAct->isChecked()) || (mainwindow_visible))
		return false;
	else
		return true;
#endif
}

void TBasePlus::switchToTray() {

	exitFullscreen();
	showAll(false); // Hide windows
	if (core->state() == TCore::Playing)
		core->stop();

	if (pref->balloon_count > 0) {
		tray->showMessage("SMPlayer",
			tr("SMPlayer is still running here"),
			QSystemTrayIcon::Information, 3000);
		pref->balloon_count--;
	}
}

void TBasePlus::closeWindow() {
	qDebug("Gui::TBasePlus::closeWindow");

	if (tray->isVisible()) {
		switchToTray();
	} else {
		TBase::closeWindow();
	}
}

void TBasePlus::quit() {
	qDebug("Gui::TBasePlus::quit");

	// Bypass switch to tray
	TBase::closeWindow();
}

void TBasePlus::retranslateStrings() {
	//qDebug("Gui::TBasePlus::retranslateStrings");

	TBase::retranslateStrings();

	quitAct->change(Images::icon("exit"), tr("&Quit"));
	showTrayAct->change(Images::icon("systray"), tr("S&how icon in system tray"));

	updateShowAllAct();

	playlistdock->setWindowTitle(tr("Playlist"));
}

void TBasePlus::updateShowAllAct() {
	if (isVisible()) 
		showAllAct->change(tr("&Hide"));
	else
		showAllAct->change(tr("&Restore"));
}

void TBasePlus::saveConfig() {
	qDebug("Gui::TBasePlus::saveConfig");

	TBase::saveConfig();

	// Store inside group derived class
	pref->beginGroup(settingsGroupName());
	pref->beginGroup("base_gui_plus");

	pref->setValue("show_tray_icon", showTrayAct->isChecked());
	pref->setValue("mainwindow_visible", isVisible());
	pref->setValue("trayicon_playlist_was_visible", trayicon_playlist_was_visible);

	pref->endGroup();
	pref->endGroup();
}

void TBasePlus::loadConfig() {
	qDebug("Gui::TBasePlus::loadConfig");

	TBase::loadConfig();

	// load from group derived class
	pref->beginGroup(settingsGroupName());
	pref->beginGroup("base_gui_plus");

	bool show_tray_icon = pref->value("show_tray_icon", false).toBool();
	showTrayAct->setChecked(show_tray_icon);

	mainwindow_visible = pref->value("mainwindow_visible", true).toBool();
	trayicon_playlist_was_visible = pref->value("trayicon_playlist_was_visible", trayicon_playlist_was_visible).toBool();

	pref->endGroup();
	pref->endGroup();

	updateShowAllAct();
}

void TBasePlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	qDebug("Gui::TBasePlus::trayIconActivated: %d", reason);

	updateShowAllAct();

	if (reason == QSystemTrayIcon::Trigger) {
		toggleShowAll();
	}
	else
	if (reason == QSystemTrayIcon::MiddleClick) {
		core->playOrPause();
	}
}

void TBasePlus::toggleShowAll() {

	// Ignore if tray is not visible
	if (tray->isVisible()) {
		showAll(!isVisible());
	}
}

void TBasePlus::showAll() {
	if (!isVisible())
		showAll(true);
}

void TBasePlus::showAll(bool b) {

	if (!b) {
		// Hide all
		trayicon_playlist_was_visible = (playlistdock->isVisible() && 
										 playlistdock->isFloating());
		if (trayicon_playlist_was_visible)
			playlistdock->hide();
		hide();
	} else {
		// Show all
		show();
		if (trayicon_playlist_was_visible) {
			playlistdock->show();
		}
	}
	updateShowAllAct();
}

void TBasePlus::resizeWindow(int w, int h) {
	// qDebug("Gui::TBasePlus::resizeWindow: %d, %d", w, h);

	if (tray->isVisible() && !isVisible())
		showAll(true);

	TBase::resizeWindow(w, h);
}

void TBasePlus::updateMediaInfo() {
	qDebug("Gui::TBasePlus::updateMediaInfo");

	TBase::updateMediaInfo();
	tray->setToolTip(windowTitle());
}

void TBasePlus::setWindowCaption(const QString& title) {

	tray->setToolTip(title);
	TBase::setWindowCaption(title);
}

// Playlist stuff
void TBasePlus::aboutToEnterFullscreen() {
	//qDebug("Gui::TBasePlus::aboutToEnterFullscreen");

	TBase::aboutToEnterFullscreen();

	fullscreen_playlist_was_visible = playlistdock->isVisible();
	fullscreen_playlist_was_floating = playlistdock->isFloating();

	int playlist_screen = QApplication::desktop()->screenNumber(playlistdock);
	int mainwindow_screen = QApplication::desktop()->screenNumber(this);

	// Hide the playlist if it's in the same screen as the main window
	if (playlist_screen == mainwindow_screen) {
		playlistdock->hide();
		playlistdock->setFloating(true);
	}

	playlistdock->setAllowedAreas(Qt::NoDockWidgetArea);
}

void TBasePlus::didExitFullscreen() {
	//qDebug("Gui::TBasePlus::didExitFullscreen");

	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea
								  | Qt::BottomDockWidgetArea
								  | Qt::LeftDockWidgetArea
								  | Qt::RightDockWidgetArea);

	TBase::didExitFullscreen();

	playlistdock->setFloating(fullscreen_playlist_was_floating);
	if (fullscreen_playlist_was_visible) {
		playlistdock->show();
	}
}

void TBasePlus::showPlaylist(bool b) {
	qDebug("Gui::TBasePlus::showPlaylist: %d", b);

	if (b) {
		exitFullscreenIfNeeded();
		playlistdock->show();
		if (playlistdock->isFloating()) {
			TDesktop::keepInsideDesktop(playlistdock);
		}
	} else {
		playlistdock->hide();
	}
}

void TBasePlus::dockVisibilityChanged(bool visible) {
	qDebug("Gui::TBasePlus::dockVisibilityChanged: %d", visible);

	if (!playlistdock->isFloating()) {
		if (visible)
			stretchWindow();
		else
			shrinkWindow();
	}
}

void TBasePlus::stretchWindow() {
	qDebug("Gui::TBasePlus::stretchWindow");

	if (pref->fullscreen
		|| pref->resize_method != Settings::TPreferences::Always)
		return;

	qDebug("Gui::TBasePlus::stretchWindow: dockWidgetArea: %d", (int) dockWidgetArea(playlistdock));

	if ((dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea) ||
		(dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea)) {
		int new_height = height() + playlistdock->height();
		qDebug("Gui::TBasePlus::stretchWindow: stretching: new height: %d", new_height);
		resize(width(), new_height);
	} else if ((dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea) ||
			   (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea)) {
		int new_width = width() + playlistdock->width();
		qDebug("Gui::TBasePlus::stretchWindow: stretching: new width: %d", new_width);
		resize(new_width, height());
	}
	TDesktop::keepInsideDesktop(this);
}

void TBasePlus::shrinkWindow() {
	qDebug("Gui::TBasePlus::shrinkWindow");

	if (pref->fullscreen
		|| pref->resize_method != Settings::TPreferences::Always)
		return;

	qDebug("Gui::TBasePlus::shrinkWindow: dockWidgetArea: %d", (int) dockWidgetArea(playlistdock));

	if ((dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea)
		|| (dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea)) {
		int new_height = height() - playlistdock->height();
		qDebug("Gui::TBasePlus::shrinkWindow: shrinking: new height: %d", new_height);
		resize(width(), new_height);
	} else if ((dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea)
			   || (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea)) {
		int new_width = width() - playlistdock->width();
		qDebug("Gui::TBasePlus::shrinkWindow: shrinking: new width: %d", new_width);
		resize(new_width, height());
	}
}

#ifdef Q_OS_OS2
// we test if xcenter is available at all. if not disable the tray action. this is possible when xcenter is not opened or crashed
void TBasePlus::trayAvailable() {
	if (!tray->isSystemTrayAvailable()) {
			optionsMenu->removeAction(showTrayAct);
	}
	else {
		optionsMenu->addAction(showTrayAct);
	}
}
#endif

} // namespace Gui

#include "moc_baseplus.cpp"

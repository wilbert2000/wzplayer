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

#include "gui/baseplus.h"

#include <QDebug>
#include <QEvent>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QDockWidget>

#include "playerwindow.h"
#include "config.h"
#include "images.h"
#include "desktop.h"
#include "core.h"
#include "gui/action/action.h"
#include "gui/action/menuopen.h"
#include "gui/action/menuplay.h"
#include "gui/action/menuvideo.h"
#include "gui/action/menuaudio.h"
#include "gui/action/menusubtitle.h"
#include "gui/action/menubrowse.h"
#include "gui/action/menuwindow.h"
#include "gui/playlist/playlist.h"


using namespace Settings;

namespace Gui {

TBasePlus::TBasePlus() :
    TBase(),
    mainwindow_visible(true),
    restore_playlist(false),
    restore_size_factor(0),
    old_size_factor(0),
    posted_restore_size_factor(false),
    block_restore(false) {

	tray = new QSystemTrayIcon(this);
	tray->setToolTip(TConfig::PROGRAM_NAME);
	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			 this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

	quitAct = new Action::TAction(this, "quit", tr("&Quit"), "exit", QKeySequence("Ctrl+Q"));
	connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));
	openMenu->addAction(quitAct);

	showTrayAct = new Action::TAction(this, "show_tray_icon", tr("S&how icon in system tray"), "systray");
	showTrayAct->setCheckable(true);
	connect(showTrayAct, SIGNAL(toggled(bool)), tray, SLOT(setVisible(bool)));

#ifndef Q_OS_OS2
	windowMenu->addSeparator();
	windowMenu->addAction(showTrayAct);
#else
	trayAvailable();
	connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(trayAvailable()));
#endif

	showAllAct = new Action::TAction(this, "restore_hide", tr("&Hide"));
    connect(showAllAct, SIGNAL(triggered()), this, SLOT(toggleShowAll()));

	context_menu = new QMenu(this);
	context_menu->addMenu(openMenu);
	context_menu->addMenu(playMenu);
	context_menu->addMenu(videoMenu);
	context_menu->addMenu(audioMenu);
	context_menu->addMenu(subtitleMenu);
	context_menu->addMenu(browseMenu);
	context_menu->addMenu(windowMenu);
	context_menu->addSeparator();
	context_menu->addAction(showAllAct);
	context_menu->addAction(quitAct);
	
	tray->setContextMenu(context_menu);

	// Playlistdock
    setAnimated(false);
	playlistdock = new QDockWidget(this);
	playlistdock->setObjectName("playlistdock");
    playlistdock->setWidget(playlist);
    playlistdock->setFloating(true);
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea
								  | Qt::BottomDockWidgetArea
								  | Qt::LeftDockWidgetArea
								  | Qt::RightDockWidgetArea);
    playlistdock->setAcceptDrops(true);
    playlistdock->hide();
    addDockWidget(Qt::BottomDockWidgetArea, playlistdock);

	connect(playlistdock, SIGNAL(topLevelChanged(bool)),
			this, SLOT(onTopLevelChanged(bool)));
    connect(playlistdock, SIGNAL(visibilityChanged(bool)),
            this, SLOT(onDockVisibilityChanged(bool)));
    connect(playerwindow, SIGNAL(videoSizeFactorChanged(double, double)),
            this, SLOT(onvideoSizeFactorChanged(double,double)));
    connect(playlist, SIGNAL(windowTitleChanged()),
            this, SLOT(setWinTitle()));
    connect(this, SIGNAL(openFileRequested()),
			this, SLOT(showAll()));

	retranslateStrings();
}

TBasePlus::~TBasePlus() {
	tray->hide();
}

void TBasePlus::retranslateStrings() {

    tray->setIcon(Images::icon("logo", 22));
    setWinTitle();
    updateShowAllAct();
}

void TBasePlus::changeEvent(QEvent* e) {

    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QMainWindow::changeEvent(e);
    }
}

void TBasePlus::setWinTitle() {

    playlistdock->setWindowTitle(playlist->windowTitle());
}

void TBasePlus::setWindowCaption(const QString& title) {

    TBase::setWindowCaption(title);
    tray->setToolTip(title);
}

void TBasePlus::updateShowAllAct() {

    if (isVisible()) {
        showAllAct->setTextAndTip(tr("&Hide"));
    } else {
        showAllAct->setTextAndTip(tr("&Restore"));
    }
}

bool TBasePlus::startHidden() {

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	return false;
#else
	if (!showTrayAct->isChecked() || mainwindow_visible)
		return false;
	else
		return true;
#endif
}

void TBasePlus::switchToTray() {

	exitFullscreen();
	showAll(false); // Hide windows
    core->stop();

	if (pref->balloon_count > 0) {
		tray->showMessage(TConfig::PROGRAM_NAME,
			tr("%1 is still running here").arg(TConfig::PROGRAM_NAME),
			QSystemTrayIcon::Information, TConfig::MESSAGE_DURATION);
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

void TBasePlus::saveConfig() {
	qDebug("Gui::TBasePlus::saveConfig");

	TBase::saveConfig();

	// Store inside group derived class
	pref->beginGroup(settingsGroupName());
	pref->beginGroup("base_gui_plus");
    pref->setValue("show_tray_icon", showTrayAct->isChecked());
	pref->setValue("mainwindow_visible", isVisible());
	pref->endGroup();
	pref->endGroup();
}

void TBasePlus::loadConfig() {
	qDebug("Gui::TBasePlus::loadConfig");

	TBase::loadConfig();

	// load from group derived class
	pref->beginGroup(settingsGroupName());
	pref->beginGroup("base_gui_plus");

    showTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
	mainwindow_visible = pref->value("mainwindow_visible", true).toBool();

    pref->endGroup();
	pref->endGroup();

    restore_playlist = playlistdock->isVisible() && playlistdock->isFloating();

	setWinTitle();
	updateShowAllAct();
}

void TBasePlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	qDebug("Gui::TBasePlus::trayIconActivated: %d", reason);

	updateShowAllAct();

	if (reason == QSystemTrayIcon::Trigger) {
		toggleShowAll();
	} else if (reason == QSystemTrayIcon::MiddleClick) {
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

    if (b) {
        show();
        if (restore_playlist) {
            playlistdock->show();
        }
	} else {
        restore_playlist = playlistdock->isVisible()
                           && playlistdock->isFloating();
        if (restore_playlist) {
            playlistdock->hide();
        }
        hide();
    }
	updateShowAllAct();
}

void TBasePlus::onMediaInfoChanged() {
	qDebug("Gui::TBasePlus::onMediaInfoChanged");

	TBase::onMediaInfoChanged();
	tray->setToolTip(windowTitle());
}


// TODO: the following is an awfull lot of code to keep the video panel
// the same size before and after docking...

void TBasePlus::showEvent(QShowEvent* event) {

    TBase::showEvent(event);
    // Cancel resizing
    restore_size_factor = 0;
}

void TBasePlus::hideEvent(QHideEvent* event) {

    TBase::hideEvent(event);
    // Block resizing
    block_restore = true;
    QTimer::singleShot(500, this, SLOT(clearBlockRestore()));
}

void TBasePlus::clearBlockRestore() {
    block_restore = false;
}

void TBasePlus::showPlaylist(bool v) {
    qDebug("Gui::TBasePlus::showPlaylist: %d", v);

    restore_playlist = false;
    if (playlistdock->isFloating()) {
        if (v) {
            restore_playlist = true;
        }
    } else if (pref->resize_on_docking && !pref->fullscreen) {
        restore_size_factor = pref->size_factor;
    }

    // Triggers onDockVisibilityChanged
    playlistdock->setVisible(v);
}

// Try to save the size factor used before the dock resized the video panel
void TBasePlus::onvideoSizeFactorChanged(double old_size, double size) {
    //qDebug() << "Gui::TBasePlus::onvideoSizeFactorChanged: currently saved old size"
    //         << old_size_factor << "currently saved restore size"
    //         << restore_size_factor << switching_to_fullscreen;

    if (switching_to_fullscreen) {
        old_size_factor = -1;
    } else if (pref->fullscreen || restore_size_factor > 0) {
        //qDebug() << "Gui::TBasePlus::onvideoSizeFactorChanged: not saving old size" << old_size;
    } else if (old_size_factor == -1) {
        //qDebug() << "Gui::TBasePlus::onvideoSizeFactorChanged: saving new size" << size;
        old_size_factor = size;
    } else {
        //qDebug() << "Gui::TBasePlus::onvideoSizeFactorChanged: saving old size" << old_size;
        old_size_factor = old_size;
    }
}

// Slot to resize the video to its previous size
void TBasePlus::resizeWindowToVideoRestoreSize() {

    // Wait until mouse released
    if (qApp->mouseButtons()) {
        QTimer::singleShot(200, this, SLOT(resizeWindowToVideoRestoreSize()));
        return;
    }

    if (restore_size_factor > 0.15) {
        qDebug() << "Gui::TBasePlus::resizeWindowToVideoRestoreSize: restoring size factor from"
                 << pref->size_factor << "to" << restore_size_factor;
        pref->size_factor = restore_size_factor;
        resizeWindowToVideo();
        restore_size_factor = 0;
    } else {
        qDebug() << "Gui::TBasePlus::resizeWindowToVideoRestoreSize: canceled resize";
    }

    old_size_factor = pref->size_factor;
    posted_restore_size_factor = false;
}

void TBasePlus::onTopLevelChanged(bool topLevel) {
    qDebug() << "Gui::TBasePlus::onTopLevelChanged: topLevel" << topLevel
             << "size factor" << pref->size_factor;

    if (pref->resize_on_docking
        && core->stateReady()
        && !pref->fullscreen
        && !switching_to_fullscreen
        && restore_size_factor == 0
        && !posted_restore_size_factor) {

        if (topLevel) {
            // We became toplevel and the video size has not yet changed
            restore_size_factor = pref->size_factor;
            old_size_factor = restore_size_factor;
            qDebug() << "Gui::TBasePlus::onTopLevelChanged: saved size factor"
                     << restore_size_factor;
        } else {
            // We are docked now and the video size already changed
            restore_size_factor = old_size_factor;
            qDebug() << "Gui::TBasePlus::onTopLevelChanged: saved old size factor"
                     << restore_size_factor;
        }
        posted_restore_size_factor = true;
        QTimer::singleShot(200, this, SLOT(resizeWindowToVideoRestoreSize()));
    }
}

void TBasePlus::onDockVisibilityChanged(bool visible) {
    qDebug() << "Gui::TBasePlus:onDockVisibilityChanged: visible" << visible
             << "floating" << playlistdock->isFloating();

    if (block_restore) {
        block_restore = false;
        qDebug() << "Gui::TBasePlus:onDockVisibilityChanged: blocked restore";
        return;
    }

    if (playlistdock->isFloating()) {
        if (visible) {
            TDesktop::keepInsideDesktop(playlistdock);
        }
        return;
    }

    if (pref->resize_on_docking
        && !pref->fullscreen
        && !switching_to_fullscreen
        && core->stateReady()
        && !posted_restore_size_factor) {

        // Restore size already set by showPlaylist() if > 0
        if (restore_size_factor == 0) {
            if (visible) {
                // Dock became visible, video size already changed
                qDebug() << "Gui::TBasePlus:onDockVisibilityChanged: selecting old size factor";
                restore_size_factor = old_size_factor;
            } else {
                // Dock is hiding, video size not yet changed
                qDebug() << "Gui::TBasePlus:onDockVisibilityChanged: selecting current size factor";
                restore_size_factor = pref->size_factor;
            }
        }
        qDebug() << "Gui::TBasePlus:onDockVisibilityChanged: posting resizeWindowToVideoRestoreSize() with size factor"
                 << restore_size_factor;
        posted_restore_size_factor = true;
        QTimer::singleShot(200, this, SLOT(resizeWindowToVideoRestoreSize()));
    }
}

#ifdef Q_OS_OS2
// we test if xcenter is available at all. if not disable the tray action. this is possible when xcenter is not opened or crashed
void TBasePlus::trayAvailable() {
	if (!tray->isSystemTrayAvailable()) {
			windowMenu->removeAction(showTrayAct);
	}
	else {
		windowMenu->addAction(showTrayAct);
	}
}
#endif

} // namespace Gui

#include "moc_baseplus.cpp"

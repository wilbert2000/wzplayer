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

#include "gui/mainwindowplus.h"

#include <QEvent>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QDockWidget>

#include "gui/playerwindow.h"
#include "config.h"
#include "images.h"
#include "gui/desktop.h"
#include "player/player.h"
#include "gui/action/action.h"
#include "gui/action/menufile.h"
#include "gui/action/menuplay.h"
#include "gui/action/menuvideo.h"
#include "gui/action/menuaudio.h"
#include "gui/action/menusubtitle.h"
#include "gui/action/menubrowse.h"
#include "gui/action/menuwindow.h"
#include "gui/playlist/playlist.h"


using namespace Settings;

namespace Gui {

TMainWindowPlus::TMainWindowPlus() :
    TMainWindow(),
    debug(logger()),
    mainwindow_visible(true),
    restore_playlist(false),
    saveSize(0),
    saveSizeVisible(false),
    saveSizeFloating(true),
    saveSizeDockArea(Qt::LeftDockWidgetArea),
    dockArea(Qt::LeftDockWidgetArea),
    postedResize(false) {

    tray = new QSystemTrayIcon(this);
    tray->setIcon(Images::icon("logo", 22));
    tray->setToolTip(TConfig::PROGRAM_NAME);
    connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    quitAct = new Action::TAction(this, "quit", tr("&Quit"), "exit",
                                  QKeySequence("Ctrl+Q"));
    quitAct->setVisible(false);
    connect(quitAct, SIGNAL(triggered()), this, SLOT(quit()));
    fileMenu->addAction(quitAct);

    showTrayAct = new Action::TAction(this, "show_tray_icon",
                                      tr("S&how icon in system tray"),
                                      "systray");
    showTrayAct->setCheckable(true);
    connect(showTrayAct, SIGNAL(toggled(bool)), tray, SLOT(setVisible(bool)));
    connect(showTrayAct, SIGNAL(toggled(bool)),
            quitAct, SLOT(setVisible(bool)));

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
    context_menu->addMenu(fileMenu);
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
    addDockWidget(Qt::LeftDockWidgetArea, playlistdock);
    playlistdock->hide();

    saveSizeTimer = new QTimer(this);
    saveSizeTimer->setInterval(750);
    saveSizeTimer->setSingleShot(true);
    connect(saveSizeTimer, SIGNAL(timeout()),
            this, SLOT(saveSizeFactor()));

    connect(playlistdock, SIGNAL(topLevelChanged(bool)),
            this, SLOT(onTopLevelChanged(bool)));
    connect(playlistdock, SIGNAL(visibilityChanged(bool)),
            this, SLOT(onDockVisibilityChanged(bool)));
    connect(playlistdock, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
            this, SLOT(onDockLocationChanged(Qt::DockWidgetArea)));
    connect(playerwindow, SIGNAL(videoSizeFactorChanged(double, double)),
            this, SLOT(onvideoSizeFactorChanged(double,double)));
    connect(playlist, SIGNAL(windowTitleChanged()),
            this, SLOT(setWinTitle()));
    connect(this, SIGNAL(openFileRequested()),
            this, SLOT(showAll()));

    retranslateStrings();
}

TMainWindowPlus::~TMainWindowPlus() {
    tray->hide();
}


void TMainWindowPlus::retranslateStrings() {

    setWinTitle();
    updateShowAllAct();
}

void TMainWindowPlus::changeEvent(QEvent* e) {

    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QMainWindow::changeEvent(e);
    }
}

void TMainWindowPlus::setWinTitle() {

    playlistdock->setWindowTitle(playlist->windowTitle());
}

void TMainWindowPlus::setWindowCaption(const QString& title) {

    TMainWindow::setWindowCaption(title);
    tray->setToolTip(title);
}

void TMainWindowPlus::updateShowAllAct() {

    if (isVisible()) {
        showAllAct->setTextAndTip(tr("&Hide"));
    } else {
        showAllAct->setTextAndTip(tr("&Restore"));
    }
}

bool TMainWindowPlus::startHidden() {

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
    return false;
#else
    if (!showTrayAct->isChecked() || mainwindow_visible)
        return false;
    else
        return true;
#endif
}

void TMainWindowPlus::switchToTray() {

    exitFullscreen();
    showAll(false); // Hide windows
    player->stop();

    if (pref->balloon_count > 0) {
        tray->showMessage(TConfig::PROGRAM_NAME,
            tr("%1 is still running here").arg(TConfig::PROGRAM_NAME),
            QSystemTrayIcon::Information, TConfig::MESSAGE_DURATION);
        pref->balloon_count--;
    }
}

void TMainWindowPlus::closeWindow() {
    logger()->debug("closeWindow");

    if (tray->isVisible()) {
        switchToTray();
    } else {
        TMainWindow::closeWindow();
    }
}

void TMainWindowPlus::quit() {
    logger()->debug("quit");

    // Bypass switch to tray
    TMainWindow::closeWindow();
}

void TMainWindowPlus::saveConfig() {
    logger()->debug("saveConfig");

    TMainWindow::saveConfig();

    pref->beginGroup("mainwindowplus");
    pref->setValue("show_tray_icon", showTrayAct->isChecked());
    pref->setValue("mainwindow_visible", isVisible());
    pref->endGroup();
}

void TMainWindowPlus::loadConfig() {
    logger()->debug("loadConfig");

    TMainWindow::loadConfig();

    pref->beginGroup("mainwindowplus");
    showTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
	mainwindow_visible = pref->value("mainwindow_visible", true).toBool();
    pref->endGroup();

    restore_playlist = playlistdock->isVisible() && playlistdock->isFloating();

    setWinTitle();
    updateShowAllAct();
}

void TMainWindowPlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    logger()->debug("trayIconActivated: %1", reason);

	updateShowAllAct();

	if (reason == QSystemTrayIcon::Trigger) {
		toggleShowAll();
	} else if (reason == QSystemTrayIcon::MiddleClick) {
        player->playOrPause();
	}
}

void TMainWindowPlus::toggleShowAll() {

	// Ignore if tray is not visible
	if (tray->isVisible()) {
		showAll(!isVisible());
	}
}

void TMainWindowPlus::showAll() {
	if (!isVisible())
		showAll(true);
}

void TMainWindowPlus::showAll(bool b) {

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

void TMainWindowPlus::onMediaInfoChanged() {
    logger()->debug("onMediaInfoChanged");

    TMainWindow::onMediaInfoChanged();
	tray->setToolTip(windowTitle());
}


// The following is an awfull lot of code to keep the video panel
// the same size before and after docking...

void TMainWindowPlus::onDockLocationChanged(Qt::DockWidgetArea area) {
    logger()->debug("onDockLocationChanged: changed from %1 to %2",
                    dockArea, area);
    dockArea = area;
}

void TMainWindowPlus::saveSizeFactor(bool checkMouse, bool saveVisible, bool visible) {

    if (checkMouse && qApp->mouseButtons()) {
        saveSizeTimer->start();
    } else {
        saveSizeTimer->stop();
        if (pref->size_factor >= 0.1) {
            logger()->debug("saveSizeFactor: overwriting save size "
                            + QString::number(saveSize)
                            + " with new size "
                            + QString::number(pref->size_factor));
            saveSize = pref->size_factor;
            saveSizeFileName = player->mdat.filename;
            saveSizeFloating = playlistdock->isFloating();
            if (saveVisible) {
                saveSizeVisible = playlistdock->isVisible();
            } else {
                saveSizeVisible = visible;
            }
            saveSizeDockArea = dockArea;
        } else {
            debug << "saveSizeFactor: ignoring small size"
                  << pref->size_factor << debug;
        }
    }
}

// Try to save the size factor used before the dock resized the video panel
void TMainWindowPlus::onvideoSizeFactorChanged(double, double) {

    if (pref->resize_on_docking
        && !pref->fullscreen
        && !switching_to_fullscreen) {
        if (saveSize == pref->size_factor || postedResize) {
            saveSizeTimer->stop();
        } else {
            saveSizeTimer->start();
        }
    }
}

void TMainWindowPlus::showPlaylist(bool v) {
    logger()->debug("showPlaylist: %1", v);

    restore_playlist = false;
    if (playlistdock->isFloating()) {
        if (v) {
            restore_playlist = true;
        }
    } else if (pref->resize_on_docking && !pref->fullscreen) {
        saveSizeFactor(false);
        postedResize = true;
        QTimer::singleShot(250, this, SLOT(restoreVideoSize()));
    }

    // Triggers onDockVisibilityChanged
    playlistdock->setVisible(v);
}

void TMainWindowPlus::reposition(const QSize& oldWinSize) {

    QSize d = (frameGeometry().size() - oldWinSize) / 2;
    QPoint p = pos() - QPoint(d.width(), d.height());
    move(p);
    TDesktop::keepInsideDesktop(this);
}

// Slot to resize the video to its saved size
void TMainWindowPlus::restoreVideoSize() {

    saveSizeTimer->stop();

    // Wait until mouse released
    if (qApp->mouseButtons()) {
        QTimer::singleShot(200, this, SLOT(restoreVideoSize()));
        return;
    }

    bool dockVertical = dockArea == Qt::LeftDockWidgetArea
                        || dockArea == Qt::RightDockWidgetArea;
    bool saveDockVertical = saveSizeDockArea == Qt::LeftDockWidgetArea
                            || saveSizeDockArea == Qt::RightDockWidgetArea;
    if (!playlistdock->isFloating()
        && !saveSizeFloating
        && saveSizeVisible == playlistdock->isVisible()
        && dockVertical == saveDockVertical) {
        logger()->debug("restoreVideoSize: areas match, canceling resize");
    } else {
        if (saveSize < 0.1) {
            debug << "restoreVideoSize: ignoring small saved size "
                  << saveSize << debug;
        } else if (saveSizeFileName == player->mdat.filename) {
            debug << "restoreVideoSize: restoring size factor from"
                  << pref->size_factor << "to" << saveSize << debug;
            QSize oldWinSize = frameGeometry().size();
            pref->size_factor = saveSize;
            resizeWindowToVideo();
            reposition(oldWinSize);
        } else {
            logger()->debug("restoreVideoSize: file name save size '%1'"
                            " mismatches '%2', canceling resize",
                            saveSizeFileName, player->mdat.filename);
        }
    }

    saveSizeFactor(false);
    postedResize = false;
}

void TMainWindowPlus::onTopLevelChanged(bool topLevel) {
    //debug << "onTopLevelChanged: topLevel" << topLevel
    //      << "size" << pref->size_factor
    //      << "saved size" << saveSize << debug;

    if (!pref->resize_on_docking
        || pref->fullscreen
        || switching_to_fullscreen
        || postedResize) {
        return;
    }

    if (topLevel) {
        // We became toplevel and the video size has not yet changed
        saveSizeFactor(false);
        //debug << "onTopLevelChanged: saved size factor" << saveSize << debug;
    } else {
        // We are docked now and the video size already changed
        saveSizeTimer->stop();
        //debug << "onTopLevelChanged: keeping saved size factor" << saveSize
        //      << debug;;
    }

    if (player->stateReady()) {
        //debug << "onTopLevelChanged: posting restoreVideoSize() with size"
        //         " factor" << saveSize << debug;
        postedResize = true;
        QTimer::singleShot(250, this, SLOT(restoreVideoSize()));
    }
}

void TMainWindowPlus::onDockVisibilityChanged(bool visible) {
    //debug << "onDockVisibilityChanged: visible" << visible
    //      << "floating" << playlistdock->isFloating()
    //      << "size" << pref->size_factor
    //      << "saved size" << saveSize
    //      << debug;

    if (playlistdock->isFloating()) {
        if (visible) {
            TDesktop::keepInsideDesktop(playlistdock);
        }
        return;
    }

    if (!pref->resize_on_docking
        || pref->fullscreen
        || switching_to_fullscreen
        || postedResize) {
        return;
    }

    if (visible) {
        // Dock became visible, video size already changed
        saveSizeTimer->stop();
        //debug << "onDockVisibilityChanged: keeping saved size" << saveSize
        //      << debug;
    } else {
        // Dock is hiding, video size not yet changed
        //debug << "onDockVisibilityChanged: saving size factor"
        //      << pref->size_factor << debug;
        saveSizeFactor(false, false, true);
    }

    if (player->stateReady()) {
        //debug << "onDockVisibilityChanged: posting restoreVideoSize() with"
        //         " size factor" << saveSize << debug;
        postedResize = true;
        QTimer::singleShot(250, this, SLOT(restoreVideoSize()));
    }
}

#ifdef Q_OS_OS2
// we test if xcenter is available at all. if not disable the tray action. this
// is possible when xcenter is not opened or crashed
void TMainWindowPlus::trayAvailable() {
	if (!tray->isSystemTrayAvailable()) {
			windowMenu->removeAction(showTrayAct);
	}
	else {
		windowMenu->addAction(showTrayAct);
	}
}
#endif

} // namespace Gui

#include "moc_mainwindowplus.cpp"
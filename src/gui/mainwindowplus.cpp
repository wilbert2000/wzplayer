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

#include <QMenu>

#include "images.h"
#include "player/player.h"
#include "gui/action/action.h"
#include "gui/action/menu/menufile.h"
#include "gui/action/menu/menuplay.h"
#include "gui/action/menu/menuvideo.h"
#include "gui/action/menu/menuaudio.h"
#include "gui/action/menu/menusubtitle.h"
#include "gui/action/menu/menubrowse.h"
#include "gui/action/menu/menuwindow.h"


using namespace Settings;

namespace Gui {

TMainWindowPlus::TMainWindowPlus() :
    TMainWindow(),
    debug(logger()),
    hideMainWindowOnStartup(false) {

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
    connect(showTrayAct, SIGNAL(toggled(bool)),
            tray, SLOT(setVisible(bool)));
    connect(showTrayAct, SIGNAL(toggled(bool)),
            quitAct, SLOT(setVisible(bool)));

    windowMenu->addSeparator();
    windowMenu->addAction(showTrayAct);

    showAllAct = new Action::TAction(this, "restore_hide", tr("&Hide"));
    connect(showAllAct, SIGNAL(triggered()),
            this, SLOT(toggleShowAll()));

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

    connect(this, SIGNAL(openFileRequested()),
            this, SLOT(showAll()));

    retranslateStrings();
}

TMainWindowPlus::~TMainWindowPlus() {
    tray->hide();
}


void TMainWindowPlus::retranslateStrings() {

    updateShowAllAct();
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

bool TMainWindowPlus::startHidden() const {

#if defined(Q_OS_WIN)
    return false;
#else
    return hideMainWindowOnStartup && showTrayAct->isChecked();
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
    WZDEBUG("");

    if (tray->isVisible()) {
        switchToTray();
    } else {
        TMainWindow::closeWindow();
    }
}

void TMainWindowPlus::quit() {
    WZDEBUG("");

    // Bypass switch to tray
    TMainWindow::closeWindow();
}

void TMainWindowPlus::saveConfig() {
    WZDEBUG("");

    TMainWindow::saveConfig();

    pref->beginGroup("mainwindowplus");
    pref->setValue("show_tray_icon", showTrayAct->isChecked());
    pref->setValue("hideMainWindowOnStartup", !isVisible());
    pref->endGroup();
}

void TMainWindowPlus::loadConfig() {
    WZDEBUG("");

    TMainWindow::loadConfig();

    pref->beginGroup("mainwindowplus");
    showTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
    hideMainWindowOnStartup = pref->value("hideMainWindowOnStartup",
                                          hideMainWindowOnStartup).toBool();
    pref->endGroup();

    updateShowAllAct();
}

void TMainWindowPlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    WZDEBUG(QString::number(reason));

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

    setVisible(b);
    updateShowAllAct();
}

void TMainWindowPlus::onMediaInfoChanged() {
    WZDEBUG("");

    TMainWindow::onMediaInfoChanged();
    tray->setToolTip(windowTitle());
}

} // namespace Gui

#include "moc_mainwindowplus.cpp"

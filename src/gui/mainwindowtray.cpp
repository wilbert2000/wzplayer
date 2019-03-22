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

#include "gui/mainwindowtray.h"

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
#include "gui/action/menu/menuview.h"
#include "gui/playlist/playlist.h"


using namespace Settings;

namespace Gui {

TMainWindowTray::TMainWindowTray() :
    TMainWindow(),
    debug(logger()),
    hideMainWindowOnStartup(false) {

    tray = new QSystemTrayIcon(this);
    tray->setIcon(Images::icon("logo", 22));
    tray->setToolTip(TConfig::PROGRAM_NAME);
    connect(tray, &QSystemTrayIcon::activated,
            this, &TMainWindowTray::onSystemTrayActivated);

    quitAct = new Action::TAction(this, "quit", tr("Quit"), "exit",
                                  QKeySequence("Ctrl+Q"));
    quitAct->setVisible(false);
    connect(quitAct, &Action::TAction::triggered, this, &TMainWindowTray::quit);
    fileMenu->addAction(quitAct);

    showTrayAct = new Action::TAction(this, "show_tray_icon",
                                      tr("Show in system tray"));
    showTrayAct->setCheckable(true);
    connect(showTrayAct, &Action::TAction::toggled,
            tray, &QSystemTrayIcon::setVisible);
    connect(showTrayAct, &Action::TAction::toggled,
            quitAct, &Action::TAction::setVisible);

    viewMenu->addSeparator();
    viewMenu->addAction(showTrayAct);

    showMainWindowAct = new Action::TAction(this, "tray_restore_hide", "");
    updateShowMainWindowAct();
    connect(showMainWindowAct, &Action::TAction::triggered,
            this, &TMainWindowTray::toggleShowMainWindow);

    QMenu* menu = createContextMenu();
    menu->addSeparator();
    menu->addAction(showMainWindowAct);
    menu->addAction(quitAct);
    tray->setContextMenu(menu);

    connect(this, &TMainWindowTray::gotMessageFromOtherInstance,
            this, &TMainWindowTray::showMainWin);
}

TMainWindowTray::~TMainWindowTray() {
    tray->hide();
}

bool TMainWindowTray::startHidden() const {

#if defined(Q_OS_WIN)
    return false;
#else
    return hideMainWindowOnStartup && showTrayAct->isChecked();
#endif
}

void TMainWindowTray::setWindowCaption(const QString& title) {

    TMainWindow::setWindowCaption(title);
    tray->setToolTip(title);
}

void TMainWindowTray::updateShowMainWindowAct() {

    if (isVisible()) {
        showMainWindowAct->setTextAndTip(tr("Hide in system tray"));
    } else {
        showMainWindowAct->setTextAndTip(tr("Restore from system tray"));
    }
}

void TMainWindowTray::switchToTray() {
    WZTRACE("");

    player->pause();
    exitFullscreen();
    showMainWindow(false);

    if (pref->balloon_count > 0) {
        tray->showMessage(TConfig::PROGRAM_NAME,
            tr("%1 is running here").arg(TConfig::PROGRAM_NAME),
            QSystemTrayIcon::Information, TConfig::MESSAGE_DURATION);
        pref->balloon_count--;
    }
}

void TMainWindowTray::closeWindow() {
    WZDEBUG("");

    if (tray->isVisible()) {
        switchToTray();
    } else {
        TMainWindow::closeWindow();
    }
}

void TMainWindowTray::quit() {
    WZDEBUG("");

    // Bypass switchToTray() in TMainWindowTray::closeWindow
    TMainWindow::closeWindow();
}

void TMainWindowTray::onSystemTrayActivated(
        QSystemTrayIcon::ActivationReason reason) {
    WZDEBUG(QString::number(reason));

    if (reason == QSystemTrayIcon::Trigger) {
        toggleShowMainWindow();
    } else if (reason == QSystemTrayIcon::MiddleClick) {
        player->playOrPause();
    }
}

void TMainWindowTray::showMainWindow(bool b) {

    setVisible(b);
    updateShowMainWindowAct();
}

void TMainWindowTray::toggleShowMainWindow() {
    WZTRACE("");

    if (tray->isVisible()) {
        showMainWindow(!isVisible());
    } else {
        // Never end up without GUI
        showMainWin();
        updateShowMainWindowAct();
    }
}

void TMainWindowTray::showMainWin() {

    if (!isVisible()) {
        showMainWindow(true);
    }
}

void TMainWindowTray::onMediaInfoChanged() {

    TMainWindow::onMediaInfoChanged();
    tray->setToolTip(windowTitle());
}

void TMainWindowTray::saveSettings() {
    WZTRACE("");

    TMainWindow::saveSettings();

    pref->beginGroup("mainwindowtray");
    pref->setValue("show_tray_icon", showTrayAct->isChecked());
    pref->setValue("hide_main_window_on_startup", !isVisible());
    pref->endGroup();
}

void TMainWindowTray::loadSettings() {
    WZTRACE("");

    TMainWindow::loadSettings();

    pref->beginGroup("mainwindowtray");
    showTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
    hideMainWindowOnStartup = pref->value("hide_main_window_on_startup",
                                          hideMainWindowOnStartup).toBool();
    pref->endGroup();

    updateShowMainWindowAct();
}

} // namespace Gui

#include "moc_mainwindowtray.cpp"

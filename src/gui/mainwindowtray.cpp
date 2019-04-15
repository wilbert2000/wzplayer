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
#include "iconprovider.h"


using namespace Settings;

namespace Gui {

TMainWindowTray::TMainWindowTray() :
    TMainWindow(),
    hideMainWindowOnStartup(false) {

    tray = new QSystemTrayIcon(this);
    tray->setIcon(Images::icon("logo", iconProvider.iconSize.width()));
    tray->setToolTip(TConfig::PROGRAM_NAME);
    connect(tray, &QSystemTrayIcon::activated,
            this, &TMainWindowTray::onSystemTrayActivated);

    closeAct->setVisible(false);
    connect(quitAct, &Action::TAction::triggered, this, &TMainWindowTray::quit);

    viewSystemTrayAct = new Action::TAction(this, "view_sytemtray",
                                      tr("Show in system tray"));
    viewSystemTrayAct->setCheckable(true);
    viewSystemTrayAct->setChecked(false);
    connect(viewSystemTrayAct, &Action::TAction::toggled,
            this, &TMainWindowTray::onViewSystemTrayActToggled);
    viewMenu->addSeparator();
    viewMenu->addAction(viewSystemTrayAct);

    showMainWindowAct = new Action::TAction(this, "view_main_window", "");
    updateShowMainWindowActText();
    connect(showMainWindowAct, &Action::TAction::triggered,
            this, &TMainWindowTray::toggleShowMainWindow);

    // Give menu no name to not show up in action editors
    QMenu* menu = createContextMenu("", tr("%1 system tray")
                                    .arg(TConfig::PROGRAM_NAME));
    menu->addSeparator();
    menu->addAction(playPauseStopAct);
    menu->addAction(quitAct);
    tray->setContextMenu(menu);
}

TMainWindowTray::~TMainWindowTray() {
    tray->hide();
}

bool TMainWindowTray::startHidden() const {

#if defined(Q_OS_WIN)
    return false;
#else
    return hideMainWindowOnStartup && viewSystemTrayAct->isChecked();
#endif
}

void TMainWindowTray::setWindowCaption(const QString& title) {

    TMainWindow::setWindowCaption(title);
    tray->setToolTip(title);
}

void TMainWindowTray::updateShowMainWindowActText() {

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
    showMainWin(false);

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

void TMainWindowTray::onViewSystemTrayActToggled(bool show) {
    WZTRACE("");

    tray->setVisible(show);
    closeAct->setVisible(show);
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

void TMainWindowTray::showMainWin(bool b) {

    setVisible(b);
    updateShowMainWindowActText();
}

void TMainWindowTray::showMainWindow() {

    if (!isVisible()) {
        showMainWin(true);
    }
}

void TMainWindowTray::toggleShowMainWindow() {
    WZTRACE("");

    if (tray->isVisible()) {
        showMainWin(!isVisible());
    } else {
        // Never end up without GUI
        showMainWindow();
        updateShowMainWindowActText();
    }
}

void TMainWindowTray::saveSettings() {
    WZTRACE("");

    TMainWindow::saveSettings();

    pref->beginGroup("mainwindowtray");
    pref->setValue("show_tray_icon", viewSystemTrayAct->isChecked());
    pref->setValue("hide_main_window_on_startup", !isVisible());
    pref->endGroup();
}

void TMainWindowTray::loadSettings() {
    WZTRACE("");

    TMainWindow::loadSettings();

    pref->beginGroup("mainwindowtray");
    viewSystemTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
    hideMainWindowOnStartup = pref->value("hide_main_window_on_startup",
                                          hideMainWindowOnStartup).toBool();
    pref->endGroup();

    updateShowMainWindowActText();
}

} // namespace Gui

#include "moc_mainwindowtray.cpp"

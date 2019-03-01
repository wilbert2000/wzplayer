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
            this, &TMainWindowTray::onSystemTrayIconActivated);

    quitAct = new Action::TAction(this, "quit", tr("Quit"), "exit",
                                  QKeySequence("Ctrl+Q"));
    quitAct->setVisible(false);
    connect(quitAct, &Action::TAction::triggered, this, &TMainWindowTray::quit);
    fileMenu->addAction(quitAct);

    showTrayAct = new Action::TAction(this, "show_tray_icon",
                                      tr("Show icon in system tray"));
    showTrayAct->setCheckable(true);
    connect(showTrayAct, &Gui::Action::TAction::toggled,
            tray, &QSystemTrayIcon::setVisible);
    connect(showTrayAct, &Gui::Action::TAction::toggled,
            quitAct, &Action::TAction::setVisible);

    viewMenu->addSeparator();
    viewMenu->addAction(showTrayAct);

    showAllAct = new Action::TAction(this, "tray_restore_hide", tr("Hide"));
    updateShowAllAct();
    connect(showAllAct, &Action::TAction::triggered,
            this, &TMainWindowTray::toggleShowAll);

    QMenu* menu = createContextMenu();
    menu->addSeparator();
    menu->addAction(showAllAct);
    menu->addAction(quitAct);
    tray->setContextMenu(menu);

    connect(this, &TMainWindowTray::openFileRequested,
            this, &TMainWindowTray::showMainWindow);
}

TMainWindowTray::~TMainWindowTray() {
    tray->hide();
}

void TMainWindowTray::setWindowCaption(const QString& title) {

    TMainWindow::setWindowCaption(title);
    tray->setToolTip(title);
}

void TMainWindowTray::updateShowAllAct() {

    if (isVisible()) {
        showAllAct->setTextAndTip(tr("Hide"));
    } else {
        showAllAct->setTextAndTip(tr("Restore"));
    }
}

bool TMainWindowTray::startHidden() const {

#if defined(Q_OS_WIN)
    return false;
#else
    return hideMainWindowOnStartup && showTrayAct->isChecked();
#endif
}

void TMainWindowTray::switchToTray() {

    exitFullscreen();
    showAll(false); // Hide windows
    playlist->stop();

    if (pref->balloon_count > 0) {
        tray->showMessage(TConfig::PROGRAM_NAME,
            tr("%1 is still running here").arg(TConfig::PROGRAM_NAME),
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

void TMainWindowTray::saveSettings() {
    WZDEBUG("");

    TMainWindow::saveSettings();

    pref->beginGroup("mainwindowtray");
    pref->setValue("show_tray_icon", showTrayAct->isChecked());
    pref->setValue("hide_main_window_on_startup", !isVisible());
    pref->endGroup();
}

void TMainWindowTray::loadSettings() {
    WZDEBUG("");

    TMainWindow::loadSettings();

    pref->beginGroup("mainwindowtray");
    showTrayAct->setChecked(pref->value("show_tray_icon", false).toBool());
    hideMainWindowOnStartup = pref->value("hide_main_window_on_startup",
                                          hideMainWindowOnStartup).toBool();
    pref->endGroup();

    updateShowAllAct();
}

void TMainWindowTray::onSystemTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    WZDEBUG(QString::number(reason));

    updateShowAllAct();

    if (reason == QSystemTrayIcon::Trigger) {
        toggleShowAll();
    } else if (reason == QSystemTrayIcon::MiddleClick) {
        player->playOrPause();
    }
}

void TMainWindowTray::toggleShowAll() {

    // Ignore if tray is not visible
    if (tray->isVisible()) {
        showAll(!isVisible());
    }
}

void TMainWindowTray::showMainWindow() {

    if (!isVisible()) {
        showAll(true);
    }
}

void TMainWindowTray::showAll(bool b) {

    setVisible(b);
    updateShowAllAct();
}

void TMainWindowTray::onMediaInfoChanged() {

    TMainWindow::onMediaInfoChanged();
    tray->setToolTip(windowTitle());
}

} // namespace Gui

#include "moc_mainwindowtray.cpp"

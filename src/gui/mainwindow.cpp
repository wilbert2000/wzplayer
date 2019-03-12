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

#include "gui/mainwindow.h"
#include "gui/playerwindow.h"
#include "gui/filedialog.h"
#include "gui/logwindow.h"
#include "gui/helpwindow.h"
#include "gui/autohidetimer.h"
#include "gui/filepropertiesdialog.h"
#include "gui/dockwidget.h"
#include "gui/msg.h"

#include "gui/playlist/playlist.h"

#include "gui/about.h"
#include "gui/timedialog.h"
#include "gui/videoequalizer.h"
#include "gui/audioequalizer.h"
#include "gui/stereo3ddialog.h"
#include "gui/updatechecker.h"
#include "gui/eqslider.h"
#include "gui/inputurl.h"

#include "gui/pref/dialog.h"
#include "gui/pref/interface.h"
#include "gui/pref/input.h"

#include "gui/action/actiongroup.h"
#include "gui/action/widgetactions.h"
#include "gui/action/editabletoolbar.h"
#include "gui/action/menu/menufile.h"
#include "gui/action/menu/menuplay.h"
#include "gui/action/menu/menuwindowsize.h"
#include "gui/action/menu/menuvideo.h"
#include "gui/action/menu/menuaudio.h"
#include "gui/action/menu/menusubtitle.h"
#include "gui/action/menu/menubrowse.h"
#include "gui/action/menu/menuview.h"
#include "gui/action/menu/menuhelp.h"

#include "player/player.h"
#include "player/process/exitmsg.h"

#include "settings/paths.h"

#include "app.h"
#include "images.h"
#include "extensions.h"
#include "colorutils.h"
#include "wztime.h"
#include "clhelp.h"
#include "version.h"
#include "desktop.h"

#include <QMessageBox>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QNetworkProxy>
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#include <windows.h>
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {

TMainWindow::TMainWindow() :
    QMainWindow(),
    debug(logger()),
    update_checker(0),
    arg_close_on_finish(-1),
    ignore_show_hide_events(false),
    save_size(true),
    center_window(false),
    file_properties_dialog(0),
    prefDialog(0),
    help_window(0),
    menubar_visible(true),
    statusbar_visible(true),
    fullscreen_menubar_visible(false),
    fullscreen_statusbar_visible(true) {

    setObjectName("mainwindow");
    setWindowTitle(TConfig::PROGRAM_NAME);
    setWindowIcon(Images::icon("logo", 64));
    setAttribute(Qt::WA_DeleteOnClose);
    setAcceptDrops(true);
    setAnimated(false); // Disable animation of docks

    createStatusBar();

    // Reset size factor
    if (pref->save_window_size_on_exit) {
        force_resize = false;
    } else {
        force_resize = true;
        pref->size_factor = 1.0;
    }

    // Resize window to default size
    resize(pref->default_size);

    createPanel();
    createLogDock();
    createPlayerWindow();
    createPlayer();
    createPlaylist();

    createVideoEqualizer();
    createAudioEqualizer();

    createActions();
    createToolbars();
    createMenus();

    setupNetworkProxy();
    changeStayOnTop(pref->stay_on_top);

    update_checker = new TUpdateChecker(this, &pref->update_checker_data);
}

TMainWindow::~TMainWindow() {

    msgSlot = 0;
    setMessageHandler(0);
}

void TMainWindow::createPanel() {

    panel = new QWidget(this);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    panel->setMinimumSize(QSize(1, 1));
    panel->setFocusPolicy(Qt::StrongFocus);
    setCentralWidget(panel);
}

void TMainWindow::createLogDock() {

    logDock = new TDockWidget(tr("Log"), this, "logdock");
    logWindow = new TLogWindow(logDock);
    logDock->setWidget(logWindow);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
}

void TMainWindow::createStatusBar() {

    setMessageHandler(statusBar());
    msgSlot = new TMsgSlot(this);

    QColor bgc(0, 0, 0);
    QColor fgc(255, 255, 255);
    int margin = 3;
    QMargins margins(margin, 0, margin, 0);

    statusBar()->setSizeGripEnabled(false);
    TColorUtils::setBackgroundColor(statusBar(), bgc);
    TColorUtils::setForegroundColor(statusBar(), fgc);
    statusBar()->setContentsMargins(1, 1, 1, 1);

    video_info_label = new QLabel(statusBar());
    video_info_label->setObjectName("video_info_label");
    TColorUtils::setBackgroundColor(video_info_label, bgc);
    TColorUtils::setForegroundColor(video_info_label, fgc);
    video_info_label->setFrameShape(QFrame::NoFrame);
    video_info_label->setContentsMargins(margins);
    statusBar()->addWidget(video_info_label);

    in_out_points_label = new QLabel(statusBar());
    in_out_points_label->setObjectName("in_out_points_label");
    TColorUtils::setBackgroundColor(in_out_points_label, bgc);
    TColorUtils::setForegroundColor(in_out_points_label, fgc);
    in_out_points_label->setFrameShape(QFrame::NoFrame);
    in_out_points_label->setContentsMargins(margins);
    statusBar()->addPermanentWidget(in_out_points_label, 0);

    time_label = new QLabel(statusBar());
    time_label->setObjectName("time_label");
    TColorUtils::setBackgroundColor(time_label, bgc);
    TColorUtils::setForegroundColor(time_label, fgc);
    time_label->setFrameShape(QFrame::NoFrame);
    time_label->setContentsMargins(margins);
    time_label->setFont(QFont("Monospace"));
    time_label->setText("00:00/00:00");
    statusBar()->addPermanentWidget(time_label, 0);
}

void TMainWindow::createPlayerWindow() {

    playerWindow = new TPlayerWindow(panel);
    playerWindow->setObjectName("playerwindow");

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(playerWindow);
    panel->setLayout(layout);

    // Connect player window mouse events
    connect(playerWindow, &Gui::TPlayerWindow::leftClicked,
            this, &TMainWindow::leftClickFunction);
    connect(playerWindow, &Gui::TPlayerWindow::rightClicked,
            this, &TMainWindow::rightClickFunction);
    connect(playerWindow, &Gui::TPlayerWindow::doubleClicked,
            this, &TMainWindow::doubleClickFunction);
    connect(playerWindow, &Gui::TPlayerWindow::middleClicked,
            this, &TMainWindow::middleClickFunction);
    connect(playerWindow, &Gui::TPlayerWindow::xbutton1Clicked,
            this, &TMainWindow::xbutton1ClickFunction);
    connect(playerWindow, &Gui::TPlayerWindow::xbutton2Clicked,
            this, &TMainWindow::xbutton2ClickFunction);

    connect(playerWindow, &Gui::TPlayerWindow::videoOutChanged,
            this, &TMainWindow::displayVideoInfo, Qt::QueuedConnection);
}

void TMainWindow::createPlayer() {

    new Player::TPlayer(this, playerWindow);

    connect(player, &Player::TPlayer::positionChanged,
            this, &TMainWindow::onPositionChanged);
    connect(player, &Player::TPlayer::durationChanged,
            this, &TMainWindow::onDurationChanged);

    connect(player, &Player::TPlayer::stateChanged,
            this, &TMainWindow::onStateChanged);
    connect(player, &Player::TPlayer::stateChanged,
            this, &TMainWindow::checkStayOnTop,
            Qt::QueuedConnection);

    connect(player, &Player::TPlayer::mediaSettingsChanged,
            this, &TMainWindow::onMediaSettingsChanged);
    connect(player, &Player::TPlayer::videoOutResolutionChanged,
            this, &TMainWindow::onVideoOutResolutionChanged);

    connect(player, &Player::TPlayer::newMediaStartedPlaying,
            this, &TMainWindow::onNewMediaStartedPlaying,
            Qt::QueuedConnection);

    connect(player, &Player::TPlayer::mediaInfoChanged,
            this, &TMainWindow::onMediaInfoChanged);

    connect(player, &Player::TPlayer::playerError,
            this, &TMainWindow::onPlayerError,
            Qt::QueuedConnection);

    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TMainWindow::displayInOutPoints);
    connect(player, &Player::TPlayer::mediaSettingsChanged,
            this, &TMainWindow::displayInOutPoints);
}

void TMainWindow::createPlaylist() {

    playlistDock = new TDockWidget(tr("Playlist"), this, "playlistdock");
    playlist = new Playlist::TPlaylist(playlistDock, this);
    playlistDock->setWidget(playlist);
    addDockWidget(Qt::LeftDockWidgetArea, playlistDock);

    connect(playlist, &Playlist::TPlaylist::playlistFinished,
            this, &TMainWindow::onPlaylistFinished);
    connect(playlist, &Playlist::TPlaylist::playlistTitleChanged,
            this, &TMainWindow::onPlaylistTitleChanged);
}

void TMainWindow::createVideoEqualizer() {

    video_equalizer = new TVideoEqualizer(this);
    video_equalizer->setBySoftware(pref->use_soft_video_eq);

    connect(video_equalizer, &TVideoEqualizer::contrastChanged,
            player, &Player::TPlayer::setContrast);
    connect(video_equalizer, &TVideoEqualizer::brightnessChanged,
            player, &Player::TPlayer::setBrightness);
    connect(video_equalizer, &TVideoEqualizer::hueChanged,
            player, &Player::TPlayer::setHue);
    connect(video_equalizer, &TVideoEqualizer::saturationChanged,
            player, &Player::TPlayer::setSaturation);
    connect(video_equalizer, &TVideoEqualizer::gammaChanged,
            player, &Player::TPlayer::setGamma);

    connect(video_equalizer, &TVideoEqualizer::requestToChangeDefaultValues,
            this, &TMainWindow::setDefaultValuesFromVideoEqualizer);
    connect(video_equalizer, &TVideoEqualizer::bySoftwareChanged,
            this, &TMainWindow::changeVideoEqualizerBySoftware);

    connect(player, &Player::TPlayer::videoEqualizerNeedsUpdate,
            this, &TMainWindow::updateVideoEqualizer);
}

void TMainWindow::createAudioEqualizer() {

    audio_equalizer = new TAudioEqualizer(this);

    connect(audio_equalizer->eq[0], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq0);
    connect(audio_equalizer->eq[1], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq1);
    connect(audio_equalizer->eq[2], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq2);
    connect(audio_equalizer->eq[3], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq3);
    connect(audio_equalizer->eq[4], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq4);
    connect(audio_equalizer->eq[5], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq5);
    connect(audio_equalizer->eq[6], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq6);
    connect(audio_equalizer->eq[7], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq7);
    connect(audio_equalizer->eq[8], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq8);
    connect(audio_equalizer->eq[9], &TEqSlider::valueChanged,
            player, &Player::TPlayer::setAudioEq9);

    connect(audio_equalizer, &TAudioEqualizer::applyClicked,
            player, &Player::TPlayer::setAudioAudioEqualizerRestart);
    connect(audio_equalizer, &TAudioEqualizer::valuesChanged,
            player, &Player::TPlayer::setAudioEqualizer);
}

void TMainWindow::createActions() {

    using namespace Action;

    // Create auto hide timer
    autoHideTimer = new TAutoHideTimer(this, playerWindow);


    // TODO: Favorites

    // Clear recents
    clearRecentsAct = new TAction(this, "recents_clear", tr("Clear recents"),
                                  "delete");
    connect(clearRecentsAct, &TAction::triggered,
            this, &TMainWindow::clearRecentsListDialog);

    // Open URL
    TAction* a = new TAction(this, "open_url", tr("Open URL..."), "",
                             QKeySequence("Ctrl+U"));
    connect(a, &TAction::triggered, this, &TMainWindow::openURL);

    // Open file
    a  = new TAction(this, "open_file", tr("Open file..."), "open",
                     Qt::CTRL | Qt::Key_F);
    connect(a, &TAction::triggered,
            playlist, &Playlist::TPlaylist::openFileDialog);

    // Open dir
    a = new TAction(this, "open_directory", tr("Open directory..."), "",
                    QKeySequence("Ctrl+D"));
    connect(a, &TAction::triggered,
            playlist, &Playlist::TPlaylist::openDirectoryDialog);

    // Open disc menu
    // DVD
    a = new TAction(this, "open_dvd_disc", tr("Open DVD disc"), "dvd");
    connect(a, &TAction::triggered, this, &TMainWindow::openDVD);

    a = new TAction(this, "open_dvd_iso", tr("Open DVD ISO file..."), "dvd_iso");
    connect(a, &TAction::triggered, this, &TMainWindow::openDVDFromISO);

    a = new TAction(this, "open_dvd_folder", tr("Open DVD folder..."), "dvd_hd");
    connect(a, &TAction::triggered, this, &TMainWindow::openDVDFromFolder);

    // BluRay
    a = new TAction(this, "open_bluray_disc", tr("Open Blu-ray disc"), "bluray");
    connect(a, &TAction::triggered, this, &TMainWindow::openBluRay);

    a = new TAction(this, "open_bluray_iso", tr("Open Blu-ray ISO file..."),
                    "bluray_iso");
    connect(a, &TAction::triggered, this, &TMainWindow::openBluRayFromISO);

    a = new TAction(this, "open_bluray_folder", tr("Open Blu-ray folder..."),
                    "bluray_hd");
    connect(a, &TAction::triggered, this, &TMainWindow::openBluRayFromFolder);

    // VCD
    a = new TAction(this, "open_vcd", tr("Open video CD"), "vcd");
    connect(a, &TAction::triggered, this, &TMainWindow::openVCD);

    // Audio
    a = new TAction(this, "open_audio_cd", tr("Open audio CD"), "cdda");
    connect(a, &TAction::triggered, this, &TMainWindow::openAudioCD);

    // Save thumbnail
#ifdef Q_OS_LINUX
    saveThumbnailAct  = new TAction(this, "save_thumbnail",
                                    tr("Save thumbnail"), "",
                                    Qt::CTRL | Qt::Key_I);
    connect(saveThumbnailAct, &TAction::triggered,
            this, &TMainWindow::saveThumbnail);
#endif

    // Close
    // Memo: Quit added by TMainwindowTray
    a = new TAction(this, "close", tr("Close"), "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    connect(a, &TAction::triggered, this, &TMainWindow::closeWindow);

    // Play menu
    // Seek forward
    seekFrameAct = new TAction(this, "seek_forward_frame", tr("Frame step"), "",
                               Qt::ALT | Qt::Key_Right);
    seekFrameAct->setData(0);
    connect(seekFrameAct, &TAction::triggered,
            player, &Player::TPlayer::frameStep);

    seek1Act = new TAction(this, "seek_forward1", "", "", Qt::Key_Right);
    seek1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
    seek1Act->setData(1);
    connect(seek1Act, &TAction::triggered, player, &Player::TPlayer::forward1);

    seek2Act = new TAction(this, "seek_forward2", "", "",
                           Qt::SHIFT | Qt::Key_Right);
    seek2Act->setData(2);
    connect(seek2Act, &TAction::triggered, player, &Player::TPlayer::forward2);

    seek3Act = new TAction(this, "seek_forward3", "", "",
                           Qt::CTRL | Qt::Key_Right);
    seek3Act->setData(3);
    connect(seek3Act, &TAction::triggered, player, &Player::TPlayer::forward3);


    // Seek backward
    seekBackFrameAct = new TAction(this, "seek_rewind_frame",
                                   tr("Frame back step"), "",
                                   Qt::ALT | Qt::Key_Left);
    seekBackFrameAct->setData(0);
    connect(seekBackFrameAct, &TAction::triggered, player,
            &Player::TPlayer::frameBackStep);

    seekBack1Act = new TAction(this, "seek_rewind1", "", "", Qt::Key_Left);
    seekBack1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
    seekBack1Act->setData(1);
    connect(seekBack1Act, &TAction::triggered,
            player, &Player::TPlayer::rewind1);

    seekBack2Act = new TAction(this, "seek_rewind2", "", "",
                               Qt::SHIFT | Qt::Key_Left);
    seekBack2Act->setData(2);
    connect(seekBack2Act, &TAction::triggered,
            player, &Player::TPlayer::rewind2);

    seekBack3Act = new TAction(this, "seek_rewind3", "", "",
                               Qt::CTRL | Qt::Key_Left);
    seekBack3Act->setData(3);
    connect(seekBack3Act, &TAction::triggered,
            player, &Player::TPlayer::rewind3);

    setSeekTexts();

    // Seek to time
    seekToTimeAct = new TAction(this, "seek_to_time", tr("Seek to time..."), "",
                                QKeySequence("Ctrl+G"));
    connect(seekToTimeAct, &TAction::triggered,
            this, &TMainWindow::showSeekToDialog);

    // Speed menu
    playSpeedGroup = new QActionGroup(this);
    playSpeedGroup->setExclusive(false);
    playSpeedGroup->setEnabled(false);

    a = new TAction(this, "speed_normal", tr("Normal speed"), "", Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::normalSpeed);

    a = new TAction(this, "spedd_half", tr("Half speed"), "",
                    Qt::META | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::halveSpeed);

    a = new TAction(this, "speed_double", tr("Double speed"), "",
                    Qt::ALT | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::doubleSpeed);

    a = new TAction(this, "speed_dec_10", tr("Speed -10%"), "",
                    Qt::SHIFT | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed10);

    a = new TAction(this, "speed_inc_10", tr("Speed +10%"), "",
                    Qt::CTRL | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed10);

    a = new TAction(this, "speed_dec_4", tr("Speed -4%"), "",
                    Qt::SHIFT | Qt::CTRL | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed4);

    a = new TAction(this, "speed_inc_4", tr("Speed +4%"), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed4);

    a = new TAction(this, "speed_dec_1", tr("Speed -1%"), "",
                    Qt::SHIFT | Qt::META | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed1);

    a = new TAction(this, "speed_inc_1", tr("Speed +1%"), "",
                    Qt::CTRL | Qt::META | Qt::Key_Z);
    playSpeedGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed1);

    // Menu in-out
    // Put in group to enable/disable together, if we disable the menu users
    // cannot discover the menu because it won't open.
    inOutGroup = new QActionGroup(this);
    inOutGroup->setExclusive(false);
    inOutGroup->setEnabled(false);

    a  = new TAction(this, "set_in", tr("Set in"), "", QKeySequence("["));
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setInPoint);

    a = new TAction(this, "set_out", tr("Set out and repeat"), "",
                    QKeySequence("]"));
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setOutPoint);

    a  = new TAction(this, "clear_in", tr("Clear in"), "",
                     QKeySequence("Shift+["));
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInPoint);

    a = new TAction(this, "clear_out", tr("Clear out and repeat"), "",
                    QKeySequence("Shift+]"));
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearOutPoint);

    a = new TAction(this, "clear_in_out", tr("Clear in-out and repeat"), "",
                    Qt::Key_Backspace);
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInOutPoints);

    a  = new TAction(this, "seek_in", tr("Seek to in"), "noicon", Qt::Key_Home);
    a->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekInPoint);

    a = new TAction(this, "seek_out", tr("Seek to out"), "", Qt::Key_End);
    inOutGroup->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekOutPoint);

    repeatInOutAct = new TAction(this, "repeat_in_out", tr("Repeat in-out"),
                                 "repeat", Qt::Key_Backslash);
    repeatInOutAct->setCheckable(true);
    inOutGroup->addAction(repeatInOutAct);
    connect(repeatInOutAct, &TAction::triggered,
            player, &Player::TPlayer::setRepeat);
    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TMainWindow::updateInOutMenu);


    // Menu video
    // Fullscreen
    fullscreenAct = new TAction(this, "fullscreen", tr("Fullscreen"), "",
                                Qt::Key_F);
    fullscreenAct->setCheckable(true);
    connect(fullscreenAct, &TAction::triggered,
            this, &TMainWindow::toggleFullscreen);
    // Exit fullscreen (not in menu)
    a = new TAction(this, "exit_fullscreen", tr("Exit fullscreen"), "",
                    Qt::Key_Escape);
    connect(a, &TAction::triggered, this, &TMainWindow::exitFullscreen);

    // Aspect menu
    aspectGroup = new TActionGroup(this, "aspectgroup");
    aspectGroup->setEnabled(false);
    aspectAutoAct = new TActionGroupItem(this, aspectGroup, "aspect_detect",
                                         tr("Auto"), TAspectRatio::AspectAuto);

    new TActionGroupItem(this, aspectGroup, "aspect_1_1",
        TAspectRatio::aspectIDToString(0), TAspectRatio::Aspect11);
    new TActionGroupItem(this, aspectGroup, "aspect_5_4",
        TAspectRatio::aspectIDToString(1), TAspectRatio::Aspect54);
    new TActionGroupItem(this, aspectGroup, "aspect_4_3",
        TAspectRatio::aspectIDToString(2), TAspectRatio::Aspect43);
    new TActionGroupItem(this, aspectGroup, "aspect_11_8",
        TAspectRatio::aspectIDToString(3), TAspectRatio::Aspect118);
    new TActionGroupItem(this, aspectGroup, "aspect_14_10",
        TAspectRatio::aspectIDToString(4), TAspectRatio::Aspect1410);
    new TActionGroupItem(this, aspectGroup, "aspect_3_2",
        TAspectRatio::aspectIDToString(5), TAspectRatio::Aspect32);
    new TActionGroupItem(this, aspectGroup, "aspect_14_9",
        TAspectRatio::aspectIDToString(6), TAspectRatio::Aspect149);
    new TActionGroupItem(this, aspectGroup, "aspect_16_10",
        TAspectRatio::aspectIDToString(7), TAspectRatio::Aspect1610);
    new TActionGroupItem(this, aspectGroup, "aspect_16_9",
        TAspectRatio::aspectIDToString(8), TAspectRatio::Aspect169);
    new TActionGroupItem(this, aspectGroup, "aspect_2_1",
        TAspectRatio::aspectIDToString(9), TAspectRatio::Aspect2);
    new TActionGroupItem(this, aspectGroup, "aspect_2.35_1",
        TAspectRatio::aspectIDToString(10), TAspectRatio::Aspect235);

    aspectDisabledAct = new TActionGroupItem(this, aspectGroup, "aspect_none",
        tr("Disabled"), TAspectRatio::AspectNone);

    connect(aspectGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setAspectRatio);
    connect(player, &Player::TPlayer::aspectRatioChanged,
            this, &TMainWindow::updateAspectMenu,
            Qt::QueuedConnection);

    nextAspectAct = new TAction(this, "aspect_next", tr("Next aspect ratio"),
                                "", Qt::Key_A);
    connect(nextAspectAct, &TAction::triggered,
            player, &Player::TPlayer::nextAspectRatio);

    // Menu window size
    windowSizeGroup = new Menu::TWindowSizeGroup(this, playerWindow);
    connect(windowSizeGroup, &Menu::TWindowSizeGroup::activated,
            this, &TMainWindow::setSizePercentage);

    doubleSizeAct = new TAction(this, "size_toggle_double",
                                tr("Toggle double size"), "", Qt::Key_D);
    connect(doubleSizeAct, &TAction::triggered,
            this, &TMainWindow::toggleDoubleSize);

    optimizeSizeAct = new TAction(this, "size_optimize", "", "",
                                  QKeySequence("`"));
    connect(optimizeSizeAct, &TAction::triggered,
            this, &TMainWindow::optimizeSizeFactor);
    connect(playerWindow, &TPlayerWindow::videoSizeFactorChanged,
            this, &TMainWindow::updateWindowSizeMenu,
            Qt::QueuedConnection);

    resizeOnLoadAct = new TAction(this, "resize_on_load", tr("Resize on load"),
                                  "", Qt::ALT | Qt::Key_R);
    resizeOnLoadAct->setCheckable(true);
    resizeOnLoadAct->setChecked(pref->resize_on_load);
    connect(resizeOnLoadAct, &TAction::triggered,
            this, &TMainWindow::onResizeOnLoadTriggered);



    // View playlist
    QAction* viewPlaylistAct = playlistDock->toggleViewAction();
    viewPlaylistAct->setObjectName("view_playlist");
    viewPlaylistAct->setIcon(Images::icon("playlist"));
    viewPlaylistAct->setShortcut(Qt::Key_P);
    updateToolTip(viewPlaylistAct);
    addAction(viewPlaylistAct);
    autoHideTimer->add(viewPlaylistAct, playlistDock);


    // Time slider
    timeslider_action = new TTimeSliderAction(this);
    timeslider_action->setObjectName("timeslider_action");
    timeslider_action->setText(tr("Time slider"));

    connect(player, &Player::TPlayer::positionChanged,
            timeslider_action, &TTimeSliderAction::setPosition);
    connect(player, &Player::TPlayer::durationChanged,
            timeslider_action, &TTimeSliderAction::setDuration);

    connect(timeslider_action, &TTimeSliderAction::positionChanged,
            player, &Player::TPlayer::seekTime);
    connect(timeslider_action, &TTimeSliderAction::percentageChanged,
            player, &Player::TPlayer::seekPercentage);
    connect(timeslider_action, &TTimeSliderAction::dragPositionChanged,
            this, &TMainWindow::onDragPositionChanged);

    connect(timeslider_action, &TTimeSliderAction::wheelUp,
            player, &Player::TPlayer::wheelUpSeeking);
    connect(timeslider_action, &TTimeSliderAction::wheelDown,
            player, &Player::TPlayer::wheelDownSeeking);

    // Volume slider action
    volumeslider_action = new TVolumeSliderAction(this, player->getVolume());
    volumeslider_action->setObjectName("volumeslider_action");
    volumeslider_action->setText(tr("Volume slider"));

    connect(volumeslider_action, &TVolumeSliderAction::valueChanged,
            player, &Player::TPlayer::setVolume);
    connect(player, &Player::TPlayer::volumeChanged,
            volumeslider_action, &TVolumeSliderAction::setValue);

    // Menu bar
    viewMenuBarAct = new TAction(this, "toggle_menubar", tr("Menu bar"), "",
                                 Qt::SHIFT | Qt::Key_F2);
    viewMenuBarAct->setCheckable(true);
    viewMenuBarAct->setChecked(true);
    connect(viewMenuBarAct, &TAction::toggled,
            menuBar(), &QMenuBar::setVisible);

    // Toolbars
    editToolbarAct = new TAction(this, "edit_toolbar1",
                                 tr("Edit main toolbar..."));
    editToolbar2Act = new TAction(this, "edit_toolbar2",
                                  tr("Edit extra toolbar..."));
    // Control bar
    editControlBarAct = new TAction(this, "edit_controlbar",
                                    tr("Edit control bar..."));
    // Status bar
    viewStatusBarAct = new TAction(this, "toggle_statusbar", tr("Status bar"),
                                   "", Qt::SHIFT | Qt::Key_F7);
    viewStatusBarAct->setCheckable(true);
    viewStatusBarAct->setChecked(true);
    connect(viewStatusBarAct, &TAction::toggled,
            statusBar(), &QStatusBar::setVisible);

    viewVideoInfoAct = new TAction(this, "toggle_video_info", tr("Video info"));
    viewVideoInfoAct->setCheckable(true);
    viewVideoInfoAct->setChecked(true);
    connect(viewVideoInfoAct, &TAction::toggled,
            video_info_label, &QLabel::setVisible);

    viewInOutPointsAct = new TAction(this, "toggle_in_out_points",
                                     tr("In-out points"));
    viewInOutPointsAct->setCheckable(true);
    viewInOutPointsAct->setChecked(true);
    connect(viewInOutPointsAct, &TAction::toggled,
            in_out_points_label, &QLabel::setVisible);

    viewVideoTimeAct = new TAction(this, "toggle_video_time", tr("Video time"));
    viewVideoTimeAct->setCheckable(true);
    viewVideoTimeAct->setChecked(true);
    connect(viewVideoTimeAct, &TAction::toggled,
            time_label, &QLabel::setVisible);

    viewFramesAct = new TAction(this, "toggle_frames", tr("Frames"));
    viewFramesAct->setCheckable(true);
    viewFramesAct->setChecked(false);
    connect(viewFramesAct, &TAction::toggled,
            this, &TMainWindow::displayFrames);

    viewPlaylistAct = new Action::TAction(this, "next_wheel_function",
                            tr("Next wheel function"), 0, Qt::Key_W);
    connect(viewPlaylistAct, &Action::TAction::triggered,
            player, &Player::TPlayer::nextWheelFunction);
} // createActions

Action::Menu::TMenuExec* TMainWindow::createContextMenu() {

    Action::Menu::TMenuExec* menu = new Action::Menu::TMenuExec(this);
    menu->addMenu(fileMenu);
    menu->addMenu(playMenu);
    menu->addMenu(videoMenu);
    menu->addMenu(audioMenu);
    menu->addMenu(subtitleMenu);
    menu->addMenu(browseMenu);
    menu->addMenu(viewMenu);
    return menu;
}

void TMainWindow::createMenus() {

    using namespace Action;

    fileMenu = new Menu::TMenuFile(this, this);
    menuBar()->addMenu(fileMenu);
    playMenu = new Menu::TMenuPlay(this, this);
    menuBar()->addMenu(playMenu);
    videoMenu = new Menu::TMenuVideo(this, video_equalizer);
    menuBar()->addMenu(videoMenu);
    audioMenu = new Menu::TMenuAudio(this, audio_equalizer);
    menuBar()->addMenu(audioMenu);
    subtitleMenu = new Menu::TMenuSubtitle(this);
    menuBar()->addMenu(subtitleMenu);
    browseMenu = new Menu::TMenuBrowse(this);
    menuBar()->addMenu(browseMenu);

    // statusbar_menu added to toolbar_menu by createToolbarMenu()
    statusbar_menu = new QMenu(this);
    statusbar_menu->menuAction()->setObjectName("statusbar_menu");
    statusbar_menu->menuAction()->setText(tr("Statusbar"));
    statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));
    statusbar_menu->addAction(viewVideoInfoAct);
    statusbar_menu->addAction(viewInOutPointsAct);
    statusbar_menu->addAction(viewVideoTimeAct);
    statusbar_menu->addAction(viewFramesAct);

    // Create the only toolbar menu with a name
    Menu::TMenu* toolbarMenu = createToolbarMenu("toolbar_menu");
    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), &QStatusBar::customContextMenuRequested,
            toolbarMenu, &Menu::TMenu::execSlot);

    viewMenu = new Menu::TMenuView(this, toolbarMenu, logDock, autoHideTimer);
    menuBar()->addMenu(viewMenu);

    helpMenu = new Menu::TMenuHelp(this);
    menuBar()->addMenu(helpMenu);

    // Context menu
    contextMenu = createContextMenu();
    TAction* a = new TAction(this, "show_context_menu", tr("Show context menu"));
    connect(a, &TAction::triggered, contextMenu, &Menu::TMenuExec::execSlot);
    playerWindow->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playerWindow, &TPlayerWindow::customContextMenuRequested,
            this, &TMainWindow::showContextMenu);
} // createMenus()

Action::Menu::TMenu* TMainWindow::createToolbarMenu(const QString& name) {

    Action::Menu::TMenu* menu = new Action::Menu::TMenu(this, this, name,
        tr("Toolbars menu"), "toolbars");

    menu->addAction(viewMenuBarAct);
    menu->addAction(toolbar->toggleViewAction());
    menu->addAction(toolbar2->toggleViewAction());
    menu->addAction(controlbar->toggleViewAction());
    menu->addAction(viewStatusBarAct);

    menu->addSeparator();
    menu->addAction(editToolbarAct);
    menu->addAction(editToolbar2Act);
    menu->addAction(editControlBarAct);

    menu->addSeparator();
    menu->addMenu(statusbar_menu);

    connect(menu, &QMenu::aboutToShow,
            autoHideTimer, &TAutoHideTimer::disable);
    connect(menu, &QMenu::aboutToHide,
            autoHideTimer, &TAutoHideTimer::enable);

    return menu;
} // createToolbarMenu

// Called by main window to show context popup on toolbars and dock widgets.
// The main window takes ownership of the returned menu and will delete it
// after use, hence the need to create a new menu every time.
QMenu* TMainWindow::createPopupMenu() {
    return createToolbarMenu("");
}

void TMainWindow::createToolbars() {

    menuBar()->setObjectName("menubar");

    // Control bar
    controlbar = new Action::TEditableToolbar(this);
    controlbar->setObjectName("controlbar");
    controlbar->setWindowTitle("Control bar");
    controlbar->toggleViewAction()->setIcon(Images::icon("controlbar"));
    QStringList actions;
    actions << "play_or_pause"
            << "stop"
            << "seek_rewind_menu"
            << "seek_forward_menu"
            << "in_out_menu|0|1"
            << "timeslider_action"
            << "separator|0|1"
            << "deinterlace_menu|0|1"
            << "aspect_menu|1|1"
            << "window_size_menu|1|0"
            << "reset_zoom_pan|0|1"
            << "separator|0|1"
            << "volumeslider_action"
            << "separator|0|1"
            << "osd_menu|0|1"
            << "view_properties|0|1"
            << "view_playlist|0|1"
            << "separator|0|1"
            << "fullscreen";
    controlbar->setDefaultActions(actions);
    addToolBar(Qt::BottomToolBarArea, controlbar);
    connect(editControlBarAct, &Action::TAction::triggered,
            controlbar, &Action::TEditableToolbar::edit);

    QAction* action = controlbar->toggleViewAction();
    action->setObjectName("toggle_controlbar");
    action->setShortcut(Qt::SHIFT | Qt::Key_F6);
    addAction(action);

    // Main toolbar
    toolbar = new Action::TEditableToolbar(this);
    toolbar->setObjectName("toolbar1");
    toolbar->setWindowTitle(tr("Main toolbar"));
    action = toolbar->toggleViewAction();
    action->setObjectName("toggle_toolbar1");
    action->setIcon(Images::icon("main_toolbar"));
    action->setShortcut(Qt::SHIFT | Qt::Key_F3);
    addAction(action);

    actions.clear();
    actions << "open_url" << "favorites_menu";
    toolbar->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar);
    connect(editToolbarAct, &Action::TAction::triggered,
            toolbar, &Action::TEditableToolbar::edit);

    // Extra toolbar
    toolbar2 = new Action::TEditableToolbar(this);
    toolbar2->setObjectName("toolbar2");
    toolbar2->setWindowTitle(tr("Extra toolbar"));
    action = toolbar2->toggleViewAction();
    action->setObjectName("toggle_toolbar2");
    action->setIcon(Images::icon("extra_toolbar"));
    action->setShortcut(Qt::SHIFT | Qt::Key_F4);
    addAction(action);

    actions.clear();
    actions << "osd_menu" << "toolbar_menu" << "stay_on_top_menu"
            << "separator" << "view_properties" << "view_playlist"
            << "view_log" << "separator" << "view_settings";
    toolbar2->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar2);
    connect(editToolbar2Act, &Action::TAction::triggered,
            toolbar2, &Action::TEditableToolbar::edit);

    // Statusbar
    statusBar()->setObjectName("statusbar");

    // Add toolbars to auto_hide_timer
    // Docks already added by createActions()
    autoHideTimer->add(controlbar->toggleViewAction(), controlbar);
    autoHideTimer->add(toolbar->toggleViewAction(), toolbar);
    autoHideTimer->add(toolbar2->toggleViewAction(), toolbar2);
    autoHideTimer->add(viewMenuBarAct, menuBar());
    autoHideTimer->add(viewStatusBarAct, statusBar());
    connect(playerWindow, &TPlayerWindow::draggingChanged,
            autoHideTimer, &TAutoHideTimer::setDraggingPlayerWindow);
}

void TMainWindow::setupNetworkProxy() {

    QNetworkProxy proxy;

    if (pref->use_proxy && !pref->proxy_host.isEmpty()) {
        proxy.setType((QNetworkProxy::ProxyType) pref->proxy_type);
        proxy.setHostName(pref->proxy_host);
        proxy.setPort(pref->proxy_port);
        if (!pref->proxy_username.isEmpty()
            && !pref->proxy_password.isEmpty()) {
            proxy.setUser(pref->proxy_username);
            proxy.setPassword(pref->proxy_password);
        }
        WZINFO("using proxy " + pref->proxy_host
               + ":" + QString::number(pref->proxy_port)
               + " type " + QString::number(pref->proxy_type));
    } else {
        // No proxy
        proxy.setType(QNetworkProxy::NoProxy);
    }

    QNetworkProxy::setApplicationProxy(proxy);
}

void TMainWindow::createSettingsDialog() {

    QApplication::setOverrideCursor(Qt::WaitCursor);

    prefDialog = new Pref::TDialog(this);
    prefDialog->setModal(false);
    connect(prefDialog, &Pref::TDialog::applied,
            this, &TMainWindow::applyNewSettings);

    QApplication::restoreOverrideCursor();
}

void TMainWindow::showSettingsDialog() {

    if (!prefDialog) {
        createSettingsDialog();
    }

    prefDialog->setData(pref, allActions);
    prefDialog->show();
}

void TMainWindow::restartApplication() {
    WZDEBUG("");

    emit requestRestart();

    // When fullscreen the window size will not yet be updated by the time it is
    // saved by saveSettings, so block saving it.
    save_size = !pref->fullscreen;

    // Close and restart with the new settings
    if (close()) {
        WZDEBUG("Closed main window, calling qApp->exit()");
        qApp->exit(TApp::START_APP);
    } else {
        WZWARN("Close canceled");
    }
    return;
}

// The user has pressed OK in settings dialog
void TMainWindow::applyNewSettings() {
    WZTRACE("");

    // Get pref from dialog
    prefDialog->getData(pref, allActions);

    // Save playlist settings
    playlist->saveSettings();

    // Commit changes to disk
    pref->save();

    // Restart TApp
    if (prefDialog->requiresRestartApp()) {
        restartApplication();
        return;
    }

    // Set color key, depends on VO
    playerWindow->setColorKey();

    // Forced demuxer
    player->mset.forced_demuxer = pref->use_lavf_demuxer ? "lavf" : "";

    // Video equalizer
    video_equalizer->setBySoftware(pref->use_soft_video_eq);

    // Interface
    // Show panel
    if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
        resize(width(), height() + 200);
        panel->show();
    }
    // Hide toolbars delay
    autoHideTimer->setInterval(pref->floating_hide_delay);

    // Keyboard and mouse
    playerWindow->setDelayLeftClick(pref->delay_left_click);
    setSeekTexts();

    // Network
    setupNetworkProxy();

    // Update log window edit control
    logWindow->edit->setMaximumBlockCount(pref->log_window_max_events);

    emit settingsChanged();

    // Enable actions to reflect changes
    setEnableActions();

    // Restart video if needed
    if (prefDialog->requiresRestartPlayer()) {
        player->restart();
    }
} // TMainWindow::applyNewSettings()

void TMainWindow::createFilePropertiesDialog() {
    WZDEBUG("");

    QApplication::setOverrideCursor(Qt::WaitCursor);

    file_properties_dialog = new TFilePropertiesDialog(this, &player->mdat);
    file_properties_dialog->setModal(false);
    connect(file_properties_dialog, &TFilePropertiesDialog::applied,
            this, &TMainWindow::applyFileProperties);
    connect(player, &Player::TPlayer::videoBitRateChanged,
            file_properties_dialog, &TFilePropertiesDialog::showInfo);
    connect(player, &Player::TPlayer::audioBitRateChanged,
            file_properties_dialog, &TFilePropertiesDialog::showInfo);
    Action::TAction* action = findChild<Action::TAction*>("view_properties");
    if (action) {
        connect(file_properties_dialog, &TFilePropertiesDialog::visibilityChanged,
                action, &Action::TAction::setChecked);
    }

    QApplication::restoreOverrideCursor();
}

void TMainWindow::setDataToFileProperties() {
    WZDEBUG("");

    Player::Info::TPlayerInfo *i = Player::Info::TPlayerInfo::obj();
    i->getInfo();
    file_properties_dialog->setCodecs(i->vcList(), i->acList(),
                                      i->demuxerList());

    // Save a copy of the demuxer, video and audio codec
    if (player->mset.original_demuxer.isEmpty())
        player->mset.original_demuxer = player->mdat.demuxer;
    if (player->mset.original_video_codec.isEmpty())
        player->mset.original_video_codec = player->mdat.video_codec;
    if (player->mset.original_audio_codec.isEmpty())
        player->mset.original_audio_codec = player->mdat.audio_codec;

    // Set demuxer, video and audio codec
    QString demuxer = player->mset.forced_demuxer;
    if (demuxer.isEmpty())
        demuxer = player->mdat.demuxer;
    QString vc = player->mset.forced_video_codec;
    if (vc.isEmpty())
        vc = player->mdat.video_codec;
    QString ac = player->mset.forced_audio_codec;
    if (ac.isEmpty())
        ac = player->mdat.audio_codec;

    file_properties_dialog->setDemuxer(demuxer, player->mset.original_demuxer);
    file_properties_dialog->setVideoCodec(vc,
                                          player->mset.original_video_codec);
    file_properties_dialog->setAudioCodec(ac,
                                          player->mset.original_audio_codec);

    file_properties_dialog->setPlayerAdditionalArguments(
                player->mset.player_additional_options);
    file_properties_dialog->setPlayerAdditionalVideoFilters(
                player->mset.player_additional_video_filters);
    file_properties_dialog->setPlayerAdditionalAudioFilters(
                player->mset.player_additional_audio_filters);

    file_properties_dialog->showInfo();
}

void TMainWindow::applyFileProperties() {
    WZDEBUG("");

    bool need_restart = false;
    bool demuxer_changed = false;

    // Demuxer
    QString prev_demuxer = player->mset.forced_demuxer;
    QString s = file_properties_dialog->demuxer();
    if (s == player->mset.original_demuxer) {
        s = "";
    }
    if (s != player->mset.forced_demuxer) {
        player->mset.forced_demuxer = s;
        need_restart = true;
    }
    if (prev_demuxer != player->mset.forced_demuxer) {
        // Demuxer changed
        demuxer_changed = true;
        player->mset.current_audio_id = TMediaSettings::NoneSelected;
        player->mset.current_sub_idx = TMediaSettings::NoneSelected;
    }

    // Video codec
    s = file_properties_dialog->videoCodec();
    if (s == player->mset.original_video_codec) {
        s = "";
    }
    if (s != player->mset.forced_video_codec) {
        player->mset.forced_video_codec = s;
        need_restart = true;
    }

    // Audio codec
    s = file_properties_dialog->audioCodec();
    if (s == player->mset.original_audio_codec) {
        s = "";
    }
    if (s != player->mset.forced_audio_codec) {
        player->mset.forced_audio_codec = s;
        need_restart = true;
    }

    // Additional options
    s = file_properties_dialog->playerAdditionalArguments();
    if (s != player->mset.player_additional_options) {
        player->mset.player_additional_options = s;
        need_restart = true;
    }

    // Additional video filters
    s = file_properties_dialog->playerAdditionalVideoFilters();
    if (s != player->mset.player_additional_video_filters) {
        player->mset.player_additional_video_filters = s;
        need_restart = true;
    }

    // Additional audio filters
    s = file_properties_dialog->playerAdditionalAudioFilters();
    if (s != player->mset.player_additional_audio_filters) {
        player->mset.player_additional_audio_filters = s;
        need_restart = true;
    }

    // Restart the video to apply
    if (need_restart) {
        if (demuxer_changed) {
            player->reload();
        } else {
            player->restart();
        }
    }
}

void TMainWindow::showFilePropertiesDialog(bool checked) {
    WZDEBUG("");

    if (checked) {
        if (!file_properties_dialog) {
            createFilePropertiesDialog();
        }
        setDataToFileProperties();
        file_properties_dialog->show();
    } else {
        file_properties_dialog->hide();
    }
}

void TMainWindow::setWindowCaption(const QString& title) {
    setWindowTitle(title);
}

QString parentOrMenuName(QAction* action) {

    QString name = action->parent()->objectName();
    if (name.isEmpty()) {
        QMenu* menu = qobject_cast<QMenu*>(action->parent());
        if (menu) {
            return "from menu '" + menu->menuAction()->objectName() + "'";
        }
    }
    return "with parent '" + name + "'";
}

QList<QAction*> TMainWindow::findNamedActions() const {

    QList<QAction*> allActions = findChildren<QAction*>();
    QList<QAction*> selectedActions;

    for (int i = 0; i < allActions.count(); i++) {
        QAction* action = allActions.at(i);
        if (action->isSeparator()) {
            WZTRACE(QString("Skipping separator %1")
                    .arg(parentOrMenuName(action)));
        } else if (action->objectName().isEmpty()) {
            WZTRACE(QString("Skipping action '' '%1' %2")
                    .arg(action->text()).arg(parentOrMenuName(action)));
        } else if (action->objectName() == "_q_qlineeditclearaction") {
            WZTRACE("Skipping action _q_qlineeditclearaction");
        } else {
            selectedActions.append(action);
            WZTRACE(QString("Selected action %1 ('%2') %3")
                        .arg(action->objectName())
                        .arg(action->text())
                        .arg(parentOrMenuName(action)));

        }
    }
    WZDEBUG(QString("Selected %1 actions out of %2 found actions")
            .arg(selectedActions.count()).arg(allActions.count()));

    return selectedActions;
}

QAction* TMainWindow::findAction(const QString &name) {

    QAction* action = findChild<QAction*>(name);
    if (action) {
        return action;
    }

    WZERROR(QString("Action '%1' not found").arg(name));
    Q_ASSERT(false);
    return action;
}

void TMainWindow::loadSettings() {
    WZDEBUG("");

    // Disable actions
    setEnableActions();

    // Get all actions with a name
    allActions = findNamedActions();
    // Load modified actions from settings
    Action::TActionsEditor::loadSettings(pref, allActions);

    pref->beginGroup(settingsGroupName());

    // Position, size and windowstate
    if (pref->save_window_size_on_exit) {
        QPoint p = pref->value("pos", pos()).toPoint();
        QSize s = pref->value("size", size()).toSize();
        int state = pref->value("state", 0).toInt();
        if (s.width() < 200 || s.height() < 200) {
            s = pref->default_size;
        }

        move(p);
        resize(s);
        if (state != Qt::WindowMinimized) {
            setWindowState((Qt::WindowStates) state);
        }

        if (p.isNull()) {
            TDesktop::centerWindow(this);
        } else {
            TDesktop::keepInsideDesktop(this);
        }
    } else {
        TDesktop::centerWindow(this);
        // Need to center again after video loaded
        center_window = true;
        center_window_pos = pos();
    }

    pref->beginGroup("actions");
    toolbar->setActionsFromStringList(pref->value("toolbar1",
        toolbar->getDefaultActions()).toStringList(), allActions);
    toolbar2->setActionsFromStringList(pref->value("toolbar2",
        toolbar2->getDefaultActions()).toStringList(), allActions);
    controlbar->setActionsFromStringList(pref->value("controlbar",
        controlbar->getDefaultActions()).toStringList(), allActions);
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    toolbar->setIconSize(pref->value("toolbar1",
        toolbar->iconSize()).toSize());
    toolbar2->setIconSize(pref->value("toolbar2",
        toolbar2->iconSize()).toSize());
    controlbar->setIconSize(pref->value("controlbar",
        controlbar->iconSize()).toSize());
    pref->endGroup();

    menubar_visible = pref->value("menubar_visible", menubar_visible).toBool();
    viewMenuBarAct->setChecked(menubar_visible);
    fullscreen_menubar_visible = pref->value("fullscreen_menubar_visible",
        fullscreen_menubar_visible).toBool();

    statusbar_visible = pref->value("statusbar_visible", statusbar_visible)
                        .toBool();
    viewStatusBarAct->setChecked(statusbar_visible);
    fullscreen_statusbar_visible = pref->value("fullscreen_statusbar_visible",
        fullscreen_statusbar_visible).toBool();


    if (!restoreState(pref->value("toolbars_state").toByteArray(),
                      TVersion::qtVersion())) {
        playlistDock->hide();
        logDock->hide();
    }

    pref->beginGroup("statusbar");
    viewVideoInfoAct->setChecked(pref->value("video_info", true).toBool());
    viewInOutPointsAct->setChecked(pref->value("in_out_points", true).toBool());
    viewVideoTimeAct->setChecked(pref->value("video_time", true).toBool());
    viewFramesAct->setChecked(pref->show_frames);
    pref->endGroup();

    pref->endGroup();

    playlist->loadSettings();
}

void TMainWindow::saveSettings() {
    WZTRACE("");

    pref->beginGroup(settingsGroupName());

    if (pref->save_window_size_on_exit && save_size) {
        pref->setValue("pos", pos());
        pref->setValue("size", size());
        pref->setValue("state", (int) windowState());
    }

    // Toolbars
    pref->beginGroup("actions");
    pref->setValue("toolbar1", toolbar->actionsToStringList());
    pref->setValue("toolbar2", toolbar2->actionsToStringList());
    pref->setValue("controlbar", controlbar->actionsToStringList());
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    pref->setValue("toolbar1", toolbar->iconSize());
    pref->setValue("toolbar2", toolbar2->iconSize());
    pref->setValue("controlbar", controlbar->iconSize());
    pref->endGroup();

    pref->setValue("menubar_visible", !menuBar()->isHidden());
    pref->setValue("fullscreen_menubar_visible", fullscreen_menubar_visible);
    pref->setValue("statusbar_visible", !statusBar()->isHidden());
    pref->setValue("fullscreen_statusbar_visible",
                   fullscreen_statusbar_visible);

    pref->setValue("toolbars_state", saveState(TVersion::qtVersion()));

    pref->beginGroup("statusbar");
    pref->setValue("video_info", viewVideoInfoAct->isChecked());
    pref->setValue("in_out_points", viewInOutPointsAct->isChecked());
    pref->setValue("video_time", viewVideoTimeAct->isChecked());
    pref->endGroup();

    pref->endGroup();

    playlist->saveSettings();
    if (help_window) {
        help_window->saveSettings(pref);
    }
    if (file_properties_dialog) {
        file_properties_dialog->saveSettings();
    }
    if (prefDialog) {
        prefDialog->saveSettings();
    }
}

void TMainWindow::save() {

    msg(tr("Saving settings"), 0);
    if (pref->clean_config) {
        pref->clean_config = false;
        pref->remove("");
        Action::TActionsEditor::saveSettings(pref, allActions);
    }
    saveSettings();
    pref->save();
}

void TMainWindow::displayVideoInfo() {

    if (player->mdat.noVideo()) {
        video_info_label->setText("");
    } else {
        QSize video_out_size = playerWindow->lastVideoOutSize();
        video_info_label->setText(tr("%1x%2", "video source width x height")
            .arg(player->mdat.video_width)
            .arg(player->mdat.video_height)
            + " " + QString::fromUtf8("\u279F") + " "
            + tr("%1x%2 %3 fps", "video out width x height and fps")
            .arg(video_out_size.width())
            .arg(video_out_size.height())
            .arg(player->mdat.video_fps));
    }
}

void TMainWindow::displayInOutPoints() {

    QString s;
    int secs = qRound(player->mset.in_point);
    if (secs > 0)
        s = tr("I: %1", "In point in statusbar").arg(TWZTime::formatTime(secs));

    secs = qRound(player->mset.out_point);
    if (secs > 0) {
        if (!s.isEmpty()) s += " ";
        s += tr("O: %1", "Out point in statusbar").arg(TWZTime::formatTime(secs));
    }

    if (player->mset.loop) {
        if (!s.isEmpty()) s += " ";
        s += tr("R", "Symbol for repeat in-out in statusbar");
    }

    in_out_points_label->setText(s);
}

void TMainWindow::displayFrames(bool b) {

    pref->show_frames = b;
    onDurationChanged(player->mdat.duration);
}


void TMainWindow::setFloatingToolbarsVisible(bool visible) {

    if (toolbar->isFloating()) {
        toolbar->setVisible(visible);
    }
    if (toolbar2->isFloating()) {
        toolbar2->setVisible(visible);
    }
    if (controlbar->isFloating()) {
        controlbar->setVisible(visible);
    }
}

void TMainWindow::showEvent(QShowEvent* event) {

    QMainWindow::showEvent(event);
    if (pref->pause_when_hidden
        && player->state() == Player::STATE_PAUSED
        && !ignore_show_hide_events) {
        WZDEBUG("unpausing player");
        player->play();
    }

    setFloatingToolbarsVisible(true);
}

void TMainWindow::hideEvent(QHideEvent* event) {

    QMainWindow::hideEvent(event);
    if (pref->pause_when_hidden
        && player->state() == Player::STATE_PLAYING
        && !ignore_show_hide_events) {
        WZDEBUG("pausing player");
        player->pause();
    }

    setFloatingToolbarsVisible(false);
}

void TMainWindow::showContextMenu() {

    // Using this event to make the context menu popup on right mouse button
    // down event, instead of waiting for the mouse button release event,
    // which would trigger the show_context_menu action if it is assigned to the
    // mouse_right_click_function.
    if (pref->mouse_right_click_function == "show_context_menu") {
        contextMenu->execSlot();
    }
}

void TMainWindow::setDefaultValuesFromVideoEqualizer() {
    WZDEBUG("");

    pref->initial_contrast = video_equalizer->contrast();
    pref->initial_brightness = video_equalizer->brightness();
    pref->initial_hue = video_equalizer->hue();
    pref->initial_saturation = video_equalizer->saturation();
    pref->initial_gamma = video_equalizer->gamma();

    QMessageBox::information(this, tr("Information"),
                             tr("The current values have been stored to be "
                                "used as default."));
}

void TMainWindow::changeVideoEqualizerBySoftware(bool b) {
    WZDEBUG(QString::number(b));

    if (b != pref->use_soft_video_eq) {
        pref->use_soft_video_eq = b;
        player->restart();
    }
}

void TMainWindow::updateVideoEqualizer() {

    video_equalizer->setContrast(player->mset.contrast);
    video_equalizer->setBrightness(player->mset.brightness);
    video_equalizer->setHue(player->mset.hue);
    video_equalizer->setSaturation(player->mset.saturation);
    video_equalizer->setGamma(player->mset.gamma);
}

void TMainWindow::updateAudioEqualizer() {
    audio_equalizer->setEqualizer(player->getAudioEqualizer());
}

void TMainWindow::updateInOutMenu() {
    repeatInOutAct->setChecked(player->mset.loop);
}

void TMainWindow::updateAspectMenu() {

    aspectGroup->setChecked(player->mset.aspect_ratio.ID());

    double aspect = player->mset.aspectToDouble();
    QString s = TAspectRatio::doubleToString(aspect);
    emit setAspectToolTip(tr("Aspect ratio %1").arg(s));

    s = tr("Auto") + "\t"
        + TAspectRatio::doubleToString(player->mdat.video_aspect_original);
    aspectAutoAct->setTextAndTip(s);

    s = tr("Disabled") + "\t" + TAspectRatio::doubleToString(
            (double) player->mdat.video_width / player->mdat.video_height);
    aspectDisabledAct->setTextAndTip(s);
}

void TMainWindow::onResizeOnLoadTriggered(bool b) {
    pref->resize_on_load = b;
}

void TMainWindow::updateWindowSizeMenu() {

    windowSizeGroup->updateWindowSizeGroup();
    doubleSizeAct->setEnabled(windowSizeGroup->isEnabled());
    optimizeSizeAct->setEnabled(windowSizeGroup->isEnabled());
    resizeOnLoadAct->setChecked(pref->resize_on_load);

    // Update text and tips
    QString txt = tr("Optimize (current size %1%)")
            .arg(windowSizeGroup->size_percentage);
    optimizeSizeAct->setTextAndTip(txt);

    emit setWindowSizeToolTip(tr("Size %1%")
                              .arg(windowSizeGroup->size_percentage));
}

// Slot called when media settings reset or loaded
void TMainWindow::onMediaSettingsChanged() {
    WZDEBUG("");

    updateInOutMenu();
    updateAspectMenu();
    // Window size menu not changed

    emit mediaSettingsChanged(&player->mset);

    updateVideoEqualizer();
    updateAudioEqualizer();

    displayInOutPoints();
}

void TMainWindow::onMediaInfoChanged() {
    WZDEBUG("");

    setWindowCaption(player->mdat.displayName() + " - "
                     + TConfig::PROGRAM_NAME);
    displayVideoInfo();
    if (file_properties_dialog && file_properties_dialog->isVisible()) {
        setDataToFileProperties();
    }
}

void TMainWindow::onNewMediaStartedPlaying() {
    WZDEBUG("");

    enterFullscreenOnPlay();

    // Recents
    pref->addRecent(player->mdat.filename, player->mdat.displayName());
    checkPendingActionsToRun();
}

void TMainWindow::onPlaylistFinished() {
    WZDEBUG("");

    // Handle "Close on end of playlist" option
    if (arg_close_on_finish != 0) {
        if ((arg_close_on_finish == 1) || (pref->close_on_finish)) {
            close();
        }
    }
}

void TMainWindow::onPlayerError(int exit_code) {
    WZERROR(QString::number(exit_code));

    QString s = Player::Process::TExitMsg::message(exit_code)
                + " (" + player->mdat.filename + ")";
    msg(s, 0);

    static bool busy = false;
    if (pref->report_player_crashes
        && player->state() != Player::STATE_STOPPING
        && !busy) {
        busy = true;
        QMessageBox::warning(this,
            tr("%1 process error").arg(pref->playerName()),
            s + " \n"
            + tr("See log for details."),
            QMessageBox::Ok);
        busy = false;
    }
}

void TMainWindow::onStateChanged(Player::TState state) {
    WZTRACE("New state " + player->stateToString());

    setEnableActions();
    autoHideTimer->setAutoHideMouse(state == Player::STATE_PLAYING);
    switch (state) {
        case Player::STATE_STOPPED:
            setWindowCaption(TConfig::PROGRAM_NAME);
            msg(tr("Ready"));
            break;
        case Player::STATE_PLAYING:
            msg(tr("Playing %1").arg(player->mdat.displayName()));
            break;
        case Player::STATE_PAUSED:
            msg(tr("Paused"));
            break;
        case Player::STATE_STOPPING:
            msg(tr("Stopping..."));
            break;
        case Player::STATE_RESTARTING:
            msg(tr("Restarting..."));
            break;
        case Player::STATE_LOADING:
            msg(tr("Loading..."));
            break;
    }
}

// Return seek string to use in menu
QString TMainWindow::timeForJumps(int secs, const QString& seekSign) const {

    int minutes = (int) secs / 60;
    int seconds = secs % 60;
    QString m = tr("%1 minute(s)").arg(minutes);
    QString s = tr("%1 second(s)").arg(seconds);

    QString txt;
    if (minutes == 0) {
        txt = s;
    } else if (seconds == 0) {
        txt = m;
    } else {
        txt = tr("%1 and %2", "combine minutes (%1) and secs (%2)")
              .arg(m).arg(s);
    }
    return tr("%1%2", "add + or - sign (%1) to seek text (%2)")
            .arg(seekSign).arg(txt);
}

void TMainWindow::setSeekTexts() {

    QString seekSign = tr("+", "sign to use in menu for forward seeking");
    seek1Act->setTextAndTip(timeForJumps(pref->seeking1, seekSign));
    seek2Act->setTextAndTip(timeForJumps(pref->seeking2, seekSign));
    seek3Act->setTextAndTip(timeForJumps(pref->seeking3, seekSign));

    seekSign = tr("-", "sign to use in menu for rewind seeking");
    seekBack1Act->setTextAndTip(timeForJumps(pref->seeking1, seekSign));
    seekBack2Act->setTextAndTip(timeForJumps(pref->seeking2, seekSign));
    seekBack3Act->setTextAndTip(timeForJumps(pref->seeking3, seekSign));
}

Action::TAction* TMainWindow::seekIntToAction(int i) const {

    switch (i) {
        case 0: return seekFrameAct;
        case 1: return seek1Act;
        case 2: return seek2Act;
        case 3: return seek3Act;
        case 4: return playlist->playNextAct;
        case 5: return seekBackFrameAct;
        case 6: return seekBack1Act;
        case 7: return seekBack2Act;
        case 8: return seekBack3Act;
        case 9: return playlist->playPrevAct;
        default:
            WZERROR(QString("Undefined seek action %1. Returning seek2Act")
                    .arg(i));
            return seek2Act;
    }
}

void TMainWindow::updateSeekDefaultAction(QAction* action) {

    int seekInt = action->data().toInt();
    if (pref->seeking_current_action != seekInt) {
        pref->seeking_current_action = seekInt;
        emit seekForwardDefaultActionChanged(seekIntToAction(seekInt));
        emit seekRewindDefaultActionChanged(seekIntToAction(seekInt + 5));
    }
}

void TMainWindow::setTimeLabel(double sec, bool changed) {

    static int lastSec = -1111;

    // TODO: <-1..0> looses sign
    int s = sec;

    if (s != lastSec) {
        lastSec = s;
        positionText = TWZTime::formatTime(s);
        changed = true;
    }

    QString frames;
    if (pref->show_frames) {
        double fps = player->mdat.video_fps;
        if (fps > 0) {
            sec -= s;
            if (sec < 0) {
                sec = -sec;
            }
            sec *= fps;
            // TODO: fix floats. Example 0.84 * 25 = 20 if floored instead of 21
            sec += 0.0001;
            s = sec;
            if (s < 10) {
                frames = ".0" + QString::number(s);
            } else {
                frames = "."  + QString::number(s);
            }
            frames += player->mdat.fuzzy_time;
            changed = true;
        }
    }

    if (changed) {
        time_label->setText(positionText + frames + durationText);
    }
}

void TMainWindow::onPositionChanged(double sec) {
    setTimeLabel(sec, false);
}

void TMainWindow::onDurationChanged(double duration) {

    durationText = "/" + TWZTime::formatTime(qRound(duration));

    if (pref->show_frames) {
        double fps = player->mdat.video_fps;
        if (fps > 0) {
            duration -= (int) duration;
            QString frames = QString::number(qRound(duration * fps));
            if (frames.length() < 2) {
                frames = "0" + frames;
            }
            durationText += "." + frames;
        }
    }

    setTimeLabel(player->mset.current_sec, true);
}

void TMainWindow::onDragPositionChanged(double t) {

    QString s = tr("Jump to %1").arg(TWZTime::formatTime(qRound(t)));
    msg(s, 1000);

    if (pref->fullscreen) {
        player->displayTextOnOSD(s);
    }
}

void TMainWindow::onPlaylistTitleChanged(QString title) {
    WZTRACE(title);

    playlistDock->setWindowTitle(title);
}

void TMainWindow::handleMessageFromOtherInstances(const QString& message) {
    WZDEBUG("msg + '" + message + "'");

    int pos = message.indexOf(' ');
    if (pos >= 0) {
        emit gotMessageFromOtherInstance();
        QString command = message.left(pos);
        QString arg = message.mid(pos + 1);
        if (command == "open_file") {
            playlist->open(arg);
        } else if (command == "open_files") {
            QStringList file_list = arg.split(" <<sep>> ");
            playlist->openFiles(file_list);
        } else if (command == "add_to_playlist") {
            QStringList file_list = arg.split(" <<sep>> ");
            playlist->add(file_list);
        } else if (command == "media_title") {
            QStringList list = arg.split(" <<sep>> ");
            player->addForcedTitle(list[0], list[1]);
        } else if (command == "action") {
            processAction(arg);
        } else if (command == "load_sub") {
            player->setInitialSubtitle(arg);
            if (player->statePOP()) {
                player->loadSub(arg);
            }
        } else {
            WZWARN(QString("Received unknown command '%1'").arg(message));
        }
    } else {
        WZWARN(QString("Received message '%1' without arguments").arg(message));
    }
}

void TMainWindow::closeEvent(QCloseEvent* e)  {
    WZDEBUG("");

    if (playlist->maybeSave()) {
        playlist->abortThread();
        player->close(Player::STATE_STOPPING);
        exitFullscreen();
        save();
        e->accept();
    } else {
        e->ignore();
    }
}

void TMainWindow::closeWindow() {
    WZDEBUG("");
    close();
}

void TMainWindow::setEnableActions() {
    WZTRACE("State " + player->stateToString());

    // Clear recent action is always enabled

    // Save thumbnail action
    saveThumbnailAct->setEnabled(
                player->mdat.selected_type == TMediaData::TYPE_FILE
                && !player->mdat.filename.isEmpty()
                && player->hasVideo());

    // Seek forward
    bool enable = player->statePOP();
    seekFrameAct->setEnabled(enable);
    seek1Act->setEnabled(enable);
    seek2Act->setEnabled(enable);
    seek3Act->setEnabled(enable);
    // Seek backwards
    seekBackFrameAct->setEnabled(enable);
    seekBack1Act->setEnabled(enable);
    seekBack2Act->setEnabled(enable);
    seekBack3Act->setEnabled(enable);

    // Seek to time
    seekToTimeAct->setEnabled(enable);

    // Play speed
    playSpeedGroup->setEnabled(enable);

    // In-out menu
    inOutGroup->setEnabled(enable);

    // Aspect menu
    bool e = enable && player->hasVideo();
    aspectGroup->setEnabled(e);
    nextAspectAct->setEnabled(e);
    updateAspectMenu();

    // Window size menu
    windowSizeGroup->setEnabled(enable);
    doubleSizeAct->setEnabled(enable);
    optimizeSizeAct->setEnabled(enable);
    // Resize on load always enabled

    // Time slider
    timeslider_action->enable(enable);

    emit enableActions();
}

void TMainWindow::clearRecentsListDialog() {

    int ret = QMessageBox::question(this, tr("Confirm deletion - %1")
                                    .arg(TConfig::PROGRAM_NAME),
                                    tr("Delete the list of recent files?"),
                                    QMessageBox::Cancel, QMessageBox::Ok);

    if (ret == QMessageBox::Ok) {
        pref->clearRecents();
    }
}

void TMainWindow::openRecent() {

    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        int i = action->data().toInt();
        playlist->open(pref->history_recents.getURL(i));
    }
}

void TMainWindow::openURL() {

    TInputURL dialog(this);

    // Get url from clipboard
    if (TApp::acceptClipboardAsURL()) {
        QString txt = QApplication::clipboard()->text();
        dialog.setURL(txt);
        Settings::pref->last_clipboard = txt;
    }

    for (int n = 0; n < pref->history_urls.count(); n++) {
        dialog.setURL(pref->history_urls[n]);
    }

    if (dialog.exec() == QDialog::Accepted) {
        QString url = dialog.url();
        if (!url.isEmpty()) {
            pref->history_urls.add(url);
            playlist->open(url);
        }
    }
}

void TMainWindow::configureDiscDevices() {
    QMessageBox::information(this, TConfig::PROGRAM_NAME + tr(" - Information"),
            tr("The CDROM / DVD drives are not configured yet.\n"
               "The settings dialog will be shown now, so you can do it."),
            QMessageBox::Ok);

    showSettingsDialog();
    prefDialog->showSection(Pref::TDialog::SECTION_DRIVES);
}

void TMainWindow::openVCD() {
    WZDEBUG("");

    if (pref->cdrom_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->openDisc(TDiscName("vcd", pref->vcd_initial_title,
                                   pref->cdrom_device));
    }
}

void TMainWindow::openAudioCD() {
    WZDEBUG("");

    if (pref->cdrom_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->open("cdda://");
    }
}

void TMainWindow::openDVD() {
    WZDEBUG("");

    if (pref->dvd_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->openDisc(TDiscName(pref->dvd_device, pref->useDVDNAV()));
    }
}

void TMainWindow::openDVDFromISO() {
    WZDEBUG("");

    if (playlist->maybeSave()) {
        QString iso = TFileDialog::getOpenFileName(
            this,
            tr("Select the ISO DVD file to open"),
            pref->last_iso,
            tr("ISO files") + + " (*.iso);;" + tr("All files") + " (*.*)");

        if (!iso.isEmpty()) {
            pref->last_iso = iso;
            player->openDisc(TDiscName(iso, pref->useDVDNAV()));
        }
    }
}

void TMainWindow::openDVDFromFolder() {
    WZDEBUG("");

    if (playlist->maybeSave()) {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Select the folder containing the DVD"),
            pref->last_dvd_directory,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            pref->last_dvd_directory = dir;
            player->openDisc(TDiscName(dir, pref->useDVDNAV()));
        }
    }
}

void TMainWindow::openBluRay() {
    WZDEBUG("");

    if (pref->bluray_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->openDisc(TDiscName("br", 0, pref->bluray_device));
    }
}

void TMainWindow::openBluRayFromISO() {
    WZDEBUG("");

    if (playlist->maybeSave()) {
        QString iso = TFileDialog::getOpenFileName(
            this,
            tr("Select the Blu-Ray ISO file to open"),
            pref->last_iso,
            tr("ISO files") + + " (*.iso);;" + tr("All files") + " (*.*)");

        if (!iso.isEmpty()) {
            pref->last_iso = iso;
            player->openDisc(TDiscName("br", 0, iso));
        }
    }
}

void TMainWindow::openBluRayFromFolder() {
    WZDEBUG("");

    if (playlist->maybeSave()) {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Select the folder containing the Blu-ray"),
            pref->last_dvd_directory,
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            pref->last_dvd_directory = dir;
            player->openDisc(TDiscName("br", 0, dir));
        }
    }
}

void TMainWindow::removeThumbnail(QString fn) {

    fn = "file://" + fn;
    QString thumb = QCryptographicHash::hash(fn.toUtf8(),
                        QCryptographicHash::Md5).toHex() + ".png";

    QString cacheDir = TPaths::genericCachePath() + "/thumbnails";
    QDir dir(cacheDir + "/large");
    if (dir.remove(thumb)) {
        WZINFO("Removed large thumb '" + thumb + "'");
    }
    dir.setPath(cacheDir + "/normal");
    if (dir.remove(thumb)) {
        WZINFO("Removed normal thumb '" + thumb + "'");
    }
}

void TMainWindow::saveThumbnailToIni(const QString& fn, const QString& time) {

    QString section;
    if (fn.startsWith('/')) {
        section = "files" + fn;
    } else {
        section = "files/" + fn;
    }

    QSettings settings("WH", "ffmpegthumbs");
    settings.beginGroup(section);
    settings.setValue("time", time);
    settings.endGroup();
    settings.sync();

    QString m = "Saved thumbnail time " + time;
    msg2(m);
    WZINFO(m + " to " + section);
}

void TMainWindow::saveThumbnail() {

    QString fn = player->mdat.filename;
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        QString canonical = fi.canonicalFilePath();
        if (canonical.isEmpty()) {
            WZWARN("Canonical path for '" + fn + "' not found");
        } else {
            QString time =TWZTime::formatTimeMS(
                qRound(player->mset.current_sec * 1000));
            saveThumbnailToIni(canonical, time);

            // Remove cached thumbnails
            removeThumbnail(fn);
            removeThumbnail(canonical);
        }
    }
}

void TMainWindow::loadSub() {
    WZDEBUG("");

    QString s = TFileDialog::getOpenFileName(
        this,
        tr("Select a subtitle file"),
        pref->last_dir,
        tr("Subtitles") + extensions.subtitles().forFilter()
           + ";;" + tr("All files") +" (*.*)");

    if (!s.isEmpty())
        player->loadSub(s);
}

void TMainWindow::loadAudioFile() {
    WZDEBUG("");

    QString s = TFileDialog::getOpenFileName(
        this, tr("Select an audio file"),
        pref->last_dir,
        tr("Audio") + extensions.audio().forFilter()+";;" +
        tr("All files") +" (*.*)");

    if (!s.isEmpty())
        player->loadAudioFile(s);
}

void TMainWindow::helpCLOptions() {

    if (help_window == 0) {
        help_window = new THelpWindow(this, "helpwindow");
        help_window->setWindowTitle(tr("%1 command line options")
                                    .arg(TConfig::PROGRAM_NAME));
        help_window->loadSettings(pref);
    }

    // Hide event clears the help window content, so recreate it
    help_window->setHtml(CLHelp::help(true));
    help_window->show();
}

void TMainWindow::helpCheckUpdates() {
    update_checker->check();
}

void TMainWindow::showConfigFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(TPaths::configPath()));
}

void TMainWindow::helpAbout() {

    TAbout d(this);
    d.exec();
}

void TMainWindow::showSeekToDialog() {

    TTimeDialog d(this);
    d.setWindowTitle(tr("Seek"));
    d.setMaximumTime((int) player->mdat.duration);
    d.setTime((int) player->mset.current_sec);
    if (d.exec() == QDialog::Accepted) {
        player->seekTime(d.time());
    }
}

void TMainWindow::showAudioDelayDialog() {

    bool ok;
    int delay = QInputDialog::getInt(this,
        tr("%1 - Audio delay").arg(TConfig::PROGRAM_NAME),
        tr("Audio delay (in milliseconds):"), player->mset.audio_delay,
        -3600000, 3600000, 1, &ok);
    if (ok) {
        player->setAudioDelay(delay);
    }
}

void TMainWindow::showSubDelayDialog() {

    bool ok;
    int delay = QInputDialog::getInt(this,
        tr("%1 - Subtitle delay").arg(TConfig::PROGRAM_NAME),
        tr("Subtitle delay (in milliseconds):"), player->mset.sub_delay,
        -3600000, 3600000, 1, &ok);
    if (ok) {
        player->setSubDelay(delay);
    }
}

void TMainWindow::showStereo3dDialog() {

    TStereo3dDialog d(this);
    d.setInputFormat(player->mset.stereo3d_in);
    d.setOutputFormat(player->mset.stereo3d_out);

    if (d.exec() == QDialog::Accepted) {
        player->setStereo3D(d.inputFormat(), d.outputFormat());
    }
}

void TMainWindow::exitFullscreen() {
    setFullscreen(false);
}

void TMainWindow::setFullscreen(bool b) {

    if (b == pref->fullscreen) {
        return;
    }

    pref->fullscreen = b;
    fullscreenAct->setChecked(b);
    emit fullscreenChanged();

    if (pref->fullscreen) {
        aboutToEnterFullscreen();
        showFullScreen();
        didEnterFullscreen();
    } else {
        aboutToExitFullscreen();
        showNormal();
        didExitFullscreen();
    }

    setFocus(); // Fixes bug #2493415
}

void TMainWindow::toggleFullscreen() {
    setFullscreen(!pref->fullscreen);
}

void TMainWindow::aboutToEnterFullscreen() {

    emit aboutToEnterFullscreenSignal();

    // Save current state
    menubar_visible = !menuBar()->isHidden();
    statusbar_visible = !statusBar()->isHidden();
    first_fullscreen_filename = player->mdat.filename;

    pref->beginGroup(settingsGroupName());
    pref->setValue("toolbars_state", saveState(TVersion::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didEnterFullscreen() {

    // Restore fullscreen state
    viewMenuBarAct->setChecked(fullscreen_menubar_visible);
    viewStatusBarAct->setChecked(fullscreen_statusbar_visible);

    pref->beginGroup(settingsGroupName());
    if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
                      TVersion::qtVersion())) {
        // First time there is no fullscreen toolbar state
        WZDEBUG("fullscreen toolbar state not restored");
        toolbar->hide();
        toolbar2->hide();
    }
    pref->endGroup();

    emit didEnterFullscreenSignal();

    autoHideTimer->start();
}

void TMainWindow::aboutToExitFullscreen() {

    autoHideTimer->stop();

    // Save fullscreen state
    fullscreen_menubar_visible = !menuBar()->isHidden();
    fullscreen_statusbar_visible = !statusBar()->isHidden();

    pref->beginGroup(settingsGroupName());
    pref->setValue("toolbars_state_fullscreen", saveState(TVersion::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didExitFullscreen() {

    viewMenuBarAct->setChecked(menubar_visible);
    viewStatusBarAct->setChecked(statusbar_visible);

    pref->beginGroup(settingsGroupName());
    if (!restoreState(pref->value("toolbars_state").toByteArray(),
                      TVersion::qtVersion())) {
        logger()->warn("didExitFullscreen: failed to restore toolbar state");
    }
    pref->endGroup();

    // Update size when current video changed in fullscreen
    if (pref->resize_on_load
        && player->mdat.filename != first_fullscreen_filename
        && player->statePOP()) {
        // Set default zoom
        pref->size_factor = pref->initial_zoom_factor;
        // Needs delay for framesize to settle down...
        QTimer::singleShot(100, this, SLOT(optimizeSizeFactor()));
    }

    emit didExitFullscreenSignal();
}

// Called by onNewMediaStartedPlaying() when a video starts playing
void TMainWindow::enterFullscreenOnPlay() {

    if (TApp::start_in_fullscreen != TApp::FS_FALSE) {
        if (pref->start_in_fullscreen || TApp::start_in_fullscreen > 0) {
            setFullscreen(true);
            // Clear TApp::start_in_fullscreen
            if (TApp::start_in_fullscreen == TApp::FS_RESTART) {
                TApp::start_in_fullscreen = TApp::FS_NOT_SET;
            }
        }
    }
}

void TMainWindow::dragEnterEvent(QDragEnterEvent *e) {
    WZDEBUG("");

    if (e->mimeData()->hasUrls()) {
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
            return;
        }
        if (e->possibleActions() & Qt::CopyAction) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
    }
    QMainWindow::dragEnterEvent(e);
}

void TMainWindow::dropEvent(QDropEvent *e) {
    WZDEBUG("");

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl& url, e->mimeData()->urls()) {
            files.append(url.toString());
        }
        playlist->openFiles(files);
        e->accept();
        return;
    }
    QMainWindow::dropEvent(e);
}

void TMainWindow::setSizeFactor(double factor) {
    WZDEBUG(QString::number(factor));

    if (player->mdat.noVideo()) {
        return;
    }

    if (pref->fullscreen) {
        // Adjust zoom to match the requested size factor
        QSize video_size = QSize(player->mdat.video_out_width,
                                 player->mdat.video_out_height) * factor;
        QSize desktop_size = TDesktop::size(this);
        double zoom = (double) video_size.width() / desktop_size.width();
        double zoomY = (double) video_size.height() / desktop_size.height();
        if (zoomY > zoom) {
            zoom = zoomY;
        }
        player->setZoom(zoom);
    } else {
        // Normal screen
        pref->size_factor = factor;
        bool center = false;
        if (isMaximized()) {
            showNormal();
            // Need center, on X windows the original pos is not restored
            center = true;
        }
        resizeStickyWindow(player->mdat.video_out_width,
                           player->mdat.video_out_height);
        if (center) {
            TDesktop::centerWindow(this);
        }
    }

    msgOSD(tr("Size %1%").arg(QString::number(qRound(factor * 100))));
}

void TMainWindow::setSizePercentage(int percentage) {
    WZDEBUG(QString::number(percentage) + "%");

    if (percentage == 33) {
        setSizeFactor((double) 1 / 3);
    } else {
        setSizeFactor((double) percentage / 100);
    }
}

void TMainWindow::toggleDoubleSize() {

    if (pref->size_factor != 1.0) {
        setSizeFactor(1.0);
    } else {
        setSizeFactor(2.0);
    }
}

void TMainWindow::hidePanel() {
    WZDEBUG("");

    if (panel->isVisible()) {
        // Exit from fullscreen
        if (pref->fullscreen) {
            setFullscreen(false);
            update();
        }

        resize(width(), height() - panel->height());
        panel->hide();
    }
}

double TMainWindow::optimizeSize(double size) const {
    WZDEBUG("size in " + QString::number(size));

    QSize res = playerWindow->resolution();
    if (res.width() <= 0 || res.height() <= 0) {
        return size;
    }
    QSize available_size = TDesktop::availableSize(this);

    // Handle fullscreen
    if (pref->fullscreen) {
        size = (double) available_size.width() / res.width();
        double size_y = (double) available_size.height() / res.height();
        if (size_y < size) {
            size = size_y;
        }
        return size;
    }

    // Return current size for VO size change caused by TPlayer::setAspectRatio
    if (player->keepSize) {
        player->clearKeepSize();
        return pref->size_factor;
    }

    QSize video_size = res * size;

    // Limit size to 0.6 of available desktop
    const double f = 0.6;
    // Adjust width
    double max = f * available_size.width();
    if (video_size.width() > max) {
        size = max / res.width();
        video_size = res * size;
    }
    // Adjust height
    max = f * available_size.height();
    if (video_size.height() > max) {
        size = max / res.height();
        video_size = res * size;
    }

    // Get 1/4 of available desktop height
    double min = available_size.height() / 4;
    if (video_size.height() < min) {
        if (size == 1.0) {
            return 2.0;
        }
        size = min / res.height();
    }

    // Round to predefined values
    int i = qRound(size * 100);
    if (i <= 0) {
        WZWARN("optimizeSize: selecting size 1 for invalid size");
        return 1;
    }
    if (i < 13) {
        return size;
    }
    if (i < 25) {
        // Can we scale up to 25%?
        video_size = res * (double) 0.25;
        if (video_size.width() > available_size.width()
                || video_size.height() > available_size.height()) {
            return size;
        }
        return 0.25;
    }
    if (i < 29) {
        return 0.25;
    }
    if (i < 42) {
        return (double) 1 / 3;
    }
    if (i < 63) {
        return 0.5;
    }
    if (i < 88) {
        return 0.75;
    }
    if (i < 113) {
        return 1;
    }
    if (i < 138) {
        return 1.25;
    }
    if (i < 168) {
        return 1.5;
    }
    if (i < 188) {
        return 1.75;
    }
    if (i < 225) {
        return 2;
    }
    if (i < 275) {
        return 2.5;
    }
    if (i < 325) {
        return 3;
    }
    if (i < 375) {
        return 3.5;
    }
    if (i < 450) {
        return 4;
    }
    return size;
}

void TMainWindow::optimizeSizeFactor() {

    if (pref->fullscreen) {
        player->setZoom(1.0);
    } else {
        setSizeFactor(optimizeSize(pref->size_factor));
    }
}

double TMainWindow::getDefaultSize() const {
    return optimizeSize(pref->initial_zoom_factor);
}

void TMainWindow::onVideoOutResolutionChanged(int w, int h) {
    WZTRACE(QString::number(w) + " x " + QString::number(h));

    if (w <= 0 || h <= 0) {
        // No video
        if (pref->hide_video_window_on_audio_files) {
            hidePanel();
        }
    } else {
        // Have video
        if (!panel->isVisible()) {
            panel->show();
        }

        // force_resize is only set for the first video when
        // pref->save_window_size_on_exit is not set.
        if (panel->width() < 64 || panel->height() < 48) {
            force_resize = true;
        }
        // Leave maximized window as is.
        if (!isMaximized() && (pref->resize_on_load || force_resize)) {
            // Get new size factor
            pref->size_factor = getDefaultSize();
            resizeStickyWindow(w, h);
        } else {
            // Adjust the size factor to the current window size
            playerWindow->updateSizeFactor();
            WZDEBUG("adjusted size factor to "
                    + QString::number(pref->size_factor));
        }
    }

    // Center window only set for the first video
    // when pref->save_window_size_on_exit not set.
    if (center_window) {
        center_window = false;
        // Only center when user did not move window
        if (center_window_pos == pos()) {
            TDesktop::centerWindow(this);
        }
    }

    force_resize = false;
}

void TMainWindow::resizeStickyWindow(int w, int h) {

    if (pref->fullscreen || isMaximized()) {
        return;
    }

    QRect avail = QApplication::desktop()->availableGeometry(this);

    // Prevent resize of window snapped by KDE, the resize will fail...
    int fh = frameGeometry().size().height();
    if (fh == avail.height() || fh == (avail.height() / 2)) {
        if (pos().y() == avail.y()
                || pos().y() + fh == avail.y() + avail.height()) {
            if (pos().x() == avail.x()
                    || pos().x() + frameGeometry().size().width()
                    == avail.x() + avail.width()) {
                WZDEBUG("Skipping resize of snapped window");
                return;
            }
        }
    }

    bool stickx = pos().x() - avail.x() + frameGeometry().size().width()
                  >= avail.width();
    bool sticky = pos().y() - avail.y() + frameGeometry().size().height()
                  >= avail.height();

    resizeMainWindow(w, h, pref->size_factor);
    TDesktop::keepInsideDesktop(this);

    QPoint p = pos();
    if (stickx) {
        int x = avail.x() + avail.width()
                - frameGeometry().size().width();
        if (x < avail.x() || x == p.x()) {
            stickx = false;
        } else {
            p.rx() = x;
        }
    }
    if (sticky) {
        int y = avail.y() + avail.height()
                - frameGeometry().size().height();
        if (y < avail.y() || y == p.y()) {
            sticky = false;
        } else {
            p.ry() = y;
        }
    }
    if (stickx || sticky) {
        move(p);
    }
}

void TMainWindow::resizeMainWindow(int w,
                                   int h,
                                   double size_factor,
                                   bool try_twice) {
    WZDEBUG("video size " + QString::number(w) + " x " + QString::number(h)
            + ", window size " + QString::number(pref->size_factor));

    QSize panel_size = QSize(w, h) * size_factor;
    if (panel_size == panel->size()) {
        WZTRACE("panel has requested size");
        return;
    }

    QSize new_size = size() + panel_size - panel->size();
    resize(new_size);

    if (panel->size() != panel_size) {
        // Resizing the main window can change the height of the toolbars,
        // which will change the height of the panel during the resize.
        // Fix by resizing once again, using the new panel height.
        if (try_twice) {
            resizeMainWindow(w, h, size_factor, false);
        } else {
            WZDEBUG(QString("resizeMainWindow: resize failed. Panel"
                            " size now %1 x %2. Wanted size %3 x %4")
                            .arg(panel->size().width())
                            .arg(panel->size().height())
                            .arg(panel_size.width())
                            .arg(panel_size.height()));
        }
    }
}

void TMainWindow::setStayOnTop(bool b) {

    bool stay_on_top = windowFlags() & Qt::WindowStaysOnTopHint;
    if (b == stay_on_top) {
        return;
    }
    WZDEBUG(QString::number(b));

    ignore_show_hide_events = true;
    bool visible = isVisible();
    QPoint old_pos = pos();

    if (b) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }

    move(old_pos);
    if (visible) {
        show();
    }
    ignore_show_hide_events = false;
}

void TMainWindow::changeStayOnTop(int stay_on_top) {

    switch (stay_on_top) {
        case TPreferences::AlwaysOnTop : setStayOnTop(true); break;
        case TPreferences::NeverOnTop  : setStayOnTop(false); break;
        case TPreferences::WhilePlayingOnTop :
            setStayOnTop(player->state() == Player::STATE_PLAYING);
            break;
    }

    pref->stay_on_top = (TPreferences::TOnTop) stay_on_top;
    emit stayOnTopChanged(stay_on_top);
}

void TMainWindow::checkStayOnTop(Player::TState) {

    if (pref->fullscreen
        || pref->stay_on_top != TPreferences::WhilePlayingOnTop) {
        return;
    }

    // On queued connection, so better use player->state()
    switch (player->state()) {
        case Player::STATE_STOPPED:
        case Player::STATE_PAUSED:
            setStayOnTop(false);
            break;
        case Player::STATE_PLAYING:
            setStayOnTop(true);
            break;
        default:
            break;
    }
}

void TMainWindow::toggleStayOnTop() {

    if (pref->stay_on_top == TPreferences::NeverOnTop)
        changeStayOnTop(TPreferences::AlwaysOnTop);
    else
        changeStayOnTop(TPreferences::NeverOnTop);
}

#if defined(Q_OS_WIN)
bool TMainWindow::winEvent (MSG* m, long* result) {

    if (m->message == WM_SYSCOMMAND) {
        if (((m->wParam & 0xFFF0) == SC_SCREENSAVE)
            || ((m->wParam & 0xFFF0) == SC_MONITORPOWER)) {
            if (player->state() == Player::STATE_PLAYING
                && player->mdat.hasVideo()) {
                logger()->debug("winEvent: not allowing screensaver");
                (*result) = 0;
                return true;
            }

            logger()->debug("winEvent: allowing screensaver");
            return false;
        }
    }
    return false;
}
#endif // defined(Q_OS_WIN)

void TMainWindow::processAction(QString action_name) {

    // Check name for checkable actions
    static QRegExp func_rx("(.*) (true|false)");
    bool value = false;
    bool booleanFunction = false;

    if (func_rx.indexIn(action_name) >= 0) {
        action_name = func_rx.cap(1);
        value = func_rx.cap(2) == "true";
        booleanFunction = true;
    }

    QAction* action = findChild<QAction*>(action_name);
    if (action) {
        if (action->isEnabled()) {
            if (action->isCheckable() && booleanFunction) {
                WZDEBUG("setting action '" + action_name + " to "
                        + QString::number(value));
                action->setChecked(value);
            } else {
                WZDEBUG("triggering action '" + action_name + "'");
                action->trigger();
            }
        } else {
            WZWARN("canceling disabled action '" + action_name + "'");
        }
    } else {
        WZWARN("action '" + action_name + "' not found");
    }
}

void TMainWindow::runActions(QString actions) {
    WZDEBUG("actions '" + actions + "'");

    actions = actions.simplified(); // Remove white space

    QAction* action;
    QStringList actionsList = actions.split(" ");
    int delay = 500; // in ms

    for (int n = 0; n < actionsList.count(); n++) {
        const QString& actionStr = actionsList.at(n);
        QString par = ""; // Parameter for action

        // Set par if the next word is a boolean value
        if (n + 1 < actionsList.count()) {
            par = actionsList.at(n + 1).toLower();
            if (par == "true" || par == "false") {
                n++;
            } else {
                par = "";
            }
        }

        action = findChild<QAction*>(actionStr);
        if (action) {
            if (action->isCheckable() && !par.isEmpty()) {
                // Set the checked state of action
                WZDEBUG("set checked action '" + actionStr + "' to " + par);
                action->setChecked(par == "true");
            } else {
                // Post to action slot trigger() with a delay of delay ms
                WZDEBUG(QString("posting action '%1' with %2ms delay")
                        .arg(actionStr).arg(delay));
                QTimer::singleShot(delay, action, SLOT(trigger()));

                // Add half a second to the delay for the next action
                delay += 500;
            }
        } else {
            WZWARN("action '" + actionStr + "' not found");
        }
    } //end for
} // void TMainWindow::runActions(QString actions)

// Slot called by onNewMediaStartedPlaying and the runActionsLater() timer
void TMainWindow::checkPendingActionsToRun() {

    QString actions;
    if (pending_actions_to_run.isEmpty()) {
        actions = pref->actions_to_run;
    } else {
        actions = pending_actions_to_run;
        pending_actions_to_run.clear();
        if (!pref->actions_to_run.isEmpty()) {
            actions = pref->actions_to_run + " " + actions;
        }
    }

    if (!actions.isEmpty()) {
        WZDEBUG("running pending actions '" + actions + "'");
        runActions(actions);
    }
}

void TMainWindow::runActionsLater(const QString& actions, bool postCheck) {

    if (pending_actions_to_run.isEmpty()) {
        pending_actions_to_run = actions;
    } else {
        pending_actions_to_run += " " + actions;
    }
    if (!pending_actions_to_run.isEmpty() && postCheck) {
        WZDEBUG(QString("posting '%1'").arg(pending_actions_to_run));
        QTimer::singleShot(0, this, SLOT(checkPendingActionsToRun()));
    }
}

void TMainWindow::leftClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_left_click_function.isEmpty()) {
        processAction(pref->mouse_left_click_function);
    }
}

void TMainWindow::rightClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_right_click_function.isEmpty()) {
        processAction(pref->mouse_right_click_function);
    }
}

void TMainWindow::doubleClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_double_click_function.isEmpty()) {
        processAction(pref->mouse_double_click_function);
    }
}

void TMainWindow::middleClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_middle_click_function.isEmpty()) {
        processAction(pref->mouse_middle_click_function);
    }
}

void TMainWindow::xbutton1ClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_xbutton1_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton1_click_function);
    }
}

void TMainWindow::xbutton2ClickFunction() {
    WZDEBUG("");

    if (!pref->mouse_xbutton2_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton2_click_function);
    }
}

} // namespace Gui

#include "moc_mainwindow.cpp"

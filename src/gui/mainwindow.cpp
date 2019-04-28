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
#include "gui/propertiesdialog.h"
#include "gui/dockwidget.h"
#include "gui/videowindow.h"
#include "gui/msg.h"

#include "gui/playlist/playlist.h"
#include "gui/playlist/favlist.h"
#include "gui/playlist/playlistwidget.h"

#include "gui/about.h"
#include "gui/audioequalizer.h"
#include "gui/eqslider.h"
#include "gui/inputurl.h"
#include "gui/stereo3ddialog.h"
#include "gui/timedialog.h"
#include "gui/timelabel.h"
#include "gui/updatechecker.h"
#include "gui/videoequalizer.h"

#include "gui/pref/dialog.h"
#include "gui/pref/interface.h"
#include "gui/pref/input.h"

#include "gui/action/menu/menufile.h"
#include "gui/action/menu/menuplay.h"
#include "gui/action/menu/menuwindowsize.h"
#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuvideo.h"
#include "gui/action/menu/menuaudio.h"
#include "gui/action/menu/menusubtitle.h"
#include "gui/action/menu/menubrowse.h"
#include "gui/action/menu/menuview.h"
#include "gui/action/menu/menuhelp.h"
#include "gui/action/editabletoolbar.h"
#include "gui/action/timeslideraction.h"
#include "gui/action/widgetactions.h"
#include "gui/action/actiongroup.h"

#include "player/player.h"
#include "player/process/exitmsg.h"
#include "settings/paths.h"
#include "app.h"
#include "images.h"
#include "extensions.h"
#include "colorutils.h"
#include "wztime.h"
#include "wztimer.h"
#include "clhelp.h"
#include "version.h"
#include "desktop.h"
#include "iconprovider.h"

#include <QMessageBox>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QNetworkProxy>
#include <QCryptographicHash>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {

TMainWindow* mainWindow = 0;

TMainWindow::TMainWindow() :
    QMainWindow(),
    optionCloseOnFinish(-1),
    ignore_show_hide_events(false),
    save_size(true),
    center_window(false),
    propertiesDialog(0),
    prefDialog(0),
    help_window(0),
    menubar_visible(true),
    statusbar_visible(true),
    fullscreen_menubar_visible(false),
    fullscreen_statusbar_visible(true) {

    mainWindow = this;

    setObjectName("main_window");
    setWindowTitle(TConfig::PROGRAM_NAME);
    setWindowIcon(Images::icon("logo", 64));
    setAttribute(Qt::WA_DeleteOnClose);
    setAcceptDrops(true);
    // Disable animation of docks.
    setAnimated(false);

    // Ready action
    readyAction = new Action::TAction(this, "ready", "", "noicon");
    readyAction->setCheckable(true);
    readyAction->setChecked(false);

    createStatusBar();

    // Reset size factor
    if (pref->save_window_size_on_exit) {
        force_resize = false;
    } else {
        force_resize = true;
        pref->size_factor = 1;
    }

    // Resize window to default size
    resize(pref->default_size);

    // Optimize size timer
    optimizeSizeTimer = new TWZTimer(this, "optimize_size_timer");
    optimizeSizeTimer->setSingleShot(true);
    optimizeSizeTimer->setInterval(100);
    connect(optimizeSizeTimer, &TWZTimer::timeout,
            this, &TMainWindow::optimizeSizeFactor);

    createPlayerWindows();
    createPlayers();
    createLogDock();
    createPlaylist();
    createFavList();

    createVideoEqualizer();
    createAudioEqualizer();

    autoHideTimer = new TAutoHideTimer(this, playerWindow);
    createActions();
    createToolbars();
    createMenus();

    titleUpdateTimer = new TWZTimer(this, "title_update_timer", false);
    titleUpdateTimer->setSingleShot(true);
    titleUpdateTimer->setInterval(200);
    connect(titleUpdateTimer, &TWZTimer::timeout,
            this, &TMainWindow::updateTitle);

    // Changes to title
    connect(playlist->getPlaylistWidget(),
            &Playlist::TPlaylistWidget::playingItemChanged,
            titleUpdateTimer, &TWZTimer::logStart);
    connect(playlist->getPlaylistWidget(),
            &Playlist::TPlaylistWidget::modifiedChanged,
            titleUpdateTimer, &TWZTimer::logStart);
    connect(player, &Player::TPlayer::streamingTitleChanged,
            titleUpdateTimer, &TWZTimer::logStart);

    // To update bitrates if properties dialog is visible
    connect(player, &Player::TPlayer::videoBitRateChanged,
            titleUpdateTimer, &TWZTimer::logStart);
    connect(player, &Player::TPlayer::audioBitRateChanged,
            titleUpdateTimer, &TWZTimer::logStart);

    setupNetworkProxy();
    changeStayOnTop(pref->stay_on_top);

    update_checker = new TUpdateChecker(this, &pref->update_checker_data);
}

TMainWindow::~TMainWindow() {

    msgSlot = 0;
    setMessageHandler(0);
}

void TMainWindow::createStatusBar() {

    statusBar()->setObjectName("statusbar");
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

    timeLabel = new TTimeLabel(statusBar());
    timeLabel->setObjectName("time_label");
    TColorUtils::setBackgroundColor(timeLabel, bgc);
    TColorUtils::setForegroundColor(timeLabel, fgc);
    timeLabel->setFrameShape(QFrame::NoFrame);
    timeLabel->setContentsMargins(margins);
    statusBar()->addPermanentWidget(timeLabel, 0);
}

void TMainWindow::createPlayerWindows() {

    playerWindow = new TPlayerWindow(this, "player_window", false);
    setCentralWidget(playerWindow);

    // Player window mouse events
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
            this, &TMainWindow::displayVideoOut, Qt::QueuedConnection);

    previewWindow = new TPlayerWindow(this, "preview_window", true);
    previewWindow->hide();
}

void TMainWindow::createPlayers() {

    Player::TPlayer* previewPlayer = new Player::TPlayer(this, "preview_player",
                                                         previewWindow, 0);
    new Player::TPlayer(this, "player", playerWindow, previewPlayer);

    connect(player, &Player::TPlayer::positionMSChanged,
            timeLabel, &TTimeLabel::setPositionMS);
    connect(player, &Player::TPlayer::durationMSChanged,
            timeLabel, &TTimeLabel::setDurationMS);

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
    connect(player, &Player::TPlayer::mediaStartedPlaying,
            this, &TMainWindow::onMediaStartedPlaying,
            Qt::QueuedConnection);
    connect(player, &Player::TPlayer::playerError,
            this, &TMainWindow::onPlayerError,
            Qt::QueuedConnection);

    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TMainWindow::displayInOutPoints);
    connect(player, &Player::TPlayer::mediaSettingsChanged,
            this, &TMainWindow::displayInOutPoints);

    connect(playerWindow, &TPlayerWindow::wheelUp,
            this, &TMainWindow::wheelUp);
    connect(playerWindow, &TPlayerWindow::wheelDown,
            this, &TMainWindow::wheelDown);
}

void TMainWindow::createLogDock() {

    logDock = new TDockWidget(this, playerWindow, "log_dock", tr("Log"));
    logWindow = new TLogWindow(logDock);
    logDock->setWidget(logWindow);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
}

void TMainWindow::createPlaylist() {

    // Setup meta type TPlaylistItem
    qRegisterMetaType<Playlist::TPlaylistItem>("Gui::Playlist::TPlaylistItem");
    qRegisterMetaTypeStreamOperators<Playlist::TPlaylistItem>(
                "Gui::Playlist::TPlaylistItem");

    playlistDock = new TDockWidget(this, playerWindow, "playlist_dock",
                                   tr("Playlist"));
    playlist = new Playlist::TPlaylist(playlistDock);
    playlistDock->setWidget(playlist);
    addDockWidget(Qt::LeftDockWidgetArea, playlistDock);

    connect(playlist, &Playlist::TPlaylist::playlistFinished,
            this, &TMainWindow::onPlaylistFinished,
            Qt::QueuedConnection);
}

void TMainWindow::createFavList() {

    favListDock = new TDockWidget(this, playerWindow, "favlist_dock",
                                  tr("Favorites"));
    favlist = new Playlist::TFavList(favListDock, playlist);
    favListDock->setWidget(favlist);
    addDockWidget(Qt::RightDockWidgetArea, favListDock);
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

    // File menu
    // Clear recents
    clearRecentsAct = new TAction(this, "recents_clear", tr("Clear recents"));
    connect(clearRecentsAct, &TAction::triggered,
            this, &TMainWindow::clearRecentsListDialog);

    // Open URL
    TAction* a = new TAction(this, "open_url", tr("Open URL..."), "noicon",
                             QKeySequence("Ctrl+U"));
    a->setIcon(iconProvider.urlIcon);
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

    a = new TAction(this, "open_dvd_iso", tr("Open DVD ISO file..."),
                    "dvd_iso");
    connect(a, &TAction::triggered, this, &TMainWindow::openDVDFromISO);

    a = new TAction(this, "open_dvd_folder", tr("Open DVD folder..."),
                    "dvd_hd");
    connect(a, &TAction::triggered, this, &TMainWindow::openDVDFromFolder);

    // BluRay
    a = new TAction(this, "open_bluray_disc", tr("Open Blu-ray disc"),
                    "bluray");
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
    // TMainWindowTray shows/hides close act
    closeAct = new TAction(this, "close", tr("Close"), "noicon");
    closeAct->setIcon(iconProvider.closeIcon);
    connect(closeAct, &TAction::triggered, this, &TMainWindow::closeWindow);

    quitAct = new Action::TAction(this, "quit", tr("Quit"), "noicon",
                                  QKeySequence("Ctrl+Q"));
    quitAct->setIcon(iconProvider.quitIcon);
    // TMainWindowTray connects quitAct

    // Play menu
    // Stop
    stopAct = new TAction(this, "stop", tr("Stop"), "", Qt::Key_MediaStop);
    connect(stopAct, &TAction::triggered, this, &TMainWindow::stop);

    // Play
    playAct = new TAction(this, "play", tr("Play"), "",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, &Action::TAction::triggered,
            this, &TMainWindow::play);

    // Pause
    pauseAct = new TAction(this, "pause", tr("Pause"), "",
                           QKeySequence("Media Pause")); // MCE remote key
    connect(pauseAct, &TAction::triggered, player, &Player::TPlayer::pause);

    // Play/pause
    playPauseAct = new TAction(this, "play_pause", tr("Play"), "play",
                               Qt::Key_Space);
    // Add MCE remote key
    playPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playPauseAct, &TAction::triggered,
            playlist, &Playlist::TPlaylist::playPause);

    // Play/pause/stop
    playPauseStopAct = new TAction(this, "play_pause_stop", tr("Play"), "play");
    connect(playPauseStopAct, &TAction::triggered,
            this, &TMainWindow::playPauseStop);

    // Play next
    playNextAct = new TAction(this, "play_next", tr("Play next"), "next",
                              QKeySequence(">"));
    playNextAct->addShortcut(QKeySequence("."));
    playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
    playNextAct->setData(4);
    connect(playNextAct, &TAction::triggered,
            playlist, &Playlist::TPlaylist::playNext);

    // Play prev
    playPrevAct = new TAction(this, "play_prev", tr("Play previous"),
                              "previous", QKeySequence("<"));
    playPrevAct->addShortcut(QKeySequence(","));
    playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
    playPrevAct->setData(4);
    connect(playPrevAct, &TAction::triggered,
            playlist, &Playlist::TPlaylist::playPrev);

    // Update enabled state play buttons when busyChanged
    connect(playlist, &Playlist::TPlaylist::busyChanged,
            this, &TMainWindow::enablePlayPauseStop);


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
    seekToTimeAct = new TAction(this, "seek_to", tr("Seek to..."), "",
                                QKeySequence("Ctrl+G"));
    connect(seekToTimeAct, &TAction::triggered,
            this, &TMainWindow::showSeekToDialog);

    // Speed menu
    new Menu::TPlaySpeedGroup(this);

    // Menu in-out
    new Menu::TInOutGroup(this);


    // Video Menu
    // Fullscreen
    fullscreenAct = new TAction(this, "fullscreen", tr("Fullscreen"), "noicon",
                                Qt::Key_F);
    fullscreenAct->setIcon(iconProvider.fullscreenIcon);
    fullscreenAct->setCheckable(true);
    connect(fullscreenAct, &TAction::triggered,
            this, &TMainWindow::toggleFullscreen);
    // Exit fullscreen (not in menu)
    a = new TAction(this, "exit_fullscreen", tr("Exit fullscreen"), "noicon",
                    Qt::Key_Escape);
    a->setIcon(iconProvider.fullscreenExitIcon);
    connect(a, &TAction::triggered, this, &TMainWindow::exitFullscreen);

    // Aspect menu
    new Menu::TAspectGroup(this);

    // Menu window size
    windowSizeGroup = new Menu::TWindowSizeGroup(this, playerWindow);

    doubleSizeAct = new TAction(this, "size_toggle_double",
                                tr("Toggle double size"), "", Qt::Key_D);
    connect(doubleSizeAct, &TAction::triggered,
            this, &TMainWindow::toggleDoubleSize);

    optimizeSizeAct = new TAction(this, "size_optimize", "", "noicon",
                                  QKeySequence("`"));
    optimizeSizeAct->setIcon(iconProvider.optimizeSizeIcon);
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

    // Menu zoom and pan
    zoomAndPanGroup = new Menu::TZoomAndPanGroup(this);

    // Video equalizer
    equalizerAct = new TAction(this, "video_equalizer", tr("Video equalizer"),
                               "", QKeySequence("E"));
    equalizerAct->setCheckable(true);
    equalizerAct->setChecked(video_equalizer->isVisible());
    connect(equalizerAct, &TAction::triggered,
            video_equalizer, &TVideoEqualizer::setVisible);
    connect(video_equalizer, &TVideoEqualizer::visibilityChanged,
            equalizerAct, &TAction::setChecked);

    resetVideoEqualizerAct = new TAction(this, "reset_video_equalizer",
                                         tr("Reset video equalizer"), "",
                                         QKeySequence("Shift+E"));
    connect(resetVideoEqualizerAct, &TAction::triggered,
            video_equalizer, &TVideoEqualizer::reset);

    // Short cuts equalizer (not in menu)
    decContrastAct = new TAction(this, "dec_contrast", tr("Dec contrast"), "",
                                 Qt::ALT | Qt::Key_1);
    connect(decContrastAct, &TAction::triggered,
            player, &Player::TPlayer::decContrast);

    incContrastAct = new TAction(this, "inc_contrast", tr("Inc contrast"), "",
                                 Qt::ALT | Qt::Key_2);
    connect(incContrastAct, &TAction::triggered,
            player, &Player::TPlayer::incContrast);

    decBrightnessAct = new TAction(this, "dec_brightness", tr("Dec brightness"),
                                   "", Qt::ALT | Qt::Key_3);
    connect(decBrightnessAct, &TAction::triggered,
            player, &Player::TPlayer::decBrightness);

    incBrightnessAct = new TAction(this, "inc_brightness", tr("Inc brightness"),
                                   "", Qt::ALT | Qt::Key_4);
    connect(incBrightnessAct, &TAction::triggered,
            player, &Player::TPlayer::incBrightness);

    decHueAct = new TAction(this, "dec_hue", tr("Dec hue"), "",
                            Qt::ALT | Qt::Key_5);
    connect(decHueAct, &TAction::triggered, player, &Player::TPlayer::decHue);

    incHueAct = new TAction(this, "inc_hue", tr("Inc hue"), "",
                            Qt::ALT | Qt::Key_6);
    connect(incHueAct, &TAction::triggered, player, &Player::TPlayer::incHue);

    decSaturationAct = new TAction(this, "dec_saturation", tr("Dec saturation"),
                                   "", Qt::ALT | Qt::Key_7);
    connect(decSaturationAct, &TAction::triggered,
            player, &Player::TPlayer::decSaturation);

    incSaturationAct = new TAction(this, "inc_saturation", tr("Inc saturation"),
                                   "", Qt::ALT | Qt::Key_8);
    connect(incSaturationAct, &TAction::triggered,
            player, &Player::TPlayer::incSaturation);

    decGammaAct = new TAction(this, "dec_gamma", tr("Dec gamma"), "",
                              Qt::ALT | Qt::Key_9);
    connect(decGammaAct, &TAction::triggered,
            player, &Player::TPlayer::decGamma);

    incGammaAct = new TAction(this, "inc_gamma", tr("Inc gamma"), "",
                              Qt::ALT | Qt::Key_0);
    connect(incGammaAct, &TAction::triggered,
            player, &Player::TPlayer::incGamma);

    // Color space
    colorSpaceGroup = new Menu::TColorSpaceGroup(this);
    connect(colorSpaceGroup, &Menu::TColorSpaceGroup::triggeredID,
            player, &Player::TPlayer::setColorSpace);

    deinterlaceGroup = new Menu::TDeinterlaceGroup(this);
    deinterlaceGroup->setChecked(player->mset.current_deinterlacer);
    connect(deinterlaceGroup, &Action::Menu::TDeinterlaceGroup::triggeredID,
            player, &Player::TPlayer::setDeinterlace);

    toggleDeinterlaceAct = new TAction(this, "toggle_deinterlacing",
                                       tr("Toggle deinterlacing"),
                                       "deinterlace", Qt::Key_I);
    toggleDeinterlaceAct->setCheckable(true);
    connect(toggleDeinterlaceAct, &TAction::triggered,
            player, &Player::TPlayer::toggleDeinterlace);

    // Transform
    flipAct = new TAction(this, "flip", tr("Flip image"));
    flipAct->setCheckable(true);
    connect(flipAct, &TAction::triggered, player, &Player::TPlayer::setFlip);

    mirrorAct = new TAction(this, "mirror", tr("Mirror image"));
    mirrorAct->setCheckable(true);
    connect(mirrorAct, &TAction::triggered,
            player, &Player::TPlayer::setMirror);

    rotateGroup = new Menu::TRotateGroup(this);
    rotateGroup->setChecked(player->mset.rotate);
    connect(rotateGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setRotate);

    // Video filters
    filterGroup = new Menu::TFilterGroup(this);

    // Denoise
    denoiseGroup = new TActionGroup(this, "denoise_group");
    denoiseGroup->setEnabled(false);
    new TActionGroupItem(this, denoiseGroup, "denoise_none", tr("Off"),
                         TMediaSettings::NoDenoise);
    new TActionGroupItem(this, denoiseGroup, "denoise_normal", tr("Normal"),
                         TMediaSettings::DenoiseNormal);
    new TActionGroupItem(this, denoiseGroup, "denoise_soft", tr("Soft"),
                         TMediaSettings::DenoiseSoft);
    connect(denoiseGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setDenoiser);

    // Unsharp
    sharpenGroup = new TActionGroup(this, "sharpen_group");
    sharpenGroup->setEnabled(false);
    new TActionGroupItem(this, sharpenGroup, "sharpen_off", tr("None"), 0);
    new TActionGroupItem(this, sharpenGroup, "blur", tr("Blur"), 1);
    new TActionGroupItem(this, sharpenGroup, "sharpen", tr("Sharpen"), 2);
    connect(sharpenGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setSharpen);

    // Stereo 3D
    stereo3DAct = new TAction(this, "stereo_3d_filter",
                              tr("Stereo 3D filter..."), "stereo3d");
    connect(stereo3DAct, &TAction::triggered,
            this, &TMainWindow::showStereo3dDialog);

    // Video tracks
    nextVideoTrackAct = new TAction(this, "next_video_track",
                                    tr("Next video track"));
    connect(nextVideoTrackAct, &TAction::triggered,
            player, &Player::TPlayer::nextVideoTrack);

    videoTrackGroup = new TActionGroup(this, "videotrack_group");
    connect(videoTrackGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setVideoTrack);
    connect(player, &Player::TPlayer::videoTrackChanged,
            videoTrackGroup, &TActionGroup::setChecked);
    connect(player, &Player::TPlayer::videoTracksChanged,
            this, &TMainWindow::updateVideoTracks);


    // Screenshots
    screenshotAct = new TAction(this, "screenshot", tr("Screenshot"), "",
                                Qt::Key_R);
    connect(screenshotAct, &TAction::triggered,
            player, &Player::TPlayer::screenshot);

    // Multiple screenshots
    screenshotsAct = new TAction(this, "screenshots", tr("Start screenshots"),
                                 "", Qt::SHIFT | Qt::Key_R);
    screenshotsAct->setCheckable(true);
    screenshotsAct->setChecked(false);
    connect(screenshotsAct, &TAction::triggered,
            this, &TMainWindow::startStopScreenshots);

    // Capture
    capturingAct = new TAction(this, "capture_stream", tr("Start capture"),
                               "record", Qt::CTRL | Qt::Key_R);
    capturingAct->setCheckable(true);
    capturingAct->setChecked(false);
    connect(capturingAct, &TAction::triggered,
            this, &TMainWindow::startStopCapture);


    // Audio menu
    // Volume
    muteAct = new TAction(this, "mute", tr("Mute"), "noicon", Qt::Key_M);
    muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
    muteAct->setCheckable(true);
    muteAct->setChecked(player->getMute());
    QIcon icset(Images::icon("volume"));
    icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
    muteAct->setIcon(icset);
    connect(muteAct, &TAction::triggered, player, &Player::TPlayer::mute);
    connect(player, &Player::TPlayer::muteChanged,
            muteAct, &TAction::setChecked);

    decVolumeAct = new TAction(this, "decrease_volume", tr("Volume -"),
                               "audio_down", Qt::Key_Minus);
    decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
    connect(decVolumeAct, &TAction::triggered,
            player, &Player::TPlayer::decVolume);

    incVolumeAct = new TAction(this, "increase_volume", tr("Volume +"),
                               "audio_up", Qt::Key_Plus);
    incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
    connect(incVolumeAct, &TAction::triggered,
            player, &Player::TPlayer::incVolume);

    // Delay
    decAudioDelayAct = new TAction(this, "dec_audio_delay", tr("Audio delay -"),
                                   "delay_down", Qt::CTRL | Qt::Key_Minus);
    connect(decAudioDelayAct, &TAction::triggered,
            player, &Player::TPlayer::decAudioDelay);

    incAudioDelayAct = new TAction(this, "inc_audio_delay", tr("Audio delay +"),
                                   "delay_up", Qt::CTRL | Qt::Key_Plus);
    connect(incAudioDelayAct, &TAction::triggered,
            player, &Player::TPlayer::incAudioDelay);

    audioDelayAct = new TAction(this, "audio_delay", tr("Set audio delay..."),
                                "", Qt::META | Qt::Key_Plus);
    connect(audioDelayAct, &TAction::triggered,
            this, &TMainWindow::showAudioDelayDialog);

    // Audio equalizer
    audioEqualizerAct = new TAction(this, "audio_equalizer",
                                    tr("Audio equalizer"), "",
                                    Qt::ALT | Qt::Key_Plus);
    audioEqualizerAct->setCheckable(true);
    audioEqualizerAct->setChecked(audio_equalizer->isVisible());
    connect(audioEqualizerAct, &TAction::triggered,
            audio_equalizer, &TAudioEqualizer::setVisible);
    connect(audio_equalizer, &TAudioEqualizer::visibilityChanged,
            audioEqualizerAct, &TAction::setChecked);

    resetAudioEqualizerAct = new TAction(this, "reset_audio_equalizer",
                                         tr("Reset audio equalizer"), "",
                                         Qt::ALT | Qt::Key_Minus);
    connect(resetAudioEqualizerAct, &TAction::triggered,
            audio_equalizer, &TAudioEqualizer::reset);

    // Stereo
    stereoGroup = new Action::Menu::TStereoGroup(this);

    // Channels
    audioChannelGroup = new Action::Menu::TAudioChannelGroup(this);

    // Audio filters
    // Normalize
    volnormAct = new TAction(this, "volnorm_filter",
                             tr("Volume normalization"));
    volnormAct->setCheckable(true);
    connect(volnormAct, &TAction::triggered,
            player, &Player::TPlayer::setVolnorm);
    // Extra stereo
    extrastereoAct = new TAction(this, "extrastereo_filter", tr("Extrastereo"));
    extrastereoAct->setCheckable(true);
    connect(extrastereoAct, &TAction::triggered,
            player, &Player::TPlayer::setExtrastereo);
    // Karaoke
    karaokeAct = new TAction(this, "karaoke_filter", tr("Karaoke"));
    karaokeAct->setCheckable(true);
    connect(karaokeAct, &TAction::triggered,
            player, &Player::TPlayer::toggleKaraoke);
    // Audio tracks
    nextAudioTrackAct = new TAction(this, "next_audio_track",
                                    tr("Next audio track"), "",
                                    QKeySequence("*"));
    connect(nextAudioTrackAct, &TAction::triggered,
            player, &Player::TPlayer::nextAudioTrack);
    audioTrackGroup = new TActionGroup(this, "audiotrack_group");
    connect(audioTrackGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setAudioTrack);
    connect(player, &Player::TPlayer::audioTrackChanged,
            audioTrackGroup, &TActionGroup::setChecked);
    connect(player, &Player::TPlayer::audioTracksChanged,
            this, &TMainWindow::updateAudioTracks);
    // Load/unload
    loadAudioAct = new TAction(this, "load_audio_file",
                               tr("Load external audio..."), "open");
    connect(loadAudioAct, &TAction::triggered,
            this, &TMainWindow::loadAudioFile);
    unloadAudioAct = new TAction(this, "unload_audio_file",
                                 tr("Unload external audio"), "unload");
    connect(unloadAudioAct, &TAction::triggered,
            player, &Player::TPlayer::unloadAudioFile);


    // Subtitles menu
    decSubPosAct = new TAction(this, "dec_sub_pos", tr("Up"), "", Qt::Key_Up);
    connect(decSubPosAct, &TAction::triggered,
            player, &Player::TPlayer::decSubPos);
    incSubPosAct = new TAction(this, "inc_sub_pos", tr("Down"), "",
                               Qt::Key_Down);
    connect(incSubPosAct, &TAction::triggered,
            player, &Player::TPlayer::incSubPos);

    incSubScaleAct = new TAction(this, "inc_sub_scale", tr("Size +"), "",
                                 Qt::Key_K);
    connect(incSubScaleAct, &TAction::triggered,
            player, &Player::TPlayer::incSubScale);
    decSubScaleAct = new TAction(this, "dec_sub_scale", tr("Size -"), "",
                                 Qt::SHIFT | Qt::Key_K);
    connect(decSubScaleAct, &TAction::triggered,
            player, &Player::TPlayer::decSubScale);

    incSubDelayAct = new TAction(this, "inc_sub_delay", tr("Delay +"), "",
                                 Qt::ALT | Qt::Key_D);
    connect(incSubDelayAct, &TAction::triggered,
            player, &Player::TPlayer::incSubDelay);
    decSubDelayAct = new TAction(this, "dec_sub_delay", tr("Delay -"), "",
                                 Qt::SHIFT | Qt::Key_D);
    connect(decSubDelayAct, &TAction::triggered,
            player, &Player::TPlayer::decSubDelay);
    subDelayAct = new TAction(this, "sub_delay", tr("Set delay..."), "",
                              Qt::META | Qt::Key_D);
    connect(subDelayAct, &TAction::triggered,
            this, &TMainWindow::showSubDelayDialog);

    incSubStepAct = new TAction(this, "inc_sub_step",
                                tr("Next line in subtitles"), "",
                                Qt::SHIFT | Qt::Key_L);
    connect(incSubStepAct, &TAction::triggered,
            player, &Player::TPlayer::incSubStep);
    decSubStepAct = new TAction(this, "dec_sub_step",
        tr("Previous line in subtitles"), "", Qt::CTRL | Qt::Key_L);
    connect(decSubStepAct, &TAction::triggered,
            player, &Player::TPlayer::decSubStep);

    seekNextSubAct = new TAction(this, "seek_next_sub",
        tr("Seek to next subtitle"), "", Qt::Key_N, pref->isMPV());
    connect(seekNextSubAct, &TAction::triggered,
            player, &Player::TPlayer::seekToNextSub);
    seekPrevSubAct = new TAction(this, "seek_prev_sub",
                                 tr("Seek to previous subtitle"), "",
                                 Qt::SHIFT | Qt::Key_N, pref->isMPV());
    connect(seekPrevSubAct, &TAction::triggered,
            player, &Player::TPlayer::seekToPrevSub);

    nextSubtitleAct = new TAction(this, "next_subtitle",
                                  tr("Next subtitle track"), "",
                                  Qt::CTRL | Qt::Key_N);
    connect(nextSubtitleAct, &TAction::triggered,
            player, &Player::TPlayer::nextSubtitle);

    subtitleTrackGroup = new TActionGroup(this, "subtitle_track_group");
    connect(subtitleTrackGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setSubtitle);
    connect(player, &Player::TPlayer::subtitlesChanged,
            this, &TMainWindow::updateSubtitleTracks);
    // Need to update all, to disable sub in secondary track
    connect(player, &Player::TPlayer::subtitleTrackChanged,
            this, &TMainWindow::updateSubtitleTracks);

    secondarySubtitleTrackGroup = new TActionGroup(this, "subtitle_track2_group");
    connect(secondarySubtitleTrackGroup, &TActionGroup::triggeredID,
            player, &Player::TPlayer::setSecondarySubtitle);
    // Need to update all, to disable sub in primary track
    connect(player, &Player::TPlayer::secondarySubtitleTrackChanged,
            this, &TMainWindow::updateSubtitleTracks);

    // Closed captions
    closedCaptionsGroup = new Menu::TClosedCaptionsGroup(this);

    // Forced subs
    useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only",
                                       tr("Forced subtitles only"));
    useForcedSubsOnlyAct->setCheckable(true);
    useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
    connect(useForcedSubsOnlyAct, &TAction::triggered,
            player, &Player::TPlayer::toggleForcedSubsOnly);

    // Load/unload subs
    loadSubsAct = new TAction(this, "load_subs", tr("Load subtitles..."),
                              "open");
    connect(loadSubsAct, &TAction::triggered, this, &TMainWindow::loadSub);
    unloadSubsAct = new TAction(this, "unload_subs", tr("Unload subtitles"),
                                "unload");
    connect(unloadSubsAct, &TAction::triggered,
            player, &Player::TPlayer::unloadSub);
    subFPSGroup = new Menu::TSubFPSGroup(this);

    // Use custom style
    useCustomSubStyleAct = new TAction(this, "use_custom_sub_style",
                                       tr("Use custom style"));
    useCustomSubStyleAct->setCheckable(true);
    useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);
    connect(useCustomSubStyleAct, &TAction::triggered,
            player, &Player::TPlayer::setUseCustomSubStyle);


    // Browse menu
    // Titles
    titleGroup = new Menu::TTitleGroup(this);

    // Chapters
    nextChapterAct = new TAction(this, "next_chapter", tr("Next chapter"), "",
                                 Qt::Key_C);
    connect(nextChapterAct, &TAction::triggered,
            player, &Player::TPlayer::nextChapter);
    prevChapterAct = new TAction(this, "prev_chapter", tr("Previous chapter"),
                                 "", Qt::SHIFT | Qt::Key_C);
    connect(prevChapterAct, &TAction::triggered,
            player, &Player::TPlayer::prevChapter);
    chapterGroup = new Menu::TChapterGroup(this, prevChapterAct,
                                           nextChapterAct);

    // Angles
    nextAngleAct = new TAction(this, "next_angle", tr("Next angle"), "",
                               Qt::SHIFT | Qt::Key_A);
    connect(nextAngleAct, &TAction::triggered,
            player, &Player::TPlayer::nextAngle);
    angleGroup = new Menu::TAngleGroup(this, nextAngleAct);

    // DVDNAV
    dvdnavUpAct = new TAction(this, "dvdnav_up", tr("DVD move up"), "",
                              Qt::META | Qt::Key_Up);
    connect(dvdnavUpAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavUp);
    dvdnavDownAct = new TAction(this, "dvdnav_down", tr("DVD move down"), "",
                                Qt::META | Qt::Key_Down);
    connect(dvdnavDownAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavDown);
    dvdnavLeftAct = new TAction(this, "dvdnav_left", tr("DVD move left"), "",
                                Qt::META | Qt::Key_Left);
    connect(dvdnavLeftAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavLeft);
    dvdnavRightAct = new TAction(this, "dvdnav_right", tr("DVD move right"),
                                 "", Qt::META | Qt::Key_Right);
    connect(dvdnavRightAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavRight);

    dvdnavSelectAct = new TAction(this, "dvdnav_select", tr("DVD select"), "",
                                  Qt::META | Qt::Key_Return);
    connect(dvdnavSelectAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavSelect);

    // Not in menu
    dvdnavMouseAct = new TAction(this, "dvdnav_mouse", tr("DVD mouse click"));
    connect(dvdnavMouseAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavMouse);
    connect(playerWindow, &TPlayerWindow::dvdnavMousePos,
            player, &Player::TPlayer::dvdnavMousePos);

    dvdnavMenuAct = new TAction(this, "dvdnav_menu", tr("DVD menu"), "",
                                Qt::CTRL | Qt::Key_Return);
    connect(dvdnavMenuAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavMenu);

    dvdnavPrevAct = new TAction(this, "dvdnav_prev", tr("DVD previous menu"),
                                "", Qt::META | Qt::Key_Escape);
    connect(dvdnavPrevAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavPrev);


    // View menu
    // OSD
    a = new TAction(this, "osd_next", tr("Next OSD level"), "", Qt::Key_O);
    connect(a, &TAction::triggered, player, &Player::TPlayer::nextOSDLevel);

    osdGroup = new Menu::TOSDGroup(this);

    a = new TAction(this, "osd_inc_scale", tr("OSD size +"), "",
                    QKeySequence(")"));
    connect(a, &TAction::triggered, player, &Player::TPlayer::incOSDScale);

    a = new TAction(this, "osd_dec_scale", tr("OSD size -"), "",
                    QKeySequence("("));
    connect(a, &TAction::triggered, player, &Player::TPlayer::decOSDScale);

    osdShowFilenameAct = new TAction(this, "osd_show_filename",
                                     tr("Show filename on OSD"));
    connect(osdShowFilenameAct, &TAction::triggered,
            player, &Player::TPlayer::showFilenameOnOSD);

    osdShowTimeAct = new TAction(this, "osd_show_time",
                                 tr("Show playback time on OSD"));
    connect(osdShowTimeAct, &TAction::triggered,
            player, &Player::TPlayer::showTimeOnOSD);

    // Stay on top
    new Menu::TStayOnTopGroup(this);

    // View properties
    viewPropertiesAct = new TAction(this, "view_properties",
                                    tr("Properties..."), "",
                                    Qt::SHIFT | Qt::Key_P);
    viewPropertiesAct->setCheckable(true);
    viewPropertiesAct->setEnabled(false);
    connect(viewPropertiesAct, &TAction::triggered,
            this, &TMainWindow::showFilePropertiesDialog);

    // View playlist
    QAction* action = playlistDock->toggleViewAction();
    action->setObjectName("view_playlist");
    action->setIcon(Images::icon("playlist"));
    action->setShortcut(Qt::Key_P);
    updateToolTip(action);
    addAction(action);
    autoHideTimer->add(action, playlistDock);

    // View favorites
    action = favListDock->toggleViewAction();
    action->setObjectName("view_favorites");
    action->setIcon(Images::icon("favorites_menu"));
    action->setShortcut(Qt::SHIFT | Qt::Key_F);
    updateToolTip(action);
    addAction(action);
    autoHideTimer->add(action, favListDock);

    // View log
    action = logDock->toggleViewAction();
    action->setObjectName("view_log");
    action->setIcon(Images::icon("log"));
    action->setShortcut(QKeySequence("L"));
    updateToolTip(action);
    addAction(action);
    autoHideTimer->add(action, logDock);

    // Browse config dir
    a = new TAction(this, "browse_config_dir", tr("Browse settings folder..."));
    connect(a, &TAction::triggered, this, &TMainWindow::browseConfigFolder);

    // Browse data dir
    a = new TAction(this, "browse_data_dir", tr("Browse data folder..."));
    connect(a, &TAction::triggered, this, &TMainWindow::browseDataFolder);

    // Settings
    a = new TAction(this, "view_settings", tr("Settings..."), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_S);
    connect(a, &QAction::triggered, this, &TMainWindow::showSettingsDialog);


    // Help
    a = new TAction(this, "help_cl_options", tr("Command line options"),
                    "cl_help");
    connect(a, &TAction::triggered, this, &TMainWindow::helpCLOptions);

    a = new TAction(this, "help_check_updates", tr("Check for updates"),
                    "pref_updates");
    connect(a, &TAction::triggered, this, &TMainWindow::helpCheckUpdates);

    a = new TAction(this, "help_about", tr("About WZPlayer"), "logo");
    connect(a, &TAction::triggered, this, &TMainWindow::helpAbout);


    // Time slider
    timeslider_action = new TTimeSliderAction(this, player);

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
            timeLabel, &QLabel::setVisible);

    timeResGroup = new TActionGroup(this, "time_res_group");
    new TActionGroupItem(this, timeResGroup, "time_res_second",
                         tr("Seconds"), TTimeLabel::RES_SECONDS);
    new TActionGroupItem(this, timeResGroup, "time_res_ms",
                         tr("Milliseconds"), TTimeLabel::RES_MS);
    new TActionGroupItem(this, timeResGroup, "time_res_frames",
                         tr("Frames or millisceconds"),
                         TTimeLabel::RES_FRAMES);
    connect(timeResGroup, &TActionGroup::triggeredID,
            timeLabel, &TTimeLabel::setTimeResolution);

    a = new Action::TAction(this, "next_wheel_function",
                            tr("Next wheel function"), 0, Qt::Key_W);
    connect(a, &Action::TAction::triggered,
            this, &TMainWindow::nextWheelFunction);
} // createActions

void TMainWindow::createMenus() {

    using namespace Action;

    fileMenu = new Menu::TMenuFile(this, this, favlist->getFavMenu());
    addAction(menuBar()->addMenu(fileMenu));
    playMenu = new Menu::TMenuPlay(this, this);
    addAction(menuBar()->addMenu(playMenu));
    videoMenu = new Menu::TMenuVideo(this, this);
    addAction(menuBar()->addMenu(videoMenu));
    audioMenu = new Menu::TMenuAudio(this, this);
    addAction(menuBar()->addMenu(audioMenu));
    subtitleMenu = new Menu::TMenuSubtitle(this, this);
    addAction(menuBar()->addMenu(subtitleMenu));
    browseMenu = new Menu::TMenuBrowse(this, this);
    addAction(menuBar()->addMenu(browseMenu));

    // Statusbar menu
    statusbarMenu = new QMenu(this);
    statusbarMenu->menuAction()->setObjectName("statusbar_menu");
    statusbarMenu->menuAction()->setText(tr("Statusbar display"));
    statusbarMenu->menuAction()->setIcon(Images::icon("statusbar"));
    statusbarMenu->addAction(viewVideoInfoAct);
    statusbarMenu->addAction(viewInOutPointsAct);
    statusbarMenu->addAction(viewVideoTimeAct);
    statusbarMenu->addSeparator();

    QMenu* menu = new QMenu(this);
    menu->menuAction()->setObjectName("time_res_menu");
    menu->menuAction()->setText(tr("Time resolution"));
    menu->addActions(timeResGroup->actions());
    statusbarMenu->addMenu(menu);

    // Toolbar context menu
    toolbarMenu = new Menu::TMenu(this, "toolbar_menu", tr("Toolbars"),
                                  "toolbars");
    toolbarMenu->addAction(viewMenuBarAct);
    toolbarMenu->addSeparator();
    toolbarMenu->addAction(toolbar->toggleViewAction());
    toolbarMenu->addAction(toolbar2->toggleViewAction());
    toolbarMenu->addAction(controlbar->toggleViewAction());

    toolbarMenu->addSeparator();
    toolbarMenu->addAction(requireAction("toggle_pl_toolbar"));
    toolbarMenu->addAction(requireAction("toggle_fav_toolbar"));

    toolbarMenu->addSeparator();
    toolbarMenu->addAction(viewStatusBarAct);
    toolbarMenu->addMenu(statusbarMenu);

    // Needed for full screen statusbar
    connect(toolbarMenu, &QMenu::aboutToShow,
            autoHideTimer, &TAutoHideTimer::disable);
    connect(toolbarMenu, &QMenu::aboutToHide,
            autoHideTimer, &TAutoHideTimer::enable);

    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), &QStatusBar::customContextMenuRequested,
            toolbarMenu, &Menu::TMenu::execSlot);


    editToolbarMenu = new Menu::TMenu(this, "toolbar_edit_menu",
                                      tr("Edit toolbars"));

    editToolbarMenu->addAction(requireAction("edit_toolbar1"));
    editToolbarMenu->addAction(requireAction("edit_toolbar2"));
    editToolbarMenu->addAction(requireAction("edit_controlbar"));

    editToolbarMenu->addSeparator();
    editToolbarMenu->addAction(requireAction("edit_pl_toolbar"));
    editToolbarMenu->addAction(requireAction("edit_fav_toolbar"));

    viewMenu = new Menu::TMenuView(this, this, toolbarMenu, editToolbarMenu);
    menuBar()->addMenu(viewMenu);

    helpMenu = new Menu::TMenuHelp(this, this);
    menuBar()->addMenu(helpMenu);

    // Context menu
    contextMenu = createContextMenu("context_menu", tr("Context menu"));
    TAction* a = new TAction(this, "show_context_menu",
                             tr("Show context menu"));
    connect(a, &TAction::triggered, contextMenu, &Menu::TMenu::execSlot);

    connect(contextMenu, &QMenu::aboutToShow,
            autoHideTimer, &TAutoHideTimer::disable);
    connect(contextMenu, &QMenu::aboutToHide,
            autoHideTimer, &TAutoHideTimer::enable);

    playerWindow->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playerWindow, &TPlayerWindow::customContextMenuRequested,
            this, &TMainWindow::showContextMenu);

    // Set context menu toolbar playlists
    Action::Menu::TMenu* m = createPopupMenu();
    m->menuAction()->setObjectName("dock_menu");
    m->menuAction()->setText(tr("Dock menu"));
    m->menuAction()->setIcon(Images::icon("dock_menu"));
    playlist->setContextMenuToolbar(m);
    favlist->setContextMenuToolbar(m);
} // createMenus()

// Called by main window to show context popup on toolbars and dock widgets.
// The main window takes ownership of the returned menu and will delete it
// after use, hence the need to create a new menu every time.
Action::Menu::TMenu* TMainWindow::createPopupMenu() {

    Action::Menu::TMenu* menu = new Action::Menu::TMenu(this);
    menu->addAction(playlistDock->toggleViewAction());
    menu->addAction(favListDock->toggleViewAction());
    menu->addAction(logDock->toggleViewAction());
    menu->addAction(viewPropertiesAct);
    menu->addSeparator();
    menu->addMenu(toolbarMenu);
    menu->addMenu(editToolbarMenu);

    connect(menu, &QMenu::aboutToShow,
            autoHideTimer, &TAutoHideTimer::disable);
    connect(menu, &QMenu::aboutToHide,
            autoHideTimer, &TAutoHideTimer::enable);

    return menu;
}

Action::Menu::TMenu* TMainWindow::createContextMenu(const QString& name,
                                                    const QString& text) {

    Action::Menu::TMenu* menu = new Action::Menu::TMenu(this, name, text);
    menu->addMenu(fileMenu);
    menu->addMenu(playMenu);
    menu->addMenu(videoMenu);
    menu->addMenu(audioMenu);
    menu->addMenu(subtitleMenu);
    menu->addMenu(browseMenu);
    menu->addMenu(viewMenu);
    return menu;
}

void TMainWindow::createToolbars() {

    menuBar()->setObjectName("menubar");

    // Control bar
    controlbar = new Action::TEditableToolbar(this, "controlbar",
                                              tr("Control bar"));
    QStringList actions;
    actions << "stop"
            << "play_pause_stop"
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
            << "view_playlist|0|1"
            << "view_favorites|0|1"
            << "view_properties|0|1"
            << "separator|0|1"
            << "fullscreen";
    controlbar->setDefaultActions(actions);
    addToolBar(Qt::BottomToolBarArea, controlbar);

    QAction* action = controlbar->toggleViewAction();
    action->setShortcut(Qt::SHIFT | Qt::Key_F6);
    addAction(action);

    // Main toolbar
    toolbar = new Action::TEditableToolbar(this, "toolbar1",
                                           tr("Main toolbar"));
    action = toolbar->toggleViewAction();
    action->setShortcut(Qt::SHIFT | Qt::Key_F3);
    addAction(action);

    actions.clear();
    actions << "open_url" << "favorites_menu";
    toolbar->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar);

    // Extra toolbar
    toolbar2 = new Action::TEditableToolbar(this, "toolbar2",
                                            tr("Extra toolbar"));
    action = toolbar2->toggleViewAction();
    action->setShortcut(Qt::SHIFT | Qt::Key_F4);
    addAction(action);

    actions.clear();
    actions << "osd_menu" << "stay_on_top_menu" << "separator"
            << "view_playlist" << "view_log" << "view_properties"
            << "separator" << "view_settings";
    toolbar2->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar2);

    // Statusbar
    statusBar()->setObjectName("statusbar");

    // Add toolbars to auto_hide_timer. Docks already added by createActions().
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

    prefDialog->setData(pref);
    prefDialog->show();
}

void TMainWindow::restartApplication() {
    WZDEBUG("");

    emit requestRestart();

    // When fullscreen the window size will not yet be updated by the time
    // it is saved by saveSettings, so block saving it.
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
    prefDialog->getData(pref);

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

    // Use custom subtitle style
    useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);

    // Interface
    // Show player window
    if (!pref->hide_video_window_on_audio_files && !playerWindow->isVisible()) {
        resize(width(), height() + 200);
        playerWindow->show();
    }
    // Hide toolbars delay
    autoHideTimer->setInterval(pref->floating_hide_delay);

    // Keyboard and mouse
    playerWindow->setDelayLeftClick(pref->delay_left_click);
    setSeekTexts();
    // Enable wiil restart player if seek_preview was not set
    if (!pref->seek_preview) {
        player->previewPlayer->stop();
    }

    // Network
    setupNetworkProxy();

    // Update log window edit control
    logWindow->edit->setMaximumBlockCount(pref->log_window_max_events);

    emit settingsChanged();

    // Enable actions to reflect changes
    enableActions();

    // Restart video if needed
    if (prefDialog->requiresRestartPlayer()) {
        player->restart();
    }
} // TMainWindow::applyNewSettings()

void TMainWindow::createFilePropertiesDialog() {
    WZDEBUG("");

    propertiesDialog = new TPropertiesDialog(this);
    connect(propertiesDialog, &TPropertiesDialog::applied,
            this, &TMainWindow::applyFileProperties);
    connect(propertiesDialog, &TPropertiesDialog::visibilityChanged,
            viewPropertiesAct, &Action::TAction::setChecked);
}

void TMainWindow::setFilePropertiesData() {
    WZTRACE("");

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Get codecs and demuxer from player
    Player::Info::TPlayerInfo* playerInfo = Player::Info::TPlayerInfo::obj();
    playerInfo->getInfo();
    // Set them in prop dialog
    propertiesDialog->setCodecs(playerInfo->vcList(),
                                playerInfo->acList(),
                                playerInfo->demuxerList());

    // Save a copy of the demuxer and codecs
    TMediaData& mdat = player->mdat;
    Settings::TMediaSettings& mset = player->mset;
    if (mset.original_demuxer.isEmpty())
        mset.original_demuxer = mdat.demuxer;
    if (mset.original_video_codec.isEmpty())
        mset.original_video_codec = mdat.video_codec;
    if (mset.original_audio_codec.isEmpty())
        mset.original_audio_codec = mdat.audio_codec;

    // Set demuxer, video and audio codec
    QString demuxer = mset.forced_demuxer;
    if (demuxer.isEmpty()) demuxer = mdat.demuxer;
    QString vc = mset.forced_video_codec;
    if (vc.isEmpty()) vc = mdat.video_codec;
    QString ac = mset.forced_audio_codec;
    if (ac.isEmpty()) ac = mdat.audio_codec;

    propertiesDialog->setDemuxer(demuxer, mset.original_demuxer);
    propertiesDialog->setVideoCodec(vc, mset.original_video_codec);
    propertiesDialog->setAudioCodec(ac, mset.original_audio_codec);

    propertiesDialog->setPlayerAdditionalArguments(
                mset.player_additional_options);
    propertiesDialog->setPlayerAdditionalVideoFilters(
                mset.player_additional_video_filters);
    propertiesDialog->setPlayerAdditionalAudioFilters(
                mset.player_additional_audio_filters);

    propertiesDialog->showInfo(playlist->getPlayingTitle(true));

    QApplication::restoreOverrideCursor();
}

void TMainWindow::applyFileProperties() {
    WZDEBUG("");

    bool need_restart = false;
    bool demuxer_changed = false;

    // Demuxer
    QString prev_demuxer = player->mset.forced_demuxer;
    QString s = propertiesDialog->demuxer();
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
    s = propertiesDialog->videoCodec();
    if (s == player->mset.original_video_codec) {
        s = "";
    }
    if (s != player->mset.forced_video_codec) {
        player->mset.forced_video_codec = s;
        need_restart = true;
    }

    // Audio codec
    s = propertiesDialog->audioCodec();
    if (s == player->mset.original_audio_codec) {
        s = "";
    }
    if (s != player->mset.forced_audio_codec) {
        player->mset.forced_audio_codec = s;
        need_restart = true;
    }

    // Additional options
    s = propertiesDialog->playerAdditionalArguments();
    if (s != player->mset.player_additional_options) {
        player->mset.player_additional_options = s;
        need_restart = true;
    }

    // Additional video filters
    s = propertiesDialog->playerAdditionalVideoFilters();
    if (s != player->mset.player_additional_video_filters) {
        player->mset.player_additional_video_filters = s;
        need_restart = true;
    }

    // Additional audio filters
    s = propertiesDialog->playerAdditionalAudioFilters();
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
        if (!propertiesDialog) {
            createFilePropertiesDialog();
        }
        setFilePropertiesData();
        propertiesDialog->show();
    } else {
        propertiesDialog->hide();
    }
}

void TMainWindow::setWindowCaption(const QString& title) {
    setWindowTitle(title);
}

QString parentOrMenuName(QAction* action) {

    QString name = action->parent()->objectName();
    if (name.isEmpty()) {
        QString s;
        QMenu* menu = qobject_cast<QMenu*>(action->parent());
        if (menu) {
            s = menu->menuAction()->objectName();
            if (!s.isEmpty()) {
                return "owned by menu \"" + s + "\"";
            }
            s = "owned by unnamed menu";
            if (menu->parent()) {
                s += " with grandparent \"" + menu->parent()->objectName()
                        + "\"";
            }
            return s;
        }
        return "not owned by menu";
    }
    return "with parent \"" + name + "\"";
}

QList<QAction*> TMainWindow::findNamedActions() const {

    QList<QAction*> allActions = findChildren<QAction*>();
    QList<QAction*> selectedActions;

    for (int i = 0; i < allActions.count(); i++) {
        QAction* action = allActions.at(i);
        if (!action->isSeparator()) {
            if (action->objectName().isEmpty()) {
                //WZTRACE(QString("Skipping unnamed %1action \"%2\" %3")
                //        .arg(action->menu() ? "menu " : "")
                //        .arg(action->text())
                //        .arg(parentOrMenuName(action)));
            } else if (action->objectName() == "_q_qlineeditclearaction") {
            } else {
                selectedActions.append(action);
                //WZTRACE(QString("Selected action %1 ('%2') %3")
                //            .arg(action->objectName())
                //            .arg(action->text())
                //            .arg(parentOrMenuName(action)));
            }
        }
    }

    WZDEBUG(QString("Selected %1 actions out of %2 found actions")
            .arg(selectedActions.count()).arg(allActions.count()));

    return selectedActions;
}

QAction* TMainWindow::requireAction(const QString &name) {

    QAction* action = findChild<QAction*>(name);
    if (action) {
        return action;
    }

    WZERROR(QString("Action '%1' not found").arg(name));
    Q_ASSERT(false);
    return action;
}

void TMainWindow::checkActionValid(QString& action, const QString& def) {

    if (!action.isEmpty() && !findChild<QAction*>(action)) {
        WZW << "Action" << action
            << "not found. Resetting it to default" << def;
        action = def;
    }
}

void TMainWindow::loadSettings() {
    WZTRACE("");

    // Get all actions with a name
    Action::TAction::allActions = findNamedActions();
    // Load modified actions from settings
    Action::TActionsEditor::loadSettings(pref);

    checkActionValid(pref->mouse_left_click_function, "play_pause");
    checkActionValid(pref->mouse_right_click_function, "show_context_menu");
    checkActionValid(pref->mouse_double_click_function, "fullscreen");
    checkActionValid(pref->mouse_middle_click_function, "next_wheel_function");
    checkActionValid(pref->mouse_xbutton1_click_function, "");
    checkActionValid(pref->mouse_xbutton2_click_function, "");

    pref->beginGroup(objectName());

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

    toolbar->loadSettings();
    toolbar2->loadSettings();
    controlbar->loadSettings();

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
        favListDock->hide();
        logDock->hide();
    }

    pref->beginGroup("statusbar");
    viewVideoInfoAct->setChecked(pref->value("video_info", true).toBool());
    viewInOutPointsAct->setChecked(pref->value("in_out_points", true).toBool());
    viewVideoTimeAct->setChecked(pref->value("video_time", true).toBool());
    int res = pref->getInt("time_resolution", TTimeLabel::RES_SECONDS,
                           TTimeLabel::RES_FRAMES, TTimeLabel::RES_SECONDS);
    timeLabel->setTimeResolution(res);
    timeResGroup->setChecked(res);
    pref->endGroup();

    pref->endGroup();

    playlist->loadSettings();
    favlist->loadSettings();
}

void TMainWindow::saveSettings() {
    WZT;

    pref->beginGroup(objectName());

    if (pref->save_window_size_on_exit && save_size) {
        pref->setValue("pos", pos());
        pref->setValue("size", size());
        pref->setValue("state", (int) windowState());
    }

    // Toolbars
    toolbar->saveSettings();
    toolbar2->saveSettings();
    controlbar->saveSettings();

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
    pref->setValue("time_resolution", timeLabel->timeResolution());
    pref->endGroup();

    pref->endGroup();

    playlist->saveSettings();
    favlist->saveSettings();
    if (help_window) {
        help_window->saveSettings(pref);
    }
    if (propertiesDialog) {
        propertiesDialog->saveSettings();
    }
    if (prefDialog) {
        prefDialog->saveSettings();
    }
}

void TMainWindow::save() {

    msg(tr("Saving settings"), 0);
    player->saveMediaSettings();
    if (pref->clean_config) {
        pref->clean_config = false;
        pref->remove("");
        Action::TActionsEditor::saveSettings(pref);
    }
    saveSettings();
    pref->save();
}

void TMainWindow::displayVideoOut() {

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
    if (player->mset.in_point_ms > 0) {
        s = tr("I: %1", "In point in statusbar")
                .arg(TWZTime::formatMS(player->mset.in_point_ms));
    }

    if (player->mset.out_point_ms > 0) {
        if (!s.isEmpty()) s += " ";
        s += tr("O: %1", "Out point in statusbar")
                .arg(TWZTime::formatMS(player->mset.out_point_ms));
    }

    if (player->mset.loop) {
        if (!s.isEmpty()) s += " ";
        s += tr("R", "Symbol for repeat in-out in statusbar");
    }

    in_out_points_label->setText(s);
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
        WZDEBUG("Unpausing player");
        player->play();
    }

    setFloatingToolbarsVisible(true);
}

void TMainWindow::hideEvent(QHideEvent* event) {

    QMainWindow::hideEvent(event);
    if (pref->pause_when_hidden
        && player->state() == Player::STATE_PLAYING
        && !ignore_show_hide_events) {
        WZDEBUG("Pausing player");
        player->pause();
    }

    setFloatingToolbarsVisible(false);
}

void TMainWindow::showContextMenu() {

    // Using this event to make the context menu popup on right mouse button
    // down event, instead of waiting for the mouse button release event,
    // which would trigger the show_context_menu action if it is assigned to
    // the mouse_right_click_function.
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
        tr("The current values have been stored to be used as default."));
}

void TMainWindow::changeVideoEqualizerBySoftware(bool b) {
    WZDEBUG(QString::number(b));

    if (b != pref->use_soft_video_eq) {
        pref->use_soft_video_eq = b;
        player->restart();
    }
}

void TMainWindow::updateVideoTracks() {

    Maps::TTracks* videos = &player->mdat.videos;
    WZTRACE(QString("%1 video tracks").arg(videos->count()));

    videoTrackGroup->clear();

    if (videos->count() == 0) {
        QAction* a = videoTrackGroup->addAction(tr("<empty>"));
        a->setEnabled(false);
    } else {
        Maps::TTracks::TTrackIterator i = videos->getIterator();
        while (i.hasNext()) {
            i.next();
            const Maps::TTrackData& track = i.value();
            QAction* action = new QAction(videoTrackGroup);
            action->setCheckable(true);
            action->setText(track.getDisplayName());
            action->setData(track.getID());
            if (track.getID() == videos->getSelectedID())
                action->setChecked(true);
        }
    }

    emit videoTrackGroupChanged(videoTrackGroup);
}

void TMainWindow::updateAudioTracks() {

    Maps::TTracks* audios = &player->mdat.audios;
    WZTRACE(QString("%1 audio tracks").arg(audios->count()));

    audioTrackGroup->clear();

    if (audios->count() == 0) {
        QAction* a = audioTrackGroup->addAction(tr("<empty>"));
        a->setEnabled(false);
    } else {
        Maps::TTracks::TTrackIterator i = audios->getIterator();
        while (i.hasNext()) {
            i.next();
            const Maps::TTrackData& track = i.value();
            QAction* action = new QAction(audioTrackGroup);
            action->setCheckable(true);
            action->setText(track.getDisplayName());
            action->setData(track.getID());
            if (track.getID() == audios->getSelectedID())
                action->setChecked(true);
        }
    }

    emit audioTrackGroupChanged(audioTrackGroup);
}

void TMainWindow::updateSubtitleTracks() {
    WZTRACE("");

    // Note: subtitles use idx not ID
    subtitleTrackGroup->clear();
    secondarySubtitleTrackGroup->clear();

    // Add none to primary
    QAction* subNoneAct = subtitleTrackGroup->addAction(tr("None"));
    subNoneAct->setData(SubData::None);
    subNoneAct->setCheckable(true);
    if (player->mset.current_sub_idx < 0) {
        subNoneAct->setChecked(true);
    }
    // Add none to secondary
    subNoneAct = secondarySubtitleTrackGroup->addAction(tr("None"));
    subNoneAct->setData(SubData::None);
    subNoneAct->setCheckable(true);
    if (player->mset.current_secondary_sub_idx < 0) {
        subNoneAct->setChecked(true);
    }

    // Add subs from mdat
    for (int idx = 0; idx < player->mdat.subs.count(); idx++) {
        SubData sub = player->mdat.subs.itemAt(idx);
        QAction *a = new QAction(subtitleTrackGroup);
        a->setCheckable(true);
        a->setText(sub.displayName());
        a->setData(idx);
        if (idx == player->mset.current_sub_idx) {
            a->setChecked(true);
        }

        if (pref->isMPV()) {
            if (idx == player->mset.current_secondary_sub_idx) {
                a->setEnabled(false);
            }

            a = new QAction(secondarySubtitleTrackGroup);
            a->setCheckable(true);
            a->setText(sub.displayName());
            a->setData(idx);
            if (idx == player->mset.current_secondary_sub_idx) {
                a->setChecked(true);
            }
            if (idx == player->mset.current_sub_idx) {
                a->setEnabled(false);
            }
        }
    }

    enableSubtitleActions();

    emit subtitleTrackGroupsChanged(subtitleTrackGroup,
                                    secondarySubtitleTrackGroup);
}

void TMainWindow::startStopScreenshots() {

    if (screenshotAct->isChecked()) {
        screenshotsAct->setText(tr("Start screenshots"));
    } else {
        screenshotsAct->setText(tr("Stop screenshots"));
    }
    player->screenshots();
}

void TMainWindow::startStopCapture() {

    if (capturingAct->isChecked()) {
        capturingAct->setText(tr("Start capture"));
    } else {
        capturingAct->setText(tr("Stop capture"));
    }
    player->switchCapturing();
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

void TMainWindow::onResizeOnLoadTriggered(bool b) {
    pref->resize_on_load = b;
    msgOSD(tr("Resize on load %1").arg(b ? tr("set") : tr("cleared")));
}

void TMainWindow::updateWindowSizeMenu() {

    windowSizeGroup->update();
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

void TMainWindow::updateTransformMenu() {

    flipAct->setChecked(player->mset.flip);
    mirrorAct->setChecked(player->mset.mirror);
    rotateGroup->setChecked(player->mset.rotate);
}

void TMainWindow::updateFilters() {

    filterGroup->updateFilters();
    denoiseGroup->setChecked(player->mset.current_denoiser);
    sharpenGroup->setChecked(player->mset.current_unsharp);
}

// Slot called when media settings reset or loaded
void TMainWindow::onMediaSettingsChanged() {
    WZTRACE("");

    displayInOutPoints();
    updateVideoEqualizer();

    Settings::TMediaSettings* mset = &player->mset;
    colorSpaceGroup->setChecked(mset->color_space);
    deinterlaceGroup->setChecked(mset->current_deinterlacer);
    updateTransformMenu();
    updateFilters();

    updateAudioEqualizer();
    stereoGroup->setChecked(mset->stereo_mode);
    audioChannelGroup->setChecked(mset->audio_use_channels);
    volnormAct->setChecked(mset->volnorm_filter);
    extrastereoAct->setChecked(mset->extrastereo_filter);
    karaokeAct->setChecked(mset->karaoke_filter);

    closedCaptionsGroup->setChecked(mset->closed_caption_channel);
    subFPSGroup->setChecked(mset->external_subtitles_fps);
}

void TMainWindow::updateTitle() {

    QString title = playlist->getPlayingTitle(true);
    // setWindowCaption == setWindowTitle() and show it to TMainWindowTray
    setWindowCaption(title + (title.isEmpty() ? "" : " - ")
                     + TConfig::PROGRAM_NAME);

    if (propertiesDialog
            && propertiesDialog->isVisible()
            // onMediaStartedPlaying() will handle these 2 when playing starts
            // TODO: not on error...
            && player->state() != Player::STATE_LOADING
            && player->state() != Player::STATE_RESTARTING) {
        propertiesDialog->showInfo(title);
    }
}

void TMainWindow::onNewMediaStartedPlaying() {
    WZTRACE("");

    enterFullscreenOnPlay();
    pref->addRecent(player->mdat.filename,
                    playlist->getPlayingTitle(false, false));
}

void TMainWindow::onMediaStartedPlaying() {
    WZTRACE("");

    if (propertiesDialog && propertiesDialog->isVisible()) {
        setFilePropertiesData();
    }
    runActionsLater(pref->actions_to_run, true);
}

void TMainWindow::onPlaylistFinished() {
    WZDEBUG("");

    exitFullscreen();

    // Handle "Close on end of playlist" option
    if (optionCloseOnFinish != 0) {
        if ((optionCloseOnFinish == 1) || (pref->close_on_finish)) {
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
            tr("%1 error").arg(pref->playerName()),
            s + "\n"
            + tr("See log for details."),
            QMessageBox::Ok);
        busy = false;
    }
}

void TMainWindow::onStateChanged(Player::TState state) {
    WZTRACE("New state " + player->stateToString());

    enableActions();
    autoHideTimer->setAutoHideMouse(state == Player::STATE_PLAYING);
    switch (state) {
        case Player::STATE_STOPPED:
            // Check pending actions
            runActionsLater("", true);
            msg(tr("Ready"));
            break;
        case Player::STATE_PLAYING:
            msg(tr("Playing %1").arg(playlist->getPlayingTitle()));
            break;
        case Player::STATE_PAUSED:
            msg(tr("Paused"));
            break;

        // Messages done by player:
        case Player::STATE_STOPPING:
             break;
        case Player::STATE_RESTARTING:
             break;
        case Player::STATE_LOADING:
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
        case 4: return playNextAct;
        case 5: return seekBackFrameAct;
        case 6: return seekBack1Act;
        case 7: return seekBack2Act;
        case 8: return seekBack3Act;
        case 9: return playPrevAct;
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

void TMainWindow::closeEvent(QCloseEvent* e)  {
    WZDEBUG("");

    if (playlist->maybeSave() && favlist->maybeSave()) {
        readyAction->setChecked(false);
        // TODO: When fullscreen the window size will not yet be updated by the
        // time it is saved by saveSettings, so block saving it.
        // Maybe use update() when comming from fs?
        save_size = !pref->fullscreen;
        exitFullscreen();
        hide();
        stop();
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

void TMainWindow::enableSubtitleActions() {

    bool e = player->statePOP()
            && (player->mdat.subs.count() > 0
                || player->mset.closed_caption_channel > 0);
    decSubPosAct->setEnabled(e);
    incSubPosAct->setEnabled(e);
    incSubScaleAct->setEnabled(e);
    decSubScaleAct->setEnabled(e);

    decSubDelayAct->setEnabled(e);
    incSubDelayAct->setEnabled(e);
    subDelayAct->setEnabled(e);

    incSubStepAct->setEnabled(e);
    decSubStepAct->setEnabled(e);

    seekNextSubAct->setEnabled(e && pref->isMPV());
    seekPrevSubAct->setEnabled(e && pref->isMPV());

    int count = player->mdat.subs.count();
    e = player->statePOP() && count > 0;
    nextSubtitleAct->setEnabled(e && count > 1);
    // Individual subs in use by the other track already disabled by
    // updateSubtitleTracks
    subtitleTrackGroup->setEnabled(e);
    secondarySubtitleTrackGroup->setEnabled(e);

    closedCaptionsGroup->setEnabled(player->statePOP() && player->hasVideo());

    // useForcedSubsOnlyAct always enabled

    e = player->statePOP() && player->hasVideo();
    loadSubsAct->setEnabled(e);
    e = e && player->hasExternalSubs();
    unloadSubsAct->setEnabled(e);
    subFPSGroup->setEnabled(e);

    // useCustomSubStyleAct always enabled
}

void TMainWindow::enablePlayPauseStop() {

    Player::TState ps = player->state();
    stopAct->setEnabled(ps == Player::STATE_PLAYING
                        || ps == Player::STATE_PAUSED
                        || ps == Player::STATE_RESTARTING
                        || ps == Player::STATE_LOADING
                        || playlist->isBusy()
                        || favlist->isBusy());

    playAct->setEnabled(!player->mdat.filename.isEmpty()
                        || playlist->hasPlayableItems());

    pauseAct->setEnabled(ps == Player::STATE_PLAYING);

    if (ps == Player::STATE_STOPPING) {
        playPauseAct->setTextAndTip(tr("Stopping player..."));
        playPauseAct->setIcon(iconProvider.iconStopping);
        playPauseAct->setEnabled(false);
        playPauseStopAct->setTextAndTip(tr("Stopping player..."));
        playPauseStopAct->setIcon(iconProvider.iconStopping);
        playPauseStopAct->setEnabled(false);
        WZTRACE("Disabled in state stopping");
    } else if (ps == Player::STATE_PLAYING) {
        playPauseAct->setTextAndTip(tr("Pause"));
        playPauseAct->setEnabled(true);
        playPauseStopAct->setTextAndTip(tr("Pause"));
        playPauseStopAct->setEnabled(true);
        if (playlist->isBusy() || favlist->isBusy()) {
            playPauseAct->setIcon(iconProvider.iconLoading);
            playPauseStopAct->setIcon(iconProvider.iconLoading);
            WZTRACE("Enabled pause in state playing while busy");
        } else {
            playPauseAct->setIcon(iconProvider.pauseIcon);
            playPauseStopAct->setIcon(iconProvider.pauseIcon);
            WZTRACE("Enabled pause in state playing");
        }
    } else if (ps == Player::STATE_PAUSED) {
        playPauseAct->setTextAndTip(tr("Play"));
        playPauseAct->setEnabled(true);
        playPauseStopAct->setTextAndTip(tr("Play"));
        playPauseStopAct->setEnabled(true);
        if (playlist->isBusy() || favlist->isBusy()) {
            playPauseAct->setIcon(iconProvider.iconLoading);
            playPauseStopAct->setIcon(iconProvider.iconLoading);
            WZTRACE("Enabled play in state paused while busy");
        } else {
            playPauseAct->setIcon(iconProvider.playIcon);
            playPauseStopAct->setIcon(iconProvider.playIcon);
            WZTRACE("Enabled play in state paused");
        }
    } else {
        QString s = player->stateToString().toLower();
        if (ps == Player::STATE_RESTARTING || ps == Player::STATE_LOADING) {
            playPauseAct->setTextAndTip(player->stateToString());
            playPauseAct->setIcon(iconProvider.iconLoading);
            playPauseAct->setEnabled(false);
            playPauseStopAct->setTextAndTip(tr("Stop %1").arg(s));
            playPauseStopAct->setIcon(iconProvider.iconStopping);
            playPauseStopAct->setEnabled(true);
            WZTRACE("Enabled stop in state " + s);
        } else if (ps == Player::STATE_STOPPED) {
            playPauseAct->setTextAndTip(tr("Play"));
            playPauseAct->setIcon(iconProvider.playIcon);
            playPauseStopAct->setTextAndTip(tr("Play"));
            playPauseStopAct->setIcon(iconProvider.playIcon);
            bool e = !player->mdat.filename.isEmpty()
                    || playlist->hasPlayableItems();
            playPauseAct->setEnabled(e);
            playPauseStopAct->setEnabled(e);
            WZTRACE(QString("%1 play in state stopped")
                    .arg(e ? "Enabled" : "Disabled"));
        }
    }

    bool e = player->stateReady() && playlist->hasPlayableItems();
    playNextAct->setEnabled(e);
    playPrevAct->setEnabled(e);
} // enablePlayPauseStop()

void TMainWindow::enableActions() {
    WZTRACE("State " + player->stateToString());

    bool enable = player->statePOP();

    // Time slider
    timeslider_action->enable(enable);

    // Play lists
    playlist->enableActions();
    favlist->enableActions();

    // File menu
    // Save thumbnail action
    saveThumbnailAct->setEnabled(
                player->mdat.selected_type == TMediaData::TYPE_FILE
                && !player->mdat.filename.isEmpty()
                && player->hasVideo());


    // Play menu
    enablePlayPauseStop();

    // Seek forward
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
    // Repeat and shuffle always enabled


    // Video menu
    // Window size menu
    bool enableVideo = enable && player->hasVideo();
    windowSizeGroup->setEnabled(enableVideo);
    doubleSizeAct->setEnabled(enableVideo);
    optimizeSizeAct->setEnabled(enableVideo);
    // Resize on load always enabled

    // Zoom and pan
    zoomAndPanGroup->setEnabled(enableVideo);

    // Video equalizer
    equalizerAct->setEnabled(enableVideo);
    resetVideoEqualizerAct->setEnabled(enableVideo);
    decContrastAct->setEnabled(enableVideo);
    incContrastAct->setEnabled(enableVideo);
    decBrightnessAct->setEnabled(enableVideo);
    incBrightnessAct->setEnabled(enableVideo);
    decHueAct->setEnabled(enableVideo);
    incHueAct->setEnabled(enableVideo);
    decSaturationAct->setEnabled(enableVideo);
    incSaturationAct->setEnabled(enableVideo);
    decGammaAct->setEnabled(enableVideo);
    incGammaAct->setEnabled(enableVideo);

    // Color space
    colorSpaceGroup->setEnabled(enableVideo && Settings::pref->isMPV());
    colorSpaceGroup->setChecked(player->mset.color_space);

    // Deinterlace
    bool enableVideoFilters = enableVideo && player->videoFiltersEnabled();
    deinterlaceGroup->setEnabled(enableVideoFilters);
    toggleDeinterlaceAct->setEnabled(enableVideoFilters);

    // Transform
    flipAct->setEnabled(enableVideoFilters);
    mirrorAct->setEnabled(enableVideoFilters);
    rotateGroup->setEnabled(enableVideoFilters);

    // Video filters
    filterGroup->setEnabled(enableVideoFilters);
    filterGroup->addLetterboxAct->setEnabled(enableVideoFilters
                                             && pref->isMPlayer());
    denoiseGroup->setEnabled(enableVideoFilters);
    sharpenGroup->setEnabled(enableVideoFilters);

    // Stereo 3D
    stereo3DAct->setEnabled(enableVideoFilters);

    // Video tracks
    nextVideoTrackAct->setEnabled(enableVideo
                                  && player->mdat.videos.count() > 1);
    videoTrackGroup->setEnabled(enableVideo);

    // ScreenShots
    bool enableScreenShots = enableVideo
            && !pref->screenshot_directory.isEmpty();
    screenshotAct->setEnabled(enableScreenShots && pref->use_screenshot);
    screenshotsAct->setEnabled(enableScreenShots && pref->use_screenshot);
    capturingAct->setEnabled(enableScreenShots);


    // Audio menu
    // Maybe global settings
    enable = pref->global_volume || (player->statePOP() && player->hasAudio());
    muteAct->setEnabled(enable);
    decVolumeAct->setEnabled(enable);
    incVolumeAct->setEnabled(enable);

    // Settings only stored in mset
    enable = player->statePOP() && player->hasAudio();
    decAudioDelayAct->setEnabled(enable);
    incAudioDelayAct->setEnabled(enable);
    audioDelayAct->setEnabled(enable);

    // Equalizer can be global
    bool enableEqualizer = pref->use_audio_equalizer
                           && (pref->global_audio_equalizer || enable);
    audioEqualizerAct->setEnabled(enableEqualizer);
    resetAudioEqualizerAct->setEnabled(enableEqualizer);

    // Stereo
    stereoGroup->setEnabled(enable);

    // Channels
    audioChannelGroup->setEnabled(enable);

    // Filters
    volnormAct->setEnabled(enable);
    extrastereoAct->setEnabled(enable);
    karaokeAct->setEnabled(enable && pref->isMPlayer());

    // Audio tracks
    nextAudioTrackAct->setEnabled(enable && player->mdat.audios.count() > 1);
    audioTrackGroup->setEnabled(enable);

    // Load/unload
    loadAudioAct->setEnabled(player->statePOP());
    unloadAudioAct->setEnabled(enable && player->mset.external_audio.count());


    // Subtitles menu
    enableSubtitleActions();


    // Browse menu
    enable = player->statePOP();
    bool enableChapters = enable && player->mdat.chapters.count() > 0;
    prevChapterAct->setEnabled(enableChapters);
    nextChapterAct->setEnabled(enableChapters);
    nextAngleAct->setEnabled(enable && player->mdat.angles > 1);

    // DVDNAV
    bool enableDVDNav = enable
            && player->mdat.detected_type == TMediaData::TYPE_DVDNAV;
    dvdnavUpAct->setEnabled(enableDVDNav);
    dvdnavDownAct->setEnabled(enableDVDNav);
    dvdnavLeftAct->setEnabled(enableDVDNav);
    dvdnavRightAct->setEnabled(enableDVDNav);

    dvdnavMenuAct->setEnabled(enableDVDNav);
    dvdnavPrevAct->setEnabled(enableDVDNav);
    dvdnavSelectAct->setEnabled(enableDVDNav);
    dvdnavMouseAct->setEnabled(enableDVDNav);


    // View menu
    // OSD
    osdShowFilenameAct->setEnabled(enableVideo);
    osdShowTimeAct->setEnabled(enableVideo);
    // Other OSD actions always enabled

    viewPropertiesAct->setEnabled(!player->mdat.filename.isEmpty());
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
    } else {
        playlist->openDisc(TDiscName("vcd", pref->vcd_initial_title,
                                     pref->cdrom_device));
    }
}

void TMainWindow::openAudioCD() {
    WZDEBUG("");

    if (pref->cdrom_device.isEmpty()) {
        configureDiscDevices();
    } else {
        playlist->openDisc(TDiscName("cdda://"));
    }
}

void TMainWindow::openDVD() {
    WZDEBUG("");

    if (pref->dvd_device.isEmpty()) {
        configureDiscDevices();
    } else {
        playlist->openDisc(TDiscName(pref->dvd_device, pref->useDVDNAV()));
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
            playlist->openDisc(TDiscName(iso, pref->useDVDNAV()));
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
            playlist->openDisc(TDiscName(dir, pref->useDVDNAV()));
        }
    }
}

void TMainWindow::openBluRay() {
    WZDEBUG("");

    if (pref->bluray_device.isEmpty()) {
        configureDiscDevices();
    } else {
        playlist->openDisc(TDiscName("br", 0, pref->bluray_device));
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
            playlist->openDisc(TDiscName("br", 0, iso));
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
            playlist->openDisc(TDiscName("br", 0, dir));
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
            QString time =TWZTime::formatTimeStampMS(player->mset.current_ms);
            saveThumbnailToIni(canonical, time);

            // Remove cached thumbnails
            removeThumbnail(fn);
            removeThumbnail(canonical);
        }
    }
}

void TMainWindow::play() {
    WZD;

    if (favListDock->isActiveWindow()
            && !favListDock->visibleRegion().isEmpty()) {
        favlist->play();
    } else if (playlist->hasPlayableItems()) {
        playlist->play();
    } else if (!player->mdat.filename.isEmpty()) {
        player->play();
    } else if (favlist->hasPlayableItems()) {
        favlist->play();
    } else {
        msg(tr("Nothing to play"));
    }
}

void TMainWindow::playPauseStop() {
    WZDEBUG("State " + player->stateToString());

    if (player->state() == Player::STATE_RESTARTING
            || player->state() == Player::STATE_LOADING) {
        stop();
    } else {
        playlist->playPause();
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
        help_window = new THelpWindow(this, "help_window");
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

void TMainWindow::browseConfigFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(TPaths::configPath()));
}

void TMainWindow::browseDataFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(TPaths::dataPath()));
}

void TMainWindow::helpAbout() {

    TAbout d(this);
    d.exec();
}

void TMainWindow::showSeekToDialog() {

    TTimeDialog d(this);
    d.setWindowTitle(tr("Seek"));
    d.setMaximumTime((int) player->mdat.getDurationSec());
    d.setTime(qRound(player->mset.currentSec()));
    if (d.exec() == QDialog::Accepted) {
        player->seekSecond(d.time());
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
    fullscreenAct->setIcon(b ? iconProvider.fullscreenExitIcon
                             : iconProvider.fullscreenIcon);
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

    pref->beginGroup(objectName());
    pref->setValue("toolbars_state", saveState(TVersion::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didEnterFullscreen() {

    // Restore fullscreen state
    viewMenuBarAct->setChecked(fullscreen_menubar_visible);
    viewStatusBarAct->setChecked(fullscreen_statusbar_visible);

    pref->beginGroup(objectName());
    if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
                      TVersion::qtVersion())) {
        // First time there is no fullscreen toolbar state
        WZDEBUG("Fullscreen tool state not restored");
        playlistDock->hide();
        favListDock->hide();
        logDock->hide();
        toolbar->hide();
        toolbar2->hide();
        controlbar->show();
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

    pref->beginGroup(objectName());
    pref->setValue("toolbars_state_fullscreen",
                   saveState(TVersion::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didExitFullscreen() {

    viewMenuBarAct->setChecked(menubar_visible);
    viewStatusBarAct->setChecked(statusbar_visible);

    pref->beginGroup(objectName());
    if (!restoreState(pref->value("toolbars_state").toByteArray(),
                      TVersion::qtVersion())) {
        WZWARN("Failed to restore tool state");
        playlistDock->hide();
        favListDock->hide();
        logDock->hide();
        toolbar->show();
        toolbar2->show();
        controlbar->show();
    }
    pref->endGroup();

    // Update size when current video changed in fullscreen
    if (pref->resize_on_load
        && player->mdat.filename != first_fullscreen_filename
        && player->statePOP()) {
        // Set default zoom
        pref->size_factor = pref->initial_zoom_factor;
        // Needs delay for framesize to settle down...
        optimizeSizeTimer->logStart();
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
    WZD;

    if (!e->isAccepted() && e->mimeData()->hasUrls()) {
        if (e->proposedAction() & (Qt::CopyAction | Qt::LinkAction)) {
            e->acceptProposedAction();
            return;
        }
        if (e->possibleActions() & Qt::CopyAction) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
        if (e->possibleActions() & Qt::LinkAction) {
            e->setDropAction(Qt::LinkAction);
            e->accept();
            return;
        }
        // TODO: e->ignore()?
    }
    QMainWindow::dragEnterEvent(e);
}

void TMainWindow::dropEvent(QDropEvent *e) {
    WZD;

    if (!e->isAccepted() && e->mimeData()->hasUrls()) {
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

void TMainWindow::stop() {
    WZT;

    // Playlist stops player
    playlist->stop();
    favlist->stop();
}

void TMainWindow::setSizeFactor(double factor) {
    WZT << factor;

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
    WZTRACE(QString::number(percentage) + "%");

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

bool TMainWindow::dockNeedsResize(TDockWidget* dock,
                                  Qt::DockWidgetArea area) const {

    return !pref->fullscreen
            && !dock->isFloating()
            && area != Qt::NoDockWidgetArea
            && readyAction->isChecked()
            && (dock == logDock || area != logDock->getArea())
            && (dock == playlistDock || area != playlistDock->getArea())
            && (dock == favListDock || area != favListDock->getArea());
}

bool TMainWindow::haveDockedDocks() const {

    return (logDock->isVisible() && !logDock->isFloating())
            || (playlistDock->isVisible() && !playlistDock->isFloating())
            || (favListDock->isVisible() && !favListDock->isFloating());
}

void TMainWindow::hidePanel() {
    WZDEBUG("");

    // Exit from fullscreen
    if (pref->fullscreen) {
        setFullscreen(false);
        update();
    }

    if (playerWindow->isVisible()) {
        QSize s(width(), height() - playerWindow->height());
        playerWindow->hide();
        if (!haveDockedDocks()) {
            resize(s);
        }
    }
}

double TMainWindow::optimizeSize(double size) const {
    WZT << "Size in" << size;

    QSize res = playerWindow->resolution();
    if (res.width() <= 0 || res.height() <= 0) {
        return size;
    }
    QSize availableSize = TDesktop::availableSize(this);

    // Handle fullscreen
    if (pref->fullscreen) {
        size = (double) availableSize.width() / res.width();
        double height = (double) availableSize.height() / res.height();
        if (height < size) {
            size = height;
        }
        return size;
    }

    // Return current size for VO size change caused by TPlayer::setAspectRatio
    if (player->keepSize) {
        WZTRACE("Keep size requested");
        player->clearKeepSize();
        return pref->size_factor;
    }

    availableSize = availableSize
            - frameSize() + playerWindow->frameSize()
            - TPlayerWindow::frame();

    QSize video_size = res * size;

    // Limit size to perc of available space
    const double f = 0.75;

    // Adjust width
    double max = f * availableSize.width();
    if (video_size.width() > max) {
        size = max / res.width();
        video_size = res * size;
    }
    // Adjust height
    max = f * availableSize.height();
    if (video_size.height() > max) {
        size = max / res.height();
        video_size = res * size;
    }

    // Get 1/4 of available height
    double min = availableSize.height() / 4;
    if (video_size.height() < min) {
        if (size == 1.0) {
            return 2.0;
        }
        // Prefer min
        size = min / res.height();
    }

    // Round to predefined values
    int i = qRound(size * 100);
    if (i <= 0) {
        WZERROR("Selecting size 1 for invalid size");
        return 1;
    }
    if (i < 13) {
        // Hires
        return size;
    }
    if (i < 25) {
        // Can we scale up to 25%?
        video_size = res * (double) 0.25;
        if (video_size.width() > availableSize.width()
                || video_size.height() > availableSize.height()) {
            // No, return size
            return size;
        }
        // Scale up to 25%
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
    WZTRACE(QString("%1 x %2").arg(w).arg(h));

    if (w <= 0 || h <= 0) {
        // No video
        if (pref->hide_video_window_on_audio_files) {
            hidePanel();
        }
    } else {
        // Have video
        if (!playerWindow->isVisible()) {
            playerWindow->show();
        }

        // force_resize is only set for the first video when
        // pref->save_window_size_on_exit is not set.
        if (playerWindow->width() < 64 || playerWindow->height() < 48) {
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
            WZDEBUG(QString("Adjusted size factor to %1")
                    .arg(pref->size_factor));
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

void TMainWindow::resizeMainWindow(int w, int h, double size_factor,
                                   bool try_twice) {
    WZDEBUG(QString("Video size %1 x %2, size factor %3")
            .arg(w).arg(h).arg(pref->size_factor));

    QSize playerSize = QSize(w, h) * size_factor + playerWindow->frame();
    if (playerSize == playerWindow->size()) {
        WZTRACE("Player has requested size");
        return;
    }

    QSize new_size = size() + playerSize - playerWindow->size();
    resize(new_size);
    emit resizedMainWindow();

    if (playerWindow->size() != playerSize) {
        // Resizing the main window can change the height of the toolbars,
        // which will change the height of the panel during the resize.
        // Fix by resizing once again, using the new panel height.
        if (try_twice) {
            resizeMainWindow(w, h, size_factor, false);
        } else {
            WZDEBUG(QString("Resize failed. Panel size now %1 x %2."
                            " Wanted size %3 x %4")
                            .arg(playerWindow->size().width())
                            .arg(playerWindow->size().height())
                            .arg(playerSize.width())
                            .arg(playerSize.height()));
        }
    }
}

void TMainWindow::setStayOnTop(bool b) {

    bool stay_on_top = windowFlags() & Qt::WindowStaysOnTopHint;
    if (b == stay_on_top) {
        return;
    }
    WZTRACE(b);

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

// Slot called by action group TStayOnTopGroup
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

// Slot called by action toggle stay on top
void TMainWindow::toggleStayOnTop() {

    if (pref->stay_on_top == TPreferences::NeverOnTop)
        changeStayOnTop(TPreferences::AlwaysOnTop);
    else
        changeStayOnTop(TPreferences::NeverOnTop);
}

// Check stay on top for TPreferences::WhilePlayingOnTop
void TMainWindow::checkStayOnTop(Player::TState) {

    if (pref->stay_on_top != TPreferences::WhilePlayingOnTop
            || pref->fullscreen) {
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

#if defined(Q_OS_WIN)
bool TMainWindow::winEvent (MSG* m, long* result) {

    if (m->message == WM_SYSCOMMAND) {
        if (((m->wParam & 0xFFF0) == SC_SCREENSAVE)
            || ((m->wParam & 0xFFF0) == SC_MONITORPOWER)) {
            if (player->state() == Player::STATE_PLAYING
                && player->mdat.hasVideo()) {
                WZDEBUG("Not allowing screensaver");
                (*result) = 0;
                return true;
            }

            WZDEBUG("Allowing screensaver");
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
                WZDEBUG("Setting action '" + action_name + " to "
                        + QString::number(value));
                action->setChecked(value);
            } else {
                WZDEBUG("Triggering action '" + action_name + "'");
                action->trigger();
            }
        } else {
            WZWARN("Canceling disabled action '" + action_name + "'");
        }
    } else {
        WZWARN("Action '" + action_name + "' not found");
    }
}

void TMainWindow::postAction(const QString& actionName, bool hasArg, bool arg) {

    QAction* action = findChild<QAction*>(actionName);
    if (action) {
        if (hasArg) {
            if (!action->isCheckable()) {
                QMessageBox::warning(this, tr("Unexpected argument"),
                    tr("Got boolean argument %1 for action %2, which is not"
                       " a checkable action. Ignoring the action.")
                    .arg(arg).arg(actionName));
                return;
            }
            if (action->isChecked() == arg) {
                WZINFO(QString("Checkable action %1 already set to %2")
                       .arg(actionName).arg(arg));
                return;
            }
        }

        // Post to action slot trigger()
        WZINFO(QString("Posting action '%1'").arg(actionName));
        QTimer::singleShot(0, action, SLOT(trigger()));
    } else {
        WZWARN("Action '" + actionName + "' not found");
        QMessageBox::warning(this, tr("Action not found"),
                             tr("Action \"%1\" not found.").arg(actionName));
    }
}

void TMainWindow::checkPendingActions() {
    WZT;

   QStringList actionList = pending_actions.split(" ", QString::SkipEmptyParts);
   if (actionList.count()) {
        const QString& action = actionList.takeFirst();

        // Set arg if the next word is a boolean
        QString arg;
        bool value;
        if (actionList.count()) {
            arg = actionList.at(0).toLower();
            if (arg == "true") {
                value = true;
                actionList.takeFirst();
            } else if (arg == "false") {
                value = false;
                actionList.takeFirst();
            } else {
                arg = "";
            }
        }

        pending_actions = actionList.join(" ");
        postAction(action, !arg.isEmpty(), value);
        if (!pending_actions.isEmpty()) {
            QTimer::singleShot(0, this, SLOT(checkPendingActions()));
        }
    }
}

void TMainWindow::runActionsLater(const QString& actions,
                                  bool postCheck,
                                  bool prepend) {
    WZD << "Scheduling actions" << actions;

    if (!actions.isEmpty()) {
        if (pending_actions.isEmpty()) {
            pending_actions = actions;
        } else if (prepend) {
            pending_actions = actions + " " + pending_actions;
        } else {
            pending_actions += " " + actions;
        }
        pending_actions = pending_actions.simplified();
    }

    if (postCheck && !pending_actions.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(checkPendingActions()));
    }
}

void TMainWindow::onReceivedMessage(const QString& msg) {
    WZD << msg;

    if (msg == "hello") {
        return;
    }
    int pos = msg.indexOf(' ');
    if (pos >= 0) {
        QString command = msg.left(pos);
        QString arg = msg.mid(pos + 1);
        if (command == "send_actions") {
            runActionsLater(arg, true);
            return;
        }
        if (command == "open_file") {
            playlist->open(arg);
            return;
        }
        if (command == "open_files") {
            playlist->openFiles(arg.split(" <<sep>> "));
            return;
        }
        if (command == "add_to_playlist") {
            playlist->getPlaylistWidget()->addFiles(arg.split(" <<sep>> "));
            return;
        }
        if (command == "media_title") {
            QStringList list = arg.split(" <<sep>> ");
            player->addForcedTitle(list[0], list[1]);
            return;
        }
        if (command == "load_sub") {
            player->setInitialSubtitle(arg);
            if (player->statePOP()) {
                player->loadSub(arg);
            }
            return;
        }
    }
    WZERROR(QString("Received malformed message '%1'").arg(msg));
}

void TMainWindow::leftClickFunction() {
    WZTRACE("");

    if (player->mdat.detected_type == TMediaData::TYPE_DVDNAV
        && playerWindow->getVideoWindow()->underMouse()) {
        player->dvdnavMouse();
    } else if (!pref->mouse_left_click_function.isEmpty()) {
        processAction(pref->mouse_left_click_function);
    }
}

void TMainWindow::rightClickFunction() {
    WZTRACE("");

    if (!pref->mouse_right_click_function.isEmpty()) {
        processAction(pref->mouse_right_click_function);
    }
}

void TMainWindow::doubleClickFunction() {
    WZTRACE("");

    if (!pref->mouse_double_click_function.isEmpty()) {
        processAction(pref->mouse_double_click_function);
    }
}

void TMainWindow::middleClickFunction() {
    WZTRACE("");

    if (!pref->mouse_middle_click_function.isEmpty()) {
        processAction(pref->mouse_middle_click_function);
    }
}

void TMainWindow::xbutton1ClickFunction() {
    WZTRACE("");

    if (!pref->mouse_xbutton1_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton1_click_function);
    }
}

void TMainWindow::xbutton2ClickFunction() {
    WZTRACE("");

    if (!pref->mouse_xbutton2_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton2_click_function);
    }
}

void TMainWindow::wheelUpFunc(Settings::TPreferences::TWheelFunction function) {
    WZDEBUGOBJ("");

    using namespace Settings;

    if (function == TPreferences::DoNothing) {
        function = (TPreferences::TWheelFunction) pref->wheel_function;
    }
    switch (function) {
        case TPreferences::Volume: player->incVolume(); break;
        case TPreferences::Zoom: player->incZoom(); break;
        case TPreferences::Seeking:
            pref->wheel_function_seeking_reverse
                    ? player->rewind(pref->seeking4)
                    : player->forward(pref->seeking4);
            break;
        case TPreferences::ChangeSpeed : player->incSpeed10(); break;
        case TPreferences::DoNothing: break;
    }
}

void TMainWindow::wheelUpSeeking() {
    wheelUpFunc(Settings::TPreferences::Seeking);
}

void TMainWindow::wheelUp() {
    wheelUpFunc(Settings::TPreferences::DoNothing);
}

void TMainWindow::wheelDownFunc(Settings::TPreferences::TWheelFunction function) {
    WZDEBUGOBJ("");

    if (function == Settings::TPreferences::DoNothing) {
        function = (Settings::TPreferences::TWheelFunction)
                   Settings::pref->wheel_function;
    }
    switch (function) {
        case Settings::TPreferences::Volume: player->decVolume(); break;
        case Settings::TPreferences::Zoom: player->decZoom(); break;
        case Settings::TPreferences::Seeking:
            Settings::pref->wheel_function_seeking_reverse
                    ? player->forward(Settings::pref->seeking4)
                    : player->rewind(Settings::pref->seeking4);
            break;
        case Settings::TPreferences::ChangeSpeed: player->decSpeed10(); break;
        default : {} // do nothing
    }
}

void TMainWindow::wheelDownSeeking() {
    wheelDownFunc(Settings::TPreferences::Seeking);
}

void TMainWindow::wheelDown() {
    wheelDownFunc(Settings::TPreferences::DoNothing);
}

void TMainWindow::nextWheelFunction() {

    int a = Settings::pref->wheel_function;

    bool done = false;
    if (((int) Settings::pref->wheel_function_cycle) == 0) {
        return;
    }
    while(!done){
        // get next a
        a = a * 2;
        if (a == 32) {
            a = 2;
        }
        // See if we are done
        if (Settings::pref->wheel_function_cycle.testFlag(
                (Settings::TPreferences::TWheelFunction)a)) {
            done = true;
        }
    }
    Settings::pref->wheel_function = a;
    QString m = "";
    switch(a){
    case Settings::TPreferences::Seeking:
        m = tr("Mouse wheel seeks now");
        break;
    case Settings::TPreferences::Volume:
        m = tr("Mouse wheel changes volume now");
        break;
    case Settings::TPreferences::Zoom:
        m = tr("Mouse wheel changes zoom level now");
        break;
    case Settings::TPreferences::ChangeSpeed:
        m = tr("Mouse wheel changes speed now");
        break;
    }
    Gui::msgOSD(m);
}

} // namespace Gui

#include "moc_mainwindow.cpp"

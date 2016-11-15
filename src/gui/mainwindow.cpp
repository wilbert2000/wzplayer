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

#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QCursor>
#include <QTimer>
#include <QStyle>
#include <QRegExp>
#include <QStatusBar>
#include <QActionGroup>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QNetworkProxy>

#include "player/player.h"
#include "player/process/exitmsg.h"
#include "version.h"
#include "gui/desktop.h"
#include "discname.h"
#include "extensions.h"
#include "colorutils.h"
#include "images.h"
#include "helper.h"
#include "mediadata.h"
#include "gui/playerwindow.h"
#include "clhelp.h"
#include "gui/filedialog.h"

#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"

#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "gui/action/timeslider.h"
#include "gui/action/widgetactions.h"
#include "gui/action/actionseditor.h"
#include "gui/action/editabletoolbar.h"
#include "gui/action/menu.h"
#include "gui/action/menufile.h"
#include "gui/action/menuplay.h"
#include "gui/action/menuvideo.h"
#include "gui/action/menuaudio.h"
#include "gui/action/menusubtitle.h"
#include "gui/action/menubrowse.h"
#include "gui/action/menuwindow.h"
#include "gui/action/menuhelp.h"

#include "gui/msg.h"
#include "gui/logwindow.h"
#include "gui/helpwindow.h"
#include "gui/playlist/playlist.h"
#include "gui/autohidetimer.h"
#include "gui/filepropertiesdialog.h"
#include "gui/inputdvddirectory.h"
#include "gui/about.h"
#include "gui/inputurl.h"
#include "gui/timedialog.h"
#include "gui/playlist/playlist.h"
#include "gui/videoequalizer.h"
#include "gui/eqslider.h"
#include "gui/audioequalizer.h"
#include "gui/stereo3ddialog.h"
#include "gui/action/tvlist.h"
#include "gui/updatechecker.h"

#include "gui/pref/dialog.h"
#include "gui/pref/interface.h"
#include "gui/pref/input.h"
#include "gui/pref/advanced.h"
#include "app.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {

using namespace Action;


TMainWindow::TMainWindow() :
    QMainWindow(),
    debug(logger()),
    toolbar_menu(0),
    help_window(0),
    pref_dialog(0),
    file_properties_dialog(0),
    switching_to_fullscreen(false),
    menubar_visible(true),
    statusbar_visible(true),
    fullscreen_menubar_visible(false),
    fullscreen_statusbar_visible(true),
    arg_close_on_finish(-1),
    ignore_show_hide_events(false),
    save_size(true),
    center_window(false),
    update_checker(0) {

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(TConfig::PROGRAM_NAME);
    setAcceptDrops(true);

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

    // Setup move window timer merging multiple move requests into one
    move_window_timer.setSingleShot(true);
    move_window_timer.setInterval(0);
    connect(&move_window_timer, SIGNAL(timeout()),
            this, SLOT(moveWindowMerged()));

    // Create objects:
    log_window = new TLogWindow(this);

    createPanel();
    setCentralWidget(panel);

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

    retranslateStrings();
}

TMainWindow::~TMainWindow() {

    msgSlot = 0;
    setMessageHandler(0);
}

QString TMainWindow::settingsGroupName() {
    return "mainwindow";
}

void TMainWindow::createStatusBar() {
    logger()->debug("createStatusBar");

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
    time_label->setText("00:00 / 00:00");
    statusBar()->addPermanentWidget(time_label, 0);
    connect(this, SIGNAL(timeChanged(QString)),
            time_label, SLOT(setText(QString)));

}

void TMainWindow::createPanel() {
    logger()->debug("createPanel");

    panel = new QWidget(this);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    panel->setMinimumSize(QSize(1, 1));
    panel->setFocusPolicy(Qt::StrongFocus);
}

void TMainWindow::createPlayerWindow() {
    logger()->debug("createPlayerWindow");

    playerwindow = new TPlayerWindow(panel);
    playerwindow->setObjectName("playerwindow");

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(playerwindow);
    panel->setLayout(layout);

    // Connect player window mouse events
    connect(playerwindow, SIGNAL(doubleClicked()),
            this, SLOT(doubleClickFunction()));
    connect(playerwindow, SIGNAL(leftClicked()),
            this, SLOT(leftClickFunction()));
    connect(playerwindow, SIGNAL(rightClicked()),
            this, SLOT(rightClickFunction()));
    connect(playerwindow, SIGNAL(middleClicked()),
            this, SLOT(middleClickFunction()));
    connect(playerwindow, SIGNAL(xbutton1Clicked()),
            this, SLOT(xbutton1ClickFunction()));
    connect(playerwindow, SIGNAL(xbutton2Clicked()),
            this, SLOT(xbutton2ClickFunction()));
    connect(playerwindow, SIGNAL(moveWindow(QPoint)),
            this, SLOT(moveWindow(QPoint)));

    connect(playerwindow, SIGNAL(videoOutChanged(const QSize&)),
            this, SLOT(displayVideoInfo()), Qt::QueuedConnection);
}

void TMainWindow::createPlayer() {
    logger()->debug("createPlayer");

    new Player::TPlayer(this, playerwindow);

    connect(player, SIGNAL(positionChanged(double)),
            this, SLOT(onPositionChanged(double)));
    connect(player, SIGNAL(durationChanged(double)),
            this, SLOT(onDurationChanged(double)));

    connect(player, SIGNAL(stateChanged(Player::TState)),
            this, SLOT(onStateChanged(Player::TState)));
    connect(player, SIGNAL(stateChanged(Player::TState)),
            this, SLOT(checkStayOnTop(Player::TState)), Qt::QueuedConnection);

    connect(player, SIGNAL(mediaSettingsChanged()),
            this, SLOT(onMediaSettingsChanged()));
    connect(player, SIGNAL(videoOutResolutionChanged(int, int)),
            this, SLOT(onVideoOutResolutionChanged(int,int)));

    connect(player, SIGNAL(newMediaStartedPlaying()),
            this, SLOT(onNewMediaStartedPlaying()), Qt::QueuedConnection);

    connect(player, SIGNAL(mediaInfoChanged()),
            this, SLOT(onMediaInfoChanged()));

    connect(player, SIGNAL(mediaStopped()),
            this, SLOT(exitFullscreenOnStop()));

    connect(player, SIGNAL(playerError(int)),
            this, SLOT(onPlayerError(int)),
            Qt::QueuedConnection);

    connect(player, SIGNAL(InOutPointsChanged()),
            this, SLOT(displayInOutPoints()));
    connect(player, SIGNAL(mediaSettingsChanged()),
            this, SLOT(displayInOutPoints()));
}

void TMainWindow::createPlaylist() {
    logger()->debug("createPlaylist");

    playlist = new Playlist::TPlaylist(this);
    connect(playlist, SIGNAL(playlistEnded()),
            this, SLOT(playlistHasFinished()));
}

void TMainWindow::createVideoEqualizer() {

    video_equalizer = new TVideoEqualizer(this);
    video_equalizer->setBySoftware(pref->use_soft_video_eq);

    connect(video_equalizer, SIGNAL(contrastChanged(int)),
            player, SLOT(setContrast(int)));
    connect(video_equalizer, SIGNAL(brightnessChanged(int)),
            player, SLOT(setBrightness(int)));
    connect(video_equalizer, SIGNAL(hueChanged(int)),
            player, SLOT(setHue(int)));
    connect(video_equalizer, SIGNAL(saturationChanged(int)),
            player, SLOT(setSaturation(int)));
    connect(video_equalizer, SIGNAL(gammaChanged(int)),
            player, SLOT(setGamma(int)));

    connect(video_equalizer, SIGNAL(requestToChangeDefaultValues()),
            this, SLOT(setDefaultValuesFromVideoEqualizer()));
    connect(video_equalizer, SIGNAL(bySoftwareChanged(bool)),
            this, SLOT(changeVideoEqualizerBySoftware(bool)));

    connect(player, SIGNAL(videoEqualizerNeedsUpdate()),
            this, SLOT(updateVideoEqualizer()));
}

void TMainWindow::createAudioEqualizer() {

    audio_equalizer = new TAudioEqualizer(this);

    connect(audio_equalizer->eq[0], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq0(int)));
    connect(audio_equalizer->eq[1], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq1(int)));
    connect(audio_equalizer->eq[2], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq2(int)));
    connect(audio_equalizer->eq[3], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq3(int)));
    connect(audio_equalizer->eq[4], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq4(int)));
    connect(audio_equalizer->eq[5], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq5(int)));
    connect(audio_equalizer->eq[6], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq6(int)));
    connect(audio_equalizer->eq[7], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq7(int)));
    connect(audio_equalizer->eq[8], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq8(int)));
    connect(audio_equalizer->eq[9], SIGNAL(valueChanged(int)),
            player, SLOT(setAudioEq9(int)));

    connect(audio_equalizer,
            SIGNAL(applyClicked(const Settings::TAudioEqualizerList&)),
            player, SLOT(setAudioAudioEqualizerRestart(
                             const Settings::TAudioEqualizerList&)));
    connect(audio_equalizer,
            SIGNAL(valuesChanged(const Settings::TAudioEqualizerList&)),
            player,
            SLOT(setAudioEqualizer(const Settings::TAudioEqualizerList&)));
}

void TMainWindow::createActions() {
    logger()->debug("createActions");

    showContextMenuAct = new Action::TAction(this, "show_context_menu",
                                             tr("Show context menu"));
    connect(showContextMenuAct, SIGNAL(triggered()),
            this, SLOT(showContextMenu()));

    nextWheelFunctionAct = new TAction(this, "next_wheel_function",
                                       tr("Next wheel function"), 0, Qt::Key_W);
    connect(nextWheelFunctionAct, SIGNAL(triggered()),
            player, SLOT(nextWheelFunction()));

    // Time slider
    timeslider_action = new TTimeSliderAction(this);
    timeslider_action->setObjectName("timeslider_action");

    connect(player, SIGNAL(positionChanged(double)),
            timeslider_action, SLOT(setPosition(double)));
    connect(player, SIGNAL(durationChanged(double)),
            timeslider_action, SLOT(setDuration(double)));

    connect(timeslider_action, SIGNAL(positionChanged(double)),
            player, SLOT(seekTime(double)));
    connect(timeslider_action, SIGNAL(percentageChanged(double)),
            player, SLOT(seekPercentage(double)));
    connect(timeslider_action, SIGNAL(dragPositionChanged(double)),
            this, SLOT(onDragPositionChanged(double)));

    connect(timeslider_action,
            SIGNAL(wheelUp(Settings::TPreferences::TWheelFunction)),
            player, SLOT(wheelUp(Settings::TPreferences::TWheelFunction)));
    connect(timeslider_action,
            SIGNAL(wheelDown(Settings::TPreferences::TWheelFunction)),
            player, SLOT(wheelDown(Settings::TPreferences::TWheelFunction)));

    // Volume slider action
    volumeslider_action = new TVolumeSliderAction(this, player->getVolume());
    volumeslider_action->setObjectName("volumeslider_action");
    connect(volumeslider_action, SIGNAL(valueChanged(int)),
            player, SLOT(setVolume(int)));
    connect(player, SIGNAL(volumeChanged(int)),
            volumeslider_action, SLOT(setValue(int)));

    // Menu bar
    viewMenuBarAct = new TAction(this, "toggle_menubar", tr("Me&nu bar"),
                                 "", Qt::Key_F2);
    viewMenuBarAct->setCheckable(true);
    viewMenuBarAct->setChecked(true);
    connect(viewMenuBarAct, SIGNAL(toggled(bool)),
            menuBar(), SLOT(setVisible(bool)));

    // Toolbars
    editToolbarAct = new TAction(this, "edit_toolbar1",
                                 tr("Edit main &toolbar..."));
    editToolbar2Act = new TAction(this, "edit_toolbar2",
                                  tr("Edit extra t&oolbar..."));

    // Control bar
    editControlBarAct = new TAction(this, "edit_controlbar",
                                    tr("Edit control &bar.."));

    // Status bar
    viewStatusBarAct = new TAction(this, "toggle_statusbar", tr("&Status bar"),
                                   "", Qt::Key_F7);
    viewStatusBarAct->setCheckable(true);
    viewStatusBarAct->setChecked(true);
    connect(viewStatusBarAct, SIGNAL(toggled(bool)),
            statusBar(), SLOT(setVisible(bool)));

    viewVideoInfoAct = new Action::TAction(this, "toggle_video_info",
                                           tr("&Video info"));
    viewVideoInfoAct->setCheckable(true);
    viewVideoInfoAct->setChecked(true);
    connect(viewVideoInfoAct, SIGNAL(toggled(bool)),
            video_info_label, SLOT(setVisible(bool)));

    viewInOutPointsAct = new Action::TAction(this, "toggle_in_out_points",
                                             tr("&In-out points"));
    viewInOutPointsAct->setCheckable(true);
    viewInOutPointsAct->setChecked(true);
    connect(viewInOutPointsAct, SIGNAL(toggled(bool)),
            in_out_points_label, SLOT(setVisible(bool)));

    viewVideoTimeAct = new Action::TAction(this, "toggle_video_time",
                                           tr("&Video time"));
    viewVideoTimeAct->setCheckable(true);
    viewVideoTimeAct->setChecked(true);
    connect(viewVideoTimeAct, SIGNAL(toggled(bool)),
            time_label, SLOT(setVisible(bool)));

    viewFramesAct = new Action::TAction(this, "toggle_frames", tr("&Frames"));
    viewFramesAct->setCheckable(true);
    viewFramesAct->setChecked(false);
    connect(viewFramesAct, SIGNAL(toggled(bool)),
            this, SLOT(displayFrames(bool)));
} // createActions

void TMainWindow::createMenus() {
    logger()->debug("createMenus");

    // MENUS
    fileMenu = new TMenuFile(this);
    menuBar()->addMenu(fileMenu);
    playMenu = new TMenuPlay(this, playlist);
    menuBar()->addMenu(playMenu);
    videoMenu = new TMenuVideo(this, playerwindow, video_equalizer);
    menuBar()->addMenu(videoMenu);
    audioMenu = new TMenuAudio(this, audio_equalizer);
    menuBar()->addMenu(audioMenu);
    subtitleMenu = new TMenuSubtitle(this);
    menuBar()->addMenu(subtitleMenu);
    browseMenu = new TMenuBrowse(this);
    menuBar()->addMenu(browseMenu);

    // statusbar_menu added to toolbar_menu by createToolbarMenu()
    statusbar_menu = new QMenu(this);
    statusbar_menu->addAction(viewVideoInfoAct);
    statusbar_menu->addAction(viewInOutPointsAct);
    statusbar_menu->addAction(viewVideoTimeAct);
    statusbar_menu->addAction(viewFramesAct);

    toolbar_menu = createToolbarMenu();

    windowMenu = new TMenuWindow(this, toolbar_menu, playlist, log_window);
    menuBar()->addMenu(windowMenu);
    auto_hide_timer->add(windowMenu->findChild<TAction*>("show_playlist"),
                         playlist);

    helpMenu = new TMenuHelp(this);
    menuBar()->addMenu(helpMenu);

    // Popup menu
    popup = new QMenu(this);
    popup->addMenu(fileMenu);
    popup->addMenu(playMenu);
    popup->addMenu(videoMenu);
    popup->addMenu(audioMenu);
    popup->addMenu(subtitleMenu);
    popup->addMenu(browseMenu);
    popup->addMenu(windowMenu);
} // createMenus()

QMenu* TMainWindow::createToolbarMenu() {
    logger()->debug("createToolbarMenu");

    // Use name "toolbar_menu" only for first
    QString name = toolbar_menu ? "" : "toolbar_menu";
    QMenu* menu = new TMenu(this, this, name, tr("&Toolbars"), "toolbars");

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

    connect(menu, SIGNAL(aboutToShow()), auto_hide_timer, SLOT(disable()));
    connect(menu, SIGNAL(aboutToHide()), auto_hide_timer, SLOT(enable()));

    return menu;
} // createToolbarMenu

// Called by main window to show context popup.
// Main window takes ownership of menu.
QMenu* TMainWindow::createPopupMenu() {
    return createToolbarMenu();
}

void TMainWindow::showStatusBarPopup(const QPoint& pos) {
    execPopup(this, toolbar_menu, statusBar()->mapToGlobal(pos));
}

void TMainWindow::createToolbars() {
    logger()->debug("createToolbars");

    menuBar()->setObjectName("menubar");

    // Control bar
    controlbar = new TEditableToolbar(this);
    controlbar->setObjectName("controlbar");
    QStringList actions;
    actions << "play_or_pause"
            << "stop"
            << "timeslider_action"
            << "rewind_menu"
            << "forward_menu"
            << "in_out_points_menu|0|1"
            << "separator|0|1"
            << "deinterlace_menu|0|1"
            << "aspect_menu|1|1"
            << "videosize_menu|1|0"
            << "reset_zoom_and_pan|0|1"
            << "separator|0|1"
            << "mute|0|1"
            << "volumeslider_action"
            << "separator|0|1"
            << "osd_menu|0|1"
            << "view_properties|0|1"
            << "show_playlist|0|1"
            << "separator|0|1"
            << "fullscreen";
    controlbar->setDefaultActions(actions);
    addToolBar(Qt::BottomToolBarArea, controlbar);
    connect(editControlBarAct, SIGNAL(triggered()),
            controlbar, SLOT(edit()));

    QAction* action = controlbar->toggleViewAction();
    action->setObjectName("toggle_controlbar");
    action->setShortcut(Qt::Key_F6);

    // Main toolbar
    toolbar = new TEditableToolbar(this);
    toolbar->setObjectName("toolbar1");
    actions.clear();
    actions << "open_url" << "favorites_menu";
    toolbar->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar);
    connect(editToolbarAct, SIGNAL(triggered()),
            toolbar, SLOT(edit()));

    action = toolbar->toggleViewAction();
    action->setObjectName("toggle_toolbar1");
    action->setShortcut(Qt::Key_F3);

    // Extra toolbar
    toolbar2 = new TEditableToolbar(this);
    toolbar2->setObjectName("toolbar2");
    actions.clear();
    actions << "osd_menu" << "toolbar_menu" << "stay_on_top_menu"
            << "separator" << "view_properties" << "show_playlist"
            << "show_log" << "separator" << "show_preferences";
    toolbar2->setDefaultActions(actions);
    addToolBar(Qt::TopToolBarArea, toolbar2);
    connect(editToolbar2Act, SIGNAL(triggered()),
            toolbar2, SLOT(edit()));

    action = toolbar2->toggleViewAction();
    action->setObjectName("toggle_toolbar2");
    action->setShortcut(Qt::Key_F4);

    // Statusbar
    statusBar()->setObjectName("statusbar");
    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showStatusBarPopup(const QPoint&)));

    // Add toolbars to auto_hide_timer
    auto_hide_timer = new TAutoHideTimer(this, playerwindow);
    auto_hide_timer->add(controlbar->toggleViewAction(), controlbar);
    auto_hide_timer->add(toolbar->toggleViewAction(), toolbar);
    auto_hide_timer->add(toolbar2->toggleViewAction(), toolbar2);
    auto_hide_timer->add(viewMenuBarAct, menuBar());
    auto_hide_timer->add(viewStatusBarAct, statusBar());
    // Playlist added by createmenus
    connect(playerwindow, SIGNAL(draggingChanged(bool)),
            auto_hide_timer, SLOT(setDraggingPlayerWindow(bool)));
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
        logger()->debug("setupNetworkProxy: using proxy: host: %1, port: %2,"
                        " type: %3",
               pref->proxy_host, pref->proxy_port, pref->proxy_type);
    } else {
        // No proxy
        proxy.setType(QNetworkProxy::NoProxy);
        logger()->debug("setupNetworkProxy: no proxy");
    }

    QNetworkProxy::setApplicationProxy(proxy);
}

void TMainWindow::sendEnableActions() {
    logger()->debug("sendEnableActions: state " + player->stateToString());

    timeslider_action->enable(player->statePOP());

    emit enableActions();
}

void TMainWindow::retranslateStrings() {
    logger()->debug("retranslateStrings");

    setWindowIcon(Images::icon("logo", 64));

    // Toolbars
    toolbar_menu->menuAction()->setText(tr("&Toolbars"));
    toolbar_menu->menuAction()->setIcon(Images::icon("toolbars"));

    // Main toolbar
    toolbar->setWindowTitle(tr("&Main toolbar"));
    toolbar->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

    // Extra toolbar
    toolbar2->setWindowTitle(tr("&Extra toolbar"));
    toolbar2->toggleViewAction()->setIcon(Images::icon("extra_toolbar"));

    // Control bar
    controlbar->setWindowTitle(tr("&Control bar"));
    controlbar->toggleViewAction()->setIcon(Images::icon("controlbar"));

    // Status bar
    statusbar_menu->menuAction()->setText(tr("St&atusbar"));
    statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));

    // Sliders
    timeslider_action->setText(tr("Time slider"));
    volumeslider_action->setText(tr("Volume slider"));

    // Playlist
    playlist->retranslateStrings();

    // Log window
    log_window->retranslateStrings();

    // Help window
    if (help_window) {
        help_window->retranslateStrings();
    }

    // Update actions view in preferences
    // It has to be done, here. The actions are translated after the
    // preferences dialog.
    if (pref_dialog)
        pref_dialog->mod_input()->actions_editor->updateView();
} // retranslateStrings()

void TMainWindow::displayVideoInfo() {

    if (player->mdat.noVideo()) {
        video_info_label->setText("");
    } else {
        QSize video_out_size = playerwindow->lastVideoOutSize();
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
        s = tr("I: %1", "In point in statusbar").arg(Helper::formatTime(secs));

    secs = qRound(player->mset.out_point);
    if (secs > 0) {
        if (!s.isEmpty()) s += " ";
        s += tr("O: %1", "Out point in statusbar").arg(Helper::formatTime(secs));
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
    logger()->debug("showEvent");

    if (event) {
        QMainWindow::showEvent(event);
    }

    if (pref->pause_when_hidden
        && player->state() == Player::STATE_PAUSED
        && !ignore_show_hide_events) {
        logger()->debug("showEvent: unpausing");
        player->play();
    }

    setFloatingToolbarsVisible(true);
}

void TMainWindow::hideEvent(QHideEvent* event) {
    logger()->debug("hideEvent");

    if (event) {
        QMainWindow::hideEvent(event);
    }

    if (pref->pause_when_hidden
        && player->state() == Player::STATE_PLAYING
        && !ignore_show_hide_events) {
        logger()->debug("hideEvent: pausing");
        player->pause();
    }

    setFloatingToolbarsVisible(false);
}

void TMainWindow::changeEvent(QEvent* e) {

    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QMainWindow::changeEvent(e);

#if QT_VERSION_MAJOR >= 5
        // Emulate show/hide events for Qt >= 5
        if(e->type() == QEvent::WindowStateChange) {
            bool was_min = static_cast<QWindowStateChangeEvent*>(e)->oldState()
                           == Qt::WindowMinimized;
            if (was_min) {
                if (!isMinimized()) {
                    showEvent(0);
                }
            } else if (isMinimized()) {
                hideEvent(0);
            }
        }
#endif

    }
}

void TMainWindow::setWindowCaption(const QString& title) {
    setWindowTitle(title);
}

void TMainWindow::createPreferencesDialog() {

    QApplication::setOverrideCursor(Qt::WaitCursor);
    pref_dialog = new Pref::TDialog(this);
    pref_dialog->setModal(false);
    connect(pref_dialog, SIGNAL(applied()),
             this, SLOT(applyNewPreferences()));
    QApplication::restoreOverrideCursor();
}

void TMainWindow::createFilePropertiesDialog() {
    logger()->debug("createFilePropertiesDialog");

    QApplication::setOverrideCursor(Qt::WaitCursor);

    file_properties_dialog = new TFilePropertiesDialog(this, &player->mdat);
    file_properties_dialog->setModal(false);
    connect(file_properties_dialog, SIGNAL(applied()),
            this, SLOT(applyFileProperties()));
    connect(player, SIGNAL(videoBitRateChanged(int)),
            file_properties_dialog, SLOT(showInfo()));
    connect(player, SIGNAL(audioBitRateChanged(int)),
            file_properties_dialog, SLOT(showInfo()));
    TAction* action = findChild<TAction*>("view_properties");
    if (action) {
        connect(file_properties_dialog, SIGNAL(visibilityChanged(bool)),
                action, SLOT(setChecked(bool)));
    }

    QApplication::restoreOverrideCursor();
}

void TMainWindow::handleMessageFromOtherInstances(const QString& message) {
    logger()->debug("handleMessageFromOtherInstances: '%1'", message);

    int pos = message.indexOf(' ');
    if (pos > -1) {
        QString command = message.left(pos);
        QString arg = message.mid(pos+1);
        logger()->debug("command: '%1'", command);
        logger()->debug("arg: '%1'", arg);

        if (command == "open_file") {
            emit openFileRequested();
            open(arg);
        }
        else
        if (command == "open_files") {
            QStringList file_list = arg.split(" <<sep>> ");
            emit openFileRequested();
            openFiles(file_list);
        }
        else
        if (command == "add_to_playlist") {
            QStringList file_list = arg.split(" <<sep>> ");
            playlist->addFiles(file_list);
        }
        else
        if (command == "media_title") {
            QStringList list = arg.split(" <<sep>> ");
            player->addForcedTitle(list[0], list[1]);
        }
        else
        if (command == "action") {
            processAction(arg);
        }
        else
        if (command == "load_sub") {
            setInitialSubtitle(arg);
            if (player->statePOP()) {
                player->loadSub(arg);
            }
        }
    }
}

TActionList TMainWindow::getAllNamedActions() {

    // Get all actions with a name
    TActionList all_actions = findChildren<QAction*>();
    for (int i = all_actions.count() - 1; i >= 0; i--) {
        QAction* a = all_actions[i];
        if (a->objectName().isEmpty() || a->isSeparator()) {
            all_actions.removeAt(i);
        }
    }

    return all_actions;
}

void TMainWindow::loadConfig() {
    logger()->debug("loadConfig");

    // Disable actions
    sendEnableActions();
    // Get all actions with a name
    TActionList all_actions = getAllNamedActions();
    // Load actions
    Action::TActionsEditor::loadFromConfig(pref, all_actions);

    pref->beginGroup(settingsGroupName());

    if (pref->save_window_size_on_exit) {
        QPoint p = pref->value("pos", pos()).toPoint();
        QSize s = pref->value("size", size()).toSize();
        int state = pref->value("state", 0).toInt();
        if (s.width() < 200 || s.height() < 200) {
            s = pref->default_size;
        }

        move(p);
        resize(s);
        setWindowState((Qt::WindowStates) state);

        if (p.isNull()) {
            TDesktop::centerWindow(this);
        }
        TDesktop::keepInsideDesktop(this);
    } else {
        TDesktop::centerWindow(this);
        // Need to center again after video loaded
        center_window = true;
        center_window_pos = pos();
    }

    pref->beginGroup("actions");
    toolbar->setActionsFromStringList(pref->value("toolbar1",
        toolbar->defaultActions()).toStringList(), all_actions);
    toolbar2->setActionsFromStringList(pref->value("toolbar2",
        toolbar2->defaultActions()).toStringList(), all_actions);
    // Using old name "controlwidget" to pick up old toolbars
    controlbar->setActionsFromStringList(pref->value("controlwidget",
        controlbar->defaultActions()).toStringList(), all_actions);
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    toolbar->setIconSize(pref->value("toolbar1",
        toolbar->iconSize()).toSize());
    toolbar2->setIconSize(pref->value("toolbar2",
        toolbar2->iconSize()).toSize());
    // Using old name "controlwidget" to pick up old toolbars
    controlbar->setIconSize(pref->value("controlwidget",
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

    state_restored = restoreState(pref->value("toolbars_state").toByteArray(),
                                  Helper::qtVersion());

    pref->beginGroup("statusbar");
    viewVideoInfoAct->setChecked(pref->value("video_info", true).toBool());
    viewInOutPointsAct->setChecked(pref->value("in_out_points", true).toBool());
    viewVideoTimeAct->setChecked(pref->value("video_time", true).toBool());
    viewFramesAct->setChecked(pref->show_frames);
    pref->endGroup();

    pref->endGroup();

    playlist->loadSettings();
    log_window->loadConfig();
}

void TMainWindow::saveConfig() {
    logger()->debug("saveConfig");

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
    // Using old name "controlwidget" for backward compat
    pref->setValue("controlwidget", controlbar->actionsToStringList());
    pref->endGroup();

    pref->beginGroup("toolbars_icon_size");
    pref->setValue("toolbar1", toolbar->iconSize());
    pref->setValue("toolbar2", toolbar2->iconSize());
    // Using old name "controlwidget" for backward compat
    pref->setValue("controlwidget", controlbar->iconSize());
    pref->endGroup();

    pref->setValue("menubar_visible", !menuBar()->isHidden());
    pref->setValue("fullscreen_menubar_visible", fullscreen_menubar_visible);
    pref->setValue("statusbar_visible", !statusBar()->isHidden());
    pref->setValue("fullscreen_statusbar_visible",
                   fullscreen_statusbar_visible);

    pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

    pref->beginGroup("statusbar");
    pref->setValue("video_info", viewVideoInfoAct->isChecked());
    pref->setValue("in_out_points", viewInOutPointsAct->isChecked());
    pref->setValue("video_time", viewVideoTimeAct->isChecked());
    pref->endGroup();

    pref->endGroup();

    playlist->saveSettings();
    log_window->saveConfig();
    if (help_window) {
        help_window->saveConfig();
    }
}

void TMainWindow::save() {

    msg(tr("Saving settings"), 0);
    if (pref->clean_config) {
        pref->clean_config = false;
        pref->remove("");
        Action::TActionsEditor::saveToConfig(pref, this);
    }
    saveConfig();
    pref->save();
}

void TMainWindow::closeEvent(QCloseEvent* e)  {
    logger()->debug("closeEvent");

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
    logger()->debug("closeWindow");

    close();
}

void TMainWindow::showPlaylist(bool) {
    // Overriden by TMainWindowPlus
    // playlist->setVisible(b);
}

void TMainWindow::showLog(bool b) {

    log_window->setVisible(b);
    if (b) {
        log_window->raise();
        log_window->activateWindow();
    }
}

void TMainWindow::showPreferencesDialog() {
    logger()->debug("showPreferencesDialog");

    if (!pref_dialog) {
        createPreferencesDialog();
    }

    pref_dialog->setData(pref);

    pref_dialog->mod_input()->actions_editor->clear();
    pref_dialog->mod_input()->actions_editor->addActions(this);

    pref_dialog->show();
}

void TMainWindow::restartApplication() {
    logger()->debug("restartApplication");

    emit requestRestart();

    // TODO:
    // When fullscreen the window size will not yet be updated by the time it is
    // saved by saveConfig. Block saving...
    save_size = !pref->fullscreen;

    // Close and restart with the new settings
    if (close()) {
        logger()->debug("restartApplication: closed main window");
        qApp->exit(TApp::NoExit);
    } else {
        // TODO: messagebox...
        logger()->warn("restartApplication: close canceled...");
    }
    return;
}

// The user has pressed OK in preferences dialog
void TMainWindow::applyNewPreferences() {
    logger()->debug("applyNewPreferences");

    QString old_player_bin = pref->player_bin;

    // Update pref from dialog
    pref_dialog->getData(pref);

    // Save playlist preferences repeat and shuffle
    playlist->saveSettings();

    // Update and save actions
    pref_dialog->mod_input()->actions_editor->applyChanges();
    Action::TActionsEditor::saveToConfig(pref, this);

    // Commit changes
    pref->save();

    // Player bin, style, icon set or language change need restart TApp
    Pref::TInterface* mod_interface = pref_dialog->mod_interface();
    if (pref->player_bin != old_player_bin
        || mod_interface->styleChanged()
        || mod_interface->iconsetChanged()
        || mod_interface->languageChanged()) {
        restartApplication();
        return;
    }

    // Keeping the current main window

    // Set color key, depends on VO
    playerwindow->setColorKey();

    // Forced demuxer
    player->mset.forced_demuxer = pref->use_lavf_demuxer ? "lavf" : "";

    // Video equalizer
    video_equalizer->setBySoftware(pref->use_soft_video_eq);

    // Subtitles
    subtitleMenu->useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);

    // Interface continued
    // Show panel
    if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
        resize(width(), height() + 200);
        panel->show();
    }
    // Hide toolbars delay
    auto_hide_timer->setInterval(pref->floating_hide_delay);
    // Recents
    if (mod_interface->recentsChanged()) {
        fileMenu->updateRecents();
    }

    // Keyboard and mouse
    playerwindow->setDelayLeftClick(pref->delay_left_click);

    // Network
    setupNetworkProxy();

    // Update log window edit control
    log_window->edit->setMaximumBlockCount(pref->log_window_max_events);

    // Reenable actions to reflect changes
    sendEnableActions();

    // TODO: move code above to preferencesChanged() signal
    emit preferencesChanged();

    // Restart video if needed
    if (pref_dialog->requiresRestart()) {
        player->restart();
    }
} // TMainWindow::applyNewPreferences()

void TMainWindow::showFilePropertiesDialog(bool checked) {
    logger()->debug("showFilePropertiesDialog");

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

void TMainWindow::setDataToFileProperties() {
    logger()->debug("setDataToFileProperties");

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
    logger()->debug("applyFileProperties");

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

void TMainWindow::onMediaInfoChanged() {
    logger()->debug("onMediaInfoChanged");

    if (file_properties_dialog && file_properties_dialog->isVisible()) {
        setDataToFileProperties();
    }

    QString title = player->mdat.displayName();
    setWindowCaption(title + " - " + TConfig::PROGRAM_NAME);
    emit mediaFileTitleChanged(player->mdat.filename, title);

    displayVideoInfo();
}

void TMainWindow::onNewMediaStartedPlaying() {
    logger()->debug("onNewMediaStartedPlaying");

    enterFullscreenOnPlay();

    // Recents
    pref->history_recents.addItem(player->mdat.filename,
                                  player->mdat.displayName());
    fileMenu->updateRecents();

    checkPendingActionsToRun();
}

void TMainWindow::updateVideoEqualizer() {
    logger()->debug("updateVideoEqualizer");

    video_equalizer->setContrast(player->mset.contrast);
    video_equalizer->setBrightness(player->mset.brightness);
    video_equalizer->setHue(player->mset.hue);
    video_equalizer->setSaturation(player->mset.saturation);
    video_equalizer->setGamma(player->mset.gamma);
}

void TMainWindow::updateAudioEqualizer() {
    logger()->debug("updateAudioEqualizer");

    audio_equalizer->setEqualizer(player->getAudioEqualizer());
}

void TMainWindow::setDefaultValuesFromVideoEqualizer() {
    logger()->debug("setDefaultValuesFromVideoEqualizer");

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
    logger()->debug("changeVideoEqualizerBySoftware: %1", b);

    if (b != pref->use_soft_video_eq) {
        pref->use_soft_video_eq = b;
        player->restart();
    }
}

void TMainWindow::openDirectory() {
    logger()->debug("openDirectory");

    QString s = TFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"),
                    pref->latest_dir);

    if (!s.isEmpty() && playlist->maybeSave()) {
        playlist->playDirectory(s);
    }
}

void TMainWindow::open(const QString &file) {
    logger()->debug("open: " + file);

    if (file.isEmpty()) {
        logger()->warn("open: filename is empty");
        return;
    }
    if (!playlist->maybeSave()) {
        return;
    }

    QFileInfo fi(file);
    if (fi.exists()) {
        if (fi.isDir()) {
            playlist->playDirectory(file);
            return;
        }
        QString ext = fi.suffix().toLower();
        if (ext == "m3u8" || ext == "m3u" || ext == "pls") {
            playlist->openPlaylist(fi.absoluteFilePath());
            return;
        }
        pref->latest_dir = fi.absolutePath();
    }

    player->open(file);
    logger()->debug("open: done");
}

void TMainWindow::openFiles(const QStringList& files, const QString& current) {
    logger()->debug("openFiles");

    if (files.empty()) {
        logger()->debug("openFiles: no files in list to open");
        return;
    }

    if (playlist->maybeSave()) {
        playlist->clear();
        playlist->addFiles(files, true, 0, current);
    }
}

void TMainWindow::openFile() {
    logger()->debug("openFile");

    QString s = TFileDialog::getOpenFileName(
        this,
        tr("Choose a file"),
        pref->latest_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;"
        + tr("Video") + extensions.video().forFilter() + ";;"
        + tr("Audio") + extensions.audio().forFilter() + ";;"
        + tr("Playlists") + extensions.playlists().forFilter() + ";;"
        + tr("Images") + extensions.images().forFilter() + ";;"
        + tr("All files") +" (*.*)");

    if (!s.isEmpty()) {
        open(s);
    }
}

void TMainWindow::openRecent() {
    logger()->debug("openRecent");

    QAction *a = qobject_cast<QAction *> (sender());
    if (a) {
        int item = a->data().toInt();
        QString filename = pref->history_recents.item(item);
        if (!filename.isEmpty())
            open(filename);
    }
}

void TMainWindow::openURL() {
    logger()->debug("openURL");

    TInputURL d(this);

    // Get url from clipboard
    QString txt = QApplication::clipboard()->text();
    if (!txt.contains("\x0a") && !txt.contains("\x0d")) {
        if (txt.contains("/")
#ifdef Q_OS_WIN
        || txt.contains("\\")
#endif
        ) {
            d.setURL(txt);
        }
    }

    for (int n = 0; n < pref->history_urls.count(); n++) {
        d.setURL(pref->history_urls.url(n));
    }

    if (d.exec() == QDialog::Accepted) {
        QString url = d.url();
        if (!url.isEmpty()) {
            pref->history_urls.addUrl(url);
            open(url);
        }
    }
}

void TMainWindow::configureDiscDevices() {
    QMessageBox::information(this, TConfig::PROGRAM_NAME + tr(" - Information"),
            tr("The CDROM / DVD drives are not configured yet.\n"
               "The configuration dialog will be shown now, "
               "so you can do it."), QMessageBox::Ok);

    showPreferencesDialog();
    pref_dialog->showSection(Pref::TDialog::SECTION_DRIVES);
}

void TMainWindow::openVCD() {
    logger()->debug("openVCD");

    if (pref->cdrom_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->openDisc(TDiscName("vcd", pref->vcd_initial_title,
                                 pref->cdrom_device));
    }
}

void TMainWindow::openAudioCD() {
    logger()->debug("openAudioCD");

    if (pref->cdrom_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->open("cdda://");
    }
}

void TMainWindow::openDVD() {
    logger()->debug("openDVD");

    if (pref->dvd_device.isEmpty()) {
        configureDiscDevices();
    } else if (playlist->maybeSave()) {
        player->openDisc(TDiscName(pref->dvd_device, pref->useDVDNAV()));
    }
}

void TMainWindow::openDVDFromFolder() {
    logger()->debug("openDVDFromFolder");

    if (playlist->maybeSave()) {
        TInputDVDDirectory *d = new TInputDVDDirectory(this);
        d->setFolder(pref->last_dvd_directory);
        if (d->exec() == QDialog::Accepted) {
            logger()->debug("openDVDFromFolder: accepted");
            openDVDFromFolder(d->folder());
        }

        delete d;
    }
}

void TMainWindow::openDVDFromFolder(const QString &directory) {

    pref->last_dvd_directory = directory;
    player->openDisc(TDiscName(directory, pref->useDVDNAV()));
}

void TMainWindow::openBluRay() {
    logger()->debug("openBluRay");

    if (pref->bluray_device.isEmpty()) {
        configureDiscDevices();
    } else {
        player->openDisc(TDiscName("br", 0, pref->bluray_device));
    }
}

void TMainWindow::openBluRayFromFolder() {
    logger()->debug("openBluRayFromFolder");

    if (playlist->maybeSave()) {
        QString dir = QFileDialog::getExistingDirectory(this,
            tr("Select the Blu-ray folder"),
            pref->last_dvd_directory, QFileDialog::ShowDirsOnly
                                      | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            pref->last_dvd_directory = dir;
            player->openDisc(TDiscName("br", 0, dir));
        }
    }
}

void TMainWindow::loadSub() {
    logger()->debug("loadSub");

    QString s = TFileDialog::getOpenFileName(
        this, tr("Choose a file"),
        pref->latest_dir,
        tr("Subtitles") + extensions.subtitles().forFilter()+ ";;" +
        tr("All files") +" (*.*)");

    if (!s.isEmpty())
        player->loadSub(s);
}

void TMainWindow::setInitialSubtitle(const QString & subtitle_file) {
    logger()->debug("setInitialSubtitle: '%1'", subtitle_file);

    player->setInitialSubtitle(subtitle_file);
}

void TMainWindow::loadAudioFile() {
    logger()->debug("loadAudioFile");

    QString s = TFileDialog::getOpenFileName(
        this, tr("Choose a file"),
        pref->latest_dir,
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
        help_window->loadConfig();
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
    #if QT_VERSION >= 0x050000
    int delay = QInputDialog::getInt(this,
        tr("%1 - Audio delay").arg(TConfig::PROGRAM_NAME),
        tr("Audio delay (in milliseconds):"), player->mset.audio_delay,
        -3600000, 3600000, 1, &ok);
    #else
    int delay = QInputDialog::getInteger(this,
        tr("%1 - Audio delay").arg(TConfig::PROGRAM_NAME),
        tr("Audio delay (in milliseconds):"), player->mset.audio_delay,
        -3600000, 3600000, 1, &ok);
    #endif
    if (ok) {
        player->setAudioDelay(delay);
    }
}

void TMainWindow::showSubDelayDialog() {
    bool ok;
    #if QT_VERSION >= 0x050000
    int delay = QInputDialog::getInt(this,
        tr("%1 - Subtitle delay").arg(TConfig::PROGRAM_NAME),
        tr("Subtitle delay (in milliseconds):"), player->mset.sub_delay,
        -3600000, 3600000, 1, &ok);
    #else
    int delay = QInputDialog::getInteger(this,
        tr("%1 - Subtitle delay").arg(TConfig::PROGRAM_NAME),
        tr("Subtitle delay (in milliseconds):"), player->mset.sub_delay,
        -3600000, 3600000, 1, &ok);
    #endif
    if (ok) {
        player->setSubDelay(delay);
    }
}

void TMainWindow::showStereo3dDialog() {
    TStereo3dDialog d(this);
    d.setInputFormat(player->mset.stereo3d_in);
    d.setOutputFormat(player->mset.stereo3d_out);

    if (d.exec() == QDialog::Accepted) {
        player->changeStereo3d(d.inputFormat(), d.outputFormat());
    }
}

void TMainWindow::exitFullscreen() {

    if (pref->fullscreen) {
        toggleFullscreen(false);
    }
}

void TMainWindow::toggleFullscreen() {
    logger()->debug("toggleFullscreen");

    toggleFullscreen(!pref->fullscreen);
}

void TMainWindow::toggleFullscreen(bool b) {
    logger()->debug("toggleFullscreen: %1", b);

    if (b == pref->fullscreen) {
        logger()->debug("toggleFullscreen: nothing to do");
        return;
    }

    switching_to_fullscreen = true;
    pref->fullscreen = b;
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
    switching_to_fullscreen = false;
}

void TMainWindow::aboutToEnterFullscreen() {
    //logger()->debug("aboutToEnterFullscreen");

    emit aboutToEnterFullscreenSignal();

    // Save current state
    menubar_visible = !menuBar()->isHidden();
    statusbar_visible = !statusBar()->isHidden();
    first_fullscreen_filename = player->mdat.filename;

    pref->beginGroup(settingsGroupName());
    pref->setValue("toolbars_state", saveState(Helper::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didEnterFullscreen() {
    //logger()->debug("didEnterFullscreen");

    // Restore fullscreen state
    viewMenuBarAct->setChecked(fullscreen_menubar_visible);
    viewStatusBarAct->setChecked(fullscreen_statusbar_visible);

    pref->beginGroup(settingsGroupName());
    state_restored = restoreState(pref->value("toolbars_state_fullscreen")
                                  .toByteArray(), Helper::qtVersion());
    if (!state_restored) {
        // First time there is no fullscreen toolbar state
        logger()->debug("didEnterFullscreen: failed to restore fullscreen"
                        " toolbar state");
        toolbar->hide();
        toolbar2->hide();
    }
    pref->endGroup();

    emit didEnterFullscreenSignal();

    auto_hide_timer->start();
}

void TMainWindow::aboutToExitFullscreen() {
    //logger()->debug("aboutToExitFullscreen");

    auto_hide_timer->stop();

    // Save fullscreen state
    fullscreen_menubar_visible = !menuBar()->isHidden();
    fullscreen_statusbar_visible = !statusBar()->isHidden();

    pref->beginGroup(settingsGroupName());
    pref->setValue("toolbars_state_fullscreen", saveState(Helper::qtVersion()));
    pref->endGroup();
}

void TMainWindow::didExitFullscreen() {
    //logger()->debug("didExitFullscreen");

    viewMenuBarAct->setChecked(menubar_visible);
    viewStatusBarAct->setChecked(statusbar_visible);

    pref->beginGroup(settingsGroupName());
    state_restored = restoreState(pref->value("toolbars_state")
                                  .toByteArray(), Helper::qtVersion());
    if  (!state_restored) {
        logger()->warn("didExitFullscreen: failed to restore toolbar state");
    }
    pref->endGroup();

    // Update size when current video changed in fullscreen
    if (pref->resize_on_load
        && player->mdat.filename != first_fullscreen_filename) {
        changeSize(getDefaultSize());;
    }

    emit didExitFullscreenSignal();
}

void TMainWindow::leftClickFunction() {
    logger()->debug("leftClickFunction");

    if (!pref->mouse_left_click_function.isEmpty()) {
        processAction(pref->mouse_left_click_function);
    }
}

void TMainWindow::rightClickFunction() {
    logger()->debug("rightClickFunction");

    if (!pref->mouse_right_click_function.isEmpty()) {
        processAction(pref->mouse_right_click_function);
    }
}

void TMainWindow::doubleClickFunction() {
    logger()->debug("doubleClickFunction");

    if (!pref->mouse_double_click_function.isEmpty()) {
        processAction(pref->mouse_double_click_function);
    }
}

void TMainWindow::middleClickFunction() {
    logger()->debug("middleClickFunction");

    if (!pref->mouse_middle_click_function.isEmpty()) {
        processAction(pref->mouse_middle_click_function);
    }
}

void TMainWindow::xbutton1ClickFunction() {
    logger()->debug("xbutton1ClickFunction");

    if (!pref->mouse_xbutton1_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton1_click_function);
    }
}

void TMainWindow::xbutton2ClickFunction() {
    logger()->debug("xbutton2ClickFunction");

    if (!pref->mouse_xbutton2_click_function.isEmpty()) {
        processAction(pref->mouse_xbutton2_click_function);
    }
}

void TMainWindow::moveWindowMerged() {

    move(pos() + move_window_diff);
    move_window_diff = QPoint(0, 0);
}

// Called by playerwindow when dragging main window
void TMainWindow::moveWindow(QPoint diff) {

    // Merge multiple moves into one for machines that cannot keep up
    move_window_diff += diff;
    // Zero timeout, calls moveWindowMerged()
    move_window_timer.start();
}

void TMainWindow::processAction(QString action_name) {

    // Check name for checkable actions
    static QRegExp func_rx("(.*) (true|false)");
    bool value = false;
    bool checkableFunction = false;

    if (func_rx.indexIn(action_name) >= 0) {
        action_name = func_rx.cap(1);
        value = func_rx.cap(2) == "true";
        checkableFunction = true;
    }

    QAction* action = findChild<QAction*>(action_name);
    if (action) {
        if (action->isEnabled()) {
            if (action->isCheckable() && checkableFunction) {
                logger()->debug("processAction: setting checked action '%1'"
                                " to %2", action_name, QString::number(value));
                action->setChecked(value);
            } else {
                logger()->debug("processAction: triggering action '%1'",
                                action_name);
                action->trigger();
            }
        } else {
            logger()->warn("processAction: canceling disabled action '%1'",
                           action_name);
        }
    } else {
        logger()->warn("processAction: action '%1' not found", action_name);
    }
}

void TMainWindow::runActions(QString actions) {
    logger()->debug("runActions");

    actions = actions.simplified(); // Remove white space

    QAction* action;
    QStringList actionsList = actions.split(" ");

    for (int n = 0; n < actionsList.count(); n++) {
        QString actionStr = actionsList[n];
        QString par = ""; //the parameter which the action takes

        //set par if the next word is a boolean value
        if (n + 1 < actionsList.count()) {
            if (actionsList[n + 1].toLower() == "true"
                || actionsList[n + 1].toLower() == "false") {
                par = actionsList[n + 1].toLower();
                n++;
            }
        }

        action = findChild<QAction*>(actionStr);
        if (action) {
            logger()->debug("runActions: running action '%1' (par '%2')",
                            actionStr, par);

            if (action->isCheckable()) {
                if (par.isEmpty()) {
                    action->trigger();
                } else {
                    action->setChecked(par == "true");
                }
            } else {
                action->trigger();
            }
        } else {
            logger()->warn("runActions: action '%1' not found", actionStr);
        }
    } //end for
}

// Called by timer and onNewMediaStartedPlaying
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

    if (actions.isEmpty()) {
        logger()->debug("checkPendingActionsToRun: no actions to run");
    } else {
        logger()->debug("checkPendingActionsToRun: running actions: '%1'",
                        actions);
        runActions(actions);
    }
}

void TMainWindow::runActionsLater(const QString& actions, bool postCheck) {

    pending_actions_to_run = actions;
    if (!pending_actions_to_run.isEmpty() && postCheck) {
        QTimer::singleShot(100, this, SLOT(checkPendingActionsToRun()));
    }
}

void TMainWindow::dragEnterEvent(QDragEnterEvent *e) {
    logger()->debug("dragEnterEvent");

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
    logger()->debug("dropEvent");

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl url, e->mimeData()->urls()) {
            files.append(url.toString());
        }
        logger()->debug("dropEvent: number of files: %1", files.count());
        openFiles(files);
        e->accept();
        return;
    }
    QMainWindow::dropEvent(e);
}

void TMainWindow::showContextMenu() {
    showContextMenu(QCursor::pos());
}

void TMainWindow::showContextMenu(QPoint p) {
    execPopup(this, popup, p);
}

// Called when a video has started to play
void TMainWindow::enterFullscreenOnPlay() {
    logger()->debug("enterFullscreenOnPlay: app start fs %1"
                    ", pref start fs %2, fs %3",
                    TApp::start_in_fullscreen,
                    pref->start_in_fullscreen,
                    pref->fullscreen);

    if (TApp::start_in_fullscreen != TApp::FS_FALSE) {
        if (pref->start_in_fullscreen || TApp::start_in_fullscreen > 0) {
            if (!pref->fullscreen) {
                toggleFullscreen(true);
            }

            // Clear TApp::start_in_fullscreen
            if (TApp::start_in_fullscreen == TApp::FS_RESTART) {
                TApp::start_in_fullscreen = TApp::FS_NOT_SET;
            }
        }
    }
}

// Called when the playlist has stopped
void TMainWindow::exitFullscreenOnStop() {

    if (pref->fullscreen) {
        toggleFullscreen(false);
    }
}

void TMainWindow::playlistHasFinished() {
    logger()->debug("playlistHasFinished");

    player->stop();

    // Handle "Close on end of playlist" option
    if (arg_close_on_finish != 0) {
        if ((arg_close_on_finish == 1) || (pref->close_on_finish)) {
            close();
        }
    }
}

void TMainWindow::onStateChanged(Player::TState state) {
    logger()->debug("onStateChanged: new state " + player->stateToString());

    sendEnableActions();
    auto_hide_timer->setAutoHideMouse(state == Player::STATE_PLAYING);
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

void TMainWindow::onPositionChanged(double sec, bool changed) {

    static int lastSec = -1111;

    // TODO: <-1..0> looses sign
    int s = sec;

    if (s != lastSec) {
        lastSec = s;
        positionText = Helper::formatTime(s);
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
            // Fix floats. Example 0.84 * 25 = 20 if floored instead of 21
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
        QString time = positionText + frames + durationText;
        emit timeChanged(time);
    }
}

void TMainWindow::onDurationChanged(double duration) {

    durationText = " / " + Helper::formatTime(qRound(duration));

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

    onPositionChanged(player->mset.current_sec, true);
}

void TMainWindow::changeSize(double factor) {
    logger()->debug("changeSize: %1", factor);

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
        resizeWindowToVideo();
        if (center) {
            TDesktop::centerWindow(this);
        }
    }

    msgOSD(tr("Size %1%").arg(QString::number(qRound(factor * 100))));
}

void TMainWindow::changeSize(int percentage) {
    logger()->debug("changeSize %1%", percentage);
    changeSize((double) percentage / 100);
}

void TMainWindow::toggleDoubleSize() {

    if (pref->size_factor != 1.0)
        changeSize(1.0);
    else changeSize(2.0);
}

void TMainWindow::hidePanel() {
    logger()->debug("hidePanel");

    if (panel->isVisible()) {
        // Exit from fullscreen mode
        if (pref->fullscreen) {
            toggleFullscreen(false);
            update();
        }

        int width = this->width();
        if (width > pref->default_size.width())
            width = pref->default_size.width();
        resize(width, height() - panel->height());
        panel->hide();
    }
}

double TMainWindow::optimizeSize(double size) {
    logger()->debug("optimizeSize: size in %1", size);

    QSize available_size = TDesktop::availableSize(this);
    QSize res = playerwindow->resolution();
    if (res.width() <= 0 || res.height() <= 0) {
        return size;
    }

    // Handle fullscreen
    if (pref->fullscreen) {
        size = (double) available_size.width() / res.width();
        double size_y = (double) available_size.height() / res.height();
        if (size_y < size) {
            size = size_y;
        }
        logger()->debug("optimizeSize: returning size %1 for fullscreen", size);
        return size;
    }

    // Return current size for VO size change by TPlayer::setAspectRatio()
    if (player->keepSize) {
        player->clearKeepSize();
        logger()->debug("optimizeSize: keepSize set, returning current size %1",
                        pref->size_factor);
        return pref->size_factor;
    }

    QSize video_size = res * size;

    // Limit size to 0.6 of available desktop
    const double f = 0.6;
    // Adjust width
    double max = f * available_size.width();
    if (video_size.width() > max) {
        logger()->debug("optizeSize: limiting width to %1", max);
        size = max / res.width();
        video_size = res * size;
    }
    // Adjust height
    max = f * available_size.height();
    if (video_size.height() > max) {
        logger()->debug("optizeSize: limiting height to %1", max);
        size = max / res.height();
        video_size = res * size;
    }

    double min = available_size.height() / 4;
    if (video_size.height() < min) {
        if (size == 1.0) {
            logger()->debug("optimzeSize: selecting size 2.0 for small video");
            return 2.0;
        }
        size = min / res.height();
        logger()->debug("optimzeSize: selecting size for minimal height %1",
                        min);
    }

    // Round to predefined values
    int factor_int = qRound(size * 100);
    const int factors[] = {25, 50, 75, 100, 125, 150, 175, 200, 300, 400};
    for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); i++) {
        int predef = factors[i];
        int d = predef / 10;
        if (d < 10) d = 10;
        if (qAbs(factor_int - predef) < d) {
            size = (double) predef / 100;
            logger()->debug("optimzeSize: rounding size to predefined value %1",
                            size);
            return size;
        }
    }

    // Make width multiple of 16
    if (!player->mdat.image) {
        int new_w = ((video_size.width() + 8) / 16) * 16;
        if (new_w != video_size.width()) {
            size = (double) new_w / res.width();
            logger()->debug("optimzeSize: rounding size to create multiple"
                                " of 16 width");
        }
    }

    logger()->debug("optimzeSize: selected size %1", size);
    return size;
}

void TMainWindow::optimizeSizeFactor() {
    logger()->debug("optimizeSizeFactor");

    if (pref->fullscreen) {
        player->setZoom(1.0);
    } else {
        changeSize(optimizeSize(pref->size_factor));
    }
}

double TMainWindow::getDefaultSize() {
    return optimizeSize(pref->initial_zoom_factor);
}

void TMainWindow::getNewGeometry(int w, int h) {

    // Get new size factor
    pref->size_factor = getDefaultSize();

    QSize desktop = TDesktop::availableSize(this);
    bool stickx = !pref->fullscreen
                  && pos().x() + frameGeometry().size().width()
                  >= desktop.width();
    bool sticky = !pref->fullscreen
                  && pos().y() + frameGeometry().size().height()
                  >= desktop.height();

    resizeWindow(w, h);

    QPoint p = pos();
    if (stickx) {
        int x = desktop.width() - frameGeometry().size().width();
        if (x < 0) {
            stickx = false;
        } else {
            p.rx() = x;
            logger()->trace("getNewGeometry: sticking to right side");
        }
    }
    if (sticky) {
        int y = desktop.height() - frameGeometry().size().height();
        if (y < 0) {
            sticky = false;
        } else {
            p.ry() = y;
            logger()->trace("getNewGeometry: sticking to bottom");
        }
    }
    if (stickx || sticky) {
        move(p);
    }
}

void TMainWindow::onVideoOutResolutionChanged(int w, int h) {
    logger()->debug("onVideoOutResolutionChanged: %1 x %2", w, h);

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
            getNewGeometry(w, h);
        } else {
            // Adjust the size factor to the current window size
            playerwindow->updateSizeFactor();
            logger()->debug("onVideoOutResolutionChanged: adjusted size factor"
                            " to %1 to match current window size",
                            pref->size_factor);
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

void TMainWindow::resizeWindow(int w, int h) {

    if (!pref->fullscreen && !isMaximized()) {
        resizeMainWindow(w, h, pref->size_factor);
        TDesktop::keepInsideDesktop(this);
    }
}

void TMainWindow::resizeWindowToVideo() {
    resizeWindow(player->mdat.video_out_width, player->mdat.video_out_height);
}

void TMainWindow::resizeMainWindow(int w, int h, double size_factor,
                                   bool try_twice) {
    logger()->debug(QString("resizeMainWindow: requested video size %1 x %2"
                            " size factor %3")
                    .arg(w).arg(h).arg(pref->size_factor));

    QSize panel_size = QSize(w, h) * size_factor;
    if (panel_size == panel->size()) {
        logger()->debug("resizeMainWindow: panel has requested size");
        return;
    }

    QSize new_size = size() + panel_size - panel->size();
    resize(new_size);

    if (panel->size() != panel_size) {
        // Resizing the main window can change the height of the tool bars,
        // which will change the height of the panel during the resize.
        // Often fixed by resizing once again, using the new panel height.
        if (try_twice) {
            resizeMainWindow(w, h, size_factor, false);
        } else {
            logger()->debug(QString("resizeMainWindow: resize failed. Panel"
                                    " size now %1 x %2. Wanted size %3 x %4")
                            .arg(panel->size().width())
                            .arg(panel->size().height())
                            .arg(panel_size.width())
                            .arg(panel_size.height()));
        }
    }
}

// Slot called when media settings reset or loaded
void TMainWindow::onMediaSettingsChanged() {
    logger()->debug("onMediaSettingsChanged");

    emit mediaSettingsChanged(&player->mset);

    updateVideoEqualizer();
    updateAudioEqualizer();

    displayInOutPoints();
}

void TMainWindow::onDragPositionChanged(double t) {

    QString s = tr("Jump to %1").arg(Helper::formatTime(qRound(t)));
    msg(s, 1000);

    if (pref->fullscreen) {
        player->displayTextOnOSD(s);
    }
}

void TMainWindow::setStayOnTop(bool b) {
    logger()->debug("setStayOnTop: %1", b);

    bool stay_on_top = windowFlags() & Qt::WindowStaysOnTopHint;
    if (b == stay_on_top) {
        logger()->debug("setStayOnTop: WindowStaysOnTopHint already uptodate");
        return;
    }

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
    logger()->debug("changeStayOnTop: %1", stay_on_top);

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
    logger()->debug("checkStayOnTop");

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

void TMainWindow::onPlayerError(int exit_code) {
    logger()->error("onPlayerError: %1", exit_code);

    QString s = Player::Process::TExitMsg::message(exit_code)
                + " (" + player->mdat.filename + ")";
    msg(s, 0);

    static bool busy = false;
    if (pref->report_player_crashes
        && !busy
        && player->state() != Player::STATE_STOPPING) {
        busy = true;
        QMessageBox::warning(this,
            tr("%1 process error").arg(pref->playerName()),
            s + " \n"
            + tr("See menu Window -> View log for details."),
            QMessageBox::Ok);
        busy = false;
    }
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

} // namespace Gui

#include "moc_mainwindow.cpp"

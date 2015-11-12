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

#include "config.h"
#include "gui/base.h"

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
#include <QDebug>

#include <cmath>

#include "version.h"
#include "playerid.h"
#include "desktop.h"
#include "discname.h"
#include "extensions.h"
#include "log.h"
#include "colorutils.h"
#include "images.h"
#include "helper.h"
#include "mediadata.h"
#include "autohidetimer.h"
#include "playerwindow.h"
#include "core.h"
#include "clhelp.h"
#include "filedialog.h"

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
#include "gui/action/menus.h"

#include "gui/links.h"
#include "gui/errordialog.h"
#include "gui/logwindow.h"
#include "gui/playlist.h"
#include "gui/filepropertiesdialog.h"
#include "gui/inputdvddirectory.h"
#include "gui/about.h"
#include "gui/inputurl.h"
#include "gui/timedialog.h"
#include "gui/playlist.h"
#include "gui/videoequalizer.h"
#include "gui/eqslider.h"
#include "gui/audioequalizer.h"
#include "gui/stereo3ddialog.h"
#include "gui/tvlist.h"

#include "gui/pref/dialog.h"
#include "gui/pref/general.h"
#include "gui/pref/interface.h"
#include "gui/pref/input.h"
#include "gui/pref/advanced.h"
#include "gui/pref/prefplaylist.h"

#ifdef FIND_SUBTITLES
#include "findsubtitleswindow.h"
#endif

#ifdef VIDEOPREVIEW
#include "videopreview.h"
#endif

#ifdef MPRIS2
#include "mpris2/mpris2.h"
#endif

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#include <QSysInfo>
#endif

#ifdef UPDATE_CHECKER
#include "gui/updatechecker.h"
#endif

#ifdef YOUTUBE_SUPPORT
#ifdef YT_USE_YTSIG
#include "codedownloader.h"
#endif
#endif

#ifdef AUTO_SHUTDOWN_PC
#include "shutdowndialog.h"
#include "shutdown.h"
#endif


using namespace Settings;

namespace Gui {

TBase::TBase()
	: QMainWindow()
	, clhelp_window(0)
	, pref_dialog(0)
	, file_dialog(0)
#ifdef FIND_SUBTITLES
	, find_subs_dialog(0)
#endif
#ifdef VIDEOPREVIEW
	, video_preview(0)
#endif
#ifdef UPDATE_CHECKER
	, update_checker(0)
#endif

	, menubar_visible(true)
	, statusbar_visible(true)
	, fullscreen_menubar_visible(false)
	, fullscreen_statusbar_visible(true)

	, arg_close_on_finish(-1)
	, arg_start_in_fullscreen(-1)
	, ignore_show_hide_events(false)
	, block_resize(false)
	, center_window(false)
	, block_update_size_factor(0)

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	, just_stopped(false)
#endif
#endif
{

#if QT_VERSION >= 0x050000
	was_minimized = isMinimized();
#endif

	// Set style before changing color of widgets:
	// TODO: from help: Warning: To ensure that the application's style is set
	// correctly, it is best to call this function before the QApplication
	// constructor, if possible.
	default_style = qApp->style()->objectName();
	if (!pref->style.isEmpty()) {
		// Remove a previous stylesheet to prevent a crash
		qApp->setStyleSheet("");
		qApp->setStyle(pref->style);
	}

	setWindowTitle("SMPlayer");
	setAcceptDrops(true);

	// Reset size factor to 1.0 and window to default size
	pref->size_factor = 1.0;
	resize(pref->default_size);

	// Create objects:
	createPanel();
	setCentralWidget(panel);

	createPlayerWindow();
	createCore();
	createPlaylist();
	createVideoEqualizer();
	createAudioEqualizer();

	log_window = new TLogWindow(0);

	createActions();
	createToolbars();
	createMenus();
	setActionsEnabled(false);

	setupNetworkProxy();
	changeStayOnTop(pref->stay_on_top);
	updateRecents();

#ifdef UPDATE_CHECKER
	update_checker = new TUpdateChecker(this, &pref->update_checker_data);
#endif

#ifdef CHECK_UPGRADED
	QTimer::singleShot(30000, this, SLOT(checkIfUpgraded()));
#endif

#ifdef MPRIS2
	if (pref->use_mpris2)
		new Mpris2(this, this);
#endif

	retranslateStrings();
}

// TODO: check leaking and ownership
TBase::~TBase() {

#ifdef VIDEOPREVIEW
	delete video_preview;
#endif
#ifdef FIND_SUBTITLES
	delete find_subs_dialog;
#endif

	delete radiolist;
	delete tvlist;
	delete favorites;
	delete log_window;
	delete playlist;
	delete core;
}

void TBase::createPanel() {

	panel = new QWidget(this);
	panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	panel->setMinimumSize(QSize(1, 1));
	panel->setFocusPolicy(Qt::StrongFocus);
}

void TBase::createPlayerWindow() {

	playerwindow = new TPlayerWindow(panel);
	playerwindow->setObjectName("playerwindow");
	playerwindow->setDelayLeftClick(pref->delay_left_click);
	playerwindow->setColorKey(pref->color_key);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addWidget(playerwindow);
	panel->setLayout(layout);

	// Connect to player window mouse events
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
}

void TBase::createCore() {

	core = new TCore(playerwindow, this);

	connect(core, SIGNAL(showTime(double)),
			 this, SLOT(gotCurrentTime(double)));
	connect(core, SIGNAL(showFrame(int)),
			 this, SIGNAL(frameChanged(int)));
	connect(core, SIGNAL(durationChanged(double)),
			 this, SLOT(gotDuration(double)));

	connect(core, SIGNAL(stateChanged(TCore::State)),
			 this, SLOT(onStateChanged(TCore::State)));
	connect(core, SIGNAL(stateChanged(TCore::State)),
			 this, SLOT(checkStayOnTop(TCore::State)), Qt::QueuedConnection);

	connect(core, SIGNAL(mediaSettingsChanged()),
			 this, SLOT(onMediaSettingsChanged()));
	connect(core, SIGNAL(videoOutResolutionChanged(int, int)),
			 this, SLOT(onVideoOutResolutionChanged(int,int)));
	connect(core, SIGNAL(needResize(int, int)),
			 this, SLOT(resizeWindow(int, int)));

	connect(core, SIGNAL(showMessage(QString, int)),
			 this, SLOT(displayMessage(QString, int)));
	connect(core, SIGNAL(showMessage(QString)),
			 this, SLOT(displayMessage(QString)));

	connect(core, SIGNAL(newMediaStartedPlaying()),
			 this, SLOT(newMediaLoaded()), Qt::QueuedConnection);

	connect(core, SIGNAL(mediaLoaded()),
			 this, SLOT(enableActionsOnPlaying()));

	connect(core, SIGNAL(mediaInfoChanged()),
			 this, SLOT(updateMediaInfo()));

	connect(core, SIGNAL(mediaStopped()),
			 this, SLOT(exitFullscreenOnStop()));

	connect(core, SIGNAL(playerFailed(QProcess::ProcessError)),
			 this, SLOT(showErrorFromPlayer(QProcess::ProcessError)));
	connect(core, SIGNAL(playerFinishedWithError(int)),
			 this, SLOT(showExitCodeFromPlayer(int)));

#ifdef YOUTUBE_SUPPORT
	connect(core, SIGNAL(signatureNotFound(const QString &)),
			 this, SLOT(YTNoSignature(const QString &)));
	connect(core, SIGNAL(noSslSupport()),
			 this, SLOT(YTNoSslSupport()));
#endif
	connect(core, SIGNAL(receivedForbidden()),
			 this, SLOT(gotForbidden()));

	connect(playerwindow, SIGNAL(wheelUp()),
			 core, SLOT(wheelUp()));
	connect(playerwindow, SIGNAL(wheelDown()),
			 core, SLOT(wheelDown()));

	connect(core, SIGNAL(mediaStopped()),
			 playerwindow, SLOT(showLogo()));
	connect(playerwindow, SIGNAL(moveOSD(const QPoint &)),
			 core, SLOT(setOSDPos(const QPoint &)));
	connect(playerwindow, SIGNAL(showMessage(QString, int, int)),
			 core, SLOT(displayMessage(QString, int, int)));
}

void TBase::createPlaylist() {

	playlist = new TPlaylist(core, this, 0);
	connect(playlist, SIGNAL(playlistEnded()),
			 this, SLOT(playlistHasFinished()));
	connect(playlist, SIGNAL(displayMessage(QString,int)),
			this, SLOT(displayMessage(QString,int)));
}

void TBase::createVideoEqualizer() {

	video_equalizer = new TVideoEqualizer(this);
	video_equalizer->setBySoftware(pref->use_soft_video_eq);

	connect(video_equalizer, SIGNAL(contrastChanged(int)),
			 core, SLOT(setContrast(int)));
	connect(video_equalizer, SIGNAL(brightnessChanged(int)),
			 core, SLOT(setBrightness(int)));
	connect(video_equalizer, SIGNAL(hueChanged(int)),
			 core, SLOT(setHue(int)));
	connect(video_equalizer, SIGNAL(saturationChanged(int)),
			 core, SLOT(setSaturation(int)));
	connect(video_equalizer, SIGNAL(gammaChanged(int)),
			 core, SLOT(setGamma(int)));

	connect(video_equalizer, SIGNAL(requestToChangeDefaultValues()),
			 this, SLOT(setDefaultValuesFromVideoEqualizer()));
	connect(video_equalizer, SIGNAL(bySoftwareChanged(bool)),
			 this, SLOT(changeVideoEqualizerBySoftware(bool)));

	connect(core, SIGNAL(videoEqualizerNeedsUpdate()),
			 this, SLOT(updateVideoEqualizer()));
}

void TBase::createAudioEqualizer() {

	audio_equalizer = new TAudioEqualizer(this);

	connect(audio_equalizer->eq[0], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq0(int)));
	connect(audio_equalizer->eq[1], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq1(int)));
	connect(audio_equalizer->eq[2], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq2(int)));
	connect(audio_equalizer->eq[3], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq3(int)));
	connect(audio_equalizer->eq[4], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq4(int)));
	connect(audio_equalizer->eq[5], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq5(int)));
	connect(audio_equalizer->eq[6], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq6(int)));
	connect(audio_equalizer->eq[7], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq7(int)));
	connect(audio_equalizer->eq[8], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq8(int)));
	connect(audio_equalizer->eq[9], SIGNAL(valueChanged(int)),
			 core, SLOT(setAudioEq9(int)));

	connect(audio_equalizer, SIGNAL(applyClicked(const Settings::TAudioEqualizerList&)),
			 core, SLOT(setAudioAudioEqualizerRestart(const Settings::TAudioEqualizerList&)));
	connect(audio_equalizer, SIGNAL(valuesChanged(const Settings::TAudioEqualizerList&)),
			 core, SLOT(setAudioEqualizer(const Settings::TAudioEqualizerList&)));
}

void TBase::createActions() {
	qDebug("Gui::TBase::createActions");

	// Menu File
	openFileAct = new TAction(this, "open_file", QT_TR_NOOP("&File..."), "open", QKeySequence("Ctrl+F"));
	connect(openFileAct, SIGNAL(triggered()), this, SLOT(openFile()));

	openDirectoryAct = new TAction(this, "open_directory", QT_TR_NOOP("D&irectory..."), "openfolder");
	connect(openDirectoryAct, SIGNAL(triggered()), this, SLOT(openDirectory()));

	openPlaylistAct = new TAction(this, "open_playlist", QT_TR_NOOP("&Playlist..."), "open_playlist");
	connect(openPlaylistAct, SIGNAL(triggered()), playlist, SLOT(load()));

	openVCDAct = new TAction(this, "open_vcd", QT_TR_NOOP("V&CD"), "vcd");
	connect(openVCDAct, SIGNAL(triggered()), this, SLOT(openVCD()));

	openAudioCDAct = new TAction(this, "open_audio_cd", QT_TR_NOOP("&Audio CD"), "cdda");
	connect(openAudioCDAct, SIGNAL(triggered()), this, SLOT(openAudioCD()));

	openDVDAct = new TAction(this, "open_dvd", QT_TR_NOOP("&DVD from drive"), "dvd");
	connect(openDVDAct, SIGNAL(triggered()), this, SLOT(openDVD()));

	openDVDFolderAct = new TAction(this, "open_dvd_folder", QT_TR_NOOP("D&VD from folder..."), "dvd_hd");
	connect(openDVDFolderAct, SIGNAL(triggered()), this, SLOT(openDVDFromFolder()));

	// Bluray section.
	openBluRayAct = new TAction(this, "open_bluray", QT_TR_NOOP("&Blu-ray from drive"), "bluray");
	connect(openBluRayAct, SIGNAL(triggered()), this, SLOT(openBluRay()));

	openBluRayFolderAct = new TAction(this, "open_bluray_folder", QT_TR_NOOP("Blu-&ray from folder..."), "bluray_hd");
	connect(openBluRayFolderAct, SIGNAL(triggered()), this, SLOT(openBluRayFromFolder()));

	openURLAct = new TAction(this, "open_url", QT_TR_NOOP("&URL..."), "url", QKeySequence("Ctrl+U"));
	connect(openURLAct, SIGNAL(triggered()), this, SLOT(openURL()));

	exitAct = new TAction(this, "close", QT_TR_NOOP("C&lose"), "close", QKeySequence("Ctrl+X"));
	connect(exitAct, SIGNAL(triggered()), this, SLOT(closeWindow()));

	clearRecentsAct = new TAction(this, "clear_recents", QT_TR_NOOP("&Clear"), "delete");
	connect(clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()));

	// Favorites
	favorites = new TFavorites(TPaths::configPath() + "/favorites.m3u8", this);
	favorites->menuAction()->setObjectName("favorites_menu");
	addAction(favorites->editAct());
	addAction(favorites->jumpAct());
	addAction(favorites->nextAct());
	addAction(favorites->previousAct());
	connect(favorites, SIGNAL(activated(QString)), this, SLOT(openFavorite(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			favorites, SLOT(getCurrentMedia(const QString&, const QString&)));

	// TV and Radio
	tvlist = new TTVList(pref->check_channels_conf_on_startup, 
						 TTVList::TV, TPaths::configPath() + "/tv.m3u8", this);
	tvlist->menuAction()->setObjectName("tv_menu");
	addAction(tvlist->editAct());
	addAction(tvlist->jumpAct());
	addAction(tvlist->nextAct());
	addAction(tvlist->previousAct());
	tvlist->nextAct()->setShortcut(Qt::Key_H);
	tvlist->previousAct()->setShortcut(Qt::Key_L);
	tvlist->nextAct()->setObjectName("next_tv");
	tvlist->previousAct()->setObjectName("previous_tv");
	tvlist->editAct()->setObjectName("edit_tv_list");
	tvlist->jumpAct()->setObjectName("jump_tv_list");
	connect(tvlist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			tvlist, SLOT(getCurrentMedia(const QString&, const QString&)));

	radiolist = new TTVList(pref->check_channels_conf_on_startup, 
							TTVList::Radio, TPaths::configPath() + "/radio.m3u8", this);
	radiolist->menuAction()->setObjectName("radio_menu");
	addAction(radiolist->editAct());
	addAction(radiolist->jumpAct());
	addAction(radiolist->nextAct());
	addAction(radiolist->previousAct());
	radiolist->nextAct()->setShortcut(Qt::SHIFT | Qt::Key_H);
	radiolist->previousAct()->setShortcut(Qt::SHIFT | Qt::Key_L);
	radiolist->nextAct()->setObjectName("next_radio");
	radiolist->previousAct()->setObjectName("previous_radio");
	radiolist->editAct()->setObjectName("edit_radio_list");
	radiolist->jumpAct()->setObjectName("jump_radio_list");
	connect(radiolist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			radiolist, SLOT(getCurrentMedia(const QString&, const QString&)));


	// Menu Play
	playAct = new TAction(this, "play", QT_TR_NOOP("P&lay"), "play");
	connect(playAct, SIGNAL(triggered()), core, SLOT(play()));

	playOrPauseAct = new TAction(this, "play_or_pause", QT_TR_NOOP("Play / Pause"), "play_pause", Qt::Key_MediaPlay);
	playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause")); // MCE remote key
	connect(playOrPauseAct, SIGNAL(triggered()), core, SLOT(playOrPause()));

	pauseAct = new TAction(this, "pause", QT_TR_NOOP("&Pause"), "pause", Qt::Key_Space);
	pauseAct->addShortcut(QKeySequence("Media Pause")); // MCE remote key
	connect(pauseAct, SIGNAL(triggered()), core, SLOT(pause()));

	stopAct = new TAction(this, "stop", QT_TR_NOOP("&Stop"), "stop", Qt::Key_MediaStop);
	connect(stopAct, SIGNAL(triggered()), core, SLOT(stop()));

	frameStepAct = new TAction(this, "frame_step", QT_TR_NOOP("&Frame step"), "frame_step", Qt::Key_Period);
	connect(frameStepAct, SIGNAL(triggered()), core, SLOT(frameStep()));

	frameBackStepAct = new TAction(this, "frame_back_step", QT_TR_NOOP("Fra&me back step"), "frame_back_step", Qt::Key_Comma);
	connect(frameBackStepAct, SIGNAL(triggered()), core, SLOT(frameBackStep()));

	rewind1Act = new TAction(this, "rewind1", "", "rewind10s", Qt::Key_Left);
	rewind1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
	connect(rewind1Act, SIGNAL(triggered()), core, SLOT(srewind()));

	rewind2Act = new TAction(this, "rewind2", "", "rewind1m", Qt::Key_Down);
	connect(rewind2Act, SIGNAL(triggered()), core, SLOT(rewind()));

	rewind3Act = new TAction(this, "rewind3", "", "rewind10m", Qt::Key_PageDown);
	connect(rewind3Act, SIGNAL(triggered()), core, SLOT(fastrewind()));

	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new TSeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");

	forward1Act = new TAction(this, "forward1", "", "forward10s", Qt::Key_Right);
	forward1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
	connect(forward1Act, SIGNAL(triggered()), core, SLOT(sforward()));

	forward2Act = new TAction(this, "forward2", "", "forward1m", Qt::Key_Up);
	connect(forward2Act, SIGNAL(triggered()), core, SLOT(forward()));

	forward3Act = new TAction(this, "forward3", "", "forward10m", Qt::Key_PageUp);
	connect(forward3Act, SIGNAL(triggered()), core, SLOT(fastforward()));

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new TSeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");

	setAMarkerAct = new TAction(this, "set_a_marker", QT_TR_NOOP("Set &A marker"), "a_marker");
	connect(setAMarkerAct, SIGNAL(triggered()), core, SLOT(setAMarker()));

	setBMarkerAct = new TAction(this, "set_b_marker", QT_TR_NOOP("Set &B marker"), "b_marker");
	connect(setBMarkerAct, SIGNAL(triggered()), core, SLOT(setBMarker()));

	clearABMarkersAct = new TAction(this, "clear_ab_markers", QT_TR_NOOP("&Clear A-B markers"), "clear_ab_markers");
	connect(clearABMarkersAct, SIGNAL(triggered()), core, SLOT(clearABMarkers()));

	repeatAct = new TAction(this, "repeat", QT_TR_NOOP("&Repeat"), "repeat");
	repeatAct->setCheckable(true);
	connect(repeatAct, SIGNAL(toggled(bool)), core, SLOT(toggleRepeat(bool)));

	gotoAct = new TAction(this, "jump_to", QT_TR_NOOP("&Jump to..."), "jumpto", QKeySequence("Ctrl+J"));
	connect(gotoAct, SIGNAL(triggered()), this, SLOT(showGotoDialog()));


	// Menu Video
	fullscreenAct = new TAction(this, "fullscreen", QT_TR_NOOP("&Fullscreen"), "fullscreen", Qt::Key_F);
	fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable(true);
	connect(fullscreenAct, SIGNAL(toggled(bool)), this, SLOT(toggleFullscreen(bool)));

	videoEqualizerAct = new TAction(this, "video_equalizer", QT_TR_NOOP("&Equalizer"), "equalizer", QKeySequence("Ctrl+E"));
	videoEqualizerAct->setCheckable(true);
	videoEqualizerAct->setChecked(video_equalizer->isVisible());
	connect(videoEqualizerAct, SIGNAL(toggled(bool)), this, SLOT(showVideoEqualizer(bool)));
	connect(video_equalizer, SIGNAL(visibilityChanged(bool)), videoEqualizerAct, SLOT(setChecked(bool)));

	// Single screenshot
	screenshotAct = new TAction(this, "screenshot", QT_TR_NOOP("&Screenshot"), "screenshot", Qt::Key_S);
	connect(screenshotAct, SIGNAL(triggered()), core, SLOT(screenshot()));

	// Multiple screenshots
	screenshotsAct = new TAction(this, "multiple_screenshots", QT_TR_NOOP("Start/stop takin&g screenshots"), "screenshots", QKeySequence("Shift+D"));
	connect(screenshotsAct, SIGNAL(triggered()), core, SLOT(screenshots()));

#ifdef CAPTURE_STREAM
	capturingAct = new TAction(this, "capture_stream", QT_TR_NOOP("Start/stop capturing stream"), "record");
	connect(capturingAct, SIGNAL(triggered()), core, SLOT(switchCapturing()) );
#endif

#ifdef VIDEOPREVIEW
	videoPreviewAct = new TAction(this, "video_preview", QT_TR_NOOP("Thumb&nail generator..."), "video_preview");
	connect(videoPreviewAct, SIGNAL(triggered()), this, SLOT(showVideoPreviewDialog()));
#endif

	flipAct = new TAction(this, "flip", QT_TR_NOOP("Fli&p image"), "flip");
	flipAct->setCheckable(true);
	connect(flipAct, SIGNAL(toggled(bool)), core, SLOT(toggleFlip(bool)));

	mirrorAct = new TAction(this, "mirror", QT_TR_NOOP("Mirr&or image"), "mirror");
	mirrorAct->setCheckable(true);
	connect(mirrorAct, SIGNAL(toggled(bool)), core, SLOT(toggleMirror(bool)));

	stereo3dAct = new TAction(this, "stereo_3d_filter", QT_TR_NOOP("Stereo &3D filter..."), "stereo3d");
	connect(stereo3dAct, SIGNAL(triggered()), this, SLOT(showStereo3dDialog()));


	// Menu Audio
	audioEqualizerAct = new TAction(this, "audio_equalizer", QT_TR_NOOP("E&qualizer"), "audio_equalizer");
	audioEqualizerAct->setCheckable(true);
	audioEqualizerAct->setChecked(audio_equalizer->isVisible());
	connect(audioEqualizerAct, SIGNAL(toggled(bool)), this, SLOT(showAudioEqualizer(bool)));
	connect(audio_equalizer, SIGNAL(visibilityChanged(bool)), audioEqualizerAct, SLOT(setChecked(bool)));

	muteAct = new TAction(this, "mute", QT_TR_NOOP("&Mute"), Qt::Key_M);
	muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
	muteAct->setCheckable(true);

	QIcon icset(Images::icon("volume"));
	icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
	muteAct->setIcon(icset);

	connect(muteAct, SIGNAL(toggled(bool)), core, SLOT(mute(bool)));
	connect(core, SIGNAL(muteChanged(bool)), muteAct, SLOT(setChecked(bool)));

	decVolumeAct = new TAction(this, "decrease_volume", QT_TR_NOOP("Volume &-"), "audio_down");
	decVolumeAct->setShortcuts(TActionsEditor::stringToShortcuts("9,/"));
	decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
	connect(decVolumeAct, SIGNAL(triggered()), core, SLOT(decVolume()));

	incVolumeAct = new TAction(this, "increase_volume", QT_TR_NOOP("Volume &+"), "audio_up");
	incVolumeAct->setShortcuts(TActionsEditor::stringToShortcuts("0,*"));
	incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
	connect(incVolumeAct, SIGNAL(triggered()), core, SLOT(incVolume()));

	decAudioDelayAct = new TAction(this, "dec_audio_delay", QT_TR_NOOP("&Delay -"), "delay_down", Qt::Key_Minus);
	connect(decAudioDelayAct, SIGNAL(triggered()), core, SLOT(decAudioDelay()));

	incAudioDelayAct = new TAction(this, "inc_audio_delay", QT_TR_NOOP("D&elay +"), "delay_up", Qt::Key_Plus);
	connect(incAudioDelayAct, SIGNAL(triggered()), core, SLOT(incAudioDelay()));

	audioDelayAct = new TAction(this, "audio_delay", QT_TR_NOOP("Set dela&y..."), "audio_delay");
	connect(audioDelayAct, SIGNAL(triggered()), this, SLOT(showAudioDelayDialog()));

	loadAudioAct = new TAction(this, "load_audio_file", QT_TR_NOOP("&Load external file..."), "open");
	connect(loadAudioAct, SIGNAL(triggered()), this, SLOT(loadAudioFile()));

	unloadAudioAct = new TAction(this, "unload_audio_file", QT_TR_NOOP("U&nload"), "unload");
	connect(unloadAudioAct, SIGNAL(triggered()), core, SLOT(unloadAudioFile()));


	// Submenu Filters
#ifdef MPLAYER_SUPPORT
	extrastereoAct = new TAction(this, "extrastereo_filter", QT_TR_NOOP("&Extrastereo"));
	extrastereoAct->setCheckable(true);
	connect(extrastereoAct, SIGNAL(toggled(bool)), core, SLOT(toggleExtrastereo(bool)));

	karaokeAct = new TAction(this, "karaoke_filter", QT_TR_NOOP("&Karaoke"));
	karaokeAct->setCheckable(true);
	connect(karaokeAct, SIGNAL(toggled(bool)), core, SLOT(toggleKaraoke(bool)));
#endif

	volnormAct = new TAction(this, "volnorm_filter", QT_TR_NOOP("Volume &normalization"));
	volnormAct->setCheckable(true);
	connect(volnormAct, SIGNAL(toggled(bool)), core, SLOT(toggleVolnorm(bool)));


	// Menu Subtitles
	loadSubsAct = new TAction(this, "load_subs", QT_TR_NOOP("&Load..."), "open");
	connect(loadSubsAct, SIGNAL(triggered()), this, SLOT(loadSub()));

	unloadSubsAct = new TAction(this, "unload_subs", QT_TR_NOOP("U&nload"), "unload");
	connect(unloadSubsAct, SIGNAL(triggered()), core, SLOT(unloadSub()));

	decSubDelayAct = new TAction(this, "dec_sub_delay", QT_TR_NOOP("Delay &-"), "delay_down", Qt::Key_Z);
	connect(decSubDelayAct, SIGNAL(triggered()), core, SLOT(decSubDelay()));

	incSubDelayAct = new TAction(this, "inc_sub_delay", QT_TR_NOOP("Delay &+"), "delay_up", Qt::Key_X);
	connect(incSubDelayAct, SIGNAL(triggered()),
			 core, SLOT(incSubDelay()));

	subDelayAct = new TAction(this, "sub_delay", QT_TR_NOOP("Se&t delay..."), "sub_delay");
	connect(subDelayAct, SIGNAL(triggered()), this, SLOT(showSubDelayDialog()));

	decSubPosAct = new TAction(this, "dec_sub_pos", QT_TR_NOOP("&Up"), "sub_up", Qt::Key_R);
	connect(decSubPosAct, SIGNAL(triggered()), core, SLOT(decSubPos()));
	incSubPosAct = new TAction(this, "inc_sub_pos", QT_TR_NOOP("&Down"), "sub_down", Qt::Key_T);
	connect(incSubPosAct, SIGNAL(triggered()), core, SLOT(incSubPos()));

	decSubScaleAct = new TAction(this, "dec_sub_scale", QT_TR_NOOP("S&ize -"), "dec_sub_scale", Qt::SHIFT | Qt::Key_R);
	connect(decSubScaleAct, SIGNAL(triggered()), core, SLOT(decSubScale()));

	incSubScaleAct = new TAction(this, "inc_sub_scale", QT_TR_NOOP("Si&ze +"), "inc_sub_scale", Qt::SHIFT | Qt::Key_T);
	connect(incSubScaleAct, SIGNAL(triggered()), core, SLOT(incSubScale()));

	decSubStepAct = new TAction(this, "dec_sub_step", QT_TR_NOOP("&Previous line in subtitles"), "dec_sub_step", Qt::Key_G);
	connect(decSubStepAct, SIGNAL(triggered()), core, SLOT(decSubStep()));

	incSubStepAct = new TAction(this, "inc_sub_step", QT_TR_NOOP("N&ext line in subtitles"), "inc_sub_step", Qt::Key_Y);
	connect(incSubStepAct, SIGNAL(triggered()), core, SLOT(incSubStep()));

#ifdef MPV_SUPPORT
	seekNextSubAct = new TAction(this, "seek_next_sub", QT_TR_NOOP("Seek to next subtitle"), "seek_next_sub", Qt::CTRL | Qt::Key_Right);
	connect(seekNextSubAct, SIGNAL(triggered()), core, SLOT(seekToNextSub()));
	seekPrevSubAct = new TAction(this, "seek_prev_sub", QT_TR_NOOP("Seek to previous subtitle"), "seek_prev_sub", Qt::CTRL | Qt::Key_Left);
	connect(seekPrevSubAct, SIGNAL(triggered()), core, SLOT(seekToPrevSub()));
#endif

	useCustomSubStyleAct = new TAction(this, "use_custom_sub_style", QT_TR_NOOP("Use custo&m style"), "use_custom_sub_style");
	useCustomSubStyleAct->setCheckable(true);
	useCustomSubStyleAct->setChecked(pref->enable_ass_styles);
	connect(useCustomSubStyleAct, SIGNAL(toggled(bool)), core, SLOT(changeUseCustomSubStyle(bool)));

	useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only", QT_TR_NOOP("&Forced subtitles only"), "forced_subs");
	useForcedSubsOnlyAct->setCheckable(true);
	useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
	connect(useForcedSubsOnlyAct, SIGNAL(toggled(bool)), core, SLOT(toggleForcedSubsOnly(bool)));

#ifdef FIND_SUBTITLES
	showFindSubtitlesDialogAct = new TAction(this, "show_find_sub_dialog", QT_TR_NOOP("Find subtitles at &OpenSubtitles.org..."), "download_subs");
	connect(showFindSubtitlesDialogAct, SIGNAL(triggered()), this, SLOT(showFindSubtitlesDialog()));

	openUploadSubtitlesPageAct = new TAction(this, "upload_subtitles", QT_TR_NOOP("Upload su&btitles to OpenSubtitles.org..."), "upload_subs");
	connect(openUploadSubtitlesPageAct, SIGNAL(triggered()), this, SLOT(openUploadSubtitlesPage()));
#endif

	// Menu Options
	showPlaylistAct = new TAction(this, "show_playlist", QT_TR_NOOP("&Playlist"), "playlist", QKeySequence("Ctrl+P"));
	showPlaylistAct->setCheckable(true);
	connect(showPlaylistAct, SIGNAL(toggled(bool)), this, SLOT(showPlaylist(bool)));
	connect(playlist, SIGNAL(visibilityChanged(bool)), showPlaylistAct, SLOT(setChecked(bool)));

	showPropertiesAct = new TAction(this, "show_file_properties", QT_TR_NOOP("View &info and properties..."), "info", QKeySequence("Ctrl+I"));
	connect(showPropertiesAct, SIGNAL(triggered()), this, SLOT(showFilePropertiesDialog()));

	showPreferencesAct = new TAction(this, "show_preferences", QT_TR_NOOP("P&references"), "prefs", QKeySequence("Ctrl+S"));
	connect(showPreferencesAct, SIGNAL(triggered()), this, SLOT(showPreferencesDialog()));

#ifdef YOUTUBE_SUPPORT
	showTubeBrowserAct = new TAction(this, "show_tube_browser", QT_TR_NOOP("&YouTube browser"), "tubebrowser", Qt::Key_F11);
	connect(showTubeBrowserAct, SIGNAL(triggered()), this, SLOT(showTubeBrowser()));
#endif
	// Show log
	showLogAct = new TAction(this, "show_smplayer_log", QT_TR_NOOP("&View log"), "log", QKeySequence("Ctrl+L"));
	showLogAct->setCheckable(true);
	connect(showLogAct, SIGNAL(triggered()), this, SLOT(showLog()));
	connect(log_window, SIGNAL(visibilityChanged(bool)), showLogAct, SLOT(setChecked(bool)));


	// Menu Help
	showFirstStepsAct = new TAction(this, "first_steps", QT_TR_NOOP("First Steps &Guide"), "guide");
	connect(showFirstStepsAct, SIGNAL(triggered()), this, SLOT(helpFirstSteps()));

	showFAQAct = new TAction(this, "faq", QT_TR_NOOP("&FAQ"), "faq");
	connect(showFAQAct, SIGNAL(triggered()), this, SLOT(helpFAQ()));

	showCLOptionsAct = new TAction(this, "cl_options", QT_TR_NOOP("&Command line options"), "cl_help");
	connect(showCLOptionsAct, SIGNAL(triggered()), this, SLOT(helpCLOptions()));

	showCheckUpdatesAct = new TAction(this, "check_updates", QT_TR_NOOP("Check for &updates"), "check_updates");
	connect(showCheckUpdatesAct, SIGNAL(triggered()), this, SLOT(helpCheckUpdates()));

#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	updateYTAct = new TAction(this, "update_youtube", QT_TR_NOOP("Update &Youtube code"), "update_youtube");
	connect(updateYTAct, SIGNAL(triggered()), this, SLOT(YTUpdateScript()));
#endif

	showConfigAct = new TAction(this, "show_config", QT_TR_NOOP("&Open configuration folder"), "show_config");
	connect(showConfigAct, SIGNAL(triggered()), this, SLOT(helpShowConfig()));

	aboutThisAct = new TAction(this, "about_smplayer", QT_TR_NOOP("About &SMPlayer"), "logo");
	connect(aboutThisAct, SIGNAL(triggered()), this, SLOT(helpAbout()));


	// Playlist
	playNextAct = new TAction(this, "play_next", QT_TR_NOOP("&Next"), "next", Qt::Key_Greater);
	playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
	connect(playNextAct, SIGNAL(triggered()), playlist, SLOT(playNext()));

	playPrevAct = new TAction(this, "play_prev", QT_TR_NOOP("Pre&vious"), "previous", Qt::Key_Less);
	playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
	connect(playPrevAct, SIGNAL(triggered()), playlist, SLOT(playPrev()));

	// Actions not in menus or buttons
	exitFullscreenAct = new TAction(this, "exit_fullscreen", QT_TR_NOOP("Exit fullscreen"), Qt::Key_Escape);
	connect(exitFullscreenAct, SIGNAL(triggered()), this, SLOT(exitFullscreen()));

	nextOSDLevelAct = new TAction(this, "next_osd", QT_TR_NOOP("OSD - Next level"), Qt::Key_O);
	connect(nextOSDLevelAct, SIGNAL(triggered()), core, SLOT(nextOSDLevel()));

	decContrastAct = new TAction(this, "dec_contrast", QT_TR_NOOP("Dec contrast"), Qt::Key_1);
	connect(decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()));

	incContrastAct = new TAction(this, "inc_contrast", QT_TR_NOOP("Inc contrast"), Qt::Key_2);
	connect(incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()));

	decBrightnessAct = new TAction(this, "dec_brightness", QT_TR_NOOP("Dec brightness"), Qt::Key_3);
	connect(decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()));

	incBrightnessAct = new TAction(this, "inc_brightness", QT_TR_NOOP("Inc brightness"), Qt::Key_4);
	connect(incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()));

	decHueAct = new TAction(this, "dec_hue", QT_TR_NOOP("Dec hue"), Qt::Key_5);
	connect(decHueAct, SIGNAL(triggered()), core, SLOT(decHue()));

	incHueAct = new TAction(this, "inc_hue", QT_TR_NOOP("Inc hue"), Qt::Key_6);
	connect(incHueAct, SIGNAL(triggered()), core, SLOT(incHue()));

	decSaturationAct = new TAction(this, "dec_saturation", QT_TR_NOOP("Dec saturation"), Qt::Key_7);
	connect(decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()));

	incSaturationAct = new TAction(this, "inc_saturation", QT_TR_NOOP("Inc saturation"), Qt::Key_8);
	connect(incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()));

	decGammaAct = new TAction(this, "dec_gamma", QT_TR_NOOP("Dec gamma"));
	connect(decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()));

	incGammaAct = new TAction(this, "inc_gamma", QT_TR_NOOP("Inc gamma"));
	connect(incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()));

	nextVideoAct = new TAction(this, "next_video", QT_TR_NOOP("Next video"));
	connect(nextVideoAct, SIGNAL(triggered()), core, SLOT(nextVideoTrack()));

	nextAudioAct = new TAction(this, "next_audio", QT_TR_NOOP("Next audio"), Qt::Key_K);
	connect(nextAudioAct, SIGNAL(triggered()), core, SLOT(nextAudioTrack()));

	nextSubtitleAct = new TAction(this, "next_subtitle", QT_TR_NOOP("Next subtitle"), Qt::Key_J);
	connect(nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()));

	nextChapterAct = new TAction(this, "next_chapter", QT_TR_NOOP("Next chapter"), Qt::Key_At);
	connect(nextChapterAct, SIGNAL(triggered()), core, SLOT(nextChapter()));

	prevChapterAct = new TAction(this, "prev_chapter", QT_TR_NOOP("Previous chapter"), Qt::Key_Exclam);
	connect(prevChapterAct, SIGNAL(triggered()), core, SLOT(prevChapter()));

	resetVideoEqualizerAct = new TAction(this, "reset_video_equalizer", QT_TR_NOOP("Reset video equalizer"));
	connect(resetVideoEqualizerAct, SIGNAL(triggered()), video_equalizer, SLOT(reset()));

	resetAudioEqualizerAct = new TAction(this, "reset_audio_equalizer", QT_TR_NOOP("Reset audio equalizer"));
	connect(resetAudioEqualizerAct, SIGNAL(triggered()), audio_equalizer, SLOT(reset()));

	showContextMenuAct = new TAction(this, "show_context_menu", QT_TR_NOOP("Show context menu"));
	connect(showContextMenuAct, SIGNAL(triggered()), this, SLOT(showContextMenu()));

	nextAspectAct = new TAction(this, "next_aspect", QT_TR_NOOP("Next aspect ratio"), "next_aspect", Qt::Key_A);
	connect(nextAspectAct, SIGNAL(triggered()), core, SLOT(nextAspectRatio()));

	nextWheelFunctionAct = new TAction(this, "next_wheel_function", QT_TR_NOOP("Next wheel function"), "next_wheel_function");
	connect(nextWheelFunctionAct, SIGNAL(triggered()), core, SLOT(nextWheelFunction()));

	showFilenameAct = new TAction(this, "show_filename", QT_TR_NOOP("Show filename on OSD"), Qt::SHIFT | Qt::Key_I);
	connect(showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()));

	showTimeAct = new TAction(this, "show_time", QT_TR_NOOP("Show playback time on OSD"), Qt::Key_I);
	connect(showTimeAct, SIGNAL(triggered()), core, SLOT(showTimeOnOSD()));

	toggleDeinterlaceAct = new TAction(this, "toggle_deinterlacing", QT_TR_NOOP("Toggle deinterlacing"), Qt::Key_D);
	connect(toggleDeinterlaceAct, SIGNAL(triggered()), core, SLOT(toggleDeinterlace()));


#if USE_ADAPTER
	// TODO: convert to new action syntax
	screenGroup = new TActionGroup(this, "screen");
	screenDefaultAct = new TActionGroupItem(this, screenGroup, "screen_default", -1);

#ifdef Q_OS_WIN
	TDeviceList display_devices = TDeviceInfo::displayDevices();
	if (!display_devices.isEmpty()) {
		for (int n = 0; n < display_devices.count(); n++) {
			int id = display_devices[n].ID().toInt();
			QString desc = display_devices[n].desc();
			TAction* screen_item = new TActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toLatin1().constData(), id);
			screen_item->change("&"+QString::number(n) + " - " + desc);
		}
	}
	else
#endif // Q_OS_WIN

	for (int n = 1; n <= 4; n++) {
		TAction* screen_item = new TActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toLatin1().constData(), n);
		screen_item->change("&"+QString::number(n));
	}

	screenGroup->setChecked(pref->adapter);
	connect(screenGroup, SIGNAL(activated(int)), core, SLOT(changeAdapter(int)));
#endif // USE_ADAPTER

#if PROGRAM_SWITCH
	// Program track
	programTrackGroup = new TActionGroup(this, "programtrack");
	connect(programTrackGroup, SIGNAL(activated(int)), core, SLOT(changeProgram(int)));
#endif

	// Video track
	videoTrackGroup = new TActionGroup(this, "videotrack");
	connect(videoTrackGroup, SIGNAL(activated(int)), core, SLOT(changeVideoTrack(int)));
	connect(core, SIGNAL(videoTrackInfoChanged()), this, SLOT(updateVideoTracks()));
	connect(core, SIGNAL(videoTrackChanged(int)), videoTrackGroup, SLOT(setCheckedSlot(int)));

	// Audio track
	audioTrackGroup = new TActionGroup(this, "audiotrack");
	connect(audioTrackGroup, SIGNAL(activated(int)), core, SLOT(changeAudioTrack(int)));
	connect(core, SIGNAL(audioTrackInfoChanged()), this, SLOT(updateAudioTracks()));
	connect(core, SIGNAL(audioTrackChanged(int)), audioTrackGroup, SLOT(setCheckedSlot(int)));

	subtitleTrackGroup = new TActionGroup(this, "subtitletrack");
	connect(subtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSubtitle(int)));
	connect(core, SIGNAL(subtitleInfoChanged()), this, SLOT(updateSubtitles()));
	connect(core, SIGNAL(subtitleTrackChanged(int)), subtitleTrackGroup, SLOT(setCheckedSlot(int)));

#ifdef MPV_SUPPORT
	// Secondary subtitle track
	secondarySubtitleTrackGroup = new TActionGroup(this, "secondarysubtitletrack");
	connect(secondarySubtitleTrackGroup, SIGNAL(activated(int)), core, SLOT(changeSecondarySubtitle(int)));
	// InfoChanged already connected by subtitleTrackGroup
	// checked not needed
	// connect(core, SIGNAL(secondarySubtitleTrackChanged(int)),
	//		 secondarySubtitleTrackGroup, SLOT(setCheckedSlot(int)));
#endif

	// Titles
	titleGroup = new TActionGroup(this, "title");
	connect(titleGroup, SIGNAL(activated(int)), core, SLOT(changeTitle(int)));
	connect(core, SIGNAL(titleTrackChanged(int)), titleGroup, SLOT(setCheckedSlot(int)));
	connect(core, SIGNAL(titleTrackInfoChanged()), this, SLOT(updateTitles()));

	// Chapters
	chapterGroup = new TActionGroup(this, "chapter");
	connect(chapterGroup, SIGNAL(activated(int)), core, SLOT(changeChapter(int)));
	connect(core, SIGNAL(chapterChanged(int)), chapterGroup, SLOT(setCheckedSlot(int)));
	// Update chapter info done by updateTitles.
	// DVDNAV only:
	connect(core, SIGNAL(chapterInfoChanged()), this, SLOT(updateChapters()));

	// Angles
	angleGroup = new TActionGroup(this, "angle");
	connect(angleGroup, SIGNAL(activated(int)), core, SLOT(changeAngle(int)));
	// Update done by updateTitles

	dvdnavUpAct = new TAction(this, "dvdnav_up", QT_TR_NOOP("DVD menu, move up"), "dvdnav_up", Qt::SHIFT | Qt::Key_Up);
	connect(dvdnavUpAct, SIGNAL(triggered()), core, SLOT(dvdnavUp()));

	dvdnavDownAct = new TAction(this, "dvdnav_down", QT_TR_NOOP("DVD menu, move down"), "dvdnav_down", Qt::SHIFT | Qt::Key_Down);
	connect(dvdnavDownAct, SIGNAL(triggered()), core, SLOT(dvdnavDown()));

	dvdnavLeftAct = new TAction(this, "dvdnav_left", QT_TR_NOOP("DVD menu, move left"), "dvdnav_left", Qt::SHIFT | Qt::Key_Left);
	connect(dvdnavLeftAct, SIGNAL(triggered()), core, SLOT(dvdnavLeft()));

	dvdnavRightAct = new TAction(this, "dvdnav_right", QT_TR_NOOP("DVD menu, move right"), "dvdnav_right", Qt::SHIFT | Qt::Key_Right);
	connect(dvdnavRightAct, SIGNAL(triggered()), core, SLOT(dvdnavRight()));

	dvdnavMenuAct = new TAction(this, "dvdnav_menu", QT_TR_NOOP("DVD &menu"), "dvdnav_menu", Qt::SHIFT | Qt::Key_Return);
	connect(dvdnavMenuAct, SIGNAL(triggered()), core, SLOT(dvdnavMenu()));

	dvdnavSelectAct = new TAction(this, "dvdnav_select", QT_TR_NOOP("DVD menu, select option"), "dvdnav_select", Qt::Key_Return);
	connect(dvdnavSelectAct, SIGNAL(triggered()), core, SLOT(dvdnavSelect()));

	dvdnavPrevAct = new TAction(this, "dvdnav_prev", QT_TR_NOOP("DVD &previous menu"), "dvdnav_prev", Qt::SHIFT | Qt::Key_Escape);
	connect(dvdnavPrevAct, SIGNAL(triggered()), core, SLOT(dvdnavPrev()));

	dvdnavMouseAct = new TAction(this, "dvdnav_mouse", QT_TR_NOOP("DVD menu, mouse click"), "dvdnav_mouse");
	connect(dvdnavMouseAct, SIGNAL(triggered()), core, SLOT(dvdnavMouse()));

	// Time slider action
	timeslider_action = new TTimeSliderAction(this, core->positionMax(), pref->time_slider_drag_delay);
	timeslider_action->setObjectName("timeslider_action");

	connect(timeslider_action, SIGNAL(posChanged(int)), core, SLOT(goToPosition(int)));
	connect(core, SIGNAL(positionChanged(int)), timeslider_action, SLOT(setPos(int)));
	connect(core, SIGNAL(durationChanged(double)), timeslider_action, SLOT(setDuration(double)));

	connect(timeslider_action, SIGNAL(draggingPos(int)), this, SLOT(displayGotoTime(int)));
	connect(timeslider_action, SIGNAL(delayedDraggingPos(int)), this, SLOT(goToPosOnDragging(int)));

	connect(timeslider_action, SIGNAL(wheelUp(Settings::TPreferences::WheelFunction)),
			core, SLOT(wheelUp(Settings::TPreferences::WheelFunction)));
	connect(timeslider_action, SIGNAL(wheelDown(Settings::TPreferences::WheelFunction)),
			core, SLOT(wheelDown(Settings::TPreferences::WheelFunction)));

	// Volume slider action
	volumeslider_action = new TVolumeSliderAction(this, core->getVolume());
	volumeslider_action->setObjectName("volumeslider_action");
	connect(volumeslider_action, SIGNAL(valueChanged(int)), core, SLOT(setVolume(int)));
	connect(core, SIGNAL(volumeChanged(int)), volumeslider_action, SLOT(setValue(int)));

	// Time label actions
	time_label_action = new TTimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	// Menu bar
	viewMenuBarAct = new TAction(this, "toggle_menubar", QT_TR_NOOP("Me&nu bar"), Qt::Key_F2);
	viewMenuBarAct->setCheckable(true);
	viewMenuBarAct->setChecked(true);
	connect(viewMenuBarAct, SIGNAL(toggled(bool)), menuBar(), SLOT(setVisible(bool)));

	// Toolbars
	editToolbarAct = new TAction(this, "edit_toolbar1", QT_TR_NOOP("Edit main &toolbar"));
	editToolbar2Act = new TAction(this, "edit_toolbar2", QT_TR_NOOP("Edit extra t&oolbar"));

	// Control bar
	editControlBarAct = new TAction(this, "edit_controlbar", QT_TR_NOOP("Edit control &bar"));

	// Status bar
	viewStatusBarAct = new TAction(this, "toggle_statusbar", QT_TR_NOOP("&Status bar"), Qt::Key_F6);
	viewStatusBarAct->setCheckable(true);
	viewStatusBarAct->setChecked(true);
	connect(viewStatusBarAct, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
} // createActions

void TBase::createMenus() {

	// MENUS
	openMenu = menuBar()->addMenu("Open");
	playMenu = menuBar()->addMenu("Play");
	videoMenu = menuBar()->addMenu("Video");
	audioMenu = menuBar()->addMenu("Audio");
	subtitlesMenu = menuBar()->addMenu("Subtitles");
	/* menuBar()->addMenu(favorites); */
	browseMenu = menuBar()->addMenu("Browse");
	optionsMenu = menuBar()->addMenu("Options");
	helpMenu = menuBar()->addMenu("Help");

	// OPEN MENU
	openMenu->addAction(openFileAct);

	recentfiles_menu = new QMenu(this);
	recentfiles_menu->menuAction()->setObjectName("recent_menu");
	openMenu->addMenu(recentfiles_menu);
	openMenu->addMenu(favorites);
	openMenu->addAction(openDirectoryAct);
	openMenu->addAction(openPlaylistAct);

	// Disc submenu
	disc_menu = new QMenu(this);
	disc_menu->menuAction()->setObjectName("disc_menu");
	disc_menu->addAction(openDVDAct);
	disc_menu->addAction(openDVDFolderAct);
	disc_menu->addAction(openBluRayAct);
	disc_menu->addAction(openBluRayFolderAct);
	disc_menu->addAction(openVCDAct);
	disc_menu->addAction(openAudioCDAct);

	openMenu->addMenu(disc_menu);

	openMenu->addAction(openURLAct);
	openMenu->addMenu(tvlist);
	openMenu->addMenu(radiolist);
	openMenu->addSeparator();
	openMenu->addAction(exitAct);

	// PLAY MENU
	playMenu->addAction(playAct);
	playMenu->addAction(pauseAct);
	/* playMenu->addAction(playOrPauseAct); */
	playMenu->addAction(stopAct);
	playMenu->addAction(frameStepAct);
	playMenu->addAction(frameBackStepAct);
	playMenu->addSeparator();
	playMenu->addAction(rewind1Act);
	playMenu->addAction(forward1Act);
	playMenu->addAction(rewind2Act);
	playMenu->addAction(forward2Act);
	playMenu->addAction(rewind3Act);
	playMenu->addAction(forward3Act);
	playMenu->addSeparator();

	// Speed submenu
	speed_menu = new TPlaySpeedMenu(this, core);
	playMenu->addMenu(speed_menu);

	// A-B submenu
	ab_menu = new QMenu(this);
	ab_menu->menuAction()->setObjectName("ab_menu");
	ab_menu->addAction(setAMarkerAct);
	ab_menu->addAction(setBMarkerAct);
	ab_menu->addAction(clearABMarkersAct);
	ab_menu->addSeparator();
	ab_menu->addAction(repeatAct);

	playMenu->addSeparator();
	playMenu->addMenu(ab_menu);

	playMenu->addSeparator();
	playMenu->addAction(gotoAct);
	playMenu->addSeparator();
	playMenu->addAction(playPrevAct);
	playMenu->addAction(playNextAct);

	// VIDEO MENU
	videotrack_menu = new QMenu(this);
	videotrack_menu->menuAction()->setObjectName("videotrack_menu");

	videoMenu->addMenu(videotrack_menu);

	videoMenu->addAction(fullscreenAct);

#if USE_ADAPTER
	// Screen submenu
	screen_menu = new QMenu(this);
	screen_menu->menuAction()->setObjectName("screen_menu");
	screen_menu->addActions(screenGroup->actions());
	videoMenu->addMenu(screen_menu);
#endif

	// Size submenu
	videosize_menu = new TVideoSizeMenu(this, playerwindow);
	videoMenu->addMenu(videosize_menu);

	// Zoom submenu
	zoom_and_pan_menu = new TVideoZoomAndPanMenu(this, core);
	videoMenu->addMenu(zoom_and_pan_menu);

	// Aspect submenu
	aspect_menu = new TAspectMenu(this, core);
	videoMenu->addMenu(aspect_menu);

	// Deinterlace submenu
	deinterlace_menu = new TDeinterlaceMenu(this, core);
	videoMenu->addMenu(deinterlace_menu);

	// Video filter submenu
	videofilter_menu = new TVideoFilterMenu(this, core);
	videoMenu->addMenu(videofilter_menu);

	// Rotate submenu
	rotate_menu = new TRotateMenu(this, core);
	videoMenu->addMenu(rotate_menu);

	videoMenu->addAction(flipAct);
	videoMenu->addAction(mirrorAct);
	videoMenu->addAction(stereo3dAct);
	videoMenu->addSeparator();
	videoMenu->addAction(videoEqualizerAct);
	videoMenu->addAction(screenshotAct);
	videoMenu->addAction(screenshotsAct);

	// Ontop submenu
	stay_on_top_menu = new TStayOnTopMenu(this);
	videoMenu->addMenu(stay_on_top_menu);

#ifdef VIDEOPREVIEW
	videoMenu->addSeparator();
	videoMenu->addAction(videoPreviewAct);
#endif

	// AUDIO MENU

	// Audio track submenu
	audiotrack_menu = new QMenu(this);
	audiotrack_menu->menuAction()->setObjectName("audiotrack_menu");

	audioMenu->addMenu(audiotrack_menu);

	audioMenu->addAction(loadAudioAct);
	audioMenu->addAction(unloadAudioAct);

	// Filter submenu
	audiofilter_menu = new QMenu(this);
	audiofilter_menu->menuAction()->setObjectName("audiofilter_menu");
	audiofilter_menu->addAction(volnormAct);

#ifdef MPLAYER_SUPPORT
	audiofilter_menu->addAction(extrastereoAct);
	audiofilter_menu->addAction(karaokeAct);
#endif

	audioMenu->addMenu(audiofilter_menu);

	// Audio channels submenu
	audiochannels_menu = new TAudioChannelMenu(this, core);
	audioMenu->addMenu(audiochannels_menu);

	// Stereo mode submenu
	stereomode_menu = new TStereoMenu(this, core);

	audioMenu->addMenu(stereomode_menu);
	audioMenu->addAction(audioEqualizerAct);
	audioMenu->addSeparator();
	audioMenu->addAction(muteAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decVolumeAct);
	audioMenu->addAction(incVolumeAct);
	audioMenu->addSeparator();
	audioMenu->addAction(decAudioDelayAct);
	audioMenu->addAction(incAudioDelayAct);
	audioMenu->addSeparator();
	audioMenu->addAction(audioDelayAct);

	// SUBTITLES MENU
	// Track submenu
	subtitles_track_menu = new QMenu(this);
	subtitles_track_menu->menuAction()->setObjectName("subtitlestrack_menu");

#ifdef MPV_SUPPORT
	secondary_subtitles_track_menu = new QMenu(this);
	secondary_subtitles_track_menu->menuAction()->setObjectName("secondary_subtitles_track_menu");
#endif

	subtitlesMenu->addMenu(subtitles_track_menu);
#ifdef MPV_SUPPORT
	subtitlesMenu->addMenu(secondary_subtitles_track_menu);
#endif
	subtitlesMenu->addSeparator();

	subtitlesMenu->addAction(loadSubsAct);
	subtitlesMenu->addAction(unloadSubsAct);

	subfps_menu = new TSubFPSMenu(this, core);
	subtitlesMenu->addMenu(subfps_menu);
	subtitlesMenu->addSeparator();

	closed_captions_menu = new TCCMenu(this, core);
	subtitlesMenu->addMenu(closed_captions_menu);
	subtitlesMenu->addSeparator();

	subtitlesMenu->addAction(decSubDelayAct);
	subtitlesMenu->addAction(incSubDelayAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(subDelayAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubPosAct);
	subtitlesMenu->addAction(incSubPosAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubScaleAct);
	subtitlesMenu->addAction(incSubScaleAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(decSubStepAct);
	subtitlesMenu->addAction(incSubStepAct);
#ifdef MPV_SUPPORT
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(seekPrevSubAct);
	subtitlesMenu->addAction(seekNextSubAct);
#endif
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(useForcedSubsOnlyAct);
	subtitlesMenu->addSeparator();
	subtitlesMenu->addAction(useCustomSubStyleAct);
#ifdef FIND_SUBTITLES
	subtitlesMenu->addSeparator(); //turbos
	subtitlesMenu->addAction(showFindSubtitlesDialogAct);
	subtitlesMenu->addAction(openUploadSubtitlesPageAct); //turbos
#endif

	// BROWSE MENU
	// Titles submenu
	titles_menu = new QMenu(this);
	titles_menu->menuAction()->setObjectName("titles_menu");

	browseMenu->addMenu(titles_menu);

	// Chapters submenu
	chapters_menu = new QMenu(this);
	chapters_menu->menuAction()->setObjectName("chapters_menu");

	browseMenu->addMenu(chapters_menu);

	// Angles submenu
	angles_menu = new QMenu(this);
	angles_menu->menuAction()->setObjectName("angles_menu");

	browseMenu->addMenu(angles_menu);

	browseMenu->addSeparator();
	browseMenu->addAction(dvdnavMenuAct);
	browseMenu->addAction(dvdnavPrevAct);

#if PROGRAM_SWITCH
	programtrack_menu = new QMenu(this);
	programtrack_menu->menuAction()->setObjectName("programtrack_menu");

	browseMenu->addSeparator();
	browseMenu->addMenu(programtrack_menu);
#endif

	// OPTIONS MENU
	optionsMenu->addAction(showPropertiesAct);
	optionsMenu->addAction(showPlaylistAct);
	optionsMenu->addAction(showLogAct);

	statusbar_menu = new QMenu(this);
	// statusbar_menu added to toolbar_menu by createToolbarMenu()
	// and filled by descendants::createMenus()
	toolbar_menu = createToolbarMenu();
	optionsMenu->addMenu(toolbar_menu);

	osd_menu = new TOSDMenu(this, core);
	optionsMenu->addMenu(osd_menu);

#ifdef YOUTUBE_SUPPORT
	#if 0
	// Check if the smplayer youtube browser is installed
	{
		QString tube_exec = TPaths::appPath() + "/smtube";
		#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		tube_exec += ".exe";
		#endif
		if (QFile::exists(tube_exec)) {
			optionsMenu->addAction(showTubeBrowserAct);
			qDebug("Gui::TBase::createMenus: %s does exist", tube_exec.toUtf8().constData());
		} else {
			qDebug("Gui::TBase::createMenus: %s does not exist", tube_exec.toUtf8().constData());
		}
	}
	#else
	optionsMenu->addAction(showTubeBrowserAct);
	#endif
#endif

	// Preferences
	optionsMenu->addSeparator();
	optionsMenu->addAction(showPreferencesAct);

	// HELP MENU
	helpMenu->addAction(showFirstStepsAct);
	helpMenu->addAction(showFAQAct);
	helpMenu->addAction(showCLOptionsAct);
	helpMenu->addSeparator();
	helpMenu->addAction(showCheckUpdatesAct);
#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	helpMenu->addAction(updateYTAct);
#endif
	helpMenu->addSeparator();
	helpMenu->addAction(showConfigAct);
	helpMenu->addSeparator();
	helpMenu->addAction(aboutThisAct);

	// POPUP MENU
	popup = new QMenu(this);
	popup->addMenu(openMenu);
	popup->addMenu(playMenu);
	popup->addMenu(videoMenu);
	popup->addMenu(audioMenu);
	popup->addMenu(subtitlesMenu);
	popup->addMenu(favorites);
	popup->addMenu(browseMenu);
	popup->addMenu(optionsMenu);
} // createMenus()

QMenu* TBase::createToolbarMenu() {

	QMenu* menu = new QMenu(this);
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
QMenu* TBase::createPopupMenu() {
	//qDebug("Gui::TBase::createPopupMenu");
	return createToolbarMenu();
}

void TBase::showStatusBarPopup(const QPoint& pos) {
	//qDebug("Gui::TBase::showStatusBarPopup: x: %d y: %d", pos.x(), pos.y());
	execPopup(this, toolbar_menu, statusBar()->mapToGlobal(pos));
}

void TBase::createToolbars() {

	menuBar()->setObjectName("menubar");

	// Control bar
	controlbar = new TEditableToolbar(this);
	controlbar->setObjectName("controlbar");
	QStringList actions;
	actions << "play_or_pause"
			<< "separator"
			<< "rewindbutton_action"
			<< "timeslider_action"
			<< "timelabel_action|0|1"
			<< "forwardbutton_action"
			<< "separator"
			<< "fullscreen"
			<< "mute"
			<< "volumeslider_action";
	controlbar->setDefaultActions(actions);
	addToolBar(Qt::BottomToolBarArea, controlbar);
	connect(editControlBarAct, SIGNAL(triggered()),
			controlbar, SLOT(edit()));

	QAction* action = controlbar->toggleViewAction();
	action->setObjectName("toggle_controlbar");
	action->setShortcut(Qt::Key_F5);

	// Main toolbar
	toolbar = new TEditableToolbar(this);
	toolbar->setObjectName("toolbar1");
	actions.clear();
	actions << "open_file" << "open_url" << "favorites_menu" << "separator"
			<< "screenshot" << "separator" << "show_file_properties"
			<< "show_playlist" << "separator" << "show_preferences"
			<< "separator" << "play_prev" << "play_next";
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
	actions << "show_tube_browser";
	toolbar2->setDefaultActions(actions);
	addToolBar(Qt::TopToolBarArea, toolbar2);
	connect(editToolbar2Act, SIGNAL(triggered()),
			toolbar2, SLOT(edit()));

	action = toolbar2->toggleViewAction();
	action->setObjectName("toggle_toolbar2");
	action->setShortcut(Qt::Key_F4);

	toolbar2->hide();

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
}

void TBase::setupNetworkProxy() {
	qDebug("Gui::TBase::setupNetworkProxy");

	QNetworkProxy proxy;

	if ((pref->use_proxy) && (!pref->proxy_host.isEmpty())) {
		proxy.setType((QNetworkProxy::ProxyType) pref->proxy_type);
		proxy.setHostName(pref->proxy_host);
		proxy.setPort(pref->proxy_port);
		if ((!pref->proxy_username.isEmpty()) && (!pref->proxy_password.isEmpty())) {
			proxy.setUser(pref->proxy_username);
			proxy.setPassword(pref->proxy_password);
		}
		qDebug("Gui::TBase::setupNetworkProxy: using proxy: host: %s, port: %d, type: %d",
			   pref->proxy_host.toUtf8().constData(), pref->proxy_port, pref->proxy_type);
	} else {
		// No proxy
		proxy.setType(QNetworkProxy::NoProxy);
		qDebug("Gui::TBase::setupNetworkProxy: no proxy");
	}

	QNetworkProxy::setApplicationProxy(proxy);
}

void TBase::setActionsEnabled(bool b) {

	// Menu Play
	playAct->setEnabled(b);
	playOrPauseAct->setEnabled(b);
	pauseAct->setEnabled(b);
	stopAct->setEnabled(b);
	frameStepAct->setEnabled(b);
	frameBackStepAct->setEnabled(b);
	rewind1Act->setEnabled(b);
	rewind2Act->setEnabled(b);
	rewind3Act->setEnabled(b);
	forward1Act->setEnabled(b);
	forward2Act->setEnabled(b);
	forward3Act->setEnabled(b);
	gotoAct->setEnabled(b);
	// Menu Speed
	speed_menu->group->setEnabled(b);
	// A-B Section
	//repeatAct->setEnabled(b);

	// Menu Video
	bool enableVideo = b && !core->mdat.noVideo();
	videosize_menu->enableVideoSize(enableVideo);
	zoom_and_pan_menu->group->setEnabled(enableVideo);
	aspect_menu->group->setActionsEnabled(enableVideo);

	// Disable video filters if using vdpau
	bool enableFilters = enableVideo;

#ifndef Q_OS_WIN
	if (enableVideo
		&& pref->vdpau.disable_video_filters
		&& pref->vo.startsWith("vdpau")) {
		enableFilters = false;
		displayMessage(tr("Video filters are disabled when using vdpau"));
	}
#endif

	deinterlace_menu->group->setActionsEnabled(enableFilters);
	videofilter_menu->setEnabledX(enableFilters);
	rotate_menu->group->setActionsEnabled(enableFilters);
	flipAct->setEnabled(enableFilters);
	mirrorAct->setEnabled(enableFilters);
	stereo3dAct->setEnabled(enableFilters);
	videoEqualizerAct->setEnabled(enableFilters);

	bool enableScreenShots = enableFilters
							 && pref->use_screenshot
							 && !pref->screenshot_directory.isEmpty()
							 && QFileInfo(pref->screenshot_directory).isDir();
	screenshotAct->setEnabled(enableScreenShots);
	screenshotsAct->setEnabled(enableScreenShots);

#ifdef CAPTURE_STREAM
	capturingAct->setEnabled(enableVideo
							 && !pref->capture_directory.isEmpty()
							 && QFileInfo(pref->capture_directory).isDir());
#endif

#if USE_ADAPTER
	screenGroup->setActionsEnabled(enableVideo
								   && pref->vo.startsWith(OVERLAY_VO));
#endif


	// Menu Audio
	loadAudioAct->setEnabled(b);
	unloadAudioAct->setEnabled(b && !core->mset.external_audio.isEmpty());

	bool enableAudio = b && core->mdat.audios.count() > 0;
	// Filters
	volnormAct->setEnabled(enableAudio);

#ifdef MPLAYER_SUPPORT
	extrastereoAct->setEnabled(enableAudio);
	karaokeAct->setEnabled(enableAudio);
#endif

	audiochannels_menu->group->setActionsEnabled(enableAudio);
	stereomode_menu->group->setActionsEnabled(enableAudio);
	audioEqualizerAct->setEnabled(enableAudio && pref->use_audio_equalizer);

	muteAct->setEnabled(enableAudio);
	decVolumeAct->setEnabled(enableAudio);
	incVolumeAct->setEnabled(enableAudio);
	decAudioDelayAct->setEnabled(enableAudio);
	incAudioDelayAct->setEnabled(enableAudio);
	audioDelayAct->setEnabled(enableAudio);


	// Menu Subtitles
	loadSubsAct->setEnabled(b);
	//unloadSubsAct->setEnabled(b);
	decSubDelayAct->setEnabled(b);
	incSubDelayAct->setEnabled(b);
	subDelayAct->setEnabled(b);
	decSubPosAct->setEnabled(b);
	incSubPosAct->setEnabled(b);
	incSubStepAct->setEnabled(b);
	decSubStepAct->setEnabled(b);
	incSubScaleAct->setEnabled(b);
	decSubScaleAct->setEnabled(b);
#ifdef MPV_SUPPORT
	seekNextSubAct->setEnabled(b);
	seekPrevSubAct->setEnabled(b);
#endif

	// Actions not in menus
	decContrastAct->setEnabled(b);
	incContrastAct->setEnabled(b);
	decBrightnessAct->setEnabled(b);
	incBrightnessAct->setEnabled(b);
	decHueAct->setEnabled(b);
	incHueAct->setEnabled(b);
	decSaturationAct->setEnabled(b);
	incSaturationAct->setEnabled(b);
	decGammaAct->setEnabled(b);
	incGammaAct->setEnabled(b);
	nextVideoAct->setEnabled(b);
	nextAudioAct->setEnabled(b);
	nextSubtitleAct->setEnabled(b);
	nextChapterAct->setEnabled(b);
	prevChapterAct->setEnabled(b);

	bool enableDVDNav = b && core->mdat.detected_type == TMediaData::TYPE_DVDNAV;
	dvdnavUpAct->setEnabled(enableDVDNav);
	dvdnavDownAct->setEnabled(enableDVDNav);
	dvdnavLeftAct->setEnabled(enableDVDNav);
	dvdnavRightAct->setEnabled(enableDVDNav);
	dvdnavMenuAct->setEnabled(enableDVDNav);
	dvdnavSelectAct->setEnabled(enableDVDNav);
	dvdnavPrevAct->setEnabled(enableDVDNav);
	dvdnavMouseAct->setEnabled(enableDVDNav);


	// Time slider
	timeslider_action->enable(b);
}

void TBase::enableActionsOnPlaying() {
	qDebug("Gui::TBase::enableActionsOnPlaying");

	setActionsEnabled(true);
	playAct->setEnabled(false);
}

void TBase::disableActionsOnStop() {
	qDebug("Gui::TBase::disableActionsOnStop");

	setActionsEnabled(false);

	playAct->setEnabled(true);
	playOrPauseAct->setEnabled(true);
	stopAct->setEnabled(true);
}

void TBase::retranslateStrings() {
	qDebug("Gui::TBase::retranslateStrings");

	setWindowIcon(Images::icon("logo", 64));

	// Rewind/forward
	setJumpTexts(); // Texts for rewind*Act and forward*Act
	rewindbutton_action->setText(tr("3 in 1 rewind"));
	forwardbutton_action->setText(tr("3 in 1 forward"));

	// MENUS
	openMenu->menuAction()->setText(tr("&Open"));
	playMenu->menuAction()->setText(tr("&Play"));
	videoMenu->menuAction()->setText(tr("&Video"));
	audioMenu->menuAction()->setText(tr("&Audio"));
	subtitlesMenu->menuAction()->setText(tr("&Subtitles"));
	browseMenu->menuAction()->setText(tr("&Browse"));
	optionsMenu->menuAction()->setText(tr("Op&tions"));
	helpMenu->menuAction()->setText(tr("&Help"));

	/*
	openMenuAct->setIcon(Images::icon("open_menu"));
	playMenuAct->setIcon(Images::icon("play_menu"));
	videoMenuAct->setIcon(Images::icon("video_menu"));
	audioMenuAct->setIcon(Images::icon("audio_menu"));
	subtitlesMenuAct->setIcon(Images::icon("subtitles_menu"));
	browseMenuAct->setIcon(Images::icon("browse_menu"));
	optionsMenuAct->setIcon(Images::icon("options_menu"));
	helpMenuAct->setIcon(Images::icon("help_menu"));
	*/

	// Menu Open
	recentfiles_menu->menuAction()->setText(tr("&Recent files"));
	recentfiles_menu->menuAction()->setIcon(Images::icon("recents"));

	disc_menu->menuAction()->setText(tr("&Disc"));
	disc_menu->menuAction()->setIcon(Images::icon("open_disc"));

	/* favorites->menuAction()->setText(tr("&Favorites")); */
	favorites->menuAction()->setText(tr("F&avorites"));
	favorites->menuAction()->setIcon(Images::icon("open_favorites"));

	tvlist->menuAction()->setText(tr("&TV"));
	tvlist->menuAction()->setIcon(Images::icon("open_tv"));

	radiolist->menuAction()->setText(tr("Radi&o"));
	radiolist->menuAction()->setIcon(Images::icon("open_radio"));

	// Menu Play
	ab_menu->menuAction()->setText(tr("&A-B section"));
	ab_menu->menuAction()->setIcon(Images::icon("ab_menu"));

	// Menu Video
	videotrack_menu->menuAction()->setText(tr("&Track", "video"));
	videotrack_menu->menuAction()->setIcon(Images::icon("video_track"));

#if USE_ADAPTER
	screen_menu->menuAction()->setText(tr("Scree&n"));
	screen_menu->menuAction()->setIcon(Images::icon("screen"));
#endif

#if USE_ADAPTER
	screenDefaultAct->change(tr("&Default"));
#endif

	// Menu Audio
	audiotrack_menu->menuAction()->setText(tr("&Track", "audio"));
	audiotrack_menu->menuAction()->setIcon(Images::icon("audio_track"));

	audiofilter_menu->menuAction()->setText(tr("&Filters"));
	audiofilter_menu->menuAction()->setIcon(Images::icon("audio_filters"));

	// Menu Subtitle
	subtitles_track_menu->menuAction()->setText(tr("&Select"));
	subtitles_track_menu->menuAction()->setIcon(Images::icon("sub"));

#ifdef MPV_SUPPORT
	secondary_subtitles_track_menu->menuAction()->setText(tr("Secondary trac&k"));
	secondary_subtitles_track_menu->menuAction()->setIcon(Images::icon("secondary_sub"));
#endif

	// Menu Browse 
	titles_menu->menuAction()->setText(tr("&Title"));
	titles_menu->menuAction()->setIcon(Images::icon("title"));

	chapters_menu->menuAction()->setText(tr("&Chapter"));
	chapters_menu->menuAction()->setIcon(Images::icon("chapter"));

	angles_menu->menuAction()->setText(tr("&Angle"));
	angles_menu->menuAction()->setIcon(Images::icon("angle"));

#if PROGRAM_SWITCH
	programtrack_menu->menuAction()->setText(tr("P&rogram", "program"));
	programtrack_menu->menuAction()->setIcon(Images::icon("program_track"));
#endif

	// OSD
	osd_menu->menuAction()->setText(tr("&OSD"));
	osd_menu->menuAction()->setIcon(Images::icon("osd"));

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

	// TODO: make sure the "<empty>" string is translated

	// PlayerWindow
	playerwindow->retranslateStrings();

	// Playlist
	playlist->retranslateStrings();

	// Log window
	log_window->retranslateStrings();

	updateRecents();

	// Update actions view in preferences
	// It has to be done, here. The actions are translated after the
	// preferences dialog.
	if (pref_dialog)
		pref_dialog->mod_input()->actions_editor->updateView();
} // retranslateStrings()

void TBase::changeEvent(QEvent* e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMainWindow::changeEvent(e);
	}
}

void TBase::setJumpTexts() {
	rewind1Act->change(tr("-%1").arg(Helper::timeForJumps(pref->seeking1)));
	rewind2Act->change(tr("-%1").arg(Helper::timeForJumps(pref->seeking2)));
	rewind3Act->change(tr("-%1").arg(Helper::timeForJumps(pref->seeking3)));

	forward1Act->change(tr("+%1").arg(Helper::timeForJumps(pref->seeking1)));
	forward2Act->change(tr("+%1").arg(Helper::timeForJumps(pref->seeking2)));
	forward3Act->change(tr("+%1").arg(Helper::timeForJumps(pref->seeking3)));
}

void TBase::setWindowCaption(const QString& title) {
	setWindowTitle(title);
}

void TBase::createPreferencesDialog() {

	QApplication::setOverrideCursor(Qt::WaitCursor);
	pref_dialog = new Pref::TDialog(this);
	pref_dialog->setModal(false);
	connect(pref_dialog, SIGNAL(applied()),
			 this, SLOT(applyNewPreferences()));
	QApplication::restoreOverrideCursor();
}

void TBase::createFilePropertiesDialog() {
	qDebug("Gui::TBase::createFilePropertiesDialog");

	QApplication::setOverrideCursor(Qt::WaitCursor);
	file_dialog = new TFilePropertiesDialog(this, core->mdat);
	file_dialog->setModal(false);
	connect(file_dialog, SIGNAL(applied()),
			 this, SLOT(applyFileProperties()));
	QApplication::restoreOverrideCursor();
}

#ifdef SINGLE_INSTANCE
void TBase::handleMessageFromOtherInstances(const QString& message) {
	qDebug("Gui::TBase::handleMessageFromOtherInstances: '%s'", message.toUtf8().constData());

	int pos = message.indexOf(' ');
	if (pos > -1) {
		QString command = message.left(pos);
		QString arg = message.mid(pos+1);
		qDebug("command: '%s'", command.toUtf8().constData());
		qDebug("arg: '%s'", arg.toUtf8().constData());

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
			/* if (core->state() == TCore::Stopped) { emit openFileRequested(); } */
			playlist->addFiles(file_list);
		}
		else
		if (command == "media_title") {
			QStringList list = arg.split(" <<sep>> ");
			core->addForcedTitle(list[0], list[1]);
		}
		else
		if (command == "action") {
			processFunction(arg);
		}
		else
		if (command == "load_sub") {
			setInitialSubtitle(arg);
			if (core->state() != TCore::Stopped) {
				core->loadSub(arg);
			}
		}
	}
}
#endif

TActionList TBase::getAllNamedActions() {

	// Get all actions with a name
	TActionList all_actions = findChildren<QAction*>();
	for (int i = all_actions.count() - 1; i >= 0; i--) {
		if (all_actions[i]->objectName().isEmpty()) {
			all_actions.removeAt(i);
		}
	}

	return all_actions;
}

void TBase::loadConfig() {
	qDebug("Gui::TBase::loadConfig");

#if ALLOW_CHANGE_STYLESHEET
	changeStyleSheet(pref->iconset);
#endif

	// Get all actions with a name
	TActionList all_actions = getAllNamedActions();
	// Load shortcuts actions from outside group derived class
	TActionsEditor::loadFromConfig(all_actions, pref);

	// Load from inside group derived class
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
		TDesktop::keepInsideDesktop(this);

		// Block resize of main window by loading of video
		// TODO: reset when video fails to load
		block_resize = true;
	} else {
		centerWindow();
		// Need to center again after video loaded
		center_window = true;
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
	viewMenuBarAct->update(menubar_visible);
	fullscreen_menubar_visible = pref->value("fullscreen_menubar_visible", fullscreen_menubar_visible).toBool();

	statusbar_visible = pref->value("statusbar_visible", statusbar_visible).toBool();
	viewStatusBarAct->update(statusbar_visible);
	fullscreen_statusbar_visible = pref->value("fullscreen_statusbar_visible", fullscreen_statusbar_visible).toBool();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();

	// Load playlist settings outside group
	playlist->loadSettings();
}

void TBase::saveConfig() {
	qDebug("Gui::TBase::saveConfig");

	pref->beginGroup(settingsGroupName());

	if (pref->save_window_size_on_exit) {
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
	pref->setValue("fullscreen_statusbar_visible", fullscreen_statusbar_visible);

	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

	pref->endGroup();

	playlist->saveSettings();
}

void TBase::closeEvent(QCloseEvent* e)  {
	qDebug("Gui::TBase::closeEvent");

	core->close();
	exitFullscreen();

	displayMessage(tr("Saving settings"), 0);
	saveConfig();
	pref->save();
	e->accept();
}

void TBase::closeWindow() {
	qDebug("Gui::TBase::closeWindow");

	close();
}

void TBase::showPlaylist() {
	showPlaylist(!playlist->isVisible());
}

void TBase::showPlaylist(bool b) {
	if (!b) {
		playlist->hide();
	} else {
		exitFullscreenIfNeeded();
		playlist->show();
	}
}

void TBase::showVideoEqualizer(bool b) {

	if (b) {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		video_equalizer->show();
	} else {
		video_equalizer->hide();
	}
}

void TBase::showAudioEqualizer() {
	showAudioEqualizer(!audio_equalizer->isVisible());
}

void TBase::showAudioEqualizer(bool b) {

	if (b) {
		exitFullscreenIfNeeded();
		audio_equalizer->show();
	} else {
		audio_equalizer->hide();
	}
}

void TBase::showPreferencesDialog() {
	qDebug("Gui::TBase::showPreferencesDialog");

	exitFullscreenIfNeeded();
	
	if (!pref_dialog) {
		createPreferencesDialog();
	}

	pref_dialog->setData(pref);

	pref_dialog->mod_input()->actions_editor->clear();
	pref_dialog->mod_input()->actions_editor->addActions(this);

	// Set playlist preferences
	Pref::TPrefPlaylist* pl = pref_dialog->mod_playlist();
	pl->setDirectoryRecursion(playlist->directoryRecursion());
	pl->setAutoGetInfo(playlist->autoGetInfo());
	pl->setSavePlaylistOnExit(playlist->savePlaylistOnExit());
	pl->setPlayFilesFromStart(playlist->playFilesFromStart());

	pref_dialog->show();
}

// The user has pressed OK in preferences dialog
void TBase::applyNewPreferences() {
	qDebug("Gui::TBase::applyNewPreferences");

	bool need_update_language = false;
	TPlayerID::Player old_player_type = TPlayerID::player(pref->mplayer_bin);
	pref_dialog->getData(pref);

	// Video equalizer
	video_equalizer->setBySoftware(pref->use_soft_video_eq);
	// Screenshots

	bool enableScreenShots = core->state() != TCore::Stopped
							 && !core->mdat.noVideo()
							 // TODO: vdpau see setActionsEnabled()
							 && pref->use_screenshot
							 && !pref->screenshot_directory.isEmpty()
							 && QFileInfo(pref->screenshot_directory).isDir();
	screenshotAct->setEnabled(enableScreenShots);
	screenshotsAct->setEnabled(enableScreenShots);

	// Setup proxy
	setupNetworkProxy();

	// Change application font
	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		if (QApplication::font() != f) {
			qDebug("Gui::TBase::applyNewPreferences: setting new font: %s", pref->default_font.toLatin1().constData());
			QApplication::setFont(f);
		}
	}
	// Use custom style
	useCustomSubStyleAct->setChecked(pref->enable_ass_styles);

	Pref::TInterface* _interface = pref_dialog->mod_interface();
	if (_interface->recentsChanged()) {
		updateRecents();
	}
	if (_interface->languageChanged()) need_update_language = true;

	if (_interface->iconsetChanged()) { 
		need_update_language = true;
		// Stylesheet
		#if ALLOW_CHANGE_STYLESHEET
		if (!_interface->guiChanged()) changeStyleSheet(pref->iconset);
		#endif
	}
	auto_hide_timer->setInterval(pref->floating_hide_delay);


	playerwindow->setDelayLeftClick(pref->delay_left_click);

	if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
		resize(width(), height() + 200);
		panel->show();
	}

	Pref::TAdvanced *advanced = pref_dialog->mod_advanced();
	if (advanced->repaintVideoBackgroundChanged()) {
		playerwindow->videoLayer()->setRepaintBackground(pref->repaint_video_background);
	}
	if (advanced->colorkeyChanged()) {
		playerwindow->setColorKey(pref->color_key);
	}
	if (advanced->monitorAspectChanged()) {
		playerwindow->setMonitorAspect(pref->monitor_aspect_double());
	}
	if (advanced->lavfDemuxerChanged()) {
		core->mset.forced_demuxer = pref->use_lavf_demuxer ? "lavf" : "";
	}

	// Update logging
	TLog::log->setEnabled(pref->log_enabled);
	// log_verbose sets requires_restart
	TLog::log->setLogFileEnabled(pref->log_file);
	TLog::log->setFilter(pref->log_filter);

	// Update playlist preferences
	Pref::TPrefPlaylist* pl = pref_dialog->mod_playlist();
	playlist->setDirectoryRecursion(pl->directoryRecursion());
	playlist->setAutoGetInfo(pl->autoGetInfo());
	playlist->setSavePlaylistOnExit(pl->savePlaylistOnExit());
	playlist->setPlayFilesFromStart(pl->playFilesFromStart());

	if (need_update_language) {
		emit loadTranslation();
	}

	setJumpTexts(); // Update texts in menus

	if (_interface->styleChanged()) {
		qDebug("Gui::TBase::applyNewPreferences: selected style: '%s'", pref->style.toUtf8().data());
		if (!pref->style.isEmpty()) {
			qApp->setStyle(pref->style);
		} else {
			qDebug("Gui::TBase::applyNewPreferences: setting default style: '%s'", default_style.toUtf8().data());
			qApp->setStyle(default_style);
		}
	}

	// Update actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
	TActionsEditor::saveToConfig(this, pref);
	pref->save();

	// Any restarts needed?
	if (_interface->guiChanged()
		|| old_player_type != TPlayerID::player(pref->mplayer_bin)) {
		// Recreate the main window
		emit requestRestart();
		close();
	} else if (pref_dialog->requiresRestart()) {
		// Restart the video
		core->restart();
	}
}


void TBase::showFilePropertiesDialog() {
	qDebug("Gui::TBase::showFilePropertiesDialog");

	exitFullscreenIfNeeded();

	if (!file_dialog) {
		createFilePropertiesDialog();
	}

	setDataToFileProperties();

	file_dialog->show();
}

void TBase::setDataToFileProperties() {

	InfoReader *i = InfoReader::obj();
	i->getInfo();
	file_dialog->setCodecs(i->vcList(), i->acList(), i->demuxerList());

	// Save a copy of the original values
	if (core->mset.original_demuxer.isEmpty()) 
		core->mset.original_demuxer = core->mdat.demuxer;

	if (core->mset.original_video_codec.isEmpty()) 
		core->mset.original_video_codec = core->mdat.video_codec;

	if (core->mset.original_audio_codec.isEmpty()) 
		core->mset.original_audio_codec = core->mdat.audio_codec;

	QString demuxer = core->mset.forced_demuxer;
	if (demuxer.isEmpty()) demuxer = core->mdat.demuxer;

	QString ac = core->mset.forced_audio_codec;
	if (ac.isEmpty()) ac = core->mdat.audio_codec;

	QString vc = core->mset.forced_video_codec;
	if (vc.isEmpty()) vc = core->mdat.video_codec;

	file_dialog->setDemuxer(demuxer, core->mset.original_demuxer);
	file_dialog->setAudioCodec(ac, core->mset.original_audio_codec);
	file_dialog->setVideoCodec(vc, core->mset.original_video_codec);

	file_dialog->setMplayerAdditionalArguments(core->mset.mplayer_additional_options);
	file_dialog->setMplayerAdditionalVideoFilters(core->mset.mplayer_additional_video_filters);
	file_dialog->setMplayerAdditionalAudioFilters(core->mset.mplayer_additional_audio_filters);
}

void TBase::applyFileProperties() {
	qDebug("Gui::TBase::applyFileProperties");

	bool need_restart = false;

#undef TEST_AND_SET
#define TEST_AND_SET(Pref, Dialog) \
	if (Pref != Dialog) { Pref = Dialog; need_restart = true; }

	bool demuxer_changed = false;

	QString prev_demuxer = core->mset.forced_demuxer;

	QString demuxer = file_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	if (prev_demuxer != core->mset.forced_demuxer) {
		// Demuxer changed
		demuxer_changed = true;
		core->mset.current_audio_id = TMediaSettings::NoneSelected;
		core->mset.current_sub_idx = TMediaSettings::NoneSelected;
	}

	QString ac = file_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);

	TEST_AND_SET(core->mset.mplayer_additional_options, file_dialog->mplayerAdditionalArguments());
	TEST_AND_SET(core->mset.mplayer_additional_video_filters, file_dialog->mplayerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.mplayer_additional_audio_filters, file_dialog->mplayerAdditionalAudioFilters());

	// Restart the video to apply
	if (need_restart) {
		if (demuxer_changed) {
			core->reload();
		} else {
			core->restart();
		}
	}
}

void TBase::updateMediaInfo() {
	qDebug("Gui::TBase::updateMediaInfo");

	if (file_dialog && file_dialog->isVisible()) {
		setDataToFileProperties();
	}

	setWindowCaption(core->mdat.displayName(pref->show_tag_in_window_title) + " - SMPlayer");

	emit videoInfoChanged(core->mdat.video_width, core->mdat.video_height, core->mdat.video_fps);
}

void TBase::newMediaLoaded() {
	qDebug("Gui::TBase::newMediaLoaded");

	enterFullscreenOnPlay();

	// Recents
	QString filename = core->mdat.filename;
	QString stream_title = core->mdat.stream_title;
	if (!stream_title.isEmpty()) {
		pref->history_recents.addItem(filename, stream_title);
	} else {
		pref->history_recents.addItem(filename);
	}
	updateRecents();

	checkPendingActionsToRun();
}

void TBase::showLog() {
	//qDebug("Gui::TBase::showLog");

	if (log_window->isVisible()) {
		log_window->hide();
	} else {
		exitFullscreenIfNeeded();
		log_window->show();
	}
}

void TBase::updateVideoTracks() {
	qDebug("Gui::TBase::updateVideoTracks");

	videoTrackGroup->clear(true);

	Maps::TTracks* videos = &core->mdat.videos;
	if (videos->count() == 0) {
		QAction* a = videoTrackGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		Maps::TTracks::TTrackIterator i = videos->getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTrackData track = i.value();
			QAction* action = new QAction(videoTrackGroup);
			action->setCheckable(true);
			action->setText(track.getDisplayName());
			action->setData(track.getID());
			if (track.getID() == videos->getSelectedID())
				action->setChecked(true);
		}
	}

	videotrack_menu->addActions(videoTrackGroup->actions());
}

void TBase::updateAudioTracks() {
	qDebug("Gui::TBase::updateAudioTracks");

	audioTrackGroup->clear(true);

	Maps::TTracks* audios = &core->mdat.audios;
	if (audios->count() == 0) {
		QAction* a = audioTrackGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		Maps::TTracks::TTrackIterator i = audios->getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTrackData track = i.value();
			QAction* action = new QAction(audioTrackGroup);
			action->setCheckable(true);
			action->setText(track.getDisplayName());
			action->setData(track.getID());
			if (track.getID() == audios->getSelectedID())
				action->setChecked(true);
		}
	}

	audiotrack_menu->addActions(audioTrackGroup->actions());
}

void TBase::updateSubtitles() {
	qDebug("Gui::TBase::updateSubtitles");

	// Note: use idx not ID
	subtitleTrackGroup->clear(true);
#ifdef MPV_SUPPORT
	secondarySubtitleTrackGroup->clear(true);
#endif

	QAction* subNoneAct = subtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
	if (core->mset.current_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}
#ifdef MPV_SUPPORT
	subNoneAct = secondarySubtitleTrackGroup->addAction(tr("&None"));
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
	if (core->mset.current_secondary_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}
#endif

	for (int idx = 0; idx < core->mdat.subs.count(); idx++) {
		SubData sub = core->mdat.subs.itemAt(idx);
		QAction *a = new QAction(subtitleTrackGroup);
		a->setCheckable(true);
		a->setText(sub.displayName());
		a->setData(idx);
		if (idx == core->mset.current_sub_idx) {
			a->setChecked(true);
		}
#ifdef MPV_SUPPORT
		a = new QAction(secondarySubtitleTrackGroup);
		a->setCheckable(true);
		a->setText(sub.displayName());
		a->setData(idx);
		if (idx == core->mset.current_secondary_sub_idx) {
			a->setChecked(true);
		}
#endif
	}

	subtitles_track_menu->addActions(subtitleTrackGroup->actions());
#ifdef MPV_SUPPORT
	secondary_subtitles_track_menu->addActions(secondarySubtitleTrackGroup->actions());
#endif

	// Enable or disable the unload subs action if there are file subs
	// or for mplayer externally loaded vob subs
	bool have_ext_subs = core->haveExternalSubs();
	unloadSubsAct->setEnabled(have_ext_subs);
	subfps_menu->group->setEnabled(have_ext_subs);

	// Enable or disable subtitle options
	bool e = core->mset.current_sub_idx >= 0;

	if (core->mset.closed_caption_channel !=0) e = true; // Enable if using closed captions

	decSubDelayAct->setEnabled(e);
	incSubDelayAct->setEnabled(e);
	subDelayAct->setEnabled(e);
	decSubPosAct->setEnabled(e);
	incSubPosAct->setEnabled(e);
	decSubScaleAct->setEnabled(e);
	incSubScaleAct->setEnabled(e);
	decSubStepAct->setEnabled(e);
	incSubStepAct->setEnabled(e);
#ifdef MPV_SUPPORT
	seekNextSubAct->setEnabled(e);
	seekPrevSubAct->setEnabled(e);
#endif
}

void TBase::updateTitles() {
	qDebug("Gui::TBase::updateTitles");

	titleGroup->clear(true);
	if (core->mdat.titles.count() == 0) {
		QAction* a = titleGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		int selected_ID = core->mdat.titles.getSelectedID();
		Maps::TTitleTracks::TTitleTrackIterator i = core->mdat.titles.getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTitleData title = i.value();
			QAction* action = new QAction(titleGroup);
			action->setCheckable(true);
			action->setText(title.getDisplayName());
			action->setData(title.getID());
			if (title.getID() == selected_ID) {
				action->setChecked(true);
			}
		}
	}

	titles_menu->addActions(titleGroup->actions());

	updateChapters();
	updateAngles();
}

void TBase::updateChapters() {
	qDebug("Gui::TBase::updateChapters");

	chapterGroup->clear(true);
	if (core->mdat.chapters.count() > 0) {
		int selected_id = core->mdat.chapters.getSelectedID();
		Maps::TChapters::TChapterIterator i = core->mdat.chapters.getIterator();
		do {
			i.next();
			const Maps::TChapterData chapter = i.value();
			QAction *a = new QAction(chapterGroup);
			a->setCheckable(true);
			a->setText(chapter.getDisplayName());
			a->setData(chapter.getID());
			if (chapter.getID() == selected_id) {
				a->setChecked(true);
			}
		} while (i.hasNext());
	} else {
		QAction* a = chapterGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	}

	chapters_menu->addActions(chapterGroup->actions());
}

// Angles
void TBase::updateAngles() {
	qDebug("Gui::TBase::updateAngels");

	angleGroup->clear(true);
	int n_angles = 0;
	int sel_title_id = core->mdat.titles.getSelectedID();
	if (sel_title_id >= 0) {
		Maps::TTitleData title = core->mdat.titles.value(sel_title_id);
		n_angles = title.getAngles();
	}
	if (n_angles > 0) {
		for (int n = 1; n <= n_angles; n++) {
			QAction *a = new QAction(angleGroup);
			a->setCheckable(true);
			a->setText(QString::number(n));
			a->setData(n);
		}
	} else {
		QAction* a = angleGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	}
	angles_menu->addActions(angleGroup->actions());
}

void TBase::updateRecents() {
	qDebug("Gui::TBase::updateRecents");

	recentfiles_menu->clear();

	int current_items = 0;

	if (pref->history_recents.count() > 0) {
		for (int n=0; n < pref->history_recents.count(); n++) {
			QString i = QString::number(n+1);
			QString fullname = pref->history_recents.item(n);
			QString filename = fullname;
			QFileInfo fi(fullname);
			//if (fi.exists()) filename = fi.fileName(); // Can be slow

			// Let's see if it looks like a file (no dvd://1 or something)
			if (fullname.indexOf(QRegExp("^.*://.*")) == -1) filename = fi.fileName();

			if (filename.size() > 85) {
				filename = filename.left(80) + "...";
			}

			QString show_name = filename;
			QString title = pref->history_recents.title(n);
			if (!title.isEmpty()) show_name = title;

			QAction* a = recentfiles_menu->addAction(QString("%1. " + show_name).arg(i.insert(i.size()-1, '&'), 3, ' '));
			a->setStatusTip(fullname);
			a->setData(n);
			connect(a, SIGNAL(triggered()), this, SLOT(openRecent()));
			current_items++;
		}
	} else {
		QAction* a = recentfiles_menu->addAction(tr("<empty>"));
		a->setEnabled(false);
	}

	recentfiles_menu->menuAction()->setVisible(current_items > 0);
	if (current_items  > 0) {
		recentfiles_menu->addSeparator();
		recentfiles_menu->addAction(clearRecentsAct);
	}
}

void TBase::clearRecentsList() {

	int ret = QMessageBox::question(this, tr("Confirm deletion - SMPlayer"),
				tr("Delete the list of recent files?"),
				QMessageBox::Cancel, QMessageBox::Ok);

	if (ret == QMessageBox::Ok) {
		// Delete items in menu
		pref->history_recents.clear();
		updateRecents();
	}
}

void TBase::updateVideoEqualizer() {
	qDebug("Gui::TBase::updateVideoEqualizer");

	video_equalizer->setContrast(core->mset.contrast);
	video_equalizer->setBrightness(core->mset.brightness);
	video_equalizer->setHue(core->mset.hue);
	video_equalizer->setSaturation(core->mset.saturation);
	video_equalizer->setGamma(core->mset.gamma);
}

void TBase::updateAudioEqualizer() {
	qDebug("Gui::TBase::updateAudioEqualizer");

	const TAudioEqualizerList l = pref->global_audio_equalizer ? pref->audio_equalizer : core->mset.audio_equalizer;
	audio_equalizer->setEqualizer(l);
}

void TBase::setDefaultValuesFromVideoEqualizer() {
	qDebug("Gui::TBase::setDefaultValuesFromVideoEqualizer");

	pref->initial_contrast = video_equalizer->contrast();
	pref->initial_brightness = video_equalizer->brightness();
	pref->initial_hue = video_equalizer->hue();
	pref->initial_saturation = video_equalizer->saturation();
	pref->initial_gamma = video_equalizer->gamma();

	QMessageBox::information(this, tr("Information"), 
							 tr("The current values have been stored to be "
								"used as default."));
}

void TBase::changeVideoEqualizerBySoftware(bool b) {
	qDebug("Gui::TBase::changeVideoEqualizerBySoftware: %d", b);

	if (b != pref->use_soft_video_eq) {
		pref->use_soft_video_eq = b;
		core->restart();
	}
}

void TBase::openDirectory(QString directory) {
	qDebug("Gui::TBase::openDirectory: '%s'", directory.toUtf8().data());

	if (Helper::directoryContainsDVD(directory)) {
		core->open(directory);
	} else {
		QFileInfo fi(directory);
		if (fi.isDir()) {
			directory = fi.absoluteFilePath();
			pref->latest_dir = directory;
			playlist->playDirectory(directory);
		} else {
			qWarning("Gui::TBase::openDirectory: directory is not valid");
		}
	}
}

void TBase::openDirectory() {
	qDebug("Gui::TBase::openDirectory");

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					pref->latest_dir);

	if (!s.isEmpty()) {
		openDirectory(s);
	}
}

void TBase::open(const QString &file) {
	qDebug() << "Gui::TBase::open:" << file;

	if (file.isEmpty()) {
		qWarning("Gui::TBase::open: filename is empty");
		return;
	}
	if (!playlist->maybeSave()) {
		return;
	}
	if (QFile::exists(file)) {
		QFileInfo fi(file);
		if (fi.isDir()) {
			openDirectory(file);
			return;
		}
		pref->latest_dir = fi.absolutePath();
	}

	core->open(file);

	qDebug("Gui::TBase::open done");
}

void TBase::openFiles(QStringList files) {
	qDebug("Gui::TBase::openFiles");

	if (files.empty()) {
		qWarning("Gui::TBase::openFiles: no files in list to open");
		return;
	}

	#ifdef Q_OS_WIN
	files = Helper::resolveSymlinks(files); // Check for Windows shortcuts
	#endif

	if (files.count() == 1) {
		open(files[0]);
	} else if (playlist->maybeSave()) {
		qDebug("Gui::TBase::openFiles: starting new playlist");
		files.sort();
		playlist->clear();
		playlist->addFiles(files);
		playlist->startPlay();
	}
}

void TBase::openFile() {
	qDebug("Gui::TBase::openFile");

	exitFullscreenIfNeeded();

	TExtensions e;
	QString s = MyFileDialog::getOpenFileName(
					   this, tr("Choose a file"), pref->latest_dir,
					   tr("Multimedia") + e.allPlayable().forFilter()+";;" +
					   tr("Video") + e.video().forFilter()+";;" +
					   tr("Audio") + e.audio().forFilter()+";;" +
					   tr("Playlists") + e.playlist().forFilter()+";;" +
					   tr("All files") +" (*.*)");

	if (!s.isEmpty()) {
		open(s);
	}
}

void TBase::openRecent() {
	qDebug("Gui::TBase::openRecent");

	QAction *a = qobject_cast<QAction *> (sender());
	if (a) {
		int item = a->data().toInt();
		QString filename = pref->history_recents.item(item);
		if (!filename.isEmpty())
			open(filename);
	}
}

void TBase::openFavorite(QString file) {
	qDebug("Gui::TBase::openFavorite");

	openFiles(QStringList() << file);
}

void TBase::openURL() {
	qDebug("Gui::TBase::openURL");

	exitFullscreenIfNeeded();

	TInputURL d(this);

	// Get url from clipboard
	QString clipboard_text = QApplication::clipboard()->text();
	if ((!clipboard_text.isEmpty()) && (clipboard_text.contains("://")) /*&& (QUrl(clipboard_text).isValid())*/) {
		d.setURL(clipboard_text);
	}

	for (int n=0; n < pref->history_urls.count(); n++) {
		d.setURL(pref->history_urls.url(n));
	}

	if (d.exec() == QDialog::Accepted) {
		QString url = d.url();
		if (!url.isEmpty()) {
			pref->history_urls.addUrl(url);
			openURL(url);
		}
	}
}

void TBase::openURL(QString url) {
	if (!url.isEmpty()
		&& (!pref->auto_add_to_playlist || playlist->maybeSave())) {
			core->openStream(url);
	}
}

void TBase::configureDiscDevices() {
	QMessageBox::information(this, tr("SMPlayer - Information"),
			tr("The CDROM / DVD drives are not configured yet.\n"
			   "The configuration dialog will be shown now, "
			   "so you can do it."), QMessageBox::Ok);
	
	showPreferencesDialog();
	pref_dialog->showSection(Pref::TDialog::Drives);
}

void TBase::openVCD() {
	qDebug("Gui::TBase::openVCD");

	if (pref->dvd_device.isEmpty()
		|| pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		// TODO: remove pref->vcd_initial_title?
		core->open(TDiscName::join(TDiscName::VCD, 0, pref->cdrom_device));
	}
}

void TBase::openAudioCD() {
	qDebug("Gui::TBase::openAudioCD");

	if ((pref->dvd_device.isEmpty()) ||
		 (pref->cdrom_device.isEmpty()))
	{
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->open("cdda://");
		}
	}
}

void TBase::openDVD() {
	qDebug("Gui::TBase::openDVD");

	if (pref->dvd_device.isEmpty() || pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else {
		if (playlist->maybeSave()) {
			core->open(TDiscName::joinDVD(pref->dvd_device, pref->use_dvdnav));
		}
	}
}

void TBase::openDVDFromFolder() {
	qDebug("Gui::TBase::openDVDFromFolder");

	if (playlist->maybeSave()) {
		TInputDVDDirectory *d = new TInputDVDDirectory(this);
		d->setFolder(pref->last_dvd_directory);

		if (d->exec() == QDialog::Accepted) {
			qDebug("Gui::TBase::openDVDFromFolder: accepted");
			openDVDFromFolder(d->folder());
		}

		delete d;
	}
}

void TBase::openDVDFromFolder(const QString &directory) {

	pref->last_dvd_directory = directory;
	core->open(TDiscName::joinDVD(directory, pref->use_dvdnav));
}

/**
 * Minimal TBase abstraction for calling openBluRay. It's called from
 * OpenBluRayFromFolder()
 */
void TBase::openBluRayFromFolder(QString directory) {

	pref->last_dvd_directory = directory;
	core->open(TDiscName::join(TDiscName::BLURAY, 0, directory));
}

/**
 * Attempts to open a bluray from pref->bluray_device. If not set, calls configureDiscDevices.
 * If successful, calls TCore::OpenBluRay(QString)
 */
void TBase::openBluRay() {
	qDebug("Gui::TBase::openBluRay");

	if (pref->dvd_device.isEmpty()
		|| pref->cdrom_device.isEmpty()
		|| pref->bluray_device.isEmpty()) {
		configureDiscDevices();
	} else {
		core->open(TDiscName::join(TDiscName::BLURAY, 0, pref->bluray_device));
	}
}

void TBase::openBluRayFromFolder() {
	qDebug("Gui::TBase::openBluRayFromFolder");

	if (playlist->maybeSave()) {
		QString dir = QFileDialog::getExistingDirectory(this, tr("Select the Blu-ray folder"),
			pref->last_dvd_directory, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (!dir.isEmpty()) {
			openBluRayFromFolder(dir);
		}
	}
}

void TBase::loadSub() {
	qDebug("Gui::TBase::loadSub");

	exitFullscreenIfNeeded();

	TExtensions e;
    QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Subtitles") + e.subtitles().forFilter()+ ";;" +
		tr("All files") +" (*.*)");

	if (!s.isEmpty()) core->loadSub(s);
}

void TBase::setInitialSubtitle(const QString & subtitle_file) {
	qDebug("Gui::TBase::setInitialSubtitle: '%s'", subtitle_file.toUtf8().constData());

	core->setInitialSubtitle(subtitle_file);
}

void TBase::loadAudioFile() {
	qDebug("Gui::TBase::loadAudioFile");

	exitFullscreenIfNeeded();

	TExtensions e;
	QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Audio") + e.audio().forFilter()+";;" +
		tr("All files") +" (*.*)");

	if (!s.isEmpty()) core->loadAudioFile(s);
}

void TBase::helpFirstSteps() {
	QDesktopServices::openUrl(QString(URL_FIRST_STEPS "?version=%1").arg(Version::printable()));
}

void TBase::helpFAQ() {
	QString url = URL_FAQ;
	/* if (!pref->language.isEmpty()) url += QString("?tr_lang=%1").arg(pref->language); */
	QDesktopServices::openUrl(QUrl(url));
}

void TBase::helpCLOptions() {
	if (clhelp_window == 0) {
		clhelp_window = new TLogWindow(this);
	}
	clhelp_window->setWindowTitle(tr("SMPlayer command line options"));
	clhelp_window->setHtml(CLHelp::help(true));
	clhelp_window->show();
}

void TBase::helpCheckUpdates() {
#ifdef UPDATE_CHECKER
	update_checker->check();
#else
	QString url = QString(URL_CHANGES "?version=%1").arg(Version::with_revision());
	QDesktopServices::openUrl(QUrl(url));
#endif
}

void TBase::helpShowConfig() {
	QDesktopServices::openUrl(QUrl::fromLocalFile(TPaths::configPath()));
}

void TBase::helpAbout() {
	TAbout d(this);
	d.exec();
}

void TBase::showGotoDialog() {

	TTimeDialog d(this);
	d.setLabel(tr("&Jump to:"));
	d.setWindowTitle(tr("SMPlayer - Seek"));
	d.setMaximumTime((int) core->mdat.duration);
	d.setTime((int) core->mset.current_sec);
	if (d.exec() == QDialog::Accepted) {
		core->goToSec(d.time());
	}
}

void TBase::showAudioDelayDialog() {
	bool ok;
	#if QT_VERSION >= 0x050000
	int delay = QInputDialog::getInt(this, tr("SMPlayer - Audio delay"),
		tr("Audio delay (in milliseconds):"), core->mset.audio_delay,
		-3600000, 3600000, 1, &ok);
	#else
	int delay = QInputDialog::getInteger(this, tr("SMPlayer - Audio delay"),
		tr("Audio delay (in milliseconds):"), core->mset.audio_delay,
		-3600000, 3600000, 1, &ok);
	#endif
	if (ok) {
		core->setAudioDelay(delay);
	}
}

void TBase::showSubDelayDialog() {
	bool ok;
	#if QT_VERSION >= 0x050000
	int delay = QInputDialog::getInt(this, tr("SMPlayer - Subtitle delay"),
		tr("Subtitle delay (in milliseconds):"), core->mset.sub_delay,
		-3600000, 3600000, 1, &ok);
	#else
	int delay = QInputDialog::getInteger(this, tr("SMPlayer - Subtitle delay"),
		tr("Subtitle delay (in milliseconds):"), core->mset.sub_delay,
		-3600000, 3600000, 1, &ok);
	#endif
	if (ok) {
		core->setSubDelay(delay);
	}
}

void TBase::showStereo3dDialog() {
	TStereo3dDialog d(this);
	d.setInputFormat(core->mset.stereo3d_in);
	d.setOutputFormat(core->mset.stereo3d_out);

	if (d.exec() == QDialog::Accepted) {
		core->changeStereo3d(d.inputFormat(), d.outputFormat());
	}
}

void TBase::exitFullscreen() {
	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void TBase::toggleFullscreen() {
	qDebug("Gui::TBase::toggleFullscreen");

	toggleFullscreen(!pref->fullscreen);
}

void TBase::unlockSizeFactor() {
	qDebug("Gui::TBase::unlockSizeFactor");
	block_update_size_factor--;
}

void TBase::toggleFullscreen(bool b) {
	qDebug("Gui::TBase::toggleFullscreen: %d", b);

	if (b == pref->fullscreen) {
		qDebug("Gui::TBase::toggleFullscreen: nothing to do, returning");
		return;
	}
	pref->fullscreen = b;

	//setUpdatesEnabled(false);
	block_update_size_factor++;

	if (pref->fullscreen) {
		aboutToEnterFullscreen();

#ifdef Q_OS_WIN
		// Hack to avoid the windows taskbar to be visible on Windows XP
		if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
			if (!pref->pause_when_hidden)
				hide();
		}
#endif

		showFullScreen();
		didEnterFullscreen();
	} else {
		aboutToExitFullscreen();
		showNormal();
		didExitFullscreen();
	}

	if (pref->add_blackborders_on_fullscreen && !core->mset.add_letterbox) {
		core->changeLetterboxOnFullscreen(pref->fullscreen);
	}

	// Update fullscreen action
	fullscreenAct->setChecked(pref->fullscreen);

	// Risky?
	QTimer::singleShot(350, this, SLOT(unlockSizeFactor()));
	//setUpdatesEnabled(true);
	//update();

	setFocus(); // Fixes bug #2493415
}

void TBase::aboutToEnterFullscreen() {
	//qDebug("Gui::TBase::aboutToEnterFullscreen");

	videosize_menu->enableVideoSize(false);

	// Save current state
	if (pref->restore_pos_after_fullscreen) {
		win_pos = pos();
		win_size = size();
	}
	was_maximized = isMaximized();

	menubar_visible = !menuBar()->isHidden();
	statusbar_visible = !statusBar()->isHidden();

	pref->beginGroup(settingsGroupName());
	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));
	pref->endGroup();
}

void TBase::didEnterFullscreen() {
	//qDebug("Gui::TBase::didEnterFullscreen");

	// Restore fullscreen state
	viewMenuBarAct->update(fullscreen_menubar_visible);
	viewStatusBarAct->update(fullscreen_statusbar_visible);

	pref->beginGroup(settingsGroupName());
	if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
					  Helper::qtVersion())) {
		toolbar->hide();
		toolbar2->hide();
	}
	pref->endGroup();

	toolbar->didEnterFullscreen();
	toolbar2->didEnterFullscreen();
	controlbar->didEnterFullscreen();

	auto_hide_timer->start();
}

void TBase::aboutToExitFullscreen() {
	//qDebug("Gui::TBase::aboutToExitFullscreen");

	auto_hide_timer->stop();

	// Save fullscreen state
	fullscreen_menubar_visible = !menuBar()->isHidden();
	fullscreen_statusbar_visible = !statusBar()->isHidden();

	pref->beginGroup(settingsGroupName());
	pref->setValue("toolbars_state_fullscreen", saveState(Helper::qtVersion()));
	pref->endGroup();

	videosize_menu->enableVideoSize(true);
	playerwindow->aboutToExitFullscreen();
}

void TBase::didExitFullscreen() {
	//qDebug("Gui::TBase::didExitFullscreen");

	// Restore normal state
	if (was_maximized) {
		showMaximized();
	}
	if (pref->restore_pos_after_fullscreen) {
		move(win_pos);
		resize(win_size);
	}

	viewMenuBarAct->update(menubar_visible);
	viewStatusBarAct->update(statusbar_visible);

	pref->beginGroup(settingsGroupName());
	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());
	pref->endGroup();

	controlbar->didExitFullscreen();
	toolbar2->didExitFullscreen();
	toolbar->didExitFullscreen();
}

void TBase::leftClickFunction() {
	qDebug("Gui::TBase::leftClickFunction");

	if (core->mdat.detected_type == TMediaData::TYPE_DVDNAV
		&& playerwindow->videoLayer()->underMouse()) {
		core->dvdnavMouse();
	} else if (!pref->mouse_left_click_function.isEmpty()) {
		processFunction(pref->mouse_left_click_function);
	}
}

void TBase::rightClickFunction() {
	qDebug("Gui::TBase::rightClickFunction");

	if (!pref->mouse_right_click_function.isEmpty()) {
		processFunction(pref->mouse_right_click_function);
	}
}

void TBase::doubleClickFunction() {
	qDebug("Gui::TBase::doubleClickFunction");

	if (!pref->mouse_double_click_function.isEmpty()) {
		processFunction(pref->mouse_double_click_function);
	}
}

void TBase::middleClickFunction() {
	qDebug("Gui::TBase::middleClickFunction");

	if (!pref->mouse_middle_click_function.isEmpty()) {
		processFunction(pref->mouse_middle_click_function);
	}
}

void TBase::xbutton1ClickFunction() {
	qDebug("Gui::TBase::xbutton1ClickFunction");

	if (!pref->mouse_xbutton1_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton1_click_function);
	}
}

void TBase::xbutton2ClickFunction() {
	qDebug("Gui::TBase::xbutton2ClickFunction");

	if (!pref->mouse_xbutton2_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton2_click_function);
	}
}

void TBase::moveWindow(QPoint diff) {

	move(pos() + diff);
}

void TBase::processFunction(QString function) {

	// Check function for checkable actions
	static QRegExp func_rx("(.*) (true|false)");
	bool value = false;
	bool checkableFunction = false;

	if (func_rx.indexIn(function) >= 0) {
		function = func_rx.cap(1);
		value = func_rx.cap(2) == "true";
		checkableFunction = true;
	}

	QAction* action = TActionsEditor::findAction(this, function);
	if (!action)
		action = TActionsEditor::findAction(playlist, function);

	if (action) {
		if (action->isEnabled()) {
			if (action->isCheckable() && checkableFunction) {
				qDebug() << "Gui::TBase::processFunction: setting checked action"
						 << function << value;
				action->setChecked(value);
			} else {
				qDebug() << "Gui::TBase::processFunction: triggering action" << function;
				action->trigger();
			}
		} else {
			qDebug() << "Gui::TBase::processFunction: canceling disabled action"
					 << function;
		}
	} else {
		qWarning() << "Gui::TBase::processFunction: action" << function << "not found";
	}
}

void TBase::runActions(QString actions) {
	qDebug("Gui::TBase::runActions");

	actions = actions.simplified(); // Remove white space

	QAction* action;
	QStringList actionsList = actions.split(" ");

	for (int n = 0; n < actionsList.count(); n++) {
		QString actionStr = actionsList[n];
		QString par = ""; //the parameter which the action takes

		//set par if the next word is a boolean value
		if ((n+1) < actionsList.count()) {
			if ((actionsList[n+1].toLower() == "true") || (actionsList[n+1].toLower() == "false")) {
				par = actionsList[n+1].toLower();
				n++;
			} //end if
		} //end if

		action = TActionsEditor::findAction(this, actionStr);
		if (!action) action = TActionsEditor::findAction(playlist, actionStr);

		if (action) {
			qDebug("Gui::TBase::runActions: running action: '%s' (par: '%s')",
				   actionStr.toUtf8().data(), par.toUtf8().data());

			if (action->isCheckable()) {
				if (par.isEmpty()) {
					//action->toggle();
					action->trigger();
				} else {
					action->setChecked((par == "true"));
				} //end if
			} else {
				action->trigger();
			} //end if
		} else {
			qWarning("Gui::TBase::runActions: action: '%s' not found",actionStr.toUtf8().data());
		} //end if
	} //end for
}

void TBase::checkPendingActionsToRun() {
	qDebug("Gui::TBase::checkPendingActionsToRun");

	QString actions;
	if (!pending_actions_to_run.isEmpty()) {
		actions = pending_actions_to_run;
		pending_actions_to_run.clear();
		if (!pref->actions_to_run.isEmpty()) {
			actions = pref->actions_to_run +" "+ actions;
		}
	} else {
		actions = pref->actions_to_run;
	}

	if (!actions.isEmpty()) {
		qDebug("Gui::TBase::checkPendingActionsToRun: actions: '%s'", actions.toUtf8().constData());
		runActions(actions);
	}
}

#ifdef CHECK_UPGRADED
void TBase::checkIfUpgraded() {
	qDebug("Gui::TBase::checkIfUpgraded");

	if ((pref->check_if_upgraded) && (pref->smplayer_stable_version != Version::stable())) {
		// Running a new version
		qDebug("Gui::TBase::checkIfUpgraded: running a new version: %s", Version::stable().toUtf8().constData());
		QString os = "other";
		#ifdef Q_OS_WIN
		os = "win";
		#endif
		#ifdef Q_OS_LINUX
		os = "linux";
		#endif
		QDesktopServices::openUrl(QString(URL_THANK_YOU "?version=%1&so=%2").arg(Version::printable()).arg(os));
	}
	pref->smplayer_stable_version = Version::stable();
}
#endif

#ifdef YOUTUBE_SUPPORT
void TBase::YTNoSslSupport() {
	qDebug("Gui::TBase::YTNoSslSupport");
	QMessageBox::warning(this, tr("Connection failed"),
		tr("The video you requested needs to open a HTTPS connection.") +"<br>"+
		tr("Unfortunately the OpenSSL component, required for it, is not available in your system.") +"<br>"+
		tr("Please, visit %1 to know how to fix this problem.")
			.arg("<a href=\"" URL_OPENSSL_INFO "\">" + tr("this link") + "</a>"));
}

void TBase::YTNoSignature(const QString & title) {
	qDebug("Gui::TBase::YTNoSignature: %s", title.toUtf8().constData());

	QString t = title;

	QString info_text;
	if (title.isEmpty()) {
		info_text = tr("Unfortunately due to changes in the Youtube page, this video can't be played.");
	} else {
		t.replace(" - YouTube", "");
		info_text = tr("Unfortunately due to changes in the Youtube page, the video '%1' can't be played.").arg(t);
	}

	#ifdef YT_USE_YTSIG
	int ret = QMessageBox::question(this, tr("Problems with Youtube"),
				info_text + "<br><br>" +
				tr("Do you want to update the Youtube code? This may fix the problem."),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	if (ret == QMessageBox::Yes) {
		YTUpdateScript();
	}
	#else
	QMessageBox::warning(this, tr("Problems with Youtube"),
		info_text + "<br><br>" +
		tr("Maybe updating SMPlayer could fix the problem."));
	#endif
}

#ifdef YT_USE_YTSIG
void TBase::YTUpdateScript() {
	static CodeDownloader* downloader = 0;
	if (!downloader) downloader = new CodeDownloader(this);
	downloader->saveAs(TPaths::configPath() + "/yt.js");
	downloader->show();
	downloader->download(QUrl(URL_YT_CODE));
}
#endif // YT_USE_YTSIG
#endif //YOUTUBE_SUPPORT

void TBase::gotForbidden() {
	qDebug("Gui::TBase::gotForbidden");

	if (!pref->report_mplayer_crashes) {
		qDebug("Gui::TBase::gotForbidden: not displaying error dialog");
		return;
	}

	static bool busy = false;

	if (busy) return;

	busy = true;
#ifdef YOUTUBE_SUPPORT
	if (core->mdat.filename.contains("youtube.com")) {
		YTNoSignature("");
	} else
#endif
	{
		QMessageBox::warning(this, tr("Error detected"), 
			tr("Unfortunately this video can't be played.") +"<br>"+
			tr("The server returned '%1'").arg("403: Forbidden"));
	}
	busy = false;
}

void TBase::dragEnterEvent(QDragEnterEvent *e) {
	qDebug("Gui::TBase::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void TBase::dropEvent(QDropEvent *e) {
	qDebug("Gui::TBase::dropEvent");

	QStringList files;

	if (e->mimeData()->hasUrls()) {
		QList <QUrl> urls = e->mimeData()->urls();
		for (int n = 0; n < urls.count(); n++) {
			QUrl url = urls[n];
			if (url.isValid()) {
				QString filename;
				if (url.scheme() == "file")
					filename = url.toLocalFile();
				else filename = url.toString();
				qDebug() << "Gui::TBase::dropEvent: adding" << filename;
				files.append(filename);
			} else {
				qWarning() << "Gui::TBase::dropEvent:: ignoring" << url.toString();
			}
		}
	}

	qDebug("Gui::TBase::dropEvent: number of files: %d", files.count());
	openFiles(files);
}

void TBase::showContextMenu() {
	showContextMenu(QCursor::pos());
}

void TBase::showContextMenu(QPoint p) {
	//qDebug("Gui::TBase::showContextMenu: %d, %d", p.x(), p.y());
	execPopup(this, popup, p);
}

// Called when a video has started to play
void TBase::enterFullscreenOnPlay() {
	qDebug("Gui::TBase::enterFullscreenOnPlay: arg_start_in_fullscreen: %d, pref->start_in_fullscreen: %d",
		   arg_start_in_fullscreen, pref->start_in_fullscreen);

	if (arg_start_in_fullscreen != 0) {
		if ((arg_start_in_fullscreen == 1) || (pref->start_in_fullscreen)) {
			if (!pref->fullscreen)
				toggleFullscreen(true);
		}
	}
}

// Called when the playlist has stopped
void TBase::exitFullscreenOnStop() {

	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void TBase::playlistHasFinished() {
	qDebug("Gui::TBase::playlistHasFinished");

	core->stop();

	qDebug("Gui::TBase::playlistHasFinished: arg_close_on_finish: %d, pref->close_on_finish: %d", arg_close_on_finish, pref->close_on_finish);

	if (arg_close_on_finish != 0) {
		if ((arg_close_on_finish == 1) || (pref->close_on_finish)) {
			#ifdef AUTO_SHUTDOWN_PC
			if (pref->auto_shutdown_pc) {
				ShutdownDialog d(this);
				if (d.exec() == QDialog::Accepted) {
					qDebug("Gui::TBase::playlistHasFinished: the PC will shut down");
					Shutdown::shutdown();
				} else {
					qDebug("Gui::TBase::playlistHasFinished: shutdown aborted");
				}
			}
			#endif
			close();
		}
	}
}

void TBase::onStateChanged(TCore::State state) {
	qDebug("Gui::TBase::onStateChanged: %s", core->stateToString().toUtf8().data());

	switch (state) {
		case TCore::Playing:
			playAct->setEnabled(false);
			displayMessage(tr("Playing %1").arg(core->mdat.filename), 3000);
			auto_hide_timer->startAutoHideMouse();
			break;
		case TCore::Paused:
			playAct->setEnabled(true);
			displayMessage(tr("Pause"), 0);
			auto_hide_timer->stopAutoHideMouse();
			break;
		case TCore::Stopped:
			disableActionsOnStop();
			setWindowCaption("SMPlayer");
			displayMessage(tr("Stop") , 3000);
			auto_hide_timer->stopAutoHideMouse();
			break;
	}

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	just_stopped = false;

	if (state == TCore::Stopped) {
		just_stopped = true;
		int time = 1000 * 60; // 1 minute
		QTimer::singleShot(time, this, SLOT(clear_just_stopped()));
	}
#endif
#endif
}

void TBase::displayMessage(QString message, int time) {
	statusBar()->showMessage(message, time);
}

void TBase::displayMessage(QString message) {
	displayMessage(message, 2000);
}

void TBase::gotCurrentTime(double sec) {
	//qDebug("Gui::TBase::gotCurrentTime: %f", sec);

	QString time =
		Helper::formatTime((int) sec) + " / " +
		Helper::formatTime(qRound(core->mdat.duration));
	time_label_action->setText(time);

	emit timeChanged(time);
}

void TBase::gotDuration(double duration) {
	Q_UNUSED(duration)

	// Uses duration in text
	gotCurrentTime(core->mset.current_sec);
}

void TBase::changeSize(int precentage) {

	bool center = false;
	if (isMaximized()) {
		showNormal();
		center = true;
	}
	core->changeSize(precentage);
	if (center)
		centerWindow();
}

void TBase::toggleDoubleSize() {
	if (pref->size_factor != 1.0)
		core->changeSize(100);
	else core->changeSize(200);
}

void TBase::centerWindow() {

	QSize center_pos = (TDesktop::availableSize(this) - frameGeometry().size()) / 2;
	if (center_pos.isValid()) {
		move(center_pos.width(), center_pos.height());
	}
}

bool TBase::optimizeSizeFactor(double factor) {

	if (qAbs(factor - pref->size_factor) < 0.05) {
		qDebug("Gui::TBase::optimizeSizeFactor: optimizing size factor from %f to predefined value %f",
			   pref->size_factor, factor);
		pref->size_factor = factor;
		return true;
	}
	return false;
}

void TBase::optimizeSizeFactor(int w, int h) {


	if (w <= 0 || h <= 0)
		return;

	// Limit size to 0.8 of available size
	const double f = 0.8;
	QSize available_size = TDesktop::availableSize(this)
						   - frameGeometry().size() + panel->size();
	QSize video_size = playerwindow->getAdjustedSize(w, h, pref->size_factor);
	double max = f * available_size.height();
	if (video_size.height() > max) {
		pref->size_factor = max / h;
		qDebug("Gui::TBase::optimizeSizeFactor: height larger as %f desktop, reducing size factor to %f",
			   f, pref->size_factor);
		video_size = playerwindow->getAdjustedSize(w, h, pref->size_factor);
	}
	max = f * available_size.width();
	if (video_size.width() > max) {
		pref->size_factor = max / w;
		qDebug("Gui::TBase::optimizeSizeFactor: width larger as %f desktop, reducing size factor to %f",
			   f, pref->size_factor);
		video_size = playerwindow->getAdjustedSize(w, h, pref->size_factor);
	}

	if (optimizeSizeFactor(0.50))
		return;
	if (optimizeSizeFactor(0.75))
		return;
	if (optimizeSizeFactor(1.00))
		return;
	if (optimizeSizeFactor(1.25))
		return;
	if (optimizeSizeFactor(1.50))
		return;
	if (optimizeSizeFactor(1.75))
		return;
	if (optimizeSizeFactor(2.00))
		return;
	if (optimizeSizeFactor(3.00))
		return;
	if (optimizeSizeFactor(4.00))
		return;

	// Make width multiple of 16
	int new_w = ((video_size.width() + 8) / 16) * 16;
	double factor = (double) new_w / w;
	qDebug("Gui::TBase::optimizeSizeFactor: optimizing width %d factor %f to multiple of 16 %d factor %f",
		   video_size.width(), pref->size_factor, new_w, factor);
	pref->size_factor = factor;
}

void TBase::hidePanel() {
	qDebug("Gui::TBase::hidePanel");

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

// Slot called when media settings reset or loaded
void TBase::onMediaSettingsChanged() {
	qDebug("Gui::TBase::onMediaSettingsChanged");

	TMediaSettings* mset = &core->mset;

	// Play
	// Speed
	// TODO: make checkable
	// speed_menu->normalSpeedAct->setChecked(core->mset.speed == 1.0);
	// A-B section
	repeatAct->setChecked(mset->loop);

	// Video
	// Aspectratio
	aspect_menu->group->setChecked(mset->aspect_ratio_id);
	// Video filters
	deinterlace_menu->group->setChecked(mset->current_deinterlacer);
	videofilter_menu->updateFilters();
	rotate_menu->group->setChecked(mset->rotate);
	flipAct->setChecked(mset->flip);
	mirrorAct->setChecked(mset->mirror);
	updateVideoEqualizer();

	// Audio filters
	// Volume normalization filter
	volnormAct->setChecked(mset->volnorm_filter);

#ifdef MPLAYER_SUPPORT
	// Karaoke
	karaokeAct->setChecked(mset->karaoke_filter);
	// Extra stereo
	extrastereoAct->setChecked(mset->extrastereo_filter);
#endif

	audiochannels_menu->group->setChecked(mset->audio_use_channels);
	stereomode_menu->group->setChecked(mset->stereo_mode);
	updateAudioEqualizer();
}

// Slot called by signal videoOutResolutionChanged
void TBase::onVideoOutResolutionChanged(int w, int h) {
	qDebug("Gui::TBase::onVideoOutResolutionChanged: %d, %d", w, h);

	if (w <= 0 || h <= 0) {
		// No video
		if (pref->hide_video_window_on_audio_files) {
			hidePanel();
		} else {
			playerwindow->showLogo();
		}
	} else {
		if (!panel->isVisible()) {
			panel->show();
		}
		// block_resize set if pref->save_window_size_on_exit selected
		if (!block_resize && !isMaximized()) {
			if (pref->resize_method != Settings::TPreferences::Never) {
				optimizeSizeFactor(w, h);
				resizeWindow(w, h);
			}
			if (center_window) {
				centerWindow();
			}
		}
	}

	block_resize = false;
	center_window = false;
}

// Slot called by signal needResize
void TBase::resizeWindow(int w, int h) {
	// qDebug("Gui::TBase::resizeWindow: %d, %d", w, h);

	if (!pref->fullscreen && !isMaximized()) {
		resizeMainWindow(w, h);
		TDesktop::keepInsideDesktop(this);
	}
}

void TBase::resizeMainWindow(int w, int h, bool try_twice) {
	qDebug("Gui::TBase::resizeMainWindow: size to scale: %d x %d, size factor %f",
		   w, h, pref->size_factor);

	// Adjust for selected size and aspect.
	QSize video_size = playerwindow->getAdjustedSize(w, h, pref->size_factor);
	if (video_size == panel->size()) {
		qDebug("Gui::TBase::resizeMainWindow: the panel is already the required size. Doing nothing.");
		return;
	}

	QSize new_size = size() + video_size - panel->size();
	qDebug("Gui::TBase::resizeMainWindow: resizing window from %d x %d to %d x %d",
		   width(), height(), new_size.width(), new_size.height());
	resize(new_size);

	if (panel->size() == video_size) {
		qDebug("Gui::TBase::resizeMainWindow: resize succeeded");
	} else {
		// Resizing the main window can change the height of the tool bars,
		// which will change the height of the panel during the resize.
		// Often fixed by resizing once again, using the new panel height.
		if (try_twice) {
			qDebug("Gui::TBase::resizeMainWindow: panel size now %d x %d. Wanted size %d x %d. Trying a second time",
				   panel->size().width(), panel->size().height(),
				   video_size.width(), video_size.height());
			resizeMainWindow(w, h, false);
		} else {
			qDebug("Gui::TBase::resizeMainWindow: resize failed. Panel size now %d x %d. Wanted size %d x %d",
				   panel->size().width(), panel->size().height(),
				   video_size.width(), video_size.height());
		}
	}

}

void TBase::resizeEvent(QResizeEvent* event) {
	qDebug() << "TBase::resizeEvent: event spontaneous:" << event->spontaneous();

	QMainWindow::resizeEvent(event);

	// Update size factor after window resized by user.
	// In TPlayerWindow::resizeEvent() event->spontaneous() does not become
	// true during an user induces resize, so its needs to be here.
	if (event->spontaneous() && block_update_size_factor == 0) {
		playerwindow->updateSizeFactor();
	}
}

void TBase::displayGotoTime(int t) {

	int jump_time = qRound(core->mdat.duration * t / core->positionMax());
	QString s = tr("Jump to %1").arg(Helper::formatTime(jump_time));
	statusBar()->showMessage(s, 1000);

	if (pref->fullscreen) {
		core->displayTextOnOSD(s);
	}
}

void TBase::goToPosOnDragging(int t) {
	if (pref->update_while_seeking) {
		core->goToPosition(t);
	}
}

void TBase::setStayOnTop(bool b) {
	qDebug("Gui::TBase::setStayOnTop: %d", b);

	if ((b && (windowFlags() & Qt::WindowStaysOnTopHint)) ||
		 (!b && (!(windowFlags() & Qt::WindowStaysOnTopHint))))
	{
		// identical do nothing
		qDebug("Gui::TBase::setStayOnTop: nothing to do");
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

void TBase::changeStayOnTop(int stay_on_top) {
	qDebug("Gui::TBase::changeStayOnTop");

	switch (stay_on_top) {
		case Settings::TPreferences::AlwaysOnTop : setStayOnTop(true); break;
		case Settings::TPreferences::NeverOnTop  : setStayOnTop(false); break;
		case Settings::TPreferences::WhilePlayingOnTop : setStayOnTop((core->state() == TCore::Playing)); break;
	}

	pref->stay_on_top = (Settings::TPreferences::OnTop) stay_on_top;
	stay_on_top_menu->group->setChecked(pref->stay_on_top);
}

void TBase::checkStayOnTop(TCore::State state) {
	qDebug("Gui::TBase::checkStayOnTop");

	if (!pref->fullscreen
		&& (pref->stay_on_top == Settings::TPreferences::WhilePlayingOnTop)) {
		setStayOnTop((state == TCore::Playing));
	}
}

void TBase::toggleStayOnTop() {
	if (pref->stay_on_top == Settings::TPreferences::AlwaysOnTop)
		changeStayOnTop(Settings::TPreferences::NeverOnTop);
	else
	if (pref->stay_on_top == Settings::TPreferences::NeverOnTop)
		changeStayOnTop(Settings::TPreferences::AlwaysOnTop);
}

// Called when a new window (equalizer, preferences..) is opened.
void TBase::exitFullscreenIfNeeded() {
	/*
	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
	*/
}

#if ALLOW_CHANGE_STYLESHEET
QString TBase::loadQss(QString filename) {
	QFile file(filename);
	file.open(QFile::ReadOnly);
	QString stylesheet = QLatin1String(file.readAll());

#ifdef USE_RESOURCES
	Images::setTheme(pref->iconset);
	QString path;
	if (Images::has_rcc) {
		path = ":/" + pref->iconset;
	} else {
		QDir current = QDir::current();
		QString td = Images::themesDirectory();
		path = current.relativeFilePath(td);
	}
#else
	QDir current = QDir::current();
	QString td = Images::themesDirectory();
	QString path = current.relativeFilePath(td);
#endif
	stylesheet.replace(QRegExp("url\\s*\\(\\s*([^\\);]+)\\s*\\)", Qt::CaseSensitive, QRegExp::RegExp2),
						QString("url(%1\\1)").arg(path + "/"));
	//qDebug("Gui::TBase::loadQss: styleSheet: %s", stylesheet.toUtf8().constData());
	return stylesheet;
}

void TBase::changeStyleSheet(QString style) {
	qDebug("Gui::TBase::changeStyleSheet: %s", style.toUtf8().constData());

	// Load default stylesheet
	QString stylesheet = loadQss(":/default-theme/style.qss");

	if (!style.isEmpty()) {
		// Check main.css
		QString qss_file = TPaths::configPath() + "/themes/" + pref->iconset + "/main.css";
		if (!QFile::exists(qss_file)) {
			qss_file = TPaths::themesPath() +"/"+ pref->iconset + "/main.css";
		}

		// Check style.qss
		if (!QFile::exists(qss_file)) {
			qss_file = TPaths::configPath() + "/themes/" + pref->iconset + "/style.qss";
			if (!QFile::exists(qss_file)) {
				qss_file = TPaths::themesPath() +"/"+ pref->iconset + "/style.qss";
			}
		}

		// Load style file
		if (QFile::exists(qss_file)) {
			qDebug("Gui::TBase::changeStyleSheet: '%s'", qss_file.toUtf8().data());
			stylesheet += loadQss(qss_file);
		}
	}

	//qDebug("Gui::TBase::changeStyleSheet: styleSheet: %s", stylesheet.toUtf8().constData());
	qApp->setStyleSheet(stylesheet);
}
#endif

void TBase::showEvent(QShowEvent* event) {
	qDebug("Gui::TBase::showEvent");

	if (event) {
		QMainWindow::showEvent(event);
	}

	//qDebug("Gui::TBase::showEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if (pref->pause_when_hidden && core->state() == TCore::Paused && !ignore_show_hide_events) {
		qDebug("Gui::TBase::showEvent: unpausing");
		core->play();
	}

	if (toolbar->isFloating()) {
		toolbar->show();
	}
	if (toolbar2->isFloating()) {
		toolbar2->show();
	}
	if (controlbar->isFloating()) {
		controlbar->show();
	}
}

void TBase::hideEvent(QHideEvent* event) {
	qDebug("Gui::TBase::hideEvent");

	if (event) {
		QMainWindow::hideEvent(event);
	}

	//qDebug("Gui::TBase::hideEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if (pref->pause_when_hidden && core->state() == TCore::Playing && !ignore_show_hide_events) {
		qDebug("Gui::TBase::hideEvent: pausing");
		core->pause();
	}

	if (toolbar->isFloating()) {
		toolbar->hide();
	}
	if (toolbar2->isFloating()) {
		toolbar2->hide();
	}
	if (controlbar->isFloating()) {
		controlbar->hide();
	}
}

#if QT_VERSION >= 0x050000
// Qt 5 doesn't call showEvent / hideEvent when the window is minimized or unminimized
bool TBase::event(QEvent* e) {
	//qDebug("Gui::TBase::event: %d", e->type());

	bool result = QMainWindow::event(e);

	if (e->type() == QEvent::WindowStateChange) {
		qDebug("Gui::TBase::event: WindowStateChange");
		if (isMinimized() && !was_minimized) {
			was_minimized = true;
			hideEvent(0);
		}
	} else if (e->type() == QEvent::ActivationChange && isActiveWindow()) {
		qDebug("Gui::TBase::event: ActivationChange: %d", was_minimized);
		if (!isMinimized() && was_minimized) {
			was_minimized = false;
			showEvent(0);
		}
	}

	return result;
}
#endif

void TBase::showExitCodeFromPlayer(int exit_code) {
	qDebug("Gui::TBase::showExitCodeFromPlayer: %d", exit_code);

	QString msg = tr("%1 has finished unexpectedly.").arg(PLAYER_NAME)
			+ " " + tr("Exit code: %1").arg(exit_code);

	if (!pref->report_mplayer_crashes) {
		qDebug("Gui::TBase::showExitCodeFromPlayer: error reporting is turned off");
		displayMessage(msg, 6000);
		return;
	}

	if (exit_code != 255) {
		TErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(PLAYER_NAME));
		d.setText(msg);
		d.setLog(TLog::log->getLogLines());
		d.exec();
	} 
}

void TBase::showErrorFromPlayer(QProcess::ProcessError e) {
	qDebug("Gui::TBase::showErrorFromPlayer");

	if (!pref->report_mplayer_crashes) {
		qDebug("Gui::TBase::showErrorFromPlayer: error reporting is turned off");
		displayMessage(tr("Player crashed or quit with errors."), 6000);
		return;
	}

	if ((e == QProcess::FailedToStart) || (e == QProcess::Crashed)) {
		TErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(PLAYER_NAME));
		if (e == QProcess::FailedToStart) {
			d.setText(tr("%1 failed to start.").arg(PLAYER_NAME) + " " + 
					  tr("Please check the %1 path in preferences.").arg(PLAYER_NAME));
		} else {
			d.setText(tr("%1 has crashed.").arg(PLAYER_NAME) + " " + 
					  tr("See the log for more info."));
		}
		d.setLog(TLog::log->getLogLines());
		d.exec();
	}
}


#ifdef FIND_SUBTITLES
void TBase::showFindSubtitlesDialog() {
	qDebug("Gui::TBase::showFindSubtitlesDialog");

	if (!find_subs_dialog) {
		find_subs_dialog = new FindSubtitlesWindow(this, Qt::Window | Qt::WindowMinMaxButtonsHint);
		find_subs_dialog->setSettings(Settings::pref);
		find_subs_dialog->setWindowIcon(windowIcon());
#if DOWNLOAD_SUBS
		connect(find_subs_dialog, SIGNAL(subtitleDownloaded(const QString &)),
				core, SLOT(loadSub(const QString &)));
#endif
	}

	find_subs_dialog->show();
	find_subs_dialog->setMovie(core->mdat.filename);
}

void TBase::openUploadSubtitlesPage() {	
	QDesktopServices::openUrl(QUrl("http://www.opensubtitles.org/upload"));
}
#endif

#ifdef VIDEOPREVIEW
void TBase::showVideoPreviewDialog() {
	qDebug("Gui::TBase::showVideoPreviewDialog");

	if (video_preview == 0) {
		video_preview = new VideoPreview(pref->mplayer_bin, this);
		video_preview->setSettings(Settings::pref);
	}

	if (!core->mdat.filename.isEmpty()) {
		video_preview->setVideoFile(core->mdat.filename);

		// DVD
		if (TMediaData::isDVD(core->mdat.selected_type)) {
			QString file = core->mdat.filename;
			TDiscData disc_data = TDiscName::split(file);
			QString dvd_folder = disc_data.device;
			if (dvd_folder.isEmpty()) dvd_folder = pref->dvd_device;
			int dvd_title = disc_data.title;
			file = disc_data.protocol + "://" + QString::number(dvd_title);

			video_preview->setVideoFile(file);
			video_preview->setDVDDevice(dvd_folder);
		} else {
			video_preview->setDVDDevice("");
		}
	}

	video_preview->setMplayerPath(pref->mplayer_bin);

	if ((video_preview->showConfigDialog(this)) && (video_preview->createThumbnails())) {
		video_preview->show();
		video_preview->adjustWindowSize();
	}
}
#endif

#ifdef YOUTUBE_SUPPORT
void TBase::showTubeBrowser() {
	qDebug("Gui::TBase::showTubeBrowser");
	QString exec = TPaths::appPath() + "/smtube";
	qDebug("Gui::TBase::showTubeBrowser: '%s'", exec.toUtf8().constData());
	if (!QProcess::startDetached(exec, QStringList())) {
		QMessageBox::warning(this, "SMPlayer",
			tr("The YouTube Browser is not installed.") +"<br>"+ 
			tr("Visit %1 to get it.").arg("<a href=http://www.smtube.org>http://www.smtube.org</a>"));
	}
}
#endif

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
/* Disable screensaver by event */
bool TBase::winEvent (MSG* m, long* result) {
	//qDebug("Gui::TBase::winEvent");
	if (m->message==WM_SYSCOMMAND) {
		if ((m->wParam & 0xFFF0)==SC_SCREENSAVE || (m->wParam & 0xFFF0)==SC_MONITORPOWER) {
			qDebug("Gui::TBase::winEvent: received SC_SCREENSAVE or SC_MONITORPOWER");
			qDebug("Gui::TBase::winEvent: avoid_screensaver: %d", pref->avoid_screensaver);
			qDebug("Gui::TBase::winEvent: playing: %d", core->state()==TCore::Playing);
			qDebug("Gui::TBase::winEvent: video: %d", !core->mdat.novideo);
			
			if ((pref->avoid_screensaver) && (core->state()==TCore::Playing) && (!core->mdat.novideo)) {
				qDebug("Gui::TBase::winEvent: not allowing screensaver");
				(*result) = 0;
				return true;
			} else {
				if ((pref->avoid_screensaver) && (just_stopped)) {
					qDebug("Gui::TBase::winEvent: file just stopped, so not allowing screensaver for a while");
					(*result) = 0;
					return true;
				} else {
					qDebug("Gui::TBase::winEvent: allowing screensaver");
					return false;
				}
			}
		}
	}
	return false;
}
#endif
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
void TBase::clear_just_stopped() {
	qDebug("Gui::TBase::clear_just_stopped");
	just_stopped = false;
}
#endif
#endif

} // namespace Gui

#include "moc_base.cpp"

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

#include "base.h"

#include "filedialog.h"
#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
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

#include "mplayerwindow.h"
#include "desktopinfo.h"
#include "helper.h"
#include "paths.h"
#include "colorutils.h"
#include "global.h"
#include "translator.h"
#include "images.h"
#include "preferences.h"
#include "discname.h"
#include "timeslider.h"
#include "logwindow.h"
#include "playlist.h"
#include "filepropertiesdialog.h"
#include "eqslider.h"
#include "videoequalizer.h"
#include "audioequalizer.h"
#include "inputdvddirectory.h"
#include "inputmplayerversion.h"
#include "inputurl.h"
#include "recents.h"
#include "urlhistory.h"
#include "about.h"
#include "errordialog.h"
#include "timedialog.h"
#include "stereo3ddialog.h"
#include "clhelp.h"
#include "mplayerversion.h"

#ifdef FIND_SUBTITLES
#include "findsubtitleswindow.h"
#endif

#ifdef VIDEOPREVIEW
#include "videopreview.h"
#endif

#include "config.h"
#include "actionseditor.h"

#include "tvlist.h"

#include "preferencesdialog.h"
#ifndef NO_USE_INI_FILES
#include "prefgeneral.h"
#endif
#include "prefinterface.h"
#include "prefinput.h"
#include "prefadvanced.h"
#include "prefplaylist.h"

#include "gui/action.h"
#include "gui/actiongroup.h"
#include "playlist.h"

#include "constants.h"
#include "links.h"

#ifdef MPRIS2
#include "mpris2/mpris2.h"
#endif

#include "extensions.h"
#include "version.h"

#ifdef Q_OS_WIN
#include "deviceinfo.h"
#include <QSysInfo>
#endif

#ifdef UPDATE_CHECKER
#include "updatechecker.h"
#endif

#ifdef YOUTUBE_SUPPORT
  #ifdef YT_USE_YTSIG
  #include "codedownloader.h"
  #endif
#endif

#ifdef REMINDER_ACTIONS
#include "sharedialog.h"
#endif

#ifdef SHAREWIDGET
#include "sharewidget.h"
#endif

#ifdef AUTO_SHUTDOWN_PC
#include "shutdowndialog.h"
#include "shutdown.h"
#endif

using namespace Global;

namespace Gui {


TBase::TBase( QWidget* parent, Qt::WindowFlags flags ) 
	: QMainWindow( parent, flags )
#if QT_VERSION >= 0x050000
	, was_minimized(false)
#endif
#ifdef UPDATE_CHECKER
	, update_checker(0)
#endif
	, block_resize(false)
{
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	just_stopped = false;
#endif
#endif
	ignore_show_hide_events = false;

	arg_close_on_finish = -1;
	arg_start_in_fullscreen = -1;

	setWindowTitle( "SMPlayer" );

	// Not created objects
	popup = 0;
	pref_dialog = 0;
	file_dialog = 0;
	clhelp_window = 0;
#ifdef FIND_SUBTITLES
	find_subs_dialog = 0;
#endif
#ifdef VIDEOPREVIEW
	video_preview = 0;
#endif

	// Create objects:
	createPanel();
	setCentralWidget(panel);

	createMplayerWindow();
	createCore();
	createTPlaylist();
	createVideoEqualizer();
	createAudioEqualizer();

	// Mouse Wheel
	/*
	connect( this, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( this, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );
	*/
	connect( mplayerwindow, SIGNAL(wheelUp()),
             core, SLOT(wheelUp()) );
	connect( mplayerwindow, SIGNAL(wheelDown()),
             core, SLOT(wheelDown()) );

	// Set style before changing color of widgets:
	// Set style
#if STYLE_SWITCHING
	qDebug( "Style name: '%s'", qApp->style()->objectName().toUtf8().data() );
	qDebug( "Style class name: '%s'", qApp->style()->metaObject()->className() );

	default_style = qApp->style()->objectName();
	if (!pref->style.isEmpty()) {
		qApp->setStyle( pref->style );
	}
#endif

#ifdef LOG_MPLAYER
	mplayer_log_window = new LogWindow(0);
#endif
#ifdef LOG_SMPLAYER
	smplayer_log_window = new LogWindow(0);
#endif

	createActions();
	createMenus();
#if AUTODISABLE_ACTIONS
	setActionsEnabled(false);
	if (playlist->count() > 0) {
		playAct->setEnabled(true);
		playOrPauseAct->setEnabled(true);
	}
#endif

#if !DOCK_PLAYLIST
	connect(playlist, SIGNAL(visibilityChanged(bool)),
			showPlaylistAct, SLOT(setChecked(bool)) );
#endif

	setAcceptDrops(true);

	resize(pref->default_size);

	panel->setFocus();

	setupNetworkProxy();

	if (pref->compact_mode) toggleCompactMode(true);
	changeStayOnTop(pref->stay_on_top);

	updateRecents();

#ifdef UPDATE_CHECKER
	update_checker = new UpdateChecker(this, &pref->update_checker_data);
#endif

#ifdef CHECK_UPGRADED
	QTimer::singleShot(30000, this, SLOT(checkIfUpgraded()));
#endif

#if defined(REMINDER_ACTIONS) && !defined(SHAREWIDGET)
	QTimer::singleShot(10000, this, SLOT(checkReminder()));
#endif

#ifdef MPRIS2
	// TODO reenable
	// if (pref->use_mpris2) new Mpris2(this, this);
#endif
}

void TBase::setupNetworkProxy() {
	qDebug("Gui::TBase::setupNetworkProxy");

	QNetworkProxy proxy;

	if ( (pref->use_proxy) && (!pref->proxy_host.isEmpty()) ) {
		proxy.setType((QNetworkProxy::ProxyType) pref->proxy_type);
		proxy.setHostName(pref->proxy_host);
		proxy.setPort(pref->proxy_port);
		if ( (!pref->proxy_username.isEmpty()) && (!pref->proxy_password.isEmpty()) ) {
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

void TBase::loadConfig(const QString &group) {
	qDebug("Gui::TBase::loadConfig");

#if ALLOW_CHANGE_STYLESHEET
	changeStyleSheet(pref->iconset);
#endif

	// Load actions from outside group derived class
	loadActions();

	if (pref->save_window_size_on_exit) {
		QSettings * set = settings;
		// Load window state from inside group derived class
		set->beginGroup(group);
		QPoint p = set->value("pos", pos()).toPoint();
		QSize s = set->value("size", size()).toSize();
		int state = set->value("state", 0).toInt();
		set->endGroup();

		if ( (s.height() < 200) && (!pref->use_mplayer_window) ) {
			s = pref->default_size;
		}

		move(p);
		resize(s);
		setWindowState((Qt::WindowStates) state);

		if (!DesktopInfo::isInsideScreen(this)) {
			move(0,0);
			qWarning("Gui::TBase::loadConfig: window is outside of the screen, moved to 0x0");
		} else {
			// Block resize of main window by loading of video
			// TODO: reset when video fails to load
			block_resize = true;
		}
	} else {
		// Center window
		QSize center_pos = (DesktopInfo::desktop_size(this) - size()) / 2;
		if (center_pos.isValid())
			move(center_pos.width(), center_pos.height());
	}

	// Load playlist settings outside group
	playlist->loadSettings();
}

void TBase::saveConfig(const QString &group) {
	qDebug("Gui::TBase::saveConfig");

	if (pref->save_window_size_on_exit) {
		QSettings * set = settings;
		set->beginGroup(group);
		set->setValue( "pos", pos() );
		set->setValue( "size", size() );
		set->setValue( "state", (int) windowState() );
		set->endGroup();
	}

	playlist->saveSettings();
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
			/* if (core->state() == Core::Stopped) { emit openFileRequested(); } */
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
			if (core->state() != Core::Stopped) {
				core->loadSub(arg);
			}
		}
	}
}
#endif

TBase::~TBase() {
	delete core; // delete before mplayerwindow, otherwise, segfault...
#ifdef LOG_MPLAYER
	delete mplayer_log_window;
#endif
#ifdef LOG_SMPLAYER
	delete smplayer_log_window;
#endif

	delete favorites;
	delete tvlist;
	delete radiolist;

//#if !DOCK_PLAYLIST
	if (playlist) {
		delete playlist;
		playlist = 0;
	}
//#endif

#ifdef FIND_SUBTITLES
	if (find_subs_dialog) {
		delete find_subs_dialog;
		find_subs_dialog = 0; // Necessary?
	}
#endif

#ifdef VIDEOPREVIEW
	if (video_preview) {
		delete video_preview;
	}
#endif
}

void TBase::createActions() {
	qDebug("Gui::TBase::createActions");

	// Menu File
	openFileAct = new TAction( QKeySequence("Ctrl+F"), this, "open_file" );
	connect( openFileAct, SIGNAL(triggered()),
             this, SLOT(openFile()) );

	openDirectoryAct = new TAction( this, "open_directory" );
	connect( openDirectoryAct, SIGNAL(triggered()),
             this, SLOT(openDirectory()) );

	openTPlaylistAct = new TAction( this, "open_playlist" );
	connect( openTPlaylistAct, SIGNAL(triggered()),
             playlist, SLOT(load()) );

	openVCDAct = new TAction( this, "open_vcd" );
	connect( openVCDAct, SIGNAL(triggered()),
             this, SLOT(openVCD()) );

	openAudioCDAct = new TAction( this, "open_audio_cd" );
	connect( openAudioCDAct, SIGNAL(triggered()),
             this, SLOT(openAudioCD()) );

	openDVDAct = new TAction( this, "open_dvd" );
	connect( openDVDAct, SIGNAL(triggered()),
             this, SLOT(openDVD()) );

	openDVDFolderAct = new TAction( this, "open_dvd_folder" );
	connect( openDVDFolderAct, SIGNAL(triggered()),
             this, SLOT(openDVDFromFolder()) );

	// Bluray section.
	openBluRayAct = new TAction( this, "open_bluray" );
	connect( openBluRayAct, SIGNAL(triggered()),
             this, SLOT(openBluRay()));

	openBluRayFolderAct = new TAction( this, "open_bluray_folder" );
	connect( openBluRayFolderAct, SIGNAL(triggered()),
             this, SLOT(openBluRayFromFolder()));

	openURLAct = new TAction( QKeySequence("Ctrl+U"), this, "open_url" );
	connect( openURLAct, SIGNAL(triggered()),
             this, SLOT(openURL()) );

	exitAct = new TAction( QKeySequence("Ctrl+X"), this, "close" );
	connect( exitAct, SIGNAL(triggered()), this, SLOT(closeWindow()) );

	clearRecentsAct = new TAction( this, "clear_recents" );
	connect( clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()) );

	// Favorites
	favorites = new Favorites(Paths::configPath() + "/favorites.m3u8", this);
	favorites->menuAction()->setObjectName( "favorites_menu" );
	addAction(favorites->editAct());
	addAction(favorites->jumpAct());
	addAction(favorites->nextAct());
	addAction(favorites->previousAct());
	connect(favorites, SIGNAL(activated(QString)), this, SLOT(openFavorite(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            favorites, SLOT(getCurrentMedia(const QString &, const QString &)));

	// TV and Radio
	tvlist = new TVList(pref->check_channels_conf_on_startup, 
                        TVList::TV, Paths::configPath() + "/tv.m3u8", this);
	tvlist->menuAction()->setObjectName( "tv_menu" );
	addAction(tvlist->editAct());
	addAction(tvlist->jumpAct());
	addAction(tvlist->nextAct());
	addAction(tvlist->previousAct());
	tvlist->nextAct()->setShortcut( Qt::Key_H );
	tvlist->previousAct()->setShortcut( Qt::Key_L );
	tvlist->nextAct()->setObjectName("next_tv");
	tvlist->previousAct()->setObjectName("previous_tv");
	tvlist->editAct()->setObjectName("edit_tv_list");
	tvlist->jumpAct()->setObjectName("jump_tv_list");
	connect(tvlist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            tvlist, SLOT(getCurrentMedia(const QString &, const QString &)));

	radiolist = new TVList(pref->check_channels_conf_on_startup, 
                           TVList::Radio, Paths::configPath() + "/radio.m3u8", this);
	radiolist->menuAction()->setObjectName( "radio_menu" );
	addAction(radiolist->editAct());
	addAction(radiolist->jumpAct());
	addAction(radiolist->nextAct());
	addAction(radiolist->previousAct());
	radiolist->nextAct()->setShortcut( Qt::SHIFT | Qt::Key_H );
	radiolist->previousAct()->setShortcut( Qt::SHIFT | Qt::Key_L );
	radiolist->nextAct()->setObjectName("next_radio");
	radiolist->previousAct()->setObjectName("previous_radio");
	radiolist->editAct()->setObjectName("edit_radio_list");
	radiolist->jumpAct()->setObjectName("jump_radio_list");
	connect(radiolist, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString &, const QString &)),
            radiolist, SLOT(getCurrentMedia(const QString &, const QString &)));


	// Menu Play
	playAct = new TAction( this, "play" );
	connect( playAct, SIGNAL(triggered()),
             core, SLOT(play()) );

	playOrPauseAct = new TAction( Qt::Key_MediaPlay, this, "play_or_pause" );
	playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause")); // MCE remote key
	connect( playOrPauseAct, SIGNAL(triggered()),
			 core, SLOT(playOrPause()) );

	pauseAct = new TAction( Qt::Key_Space, this, "pause" );
	pauseAct->addShortcut(QKeySequence("Media Pause")); // MCE remote key
	connect( pauseAct, SIGNAL(triggered()),
             core, SLOT(pause()) );

	stopAct = new TAction( Qt::Key_MediaStop, this, "stop" );
	connect( stopAct, SIGNAL(triggered()),
             core, SLOT(stop()) );

	frameStepAct = new TAction( Qt::Key_Period, this, "frame_step" );
	connect( frameStepAct, SIGNAL(triggered()),
             core, SLOT(frameStep()) );

	frameBackStepAct = new TAction( Qt::Key_Comma, this, "frame_back_step" );
	connect( frameBackStepAct, SIGNAL(triggered()),
             core, SLOT(frameBackStep()) );

	rewind1Act = new TAction( Qt::Key_Left, this, "rewind1" );
	rewind1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
	connect( rewind1Act, SIGNAL(triggered()),
             core, SLOT(srewind()) );

	rewind2Act = new TAction( Qt::Key_Down, this, "rewind2" );
	connect( rewind2Act, SIGNAL(triggered()),
             core, SLOT(rewind()) );

	rewind3Act = new TAction( Qt::Key_PageDown, this, "rewind3" );
	connect( rewind3Act, SIGNAL(triggered()),
             core, SLOT(fastrewind()) );

	forward1Act = new TAction( Qt::Key_Right, this, "forward1" );
	forward1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
	connect( forward1Act, SIGNAL(triggered()),
             core, SLOT(sforward()) );

	forward2Act = new TAction( Qt::Key_Up, this, "forward2" );
	connect( forward2Act, SIGNAL(triggered()),
             core, SLOT(forward()) );

	forward3Act = new TAction( Qt::Key_PageUp, this, "forward3" );
	connect( forward3Act, SIGNAL(triggered()),
             core, SLOT(fastforward()) );

	setAMarkerAct = new TAction( this, "set_a_marker" );
	connect( setAMarkerAct, SIGNAL(triggered()),
             core, SLOT(setAMarker()) );

	setBMarkerAct = new TAction( this, "set_b_marker" );
	connect( setBMarkerAct, SIGNAL(triggered()),
             core, SLOT(setBMarker()) );

	clearABMarkersAct = new TAction( this, "clear_ab_markers" );
	connect( clearABMarkersAct, SIGNAL(triggered()),
             core, SLOT(clearABMarkers()) );

	repeatAct = new TAction( this, "repeat" );
	repeatAct->setCheckable( true );
	connect( repeatAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleRepeat(bool)) );

	gotoAct = new TAction( QKeySequence("Ctrl+J"), this, "jump_to" );
	connect( gotoAct, SIGNAL(triggered()),
             this, SLOT(showGotoDialog()) );

	// Submenu Speed
	normalSpeedAct = new TAction( Qt::Key_Backspace, this, "normal_speed" );
	connect( normalSpeedAct, SIGNAL(triggered()),
             core, SLOT(normalSpeed()) );

	halveSpeedAct = new TAction( Qt::Key_BraceLeft, this, "halve_speed" );
	connect( halveSpeedAct, SIGNAL(triggered()),
             core, SLOT(halveSpeed()) );

	doubleSpeedAct = new TAction( Qt::Key_BraceRight, this, "double_speed" );
	connect( doubleSpeedAct, SIGNAL(triggered()),
             core, SLOT(doubleSpeed()) );

	decSpeed10Act = new TAction( Qt::Key_BracketLeft, this, "dec_speed" );
	connect( decSpeed10Act, SIGNAL(triggered()),
             core, SLOT(decSpeed10()) );

	incSpeed10Act = new TAction( Qt::Key_BracketRight, this, "inc_speed" );
	connect( incSpeed10Act, SIGNAL(triggered()),
             core, SLOT(incSpeed10()) );

	decSpeed4Act = new TAction( this, "dec_speed_4" );
	connect( decSpeed4Act, SIGNAL(triggered()),
             core, SLOT(decSpeed4()) );

	incSpeed4Act = new TAction( this, "inc_speed_4" );
	connect( incSpeed4Act, SIGNAL(triggered()),
             core, SLOT(incSpeed4()) );

	decSpeed1Act = new TAction( this, "dec_speed_1" );
	connect( decSpeed1Act, SIGNAL(triggered()),
             core, SLOT(decSpeed1()) );

	incSpeed1Act = new TAction( this, "inc_speed_1" );
	connect( incSpeed1Act, SIGNAL(triggered()),
             core, SLOT(incSpeed1()) );


	// Menu Video
	fullscreenAct = new TAction( Qt::Key_F, this, "fullscreen" );
	fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable( true );
	connect( fullscreenAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleFullscreen(bool)) );

	compactAct = new TAction( QKeySequence("Ctrl+C"), this, "compact" );
	compactAct->setCheckable( true );
	connect( compactAct, SIGNAL(toggled(bool)),
             this, SLOT(toggleCompactMode(bool)) );

	videoEqualizerAct = new TAction( QKeySequence("Ctrl+E"), this, "video_equalizer" );
	videoEqualizerAct->setCheckable( true );
	connect( videoEqualizerAct, SIGNAL(toggled(bool)),
             this, SLOT(showVideoEqualizer(bool)) );

	// Single screenshot
	screenshotAct = new TAction( Qt::Key_S, this, "screenshot" );
	connect( screenshotAct, SIGNAL(triggered()),
             core, SLOT(screenshot()) );

	// Multiple screenshots
	screenshotsAct = new TAction( QKeySequence("Shift+D"), this, "multiple_screenshots" );
	connect( screenshotsAct, SIGNAL(triggered()),
             core, SLOT(screenshots()) );

#ifdef VIDEOPREVIEW
	videoPreviewAct = new TAction( this, "video_preview" );
	connect( videoPreviewAct, SIGNAL(triggered()),
             this, SLOT(showVideoPreviewDialog()) );
#endif

	flipAct = new TAction( this, "flip" );
	flipAct->setCheckable( true );
	connect( flipAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleFlip(bool)) );

	mirrorAct = new TAction( this, "mirror" );
	mirrorAct->setCheckable( true );
	connect( mirrorAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleMirror(bool)) );

	stereo3dAct = new TAction( this, "stereo_3d_filter" );
	connect( stereo3dAct, SIGNAL(triggered()),
             this, SLOT(showStereo3dDialog()) );

	// Submenu filter
	postProcessingAct = new TAction( this, "postprocessing" );
	postProcessingAct->setCheckable( true );
	connect( postProcessingAct, SIGNAL(toggled(bool)),
             core, SLOT(togglePostprocessing(bool)) );

	phaseAct = new TAction( this, "autodetect_phase" );
	phaseAct->setCheckable( true );
	connect( phaseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleAutophase(bool)) );

	deblockAct = new TAction( this, "deblock" );
	deblockAct->setCheckable( true );
	connect( deblockAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDeblock(bool)) );

	deringAct = new TAction( this, "dering" );
	deringAct->setCheckable( true );
	connect( deringAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleDering(bool)) );

	gradfunAct = new TAction( this, "gradfun" );
	gradfunAct->setCheckable( true );
	connect( gradfunAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleGradfun(bool)) );


	addNoiseAct = new TAction( this, "add_noise" );
	addNoiseAct->setCheckable( true );
	connect( addNoiseAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleNoise(bool)) );

	addLetterboxAct = new TAction( this, "add_letterbox" );
	addLetterboxAct->setCheckable( true );
	connect( addLetterboxAct, SIGNAL(toggled(bool)),
             core, SLOT(changeLetterbox(bool)) );

	upscaleAct = new TAction( this, "upscaling" );
	upscaleAct->setCheckable( true );
	connect( upscaleAct, SIGNAL(toggled(bool)),
             core, SLOT(changeUpscale(bool)) );


	// Menu Audio
	audioEqualizerAct = new TAction( this, "audio_equalizer" );
	audioEqualizerAct->setCheckable( true );
	connect( audioEqualizerAct, SIGNAL(toggled(bool)),
             this, SLOT(showAudioEqualizer(bool)) );

	muteAct = new TAction( Qt::Key_M, this, "mute" );
	muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
	muteAct->setCheckable( true );
	connect( muteAct, SIGNAL(toggled(bool)),
             core, SLOT(mute(bool)) );

#if USE_MULTIPLE_SHORTCUTS
	decVolumeAct = new TAction( this, "decrease_volume" );
	decVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("9,/") );
	decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
#else
	decVolumeAct = new TAction( Qt::Key_9, this, "dec_volume" );
#endif
	connect( decVolumeAct, SIGNAL(triggered()),
             core, SLOT(decVolume()) );

#if USE_MULTIPLE_SHORTCUTS
	incVolumeAct = new TAction( this, "increase_volume" );
	incVolumeAct->setShortcuts( ActionsEditor::stringToShortcuts("0,*") );
	incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
#else
	incVolumeAct = new TAction( Qt::Key_0, this, "inc_volume" );
#endif
	connect( incVolumeAct, SIGNAL(triggered()),
             core, SLOT(incVolume()) );

	decAudioDelayAct = new TAction( Qt::Key_Minus, this, "dec_audio_delay" );
	connect( decAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(decAudioDelay()) );

	incAudioDelayAct = new TAction( Qt::Key_Plus, this, "inc_audio_delay" );
	connect( incAudioDelayAct, SIGNAL(triggered()),
             core, SLOT(incAudioDelay()) );

	audioDelayAct = new TAction( this, "audio_delay" );
	connect( audioDelayAct, SIGNAL(triggered()),
             this, SLOT(showAudioDelayDialog()) );

	loadAudioAct = new TAction( this, "load_audio_file" );
	connect( loadAudioAct, SIGNAL(triggered()),
             this, SLOT(loadAudioFile()) );

	unloadAudioAct = new TAction( this, "unload_audio_file" );
	connect( unloadAudioAct, SIGNAL(triggered()),
             core, SLOT(unloadAudioFile()) );


	// Submenu Filters
	extrastereoAct = new TAction( this, "extrastereo_filter" );
	extrastereoAct->setCheckable( true );
	connect( extrastereoAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleExtrastereo(bool)) );

	karaokeAct = new TAction( this, "karaoke_filter" );
	karaokeAct->setCheckable( true );
	connect( karaokeAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleKaraoke(bool)) );

	volnormAct = new TAction( this, "volnorm_filter" );
	volnormAct->setCheckable( true );
	connect( volnormAct, SIGNAL(toggled(bool)),
             core, SLOT(toggleVolnorm(bool)) );


	// Menu Subtitles
	loadSubsAct = new TAction( this, "load_subs" );
	connect( loadSubsAct, SIGNAL(triggered()),
             this, SLOT(loadSub()) );

	unloadSubsAct = new TAction( this, "unload_subs" );
	connect( unloadSubsAct, SIGNAL(triggered()),
             core, SLOT(unloadSub()) );

	decSubDelayAct = new TAction( Qt::Key_Z, this, "dec_sub_delay" );
	connect( decSubDelayAct, SIGNAL(triggered()),
             core, SLOT(decSubDelay()) );

	incSubDelayAct = new TAction( Qt::Key_X, this, "inc_sub_delay" );
	connect( incSubDelayAct, SIGNAL(triggered()),
             core, SLOT(incSubDelay()) );

	subDelayAct = new TAction( this, "sub_delay" );
	connect( subDelayAct, SIGNAL(triggered()),
             this, SLOT(showSubDelayDialog()) );

	decSubPosAct = new TAction( Qt::Key_R, this, "dec_sub_pos" );
	connect( decSubPosAct, SIGNAL(triggered()),
             core, SLOT(decSubPos()) );
	incSubPosAct = new TAction( Qt::Key_T, this, "inc_sub_pos" );
	connect( incSubPosAct, SIGNAL(triggered()),
             core, SLOT(incSubPos()) );

	decSubScaleAct = new TAction( Qt::SHIFT | Qt::Key_R, this, "dec_sub_scale" );
	connect( decSubScaleAct, SIGNAL(triggered()),
             core, SLOT(decSubScale()) );

	incSubScaleAct = new TAction( Qt::SHIFT | Qt::Key_T, this, "inc_sub_scale" );
	connect( incSubScaleAct, SIGNAL(triggered()),
             core, SLOT(incSubScale()) );
    
	decSubStepAct = new TAction( Qt::Key_G, this, "dec_sub_step" );
	connect( decSubStepAct, SIGNAL(triggered()),
             core, SLOT(decSubStep()) );

	incSubStepAct = new TAction( Qt::Key_Y, this, "inc_sub_step" );
	connect( incSubStepAct, SIGNAL(triggered()),
             core, SLOT(incSubStep()) );

	useCustomSubStyleAct = new TAction(this, "use_custom_sub_style");
	useCustomSubStyleAct->setCheckable(true);
	connect( useCustomSubStyleAct, SIGNAL(toggled(bool)), core, SLOT(changeUseCustomSubStyle(bool)) );

	useForcedSubsOnlyAct = new TAction(this, "use_forced_subs_only");
	useForcedSubsOnlyAct->setCheckable(true);
	connect( useForcedSubsOnlyAct, SIGNAL(toggled(bool)), core, SLOT(toggleForcedSubsOnly(bool)) );

#ifdef FIND_SUBTITLES
	showFindSubtitlesDialogAct = new TAction( this, "show_find_sub_dialog" );
	connect( showFindSubtitlesDialogAct, SIGNAL(triggered()), 
             this, SLOT(showFindSubtitlesDialog()) );

	openUploadSubtitlesPageAct = new TAction( this, "upload_subtitles" );		//turbos
	connect( openUploadSubtitlesPageAct, SIGNAL(triggered()),					//turbos
             this, SLOT(openUploadSubtitlesPage()) );							//turbos
#endif

	// Menu Options
	showPlaylistAct = new TAction( QKeySequence("Ctrl+L"), this, "show_playlist" );
	showPlaylistAct->setCheckable( true );
	connect( showPlaylistAct, SIGNAL(toggled(bool)),
			 this, SLOT(showPlaylist(bool)) );

	showPropertiesAct = new TAction( QKeySequence("Ctrl+I"), this, "show_file_properties" );
	connect( showPropertiesAct, SIGNAL(triggered()),
             this, SLOT(showFilePropertiesDialog()) );

	showPreferencesAct = new TAction( QKeySequence("Ctrl+P"), this, "show_preferences" );
	connect( showPreferencesAct, SIGNAL(triggered()),
             this, SLOT(showPreferencesDialog()) );

#ifdef YOUTUBE_SUPPORT
	showTubeBrowserAct = new TAction( Qt::Key_F11, this, "show_tube_browser" );
	connect( showTubeBrowserAct, SIGNAL(triggered()),
             this, SLOT(showTubeBrowser()) );
#endif

	// Submenu Logs
#ifdef LOG_MPLAYER
	showLogMplayerAct = new TAction( QKeySequence("Ctrl+M"), this, "show_mplayer_log" );
	connect( showLogMplayerAct, SIGNAL(triggered()),
             this, SLOT(showMplayerLog()) );
#endif

#ifdef LOG_SMPLAYER
	showLogSmplayerAct = new TAction( QKeySequence("Ctrl+S"), this, "show_smplayer_log" );
	connect( showLogSmplayerAct, SIGNAL(triggered()),
             this, SLOT(showLog()) );
#endif

	// Menu Help
	showFirstStepsAct = new TAction( this, "first_steps" );
	connect( showFirstStepsAct, SIGNAL(triggered()),
             this, SLOT(helpFirstSteps()) );

	showFAQAct = new TAction( this, "faq" );
	connect( showFAQAct, SIGNAL(triggered()),
             this, SLOT(helpFAQ()) );

	showCLOptionsAct = new TAction( this, "cl_options" );
	connect( showCLOptionsAct, SIGNAL(triggered()),
             this, SLOT(helpCLOptions()) );

	showCheckUpdatesAct = new TAction( this, "check_updates" );
	connect( showCheckUpdatesAct, SIGNAL(triggered()),
             this, SLOT(helpCheckUpdates()) );

#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	updateYTAct = new TAction( this, "update_youtube" );
	connect( updateYTAct, SIGNAL(triggered()),
             this, SLOT(YTUpdateScript()) );
#endif

	showConfigAct = new TAction( this, "show_config" );
	connect( showConfigAct, SIGNAL(triggered()),
             this, SLOT(helpShowConfig()) );

#ifdef REMINDER_ACTIONS
	donateAct = new TAction( this, "donate" );
	connect( donateAct, SIGNAL(triggered()),
             this, SLOT(helpDonate()) );
#endif

	aboutThisAct = new TAction( this, "about_smplayer" );
	connect( aboutThisAct, SIGNAL(triggered()),
             this, SLOT(helpAbout()) );

#ifdef SHARE_MENU
	facebookAct = new TAction (this, "facebook");
	twitterAct = new TAction (this, "twitter");
	gmailAct = new TAction (this, "gmail");
	hotmailAct = new TAction (this, "hotmail");
	yahooAct = new TAction (this, "yahoo");

	connect( facebookAct, SIGNAL(triggered()),
             this, SLOT(shareSMPlayer()) );
	connect( twitterAct, SIGNAL(triggered()),
             this, SLOT(shareSMPlayer()) );
	connect( gmailAct, SIGNAL(triggered()),
             this, SLOT(shareSMPlayer()) );
	connect( hotmailAct, SIGNAL(triggered()),
             this, SLOT(shareSMPlayer()) );
	connect( yahooAct, SIGNAL(triggered()),
             this, SLOT(shareSMPlayer()) );
#endif

	// OSD
	incOSDScaleAct = new TAction(Qt::SHIFT | Qt::Key_U, this, "inc_osd_scale");
	connect(incOSDScaleAct, SIGNAL(triggered()), core, SLOT(incOSDScale()));

	decOSDScaleAct = new TAction(Qt::SHIFT | Qt::Key_Y, this, "dec_osd_scale");
	connect(decOSDScaleAct, SIGNAL(triggered()), core, SLOT(decOSDScale()));


	// TPlaylist
	playNextAct = new TAction(Qt::Key_Greater, this, "play_next");
	playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
	connect( playNextAct, SIGNAL(triggered()), playlist, SLOT(playNext()) );

	playPrevAct = new TAction(Qt::Key_Less, this, "play_prev");
	playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
	connect( playPrevAct, SIGNAL(triggered()), playlist, SLOT(playPrev()) );

	// Pan
	moveUpAct = new TAction(Qt::ALT | Qt::Key_Up, this, "move_up");
	connect( moveUpAct, SIGNAL(triggered()), core, SLOT(panDown()) );

	moveDownAct = new TAction(Qt::ALT | Qt::Key_Down, this, "move_down");
	connect( moveDownAct, SIGNAL(triggered()), core, SLOT(panUp()) );

	moveLeftAct = new TAction(Qt::ALT | Qt::Key_Left, this, "move_left");
	connect( moveLeftAct, SIGNAL(triggered()), core, SLOT(panRight()) );

	moveRightAct = new TAction(Qt::ALT | Qt::Key_Right, this, "move_right");
	connect( moveRightAct, SIGNAL(triggered()), core, SLOT(panLeft()) );

	// Zoom
	incZoomAct = new TAction(Qt::Key_E, this, "inc_zoom");
	connect( incZoomAct, SIGNAL(triggered()), core, SLOT(incZoom()) );

	decZoomAct = new TAction(Qt::Key_W, this, "dec_zoom");
	connect( decZoomAct, SIGNAL(triggered()), core, SLOT(decZoom()) );

	resetZoomAct = new TAction(Qt::SHIFT | Qt::Key_E, this, "reset_zoom");
	connect( resetZoomAct, SIGNAL(triggered()), core, SLOT(resetZoomAndPan()) );

	autoZoomAct = new TAction(Qt::SHIFT | Qt::Key_W, this, "auto_zoom");
	connect( autoZoomAct, SIGNAL(triggered()), core, SLOT(autoZoom()) );

	autoZoom169Act = new TAction(Qt::SHIFT | Qt::Key_A, this, "zoom_169");
	connect( autoZoom169Act, SIGNAL(triggered()), core, SLOT(autoZoomFor169()) );

	autoZoom235Act = new TAction(Qt::SHIFT | Qt::Key_S, this, "zoom_235");
	connect( autoZoom235Act, SIGNAL(triggered()), core, SLOT(autoZoomFor235()) );


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act = new TAction( Qt::Key_Slash, this, "dec_volume2" );
	connect( decVolume2Act, SIGNAL(triggered()), core, SLOT(decVolume()) );

	incVolume2Act = new TAction( Qt::Key_Asterisk, this, "inc_volume2" );
	connect( incVolume2Act, SIGNAL(triggered()), core, SLOT(incVolume()) );
#endif
	// Exit fullscreen
	exitFullscreenAct = new TAction( Qt::Key_Escape, this, "exit_fullscreen" );
	connect( exitFullscreenAct, SIGNAL(triggered()), this, SLOT(exitFullscreen()) );

	nextOSDLevelAct = new TAction( Qt::Key_O, this, "next_osd");
	connect( nextOSDLevelAct, SIGNAL(triggered()), core, SLOT(nextOSDLevel()) );

	decContrastAct = new TAction( Qt::Key_1, this, "dec_contrast");
	connect( decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()) );

	incContrastAct = new TAction( Qt::Key_2, this, "inc_contrast");
	connect( incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()) );

	decBrightnessAct = new TAction( Qt::Key_3, this, "dec_brightness");
	connect( decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()) );

	incBrightnessAct = new TAction( Qt::Key_4, this, "inc_brightness");
	connect( incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()) );

	decHueAct = new TAction(Qt::Key_5, this, "dec_hue");
	connect( decHueAct, SIGNAL(triggered()), core, SLOT(decHue()) );

	incHueAct = new TAction( Qt::Key_6, this, "inc_hue");
	connect( incHueAct, SIGNAL(triggered()), core, SLOT(incHue()) );

	decSaturationAct = new TAction( Qt::Key_7, this, "dec_saturation");
	connect( decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()) );

	incSaturationAct = new TAction( Qt::Key_8, this, "inc_saturation");
	connect( incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()) );

	decGammaAct = new TAction( this, "dec_gamma");
	connect( decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()) );

	incGammaAct = new TAction( this, "inc_gamma");
	connect( incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()) );

	nextVideoAct = new TAction( this, "next_video");
	connect( nextVideoAct, SIGNAL(triggered()), core, SLOT(nextVideoTrack()) );

	nextAudioAct = new TAction( Qt::Key_K, this, "next_audio");
	connect( nextAudioAct, SIGNAL(triggered()), core, SLOT(nextAudioTrack()) );

	nextSubtitleAct = new TAction( Qt::Key_J, this, "next_subtitle");
	connect( nextSubtitleAct, SIGNAL(triggered()), core, SLOT(nextSubtitle()) );

	nextChapterAct = new TAction( Qt::Key_At, this, "next_chapter");
	connect( nextChapterAct, SIGNAL(triggered()), core, SLOT(nextChapter()) );

	prevChapterAct = new TAction( Qt::Key_Exclam, this, "prev_chapter");
	connect( prevChapterAct, SIGNAL(triggered()), core, SLOT(prevChapter()) );

	doubleSizeAct = new TAction( Qt::CTRL | Qt::Key_D, this, "toggle_double_size");
	connect( doubleSizeAct, SIGNAL(triggered()), this, SLOT(toggleDoubleSize()) );

	resetVideoEqualizerAct = new TAction( this, "reset_video_equalizer");
	connect( resetVideoEqualizerAct, SIGNAL(triggered()), video_equalizer, SLOT(reset()) );

	resetAudioEqualizerAct = new TAction( this, "reset_audio_equalizer");
	connect( resetAudioEqualizerAct, SIGNAL(triggered()), audio_equalizer, SLOT(reset()) );

	showContextMenuAct = new TAction( this, "show_context_menu");
	connect( showContextMenuAct, SIGNAL(triggered()), 
             this, SLOT(showPopupMenu()) );

	nextAspectAct = new TAction( Qt::Key_A, this, "next_aspect");
	connect( nextAspectAct, SIGNAL(triggered()), 
             core, SLOT(nextAspectRatio()) );

	nextWheelFunctionAct = new TAction(this, "next_wheel_function");
	connect( nextWheelFunctionAct, SIGNAL(triggered()),
			 core, SLOT(nextWheelFunction()) );

	showFilenameAct = new TAction(Qt::SHIFT | Qt::Key_I, this, "show_filename");
	connect( showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()) );

	showTimeAct = new TAction(Qt::Key_I, this, "show_time");
	connect( showTimeAct, SIGNAL(triggered()), core, SLOT(showTimeOnOSD()) );

	toggleDeinterlaceAct = new TAction(Qt::Key_D, this, "toggle_deinterlacing");
	connect( toggleDeinterlaceAct, SIGNAL(triggered()), core, SLOT(toggleDeinterlace()) );


	// Group actions

	// OSD
	osdGroup = new TActionGroup(this);
	osdNoneAct = new TActionGroupItem(this, osdGroup, "osd_none", Preferences::None);
	osdSeekAct = new TActionGroupItem(this, osdGroup, "osd_seek", Preferences::Seek);
	osdTimerAct = new TActionGroupItem(this, osdGroup, "osd_timer", Preferences::SeekTimer);
	osdTotalAct = new TActionGroupItem(this, osdGroup, "osd_total", Preferences::SeekTimerTotal);
	connect( osdGroup, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)) );

	// Denoise
	denoiseGroup = new TActionGroup(this);
	denoiseNoneAct = new TActionGroupItem(this, denoiseGroup, "denoise_none", MediaSettings::NoDenoise);
	denoiseNormalAct = new TActionGroupItem(this, denoiseGroup, "denoise_normal", MediaSettings::DenoiseNormal);
	denoiseSoftAct = new TActionGroupItem(this, denoiseGroup, "denoise_soft", MediaSettings::DenoiseSoft);
	connect( denoiseGroup, SIGNAL(activated(int)), core, SLOT(changeDenoise(int)) );

	// Unsharp group
	unsharpGroup = new TActionGroup(this);
	unsharpNoneAct = new TActionGroupItem(this, unsharpGroup, "unsharp_off", 0);
	blurAct = new TActionGroupItem(this, unsharpGroup, "blur", 1);
	sharpenAct = new TActionGroupItem(this, unsharpGroup, "sharpen", 2);
	connect( unsharpGroup, SIGNAL(activated(int)), core, SLOT(changeUnsharp(int)) );

	// Video size
	sizeGroup = new TActionGroup(this);
	size50 = new TActionGroupItem(this, sizeGroup, "5&0%", "size_50", 50);
	size75 = new TActionGroupItem(this, sizeGroup, "7&5%", "size_75", 75);
	size100 = new TActionGroupItem(this, sizeGroup, "&100%", "size_100", 100);
	size125 = new TActionGroupItem(this, sizeGroup, "1&25%", "size_125", 125);
	size150 = new TActionGroupItem(this, sizeGroup, "15&0%", "size_150", 150);
	size175 = new TActionGroupItem(this, sizeGroup, "1&75%", "size_175", 175);
	size200 = new TActionGroupItem(this, sizeGroup, "&200%", "size_200", 200);
	size300 = new TActionGroupItem(this, sizeGroup, "&300%", "size_300", 300);
	size400 = new TActionGroupItem(this, sizeGroup, "&400%", "size_400", 400);
	size100->setShortcut( Qt::CTRL | Qt::Key_1 );
	size200->setShortcut( Qt::CTRL | Qt::Key_2 );
	connect( sizeGroup, SIGNAL(activated(int)), core, SLOT(changeSize(int)) );
	// mplayerwindow updates group when size changed
	mplayerwindow->setSizeGroup(sizeGroup);

	// Deinterlace
	deinterlaceGroup = new TActionGroup(this);
	deinterlaceNoneAct = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_none", MediaSettings::NoDeinterlace);
	deinterlaceL5Act = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_l5", MediaSettings::L5);
	deinterlaceYadif0Act = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif0", MediaSettings::Yadif);
	deinterlaceYadif1Act = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_yadif1", MediaSettings::Yadif_1);
	deinterlaceLBAct = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_lb", MediaSettings::LB);
	deinterlaceKernAct = new TActionGroupItem(this, deinterlaceGroup, "deinterlace_kern", MediaSettings::Kerndeint);
	connect( deinterlaceGroup, SIGNAL(activated(int)),
             core, SLOT(changeDeinterlace(int)) );

	// Audio channels
	channelsGroup = new TActionGroup(this);
	/* channelsDefaultAct = new TActionGroupItem(this, channelsGroup, "channels_default", MediaSettings::ChDefault); */
	channelsStereoAct = new TActionGroupItem(this, channelsGroup, "channels_stereo", MediaSettings::ChStereo);
	channelsSurroundAct = new TActionGroupItem(this, channelsGroup, "channels_surround", MediaSettings::ChSurround);
	channelsFull51Act = new TActionGroupItem(this, channelsGroup, "channels_ful51", MediaSettings::ChFull51);
	channelsFull61Act = new TActionGroupItem(this, channelsGroup, "channels_ful61", MediaSettings::ChFull61);
	channelsFull71Act = new TActionGroupItem(this, channelsGroup, "channels_ful71", MediaSettings::ChFull71);
	connect( channelsGroup, SIGNAL(activated(int)),
             core, SLOT(setAudioChannels(int)) );

	// Stereo mode
	stereoGroup = new TActionGroup(this);
	stereoAct = new TActionGroupItem(this, stereoGroup, "stereo", MediaSettings::Stereo);
	leftChannelAct = new TActionGroupItem(this, stereoGroup, "left_channel", MediaSettings::Left);
	rightChannelAct = new TActionGroupItem(this, stereoGroup, "right_channel", MediaSettings::Right);
	monoAct = new TActionGroupItem(this, stereoGroup, "mono", MediaSettings::Mono);
	reverseAct = new TActionGroupItem(this, stereoGroup, "reverse_channels", MediaSettings::Reverse);
	connect( stereoGroup, SIGNAL(activated(int)),
             core, SLOT(setStereoMode(int)) );

	// Video aspect
	aspectGroup = new TActionGroup(this);
	aspectDetectAct = new TActionGroupItem(this, aspectGroup, "aspect_detect", MediaSettings::AspectAuto);
	aspect11Act = new TActionGroupItem(this, aspectGroup, "aspect_1:1", MediaSettings::Aspect11 );
	aspect54Act = new TActionGroupItem(this, aspectGroup, "aspect_5:4", MediaSettings::Aspect54 );
	aspect43Act = new TActionGroupItem(this, aspectGroup, "aspect_4:3", MediaSettings::Aspect43);
	aspect118Act = new TActionGroupItem(this, aspectGroup, "aspect_11:8", MediaSettings::Aspect118 );
	aspect1410Act = new TActionGroupItem(this, aspectGroup, "aspect_14:10", MediaSettings::Aspect1410 );
	aspect32Act = new TActionGroupItem(this, aspectGroup, "aspect_3:2", MediaSettings::Aspect32);
	aspect149Act = new TActionGroupItem(this, aspectGroup, "aspect_14:9", MediaSettings::Aspect149 );
	aspect1610Act = new TActionGroupItem(this, aspectGroup, "aspect_16:10", MediaSettings::Aspect1610 );
	aspect169Act = new TActionGroupItem(this, aspectGroup, "aspect_16:9", MediaSettings::Aspect169 );
	aspect235Act = new TActionGroupItem(this, aspectGroup, "aspect_2.35:1", MediaSettings::Aspect235 );
	{
		QAction * sep = new QAction(aspectGroup);
		sep->setSeparator(true);
	}
	aspectNoneAct = new TActionGroupItem(this, aspectGroup, "aspect_none", MediaSettings::AspectNone);

	connect( aspectGroup, SIGNAL(activated(int)),
             core, SLOT(changeAspectRatio(int)) );

	// Rotate
	rotateGroup = new TActionGroup(this);
	rotateNoneAct = new TActionGroupItem(this, rotateGroup, "rotate_none", MediaSettings::NoRotate);
	rotateClockwiseFlipAct = new TActionGroupItem(this, rotateGroup, "rotate_clockwise_flip", MediaSettings::Clockwise_flip);
	rotateClockwiseAct = new TActionGroupItem(this, rotateGroup, "rotate_clockwise", MediaSettings::Clockwise);
	rotateCounterclockwiseAct = new TActionGroupItem(this, rotateGroup, "rotate_counterclockwise", MediaSettings::Counterclockwise);
	rotateCounterclockwiseFlipAct = new TActionGroupItem(this, rotateGroup, "rotate_counterclockwise_flip", MediaSettings::Counterclockwise_flip);
	connect( rotateGroup, SIGNAL(activated(int)),
             core, SLOT(changeRotate(int)) );

	// On Top
	onTopActionGroup = new TActionGroup(this);
	onTopAlwaysAct = new TActionGroupItem( this,onTopActionGroup,"on_top_always",Preferences::AlwaysOnTop);
	onTopNeverAct = new TActionGroupItem( this,onTopActionGroup,"on_top_never",Preferences::NeverOnTop);
	onTopWhilePlayingAct = new TActionGroupItem( this,onTopActionGroup,"on_top_playing",Preferences::WhilePlayingOnTop);
	connect( onTopActionGroup , SIGNAL(activated(int)),
             this, SLOT(changeStayOnTop(int)) );

	toggleStayOnTopAct = new TAction( this, "toggle_stay_on_top");
	connect( toggleStayOnTopAct, SIGNAL(triggered()), this, SLOT(toggleStayOnTop()) );


#if USE_ADAPTER
	screenGroup = new TActionGroup(this);
	screenDefaultAct = new TActionGroupItem(this, screenGroup, "screen_default", -1);
	#ifdef Q_OS_WIN
	DeviceList display_devices = DeviceInfo::displayDevices();
	if (!display_devices.isEmpty()) {
		for (int n = 0; n < display_devices.count(); n++) {
			int id = display_devices[n].ID().toInt();
			QString desc = display_devices[n].desc();
			TAction * screen_item = new TActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toLatin1().constData(), id);
			screen_item->change( "&"+QString::number(n) + " - " + desc);
		}
	}
	else
	#endif // Q_OS_WIN
	for (int n = 1; n <= 4; n++) {
		TAction * screen_item = new TActionGroupItem(this, screenGroup, QString("screen_%1").arg(n).toLatin1().constData(), n);
		screen_item->change( "&"+QString::number(n) );
	}

	connect( screenGroup, SIGNAL(activated(int)),
             core, SLOT(changeAdapter(int)) );
#endif

#if PROGRAM_SWITCH
	// Program track
	programTrackGroup = new TActionGroup(this);
	connect( programTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeProgram(int)) );
#endif

	// Video track
	videoTrackGroup = new TActionGroup(this);
	connect( videoTrackGroup, SIGNAL(activated(int)),
			 core, SLOT(changeVideoTrack(int)) );
	connect( core, SIGNAL(videoTrackInfoChanged()),
			 this, SLOT(updateVideoTracks()) );
	connect( core, SIGNAL(videoTrackChanged(int)),
			 videoTrackGroup, SLOT(setCheckedSlot(int)) );

	// Audio track
	audioTrackGroup = new TActionGroup(this);
	connect( audioTrackGroup, SIGNAL(activated(int)),
			 core, SLOT(changeAudioTrack(int)) );
	connect( core, SIGNAL(audioTrackInfoChanged()),
			 this, SLOT(updateAudioTracks()) );
	connect( core, SIGNAL(audioTrackChanged(int)),
			 audioTrackGroup, SLOT(setCheckedSlot(int)) );

	subtitleTrackGroup = new TActionGroup(this);
	connect( subtitleTrackGroup, SIGNAL(activated(int)),
			 core, SLOT(changeSubtitle(int)) );
	connect( core, SIGNAL(subtitleInfoChanged()),
			 this, SLOT(updateSubtitles()) );
	connect( core, SIGNAL(subtitleTrackChanged(int)),
			 subtitleTrackGroup, SLOT(setCheckedSlot(int)) );

#ifdef MPV_SUPPORT
	// Secondary subtitle track
	secondarySubtitleTrackGroup = new TActionGroup(this);
	connect( secondarySubtitleTrackGroup, SIGNAL(activated(int)), 
	         core, SLOT(changeSecondarySubtitle(int)) );
	// InfoChanged already connected by subtitleTrackGroup
	// checked not needed
	// connect( core, SIGNAL(secondarySubtitleTrackChanged(int)),
	//		 secondarySubtitleTrackGroup, SLOT(setCheckedSlot(int)) );
#endif

	// Titles
	titleGroup = new TActionGroup(this);
	connect( titleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeTitle(int)) );
	connect( core, SIGNAL(titleTrackChanged(int)),
			 titleGroup, SLOT(setCheckedSlot(int)));
	connect( core, SIGNAL(titleTrackInfoChanged()),
			 this, SLOT(updateTitles()));

	// Chapters
	chapterGroup = new TActionGroup(this);
	connect( chapterGroup, SIGNAL(activated(int)),
			 core, SLOT(changeChapter(int)) );
	connect( core, SIGNAL(chapterChanged(int)),
			 chapterGroup, SLOT(setCheckedSlot(int)));
	// Update chapter info done by updateTitles.
	// DVDNAV only:
	connect( core, SIGNAL(chapterInfoChanged()),
			 this, SLOT(updateChapters()));

	// Angles
	angleGroup = new TActionGroup(this);
	connect( angleGroup, SIGNAL(activated(int)),
			 core, SLOT(changeAngle(int)) );
	// Update done by updateTitles


	ccGroup = new TActionGroup(this);
	ccNoneAct = new TActionGroupItem(this, ccGroup, "cc_none", 0);
	ccChannel1Act = new TActionGroupItem(this, ccGroup, "cc_ch_1", 1);
	ccChannel2Act = new TActionGroupItem(this, ccGroup, "cc_ch_2", 2);
	ccChannel3Act = new TActionGroupItem(this, ccGroup, "cc_ch_3", 3);
	ccChannel4Act = new TActionGroupItem(this, ccGroup, "cc_ch_4", 4);
	connect( ccGroup, SIGNAL(activated(int)),
             core, SLOT(changeClosedCaptionChannel(int)) );

	subFPSGroup = new TActionGroup(this);
	subFPSNoneAct = new TActionGroupItem(this, subFPSGroup, "sub_fps_none", MediaSettings::SFPS_None);
	/* subFPS23Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_23", MediaSettings::SFPS_23); */
	subFPS23976Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_23976", MediaSettings::SFPS_23976);
	subFPS24Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_24", MediaSettings::SFPS_24);
	subFPS25Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_25", MediaSettings::SFPS_25);
	subFPS29970Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_29970", MediaSettings::SFPS_29970);
	subFPS30Act = new TActionGroupItem(this, subFPSGroup, "sub_fps_30", MediaSettings::SFPS_30);
	connect( subFPSGroup, SIGNAL(activated(int)),
             core, SLOT(changeExternalSubFPS(int)) );


	dvdnavUpAct = new TAction(Qt::SHIFT | Qt::Key_Up, this, "dvdnav_up");
	connect( dvdnavUpAct, SIGNAL(triggered()), core, SLOT(dvdnavUp()) );

	dvdnavDownAct = new TAction(Qt::SHIFT | Qt::Key_Down, this, "dvdnav_down");
	connect( dvdnavDownAct, SIGNAL(triggered()), core, SLOT(dvdnavDown()) );

	dvdnavLeftAct = new TAction(Qt::SHIFT | Qt::Key_Left, this, "dvdnav_left");
	connect( dvdnavLeftAct, SIGNAL(triggered()), core, SLOT(dvdnavLeft()) );

	dvdnavRightAct = new TAction(Qt::SHIFT | Qt::Key_Right, this, "dvdnav_right");
	connect( dvdnavRightAct, SIGNAL(triggered()), core, SLOT(dvdnavRight()) );

	dvdnavMenuAct = new TAction(Qt::SHIFT | Qt::Key_Return, this, "dvdnav_menu");
	connect( dvdnavMenuAct, SIGNAL(triggered()), core, SLOT(dvdnavMenu()) );

	dvdnavSelectAct = new TAction(Qt::Key_Return, this, "dvdnav_select");
	connect( dvdnavSelectAct, SIGNAL(triggered()), core, SLOT(dvdnavSelect()) );

	dvdnavPrevAct = new TAction(Qt::SHIFT | Qt::Key_Escape, this, "dvdnav_prev");
	connect( dvdnavPrevAct, SIGNAL(triggered()), core, SLOT(dvdnavPrev()) );

	dvdnavMouseAct = new TAction( this, "dvdnav_mouse");
	connect( dvdnavMouseAct, SIGNAL(triggered()), core, SLOT(dvdnavMouse()) );
}

#if AUTODISABLE_ACTIONS
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
	//repeatAct->setEnabled(b);
	gotoAct->setEnabled(b);

	// Menu Speed
	normalSpeedAct->setEnabled(b);
	halveSpeedAct->setEnabled(b);
	doubleSpeedAct->setEnabled(b);
	decSpeed10Act->setEnabled(b);
	incSpeed10Act->setEnabled(b);
	decSpeed4Act->setEnabled(b);
	incSpeed4Act->setEnabled(b);
	decSpeed1Act->setEnabled(b);
	incSpeed1Act->setEnabled(b);

	// Menu Video
	videoEqualizerAct->setEnabled(b);
	screenshotAct->setEnabled(b);
	screenshotsAct->setEnabled(b);
	flipAct->setEnabled(b);
	mirrorAct->setEnabled(b);
	stereo3dAct->setEnabled(b);
	postProcessingAct->setEnabled(b);
	phaseAct->setEnabled(b);
	deblockAct->setEnabled(b);
	deringAct->setEnabled(b);
	gradfunAct->setEnabled(b);
	addNoiseAct->setEnabled(b);
	addLetterboxAct->setEnabled(b);
	upscaleAct->setEnabled(b);

	// Menu Audio
	audioEqualizerAct->setEnabled(b);
	muteAct->setEnabled(b);
	decVolumeAct->setEnabled(b);
	incVolumeAct->setEnabled(b);
	decAudioDelayAct->setEnabled(b);
	incAudioDelayAct->setEnabled(b);
	audioDelayAct->setEnabled(b);
	extrastereoAct->setEnabled(b);
	karaokeAct->setEnabled(b);
	volnormAct->setEnabled(b);
	loadAudioAct->setEnabled(b);
	//unloadAudioAct->setEnabled(b);

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

	// Actions not in menus
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act->setEnabled(b);
	incVolume2Act->setEnabled(b);
#endif
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
	doubleSizeAct->setEnabled(b);

	// Moving and zoom
	moveUpAct->setEnabled(b);
	moveDownAct->setEnabled(b);
	moveLeftAct->setEnabled(b);
	moveRightAct->setEnabled(b);
	incZoomAct->setEnabled(b);
	decZoomAct->setEnabled(b);
	resetZoomAct->setEnabled(b);
	autoZoomAct->setEnabled(b);
	autoZoom169Act->setEnabled(b);
	autoZoom235Act->setEnabled(b);

	dvdnavUpAct->setEnabled(b);
	dvdnavDownAct->setEnabled(b);
	dvdnavLeftAct->setEnabled(b);
	dvdnavRightAct->setEnabled(b);
	dvdnavMenuAct->setEnabled(b);
	dvdnavSelectAct->setEnabled(b);
	dvdnavPrevAct->setEnabled(b);
	dvdnavMouseAct->setEnabled(b);

	// Groups
	denoiseGroup->setActionsEnabled(b);
	unsharpGroup->setActionsEnabled(b);
	// sizeGroup handled by mplayerwindow
	deinterlaceGroup->setActionsEnabled(b);
	aspectGroup->setActionsEnabled(b);
	rotateGroup->setActionsEnabled(b);
#if USE_ADAPTER
	screenGroup->setActionsEnabled(b);
#endif
	channelsGroup->setActionsEnabled(b);
	stereoGroup->setActionsEnabled(b);
}

void TBase::enableActionsOnPlaying() {
	qDebug("Gui::TBase::enableActionsOnPlaying");

	setActionsEnabled(true);

	playAct->setEnabled(false);

	// Screenshot option
	bool screenshots_enabled = ( (pref->use_screenshot) && 
                                 (!pref->screenshot_directory.isEmpty()) &&
                                 (QFileInfo(pref->screenshot_directory).isDir()) );

	screenshotAct->setEnabled( screenshots_enabled );
	screenshotsAct->setEnabled( screenshots_enabled );

	// Disable the compact action if not using video window
	compactAct->setEnabled( panel->isVisible() );

	// Enable or disable the audio equalizer
	audioEqualizerAct->setEnabled(pref->use_audio_equalizer);

	// Disable audio actions if there's not audio track
	if ((core->mdat.audios.count() == 0) && (core->mset.external_audio.isEmpty())) {
		audioEqualizerAct->setEnabled(false);
		muteAct->setEnabled(false);
		decVolumeAct->setEnabled(false);
		incVolumeAct->setEnabled(false);
		decAudioDelayAct->setEnabled(false);
		incAudioDelayAct->setEnabled(false);
		audioDelayAct->setEnabled(false);
		extrastereoAct->setEnabled(false);
		karaokeAct->setEnabled(false);
		volnormAct->setEnabled(false);
		channelsGroup->setActionsEnabled(false);
		stereoGroup->setActionsEnabled(false);
	}

	// Disable video actions if it's an audio file
	if (core->mdat.noVideo()) {
		videoEqualizerAct->setEnabled(false);
		screenshotAct->setEnabled(false);
		screenshotsAct->setEnabled(false);
		flipAct->setEnabled(false);
		mirrorAct->setEnabled(false);
		stereo3dAct->setEnabled(false);
		postProcessingAct->setEnabled(false);
		phaseAct->setEnabled(false);
		deblockAct->setEnabled(false);
		deringAct->setEnabled(false);
		gradfunAct->setEnabled(false);
		addNoiseAct->setEnabled(false);
		addLetterboxAct->setEnabled(false);
		upscaleAct->setEnabled(false);
		doubleSizeAct->setEnabled(false);

		// Moving and zoom
		moveUpAct->setEnabled(false);
		moveDownAct->setEnabled(false);
		moveLeftAct->setEnabled(false);
		moveRightAct->setEnabled(false);
		incZoomAct->setEnabled(false);
		decZoomAct->setEnabled(false);
		resetZoomAct->setEnabled(false);
		autoZoomAct->setEnabled(false);
		autoZoom169Act->setEnabled(false);
		autoZoom235Act->setEnabled(false);

		denoiseGroup->setActionsEnabled(false);
		unsharpGroup->setActionsEnabled(false);
		// sizeGroup handled by mplayerwindow
		deinterlaceGroup->setActionsEnabled(false);
		aspectGroup->setActionsEnabled(false);
		rotateGroup->setActionsEnabled(false);
#if USE_ADAPTER
		screenGroup->setActionsEnabled(false);
#endif
	}

#if USE_ADAPTER
	screenGroup->setActionsEnabled(pref->vo.startsWith(OVERLAY_VO));
#endif

#ifndef Q_OS_WIN
	// Disable video filters if using vdpau
	if ((pref->vdpau.disable_video_filters) && (pref->vo.startsWith("vdpau"))) {
		screenshotAct->setEnabled(false);
		screenshotsAct->setEnabled(false);
		flipAct->setEnabled(false);
		mirrorAct->setEnabled(false);
		stereo3dAct->setEnabled(false);
		postProcessingAct->setEnabled(false);
		phaseAct->setEnabled(false);
		deblockAct->setEnabled(false);
		deringAct->setEnabled(false);
		gradfunAct->setEnabled(false);
		addNoiseAct->setEnabled(false);
		addLetterboxAct->setEnabled(false);
		upscaleAct->setEnabled(false);

		deinterlaceGroup->setActionsEnabled(false);
		rotateGroup->setActionsEnabled(false);
		denoiseGroup->setActionsEnabled(false);
		unsharpGroup->setActionsEnabled(false);

		displayMessage( tr("Video filters are disabled when using vdpau") );
	}
#endif

	if (!core->mdat.detected_type == MediaData::TYPE_DVDNAV) {
		dvdnavUpAct->setEnabled(false);
		dvdnavDownAct->setEnabled(false);
		dvdnavLeftAct->setEnabled(false);
		dvdnavRightAct->setEnabled(false);
		dvdnavMenuAct->setEnabled(false);
		dvdnavSelectAct->setEnabled(false);
		dvdnavPrevAct->setEnabled(false);
		dvdnavMouseAct->setEnabled(false);
	}
}

void TBase::disableActionsOnStop() {
	qDebug("Gui::TBase::disableActionsOnStop");

	setActionsEnabled(false);

	playAct->setEnabled(true);
	playOrPauseAct->setEnabled(true);
	stopAct->setEnabled(true);
}
#endif // AUTODISABLE_ACTIONS

void TBase::togglePlayAction(Core::State state) {
	qDebug("Gui::TBase::togglePlayAction");

#if AUTODISABLE_ACTIONS
	if (state == Core::Playing)
		playAct->setEnabled(false);
	else
		playAct->setEnabled(true);
#endif
}

void TBase::retranslateStrings() {
	qDebug("Gui::TBase::retranslateStrings");

	setWindowIcon( Images::icon("logo", 64) );

	// ACTIONS

	// Menu File
	openFileAct->change( Images::icon("open"), tr("&File...") );
	openDirectoryAct->change( Images::icon("openfolder"), tr("D&irectory...") );
	openTPlaylistAct->change( Images::icon("open_playlist"), tr("&TPlaylist...") );
	openVCDAct->change( Images::icon("vcd"), tr("V&CD") );
	openAudioCDAct->change( Images::icon("cdda"), tr("&Audio CD") );
	openDVDAct->change( Images::icon("dvd"), tr("&DVD from drive") );
	openDVDFolderAct->change( Images::icon("dvd_hd"), tr("D&VD from folder...") );
	openBluRayAct->change( Images::icon("bluray"), tr("&Blu-ray from drive") );
	openBluRayFolderAct->change( Images::icon("bluray_hd"), tr("Blu-&ray from folder...") );
	openURLAct->change( Images::icon("url"), tr("&URL...") );
	exitAct->change( Images::icon("close"), tr("C&lose") );

	// Favorites
	/*
	favorites->editAct()->setText( tr("&Edit...") );
	favorites->addCurrentAct()->setText( tr("&Add current media") );
	*/

	// TV & Radio submenus
	/*
	tvlist->editAct()->setText( tr("&Edit...") );
	radiolist->editAct()->setText( tr("&Edit...") );
	tvlist->addCurrentAct()->setText( tr("&Add current media") );
	radiolist->addCurrentAct()->setText( tr("&Add current media") );
	tvlist->jumpAct()->setText( tr("&Jump...") );
	radiolist->jumpAct()->setText( tr("&Jump...") );
	tvlist->nextAct()->setText( tr("Next TV channel") );
	tvlist->previousAct()->setText( tr("Previous TV channel") );
	radiolist->nextAct()->setText( tr("Next radio channel") );
	radiolist->previousAct()->setText( tr("Previous radio channel") );
	*/

	// Menu Play
	playAct->change( tr("P&lay") );
	playAct->setIcon( Images::icon("play") );

	pauseAct->change( Images::icon("pause"), tr("&Pause"));
	stopAct->change( Images::icon("stop"), tr("&Stop") );
	frameStepAct->change( Images::icon("frame_step"), tr("&Frame step") );
	frameBackStepAct->change( Images::icon("frame_back_step"), tr("Fra&me back step") );

	playOrPauseAct->change( tr("Play / Pause") );
	playOrPauseAct->setIcon( Images::icon("play_pause") );

	setJumpTexts(); // Texts for rewind*Act and forward*Act

	// Submenu A-B
	setAMarkerAct->change( Images::icon("a_marker"), tr("Set &A marker") );
	setBMarkerAct->change( Images::icon("b_marker"), tr("Set &B marker") );
	clearABMarkersAct->change( Images::icon("clear_markers"), tr("&Clear A-B markers") );
	repeatAct->change( Images::icon("repeat"), tr("&Repeat") );

	gotoAct->change( Images::icon("jumpto"), tr("&Jump to...") );

	// Submenu speed
	normalSpeedAct->change( tr("&Normal speed") );
	halveSpeedAct->change( tr("&Half speed") );
	doubleSpeedAct->change( tr("&Double speed") );
	decSpeed10Act->change( tr("Speed &-10%") );
	incSpeed10Act->change( tr("Speed &+10%") );
	decSpeed4Act->change( tr("Speed -&4%") );
	incSpeed4Act->change( tr("&Speed +4%") );
	decSpeed1Act->change( tr("Speed -&1%") );
	incSpeed1Act->change( tr("S&peed +1%") );

	// Menu Video
	fullscreenAct->change( Images::icon("fullscreen"), tr("&Fullscreen") );
	compactAct->change( Images::icon("compact"), tr("&Compact mode") );
	videoEqualizerAct->change( Images::icon("equalizer"), tr("&Equalizer") );
	screenshotAct->change( Images::icon("screenshot"), tr("&Screenshot") );
	screenshotsAct->change( Images::icon("screenshots"), tr("Start/stop takin&g screenshots") );
#ifdef VIDEOPREVIEW
	videoPreviewAct->change( Images::icon("video_preview"), tr("Thumb&nail Generator...") );
#endif
	flipAct->change( Images::icon("flip"), tr("Fli&p image") );
	mirrorAct->change( Images::icon("mirror"), tr("Mirr&or image") );
	stereo3dAct->change( Images::icon("stereo3d"), tr("Stereo &3D filter") );

	decZoomAct->change( tr("Zoom &-") );
	incZoomAct->change( tr("Zoom &+") );
	resetZoomAct->change( tr("&Reset") );
	autoZoomAct->change( tr("&Auto zoom") );
	autoZoom169Act->change( tr("Zoom for &16:9") );
	autoZoom235Act->change( tr("Zoom for &2.35:1") );
	moveLeftAct->change( tr("Move &left") );
	moveRightAct->change( tr("Move &right") );
	moveUpAct->change( tr("Move &up") );
	moveDownAct->change( tr("Move &down") );

	// Submenu Filters
	postProcessingAct->change( tr("&Postprocessing") );
	phaseAct->change( tr("&Autodetect phase") );
	deblockAct->change( tr("&Deblock") );
	deringAct->change( tr("De&ring") );
	gradfunAct->change( tr("Debanding (&gradfun)") );
	addNoiseAct->change( tr("Add n&oise") );
	addLetterboxAct->change( Images::icon("letterbox"), tr("Add &black borders") );
	upscaleAct->change( Images::icon("upscaling"), tr("Soft&ware scaling") );

	// Menu Audio
	audioEqualizerAct->change( Images::icon("audio_equalizer"), tr("E&qualizer") );
	QIcon icset( Images::icon("volume") );
	icset.addPixmap( Images::icon("mute"), QIcon::Normal, QIcon::On  );
	muteAct->change( icset, tr("&Mute") );
	decVolumeAct->change( Images::icon("audio_down"), tr("Volume &-") );
	incVolumeAct->change( Images::icon("audio_up"), tr("Volume &+") );
	decAudioDelayAct->change( Images::icon("delay_down"), tr("&Delay -") );
	incAudioDelayAct->change( Images::icon("delay_up"), tr("D&elay +") );
	audioDelayAct->change( Images::icon("audio_delay"), tr("Set dela&y...") );
	loadAudioAct->change( Images::icon("open"), tr("&Load external file...") );
	unloadAudioAct->change( Images::icon("unload"), tr("U&nload") );

	// Submenu Filters
	extrastereoAct->change( tr("&Extrastereo") );
	karaokeAct->change( tr("&Karaoke") );
	volnormAct->change( tr("Volume &normalization") );

	// Menu Subtitles
	loadSubsAct->change( Images::icon("open"), tr("&Load...") );
	unloadSubsAct->change( Images::icon("unload"), tr("U&nload") );
	decSubDelayAct->change( Images::icon("delay_down"), tr("Delay &-") );
	incSubDelayAct->change( Images::icon("delay_up"), tr("Delay &+") );
	subDelayAct->change( Images::icon("sub_delay"), tr("Se&t delay...") );
	decSubPosAct->change( Images::icon("sub_up"), tr("&Up") );
	incSubPosAct->change( Images::icon("sub_down"), tr("&Down") );
	decSubScaleAct->change( Images::icon("dec_sub_scale"), tr("S&ize -") );
	incSubScaleAct->change( Images::icon("inc_sub_scale"), tr("Si&ze +") );
	decSubStepAct->change( Images::icon("dec_sub_step"), 
                           tr("&Previous line in subtitles") );
	incSubStepAct->change( Images::icon("inc_sub_step"), 
                           tr("N&ext line in subtitles") );
	useCustomSubStyleAct->change( Images::icon("use_custom_sub_style"), tr("Use custo&m style") );
	useForcedSubsOnlyAct->change( Images::icon("forced_subs"), tr("&Forced subtitles only") );

#ifdef FIND_SUBTITLES
	showFindSubtitlesDialogAct->change( Images::icon("download_subs"), tr("Find subtitles at &OpenSubtitles.org...") );
	openUploadSubtitlesPageAct->change( Images::icon("upload_subs"), tr("Upload su&btitles to OpenSubtitles.org...") );
#endif

	ccNoneAct->change( tr("&Off", "closed captions menu") );
	ccChannel1Act->change( "&1" );
	ccChannel2Act->change( "&2" );
	ccChannel3Act->change( "&3" );
	ccChannel4Act->change( "&4" );

	subFPSNoneAct->change( tr("&Default", "subfps menu") );
	/* subFPS23Act->change( "2&3" ); */
	subFPS23976Act->change( "23.9&76" );
	subFPS24Act->change( "2&4" );
	subFPS25Act->change( "2&5" );
	subFPS29970Act->change( "29.&970" );
	subFPS30Act->change( "3&0" );

	// Menu Options
	showPlaylistAct->change( Images::icon("playlist"), tr("&TPlaylist") );
	showPropertiesAct->change( Images::icon("info"), tr("View &info and properties...") );
	showPreferencesAct->change( Images::icon("prefs"), tr("P&references") );
#ifdef YOUTUBE_SUPPORT
	showTubeBrowserAct->change( Images::icon("tubebrowser"), tr("&YouTube%1 browser").arg(QChar(0x2122)) );
#endif

	// Submenu Logs
#ifdef LOG_MPLAYER
	showLogMplayerAct->change(PLAYER_NAME);
#endif
#ifdef LOG_SMPLAYER
	showLogSmplayerAct->change( "SMPlayer" );
#endif

	// Menu Help
	showFirstStepsAct->change( Images::icon("guide"), tr("First Steps &Guide") );
	showFAQAct->change( Images::icon("faq"), tr("&FAQ") );
	showCLOptionsAct->change( Images::icon("cl_help"), tr("&Command line options") );
	showCheckUpdatesAct->change( Images::icon("check_updates"), tr("Check for &updates") );

#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	updateYTAct->change( Images::icon("update_youtube"), tr("Update &Youtube code") );
#endif

	showConfigAct->change( Images::icon("show_config"), tr("&Open configuration folder") );
#ifdef REMINDER_ACTIONS
	donateAct->change( Images::icon("donate"), tr("&Donate / Share with your friends") );
#endif
	aboutThisAct->change( Images::icon("logo"), tr("About &SMPlayer") );

#ifdef SHARE_MENU
	facebookAct->change("&Facebook");
	twitterAct->change("&Twitter");
	gmailAct->change("&Gmail");
	hotmailAct->change("&Hotmail");
	yahooAct->change("&Yahoo!");
#endif

	// OSD
	incOSDScaleAct->change(tr("Size &+"));
	decOSDScaleAct->change(tr("Size &-"));

	// TPlaylist
	playNextAct->change( tr("&Next") );
	playPrevAct->change( tr("Pre&vious") );

	playNextAct->setIcon( Images::icon("next") );
	playPrevAct->setIcon( Images::icon("previous") );


	// Actions not in menus or buttons
	// Volume 2
#if !USE_MULTIPLE_SHORTCUTS
	decVolume2Act->change( tr("Dec volume (2)") );
	incVolume2Act->change( tr("Inc volume (2)") );
#endif
	// Exit fullscreen
	exitFullscreenAct->change( tr("Exit fullscreen") );

	nextOSDLevelAct->change( tr("OSD - Next level") );
	decContrastAct->change( tr("Dec contrast") );
	incContrastAct->change( tr("Inc contrast") );
	decBrightnessAct->change( tr("Dec brightness") );
	incBrightnessAct->change( tr("Inc brightness") );
	decHueAct->change( tr("Dec hue") );
	incHueAct->change( tr("Inc hue") );
	decSaturationAct->change( tr("Dec saturation") );
	incSaturationAct->change( tr("Inc saturation") );
	decGammaAct->change( tr("Dec gamma") );
	incGammaAct->change( tr("Inc gamma") );
	nextVideoAct->change( tr("Next video") );
	nextAudioAct->change( tr("Next audio") );
	nextSubtitleAct->change( tr("Next subtitle") );
	nextChapterAct->change( tr("Next chapter") );
	prevChapterAct->change( tr("Previous chapter") );
	doubleSizeAct->change( tr("&Toggle double size") );
	resetVideoEqualizerAct->change( tr("Reset video equalizer") );
	resetAudioEqualizerAct->change( tr("Reset audio equalizer") );
	showContextMenuAct->change( tr("Show context menu") );
	nextAspectAct->change( Images::icon("next_aspect"), tr("Next aspect ratio") );
	nextWheelFunctionAct->change( Images::icon("next_wheel_function"), tr("Next wheel function") );

	showFilenameAct->change( tr("Show filename on OSD") );
	showTimeAct->change( tr("Show playback time on OSD") );
	toggleDeinterlaceAct->change( tr("Toggle deinterlacing") );


	// Action groups
	osdNoneAct->change( tr("Subtitles onl&y") );
	osdSeekAct->change( tr("Volume + &Seek") );
	osdTimerAct->change( tr("Volume + Seek + &Timer") );
	osdTotalAct->change( tr("Volume + Seek + Timer + T&otal time") );


	// MENUS
	openMenu->menuAction()->setText( tr("&Open") );
	playMenu->menuAction()->setText( tr("&Play") );
	videoMenu->menuAction()->setText( tr("&Video") );
	audioMenu->menuAction()->setText( tr("&Audio") );
	subtitlesMenu->menuAction()->setText( tr("&Subtitles") );
	browseMenu->menuAction()->setText( tr("&Browse") );
	optionsMenu->menuAction()->setText( tr("Op&tions") );
	helpMenu->menuAction()->setText( tr("&Help") );

	/*
	openMenuAct->setIcon( Images::icon("open_menu") );
	playMenuAct->setIcon( Images::icon("play_menu") );
	videoMenuAct->setIcon( Images::icon("video_menu") );
	audioMenuAct->setIcon( Images::icon("audio_menu") );
	subtitlesMenuAct->setIcon( Images::icon("subtitles_menu") );
	browseMenuAct->setIcon( Images::icon("browse_menu") );
	optionsMenuAct->setIcon( Images::icon("options_menu") );
	helpMenuAct->setIcon( Images::icon("help_menu") );
	*/

	// Menu Open
	recentfiles_menu->menuAction()->setText( tr("&Recent files") );
	recentfiles_menu->menuAction()->setIcon( Images::icon("recents") );
	clearRecentsAct->change( Images::icon("delete"), tr("&Clear") );

	disc_menu->menuAction()->setText( tr("&Disc") );
	disc_menu->menuAction()->setIcon( Images::icon("open_disc") );

	/* favorites->menuAction()->setText( tr("&Favorites") ); */
	favorites->menuAction()->setText( tr("F&avorites") );
	favorites->menuAction()->setIcon( Images::icon("open_favorites") ); 

	tvlist->menuAction()->setText( tr("&TV") );
	tvlist->menuAction()->setIcon( Images::icon("open_tv") );

	radiolist->menuAction()->setText( tr("Radi&o") );
	radiolist->menuAction()->setIcon( Images::icon("open_radio") );

	// Menu Play
	speed_menu->menuAction()->setText( tr("Sp&eed") );
	speed_menu->menuAction()->setIcon( Images::icon("speed") );

	ab_menu->menuAction()->setText( tr("&A-B section") );
	ab_menu->menuAction()->setIcon( Images::icon("ab_menu") );

	// Menu Video
	videotrack_menu->menuAction()->setText( tr("&Track", "video") );
	videotrack_menu->menuAction()->setIcon( Images::icon("video_track") );

	videosize_menu->menuAction()->setText( tr("Si&ze") );
	videosize_menu->menuAction()->setIcon( Images::icon("video_size") );

	/*
	panscan_menu->menuAction()->setText( tr("&Pan && scan") );
	panscan_menu->menuAction()->setIcon( Images::icon("panscan") );
	*/
	zoom_menu->menuAction()->setText( tr("Zoo&m") );
	zoom_menu->menuAction()->setIcon( Images::icon("zoom") );

	aspect_menu->menuAction()->setText( tr("&Aspect ratio") );
	aspect_menu->menuAction()->setIcon( Images::icon("aspect") );

	deinterlace_menu->menuAction()->setText( tr("&Deinterlace") );
	deinterlace_menu->menuAction()->setIcon( Images::icon("deinterlace") );

	videofilter_menu->menuAction()->setText( tr("F&ilters") );
	videofilter_menu->menuAction()->setIcon( Images::icon("video_filters") );

	rotate_menu->menuAction()->setText( tr("&Rotate") );
	rotate_menu->menuAction()->setIcon( Images::icon("rotate") );

	ontop_menu->menuAction()->setText( tr("S&tay on top") );
	ontop_menu->menuAction()->setIcon( Images::icon("ontop") );

#if USE_ADAPTER
	screen_menu->menuAction()->setText( tr("Scree&n") );
	screen_menu->menuAction()->setIcon( Images::icon("screen") );
#endif

	denoise_menu->menuAction()->setText( tr("De&noise") );
	denoise_menu->menuAction()->setIcon( Images::icon("denoise") );

	unsharp_menu->menuAction()->setText( tr("Blur/S&harp") );
	unsharp_menu->menuAction()->setIcon( Images::icon("unsharp") );

	aspectDetectAct->change( tr("&Auto") );
	aspect11Act->change( "1&:1" );
	aspect32Act->change( "&3:2" );
	aspect43Act->change( "&4:3" );
	aspect118Act->change( "11:&8" );
	aspect54Act->change( "&5:4" );
	aspect149Act->change( "&14:9" );
	aspect1410Act->change( "1&4:10" );
	aspect169Act->change( "16:&9" );
	aspect1610Act->change( "1&6:10" );
	aspect235Act->change( "&2.35:1" );
	aspectNoneAct->change( tr("&Disabled") );

	deinterlaceNoneAct->change( tr("&None") );
	deinterlaceL5Act->change( tr("&Lowpass5") );
	deinterlaceYadif0Act->change( tr("&Yadif (normal)") );
	deinterlaceYadif1Act->change( tr("Y&adif (double framerate)") );
	deinterlaceLBAct->change( tr("Linear &Blend") );
	deinterlaceKernAct->change( tr("&Kerndeint") );

	denoiseNoneAct->change( tr("&Off", "denoise menu") );
	denoiseNormalAct->change( tr("&Normal","denoise menu") );
	denoiseSoftAct->change( tr("&Soft", "denoise menu") );

	unsharpNoneAct->change( tr("&None", "unsharp menu") );
	blurAct->change( tr("&Blur", "unsharp menu") );
	sharpenAct->change( tr("&Sharpen", "unsharp menu") );

	rotateNoneAct->change( tr("&Off") );
	rotateClockwiseFlipAct->change( tr("&Rotate by 90 degrees clockwise and flip") );
	rotateClockwiseAct->change( tr("Rotate by 90 degrees &clockwise") );
	rotateCounterclockwiseAct->change( tr("Rotate by 90 degrees counterclock&wise") );
	rotateCounterclockwiseFlipAct->change( tr("Rotate by 90 degrees counterclockwise and &flip") );

	onTopAlwaysAct->change( tr("&Always") );
	onTopNeverAct->change( tr("&Never") );
	onTopWhilePlayingAct->change( tr("While &playing") );
	toggleStayOnTopAct->change( tr("Toggle stay on top") );

#if USE_ADAPTER
	screenDefaultAct->change( tr("&Default") );
#endif

	// Menu Audio
	audiotrack_menu->menuAction()->setText( tr("&Track", "audio") );
	audiotrack_menu->menuAction()->setIcon( Images::icon("audio_track") );

	audiofilter_menu->menuAction()->setText( tr("&Filters") );
	audiofilter_menu->menuAction()->setIcon( Images::icon("audio_filters") );

	audiochannels_menu->menuAction()->setText( tr("&Channels") );
	audiochannels_menu->menuAction()->setIcon( Images::icon("audio_channels") );

	stereomode_menu->menuAction()->setText( tr("&Stereo mode") );
	stereomode_menu->menuAction()->setIcon( Images::icon("stereo_mode") );

	/* channelsDefaultAct->change( tr("&Default") ); */
	channelsStereoAct->change( tr("&Stereo") );
	channelsSurroundAct->change( tr("&4.0 Surround") );
	channelsFull51Act->change( tr("&5.1 Surround") );
	channelsFull61Act->change( tr("&6.1 Surround") );
	channelsFull71Act->change( tr("&7.1 Surround") );

	stereoAct->change( tr("&Stereo") );
	leftChannelAct->change( tr("&Left channel") );
	rightChannelAct->change( tr("&Right channel") );
	monoAct->change( tr("&Mono") );
	reverseAct->change( tr("Re&verse") );

	// Menu Subtitle
	subtitles_track_menu->menuAction()->setText( tr("&Select") );
	subtitles_track_menu->menuAction()->setIcon( Images::icon("sub") );

#ifdef MPV_SUPPORT
	secondary_subtitles_track_menu->menuAction()->setText( tr("Secondary trac&k") );
	secondary_subtitles_track_menu->menuAction()->setIcon( Images::icon("secondary_sub") );
#endif

	closed_captions_menu->menuAction()->setText( tr("&Closed captions") );
	closed_captions_menu->menuAction()->setIcon( Images::icon("closed_caption") );

	subfps_menu->menuAction()->setText( tr("F&rames per second") );
	subfps_menu->menuAction()->setIcon( Images::icon("subfps") );

	// Menu Browse 
	titles_menu->menuAction()->setText( tr("&Title") );
	titles_menu->menuAction()->setIcon( Images::icon("title") );

	chapters_menu->menuAction()->setText( tr("&Chapter") );
	chapters_menu->menuAction()->setIcon( Images::icon("chapter") );

	angles_menu->menuAction()->setText( tr("&Angle") );
	angles_menu->menuAction()->setIcon( Images::icon("angle") );

#if PROGRAM_SWITCH
	programtrack_menu->menuAction()->setText( tr("P&rogram", "program") );
	programtrack_menu->menuAction()->setIcon( Images::icon("program_track") );
#endif


	dvdnavUpAct->change(Images::icon("dvdnav_up"), tr("DVD menu, move up"));
	dvdnavDownAct->change(Images::icon("dvdnav_down"), tr("DVD menu, move down"));
	dvdnavLeftAct->change(Images::icon("dvdnav_left"), tr("DVD menu, move left"));
	dvdnavRightAct->change(Images::icon("dvdnav_right"), tr("DVD menu, move right"));
	dvdnavMenuAct->change(Images::icon("dvdnav_menu"), tr("DVD &menu"));
	dvdnavSelectAct->change(Images::icon("dvdnav_select"), tr("DVD menu, select option"));
	dvdnavPrevAct->change(Images::icon("dvdnav_prev"), tr("DVD &previous menu"));
	dvdnavMouseAct->change(Images::icon("dvdnav_mouse"), tr("DVD menu, mouse click"));

	// Menu Options
	osd_menu->menuAction()->setText( tr("&OSD") );
	osd_menu->menuAction()->setIcon( Images::icon("osd") );

#ifdef SHARE_MENU
	share_menu->menuAction()->setText( tr("S&hare SMPlayer with your friends") );
	share_menu->menuAction()->setIcon( Images::icon("share") );
#endif

#if defined(LOG_MPLAYER) || defined(LOG_SMPLAYER)
	logs_menu->menuAction()->setText( tr("&View logs") );
	logs_menu->menuAction()->setIcon( Images::icon("logs") );
#endif

	// TODO: make sure the "<empty>" string is translated

	// Playlist
	playlist->retranslateStrings();

	// Other things
#ifdef LOG_MPLAYER
	mplayer_log_window->setWindowTitle( tr("%1 log").arg(PLAYER_NAME) );
#endif
#ifdef LOG_SMPLAYER
	smplayer_log_window->setWindowTitle( tr("SMPlayer log") );
#endif

	updateRecents();
	updateWidgets();

	// Update actions view in preferences
	// It has to be done, here. The actions are translated after the
	// preferences dialog.
	if (pref_dialog) pref_dialog->mod_input()->actions_editor->updateView();
}

void TBase::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	rewind1Act->setIcon( Images::icon("rewind10s") );
	rewind2Act->setIcon( Images::icon("rewind1m") );
	rewind3Act->setIcon( Images::icon("rewind10m") );

	forward1Act->setIcon( Images::icon("forward10s") );
	forward2Act->setIcon( Images::icon("forward1m") );
	forward3Act->setIcon( Images::icon("forward10m") );
}

void TBase::setWindowCaption(const QString & title) {
	setWindowTitle(title);
}

void TBase::createCore() {

	core = new Core( mplayerwindow, this, SEEKBAR_RESOLUTION);

	connect( core, SIGNAL(widgetsNeedUpdate()),
             this, SLOT(updateWidgets()) );
	connect( core, SIGNAL(videoEqualizerNeedsUpdate()),
             this, SLOT(updateVideoEqualizer()) );

	connect( core, SIGNAL(audioEqualizerNeedsUpdate()),
             this, SLOT(updateAudioEqualizer()) );

	connect( core, SIGNAL(showFrame(int)),
             this, SIGNAL(frameChanged(int)) );

	connect( core, SIGNAL(ABMarkersChanged(int,int)),
             this, SIGNAL(ABMarkersChanged(int,int)) );

	connect( core, SIGNAL(showTime(double)),
             this, SLOT(gotCurrentTime(double)) );
	connect( core, SIGNAL(newDuration(double)),
			 this, SLOT(gotDuration(double)) );

	connect( core, SIGNAL(needResize(int, int)),
             this, SLOT(resizeWindow(int,int)) );

	connect( core, SIGNAL(showMessage(QString,int)),
             this, SLOT(displayMessage(QString,int)) );
	connect( core, SIGNAL(showMessage(QString)),
             this, SLOT(displayMessage(QString)) );

	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(displayState(Core::State)) );
	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(checkStayOnTop(Core::State)), Qt::QueuedConnection );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(enterFullscreenOnPlay()), Qt::QueuedConnection );
	connect( core, SIGNAL(mediaStoppedByUser()),
             this, SLOT(exitFullscreenOnStop()) );

	connect( core, SIGNAL(mediaStoppedByUser()),
			 mplayerwindow, SLOT(showLogo()) );

	connect( core, SIGNAL(mediaLoaded()),
             this, SLOT(enableActionsOnPlaying()) );

	connect( core, SIGNAL(noFileToPlay()), this, SLOT(gotNoFileToPlay()) );

	connect( core, SIGNAL(mediaFinished()),
             this, SLOT(disableActionsOnStop()) );
	connect( core, SIGNAL(mediaStoppedByUser()),
             this, SLOT(disableActionsOnStop()) );

	connect( core, SIGNAL(stateChanged(Core::State)),
             this, SLOT(togglePlayAction(Core::State)) );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(newMediaLoaded()), Qt::QueuedConnection );
	connect( core, SIGNAL(mediaInfoChanged()),
             this, SLOT(updateMediaInfo()) );

	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(checkPendingActionsToRun()), Qt::QueuedConnection );
#if REPORT_OLD_MPLAYER
	connect( core, SIGNAL(mediaStartPlay()),
             this, SLOT(checkMplayerVersion()), Qt::QueuedConnection );
#endif
	connect( core, SIGNAL(failedToParseMplayerVersion(QString)),
             this, SLOT(askForMplayerVersion(QString)) );

	connect( core, SIGNAL(playerFailed(QProcess::ProcessError)),
			 this, SLOT(showErrorFromPlayer(QProcess::ProcessError)) );

	connect( core, SIGNAL(playerFinishedWithError(int)),
			 this, SLOT(showExitCodeFromPlayer(int)) );

	connect( core, SIGNAL(noVideo()), this, SLOT(slotNoVideo()) );

	// Log mplayer output
#ifdef LOG_MPLAYER
	connect( core, SIGNAL(aboutToStartPlaying()),
             this, SLOT(clearMplayerLog()) );
	connect( core, SIGNAL(logLineAvailable(QString)),
             this, SLOT(recordMplayerLog(QString)) );

	connect( core, SIGNAL(mediaLoaded()), 
             this, SLOT(autosaveMplayerLog()) );
#endif

#ifdef YOUTUBE_SUPPORT
	connect(core, SIGNAL(signatureNotFound(const QString &)),
            this, SLOT(YTNoSignature(const QString &)));
	connect(core, SIGNAL(noSslSupport()),
            this, SLOT(YTNoSslSupport()));
#endif
	connect(core, SIGNAL(receivedForbidden()), this, SLOT(gotForbidden()));

	connect(mplayerwindow, SIGNAL(moveOSD(const QPoint &)),
			core, SLOT(setOSDPos(const QPoint &)));
	connect(mplayerwindow, SIGNAL(showMessage(QString, int, int)),
			 core, SLOT(displayMessage(QString, int, int)) );
}

void TBase::createMplayerWindow() {
    mplayerwindow = new MplayerWindow( panel );
#if USE_COLORKEY
	mplayerwindow->setColorKey( pref->color_key );
#endif
	mplayerwindow->setDelayLeftClick(pref->delay_left_click);

#if LOGO_ANIMATION
	mplayerwindow->setAnimatedLogo( pref->animated_logo);
#endif

#ifdef SHAREWIDGET
	sharewidget = new ShareWidget(Global::settings, mplayerwindow);
	mplayerwindow->setCornerWidget(sharewidget);
	#ifdef REMINDER_ACTIONS
	connect(sharewidget, SIGNAL(supportClicked()), this, SLOT(helpDonate()));
	#endif
#endif

	QVBoxLayout * layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addWidget(mplayerwindow);
	panel->setLayout(layout);

	// mplayerwindow
	/*
    connect( mplayerwindow, SIGNAL(rightButtonReleased(QPoint)),
	         this, SLOT(showPopupMenu(QPoint)) );
	*/

	// mplayerwindow mouse events
	connect( mplayerwindow, SIGNAL(doubleClicked()),
             this, SLOT(doubleClickFunction()) );
	connect( mplayerwindow, SIGNAL(leftClicked()),
             this, SLOT(leftClickFunction()) );
	connect( mplayerwindow, SIGNAL(rightClicked()),
             this, SLOT(rightClickFunction()) );
	connect( mplayerwindow, SIGNAL(middleClicked()),
             this, SLOT(middleClickFunction()) );
	connect( mplayerwindow, SIGNAL(xbutton1Clicked()),
             this, SLOT(xbutton1ClickFunction()) );
	connect( mplayerwindow, SIGNAL(xbutton2Clicked()),
             this, SLOT(xbutton2ClickFunction()) );
}

void TBase::createVideoEqualizer() {
	// Equalizer
	video_equalizer = new VideoEqualizer(this);
	connect( video_equalizer, SIGNAL(contrastChanged(int)), 
             core, SLOT(setContrast(int)) );
	connect( video_equalizer, SIGNAL(brightnessChanged(int)), 
             core, SLOT(setBrightness(int)) );
	connect( video_equalizer, SIGNAL(hueChanged(int)), 
             core, SLOT(setHue(int)) );
	connect( video_equalizer, SIGNAL(saturationChanged(int)), 
             core, SLOT(setSaturation(int)) );
	connect( video_equalizer, SIGNAL(gammaChanged(int)), 
             core, SLOT(setGamma(int)) );

	connect( video_equalizer, SIGNAL(visibilityChanged()),
             this, SLOT(updateWidgets()) );
	connect( video_equalizer, SIGNAL(requestToChangeDefaultValues()),
             this, SLOT(setDefaultValuesFromVideoEqualizer()) );
	connect( video_equalizer, SIGNAL(bySoftwareChanged(bool)),
             this, SLOT(changeVideoEqualizerBySoftware(bool)) );
}

void TBase::createAudioEqualizer() {
	// Audio Equalizer
	audio_equalizer = new AudioEqualizer(this);

	connect( audio_equalizer->eq[0], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq0(int)) );
	connect( audio_equalizer->eq[1], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq1(int)) );
	connect( audio_equalizer->eq[2], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq2(int)) );
	connect( audio_equalizer->eq[3], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq3(int)) );
	connect( audio_equalizer->eq[4], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq4(int)) );
	connect( audio_equalizer->eq[5], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq5(int)) );
	connect( audio_equalizer->eq[6], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq6(int)) );
	connect( audio_equalizer->eq[7], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq7(int)) );
	connect( audio_equalizer->eq[8], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq8(int)) );
	connect( audio_equalizer->eq[9], SIGNAL(valueChanged(int)), 
             core, SLOT(setAudioEq9(int)) );

	connect( audio_equalizer, SIGNAL(applyClicked(AudioEqualizerList)), 
             core, SLOT(setAudioAudioEqualizerRestart(AudioEqualizerList)) );

	connect( audio_equalizer, SIGNAL(valuesChanged(AudioEqualizerList)),
             core, SLOT(setAudioEqualizer(AudioEqualizerList)) );

	connect( audio_equalizer, SIGNAL(visibilityChanged()),
             this, SLOT(updateWidgets()) );
}

void TBase::createTPlaylist() {
#if DOCK_PLAYLIST
	playlist = new TPlaylist(core, this, 0);
#else
	//playlist = new TPlaylist(core, this, "playlist");
	playlist = new TPlaylist(core, 0);
#endif

	/*
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(exitFullscreenOnStop()) );
	*/
	connect( playlist, SIGNAL(playlistEnded()),
             this, SLOT(playlistHasFinished()) );

	/*
	connect( playlist, SIGNAL(visibilityChanged()),
             this, SLOT(playlistVisibilityChanged()) );
	*/

}

void TBase::createPanel() {
	panel = new QWidget( this );
	panel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	panel->setMinimumSize( QSize(1,1) );
	panel->setFocusPolicy( Qt::StrongFocus );

	// panel
	/*
	panel->setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( panel, QColor(0,0,0) );
	*/
}

void TBase::createPreferencesDialog() {
	QApplication::setOverrideCursor(Qt::WaitCursor);
	pref_dialog = new PreferencesDialog(this);
	pref_dialog->setModal(false);
	/* pref_dialog->mod_input()->setActionsList( actions_list ); */
	connect( pref_dialog, SIGNAL(applied()),
             this, SLOT(applyNewPreferences()) );
	QApplication::restoreOverrideCursor();
}

void TBase::createFilePropertiesDialog() {
	qDebug("Gui::TBase::createFilePropertiesDialog");
	QApplication::setOverrideCursor(Qt::WaitCursor);
	file_dialog = new FilePropertiesDialog(this);
	file_dialog->setModal(false);
	connect( file_dialog, SIGNAL(applied()),
             this, SLOT(applyFileProperties()) );
	QApplication::restoreOverrideCursor();
}


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
	/*
	recentfiles_menu->addAction( clearRecentsAct );
	recentfiles_menu->addSeparator();
	*/

	openMenu->addMenu( recentfiles_menu );
	openMenu->addMenu(favorites);
	openMenu->addAction(openDirectoryAct);
	openMenu->addAction(openTPlaylistAct);

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
/* #ifndef Q_OS_WIN */
	openMenu->addMenu(tvlist);
	openMenu->addMenu(radiolist);
/* #endif */
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
	speed_menu = new QMenu(this);
	speed_menu->menuAction()->setObjectName("speed_menu");
	speed_menu->addAction(normalSpeedAct);
	speed_menu->addSeparator();
	speed_menu->addAction(halveSpeedAct);
	speed_menu->addAction(doubleSpeedAct);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed10Act);
	speed_menu->addAction(incSpeed10Act);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed4Act);
	speed_menu->addAction(incSpeed4Act);
	speed_menu->addSeparator();
	speed_menu->addAction(decSpeed1Act);
	speed_menu->addAction(incSpeed1Act);

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
	videoMenu->addAction(compactAct);

#if USE_ADAPTER
	// Screen submenu
	screen_menu = new QMenu(this);
	screen_menu->menuAction()->setObjectName("screen_menu");
	screen_menu->addActions( screenGroup->actions() );
	videoMenu->addMenu(screen_menu);
#endif

	// Size submenu
	videosize_menu = new QMenu(this);
	videosize_menu->menuAction()->setObjectName("videosize_menu");
	videosize_menu->addActions( sizeGroup->actions() );
	videosize_menu->addSeparator();
	videosize_menu->addAction(doubleSizeAct);
	videoMenu->addMenu(videosize_menu);

	// Zoom submenu
	zoom_menu = new QMenu(this);
	zoom_menu->menuAction()->setObjectName("zoom_menu");
	zoom_menu->addAction(resetZoomAct);
	zoom_menu->addSeparator();
	zoom_menu->addAction(autoZoomAct);
	zoom_menu->addAction(autoZoom169Act);
	zoom_menu->addAction(autoZoom235Act);
	zoom_menu->addSeparator();
	zoom_menu->addAction(decZoomAct);
	zoom_menu->addAction(incZoomAct);
	zoom_menu->addSeparator();
	zoom_menu->addAction(moveLeftAct);
	zoom_menu->addAction(moveRightAct);
	zoom_menu->addAction(moveUpAct);
	zoom_menu->addAction(moveDownAct);

	videoMenu->addMenu(zoom_menu);

	// Aspect submenu
	aspect_menu = new QMenu(this);
	aspect_menu->menuAction()->setObjectName("aspect_menu");
	aspect_menu->addActions( aspectGroup->actions() );

	videoMenu->addMenu(aspect_menu);

	// Deinterlace submenu
	deinterlace_menu = new QMenu(this);
	deinterlace_menu->menuAction()->setObjectName("deinterlace_menu");
	deinterlace_menu->addActions( deinterlaceGroup->actions() );

	videoMenu->addMenu(deinterlace_menu);

	// Video filter submenu
	videofilter_menu = new QMenu(this);
	videofilter_menu->menuAction()->setObjectName("videofilter_menu");
	videofilter_menu->addAction(postProcessingAct);
	videofilter_menu->addAction(deblockAct);
	videofilter_menu->addAction(deringAct);
	videofilter_menu->addAction(gradfunAct);
	videofilter_menu->addAction(addNoiseAct);
	videofilter_menu->addAction(addLetterboxAct);
	videofilter_menu->addAction(upscaleAct);
	videofilter_menu->addAction(phaseAct);

	// Denoise submenu
	denoise_menu = new QMenu(this);
	denoise_menu->menuAction()->setObjectName("denoise_menu");
	denoise_menu->addActions(denoiseGroup->actions());
	videofilter_menu->addMenu(denoise_menu);

	// Unsharp submenu
	unsharp_menu = new QMenu(this);
	unsharp_menu->menuAction()->setObjectName("unsharp_menu");
	unsharp_menu->addActions(unsharpGroup->actions());
	videofilter_menu->addMenu(unsharp_menu);
	/*
	videofilter_menu->addSeparator();
	videofilter_menu->addActions(denoiseGroup->actions());
	videofilter_menu->addSeparator();
	videofilter_menu->addActions(unsharpGroup->actions());
	*/
	videoMenu->addMenu(videofilter_menu);

	// Rotate submenu
	rotate_menu = new QMenu(this);
	rotate_menu->menuAction()->setObjectName("rotate_menu");
	rotate_menu->addActions(rotateGroup->actions());

	videoMenu->addMenu(rotate_menu);

	videoMenu->addAction(flipAct);
	videoMenu->addAction(mirrorAct);
	videoMenu->addAction(stereo3dAct);
	videoMenu->addSeparator();
	videoMenu->addAction(videoEqualizerAct);
	videoMenu->addAction(screenshotAct);
	videoMenu->addAction(screenshotsAct);

	// Ontop submenu
	ontop_menu = new QMenu(this);
	ontop_menu->menuAction()->setObjectName("ontop_menu");
	ontop_menu->addActions(onTopActionGroup->actions());

	videoMenu->addMenu(ontop_menu);

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
	audiofilter_menu->addAction(extrastereoAct);
	audiofilter_menu->addAction(karaokeAct);
	audiofilter_menu->addAction(volnormAct);

	audioMenu->addMenu(audiofilter_menu);

	// Audio channels submenu
	audiochannels_menu = new QMenu(this);
	audiochannels_menu->menuAction()->setObjectName("audiochannels_menu");
	audiochannels_menu->addActions( channelsGroup->actions() );

	audioMenu->addMenu(audiochannels_menu);

	// Stereo mode submenu
	stereomode_menu = new QMenu(this);
	stereomode_menu->menuAction()->setObjectName("stereomode_menu");
	stereomode_menu->addActions( stereoGroup->actions() );

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

	subfps_menu = new QMenu(this);
	subfps_menu->menuAction()->setObjectName("subfps_menu");
	subfps_menu->addAction( subFPSNoneAct );
	/* subfps_menu->addAction( subFPS23Act ); */
	subfps_menu->addAction( subFPS23976Act );
	subfps_menu->addAction( subFPS24Act );
	subfps_menu->addAction( subFPS25Act );
	subfps_menu->addAction( subFPS29970Act );
	subfps_menu->addAction( subFPS30Act );
	subtitlesMenu->addMenu(subfps_menu);
	subtitlesMenu->addSeparator();

	closed_captions_menu = new QMenu(this);
	closed_captions_menu->menuAction()->setObjectName("closed_captions_menu");
	closed_captions_menu->addAction( ccNoneAct);
	closed_captions_menu->addAction( ccChannel1Act);
	closed_captions_menu->addAction( ccChannel2Act);
	closed_captions_menu->addAction( ccChannel3Act);
	closed_captions_menu->addAction( ccChannel4Act);
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
#ifdef YOUTUBE_SUPPORT
	#if 0
	// Check if the smplayer youtube browser is installed
	{
		QString tube_exec = Paths::appPath() + "/smtube";
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

	// OSD submenu
	osd_menu = new QMenu(this);
	osd_menu->menuAction()->setObjectName("osd_menu");
	osd_menu->addActions(osdGroup->actions());
	osd_menu->addSeparator();
	osd_menu->addAction(decOSDScaleAct);
	osd_menu->addAction(incOSDScaleAct);


	optionsMenu->addMenu(osd_menu);

	// Logs submenu
#if defined(LOG_MPLAYER) || defined(LOG_SMPLAYER)
	logs_menu = new QMenu(this);
	#ifdef LOG_MPLAYER
	logs_menu->addAction(showLogMplayerAct);
	#endif
	#ifdef LOG_SMPLAYER
	logs_menu->addAction(showLogSmplayerAct);
	#endif
	optionsMenu->addMenu(logs_menu);
#endif

	optionsMenu->addAction(showPreferencesAct);

	/*
	Favorites * fav = new Favorites(Paths::configPath() + "/test.fav", this);
	connect(fav, SIGNAL(activated(QString)), this, SLOT(open(QString)));
	optionsMenu->addMenu( fav->menu() )->setText("Favorites");
	*/

	// HELP MENU
	// Share submenu
#ifdef SHARE_MENU
	share_menu = new QMenu(this);
	share_menu->addAction(facebookAct);
	share_menu->addAction(twitterAct);
	share_menu->addAction(gmailAct);
	share_menu->addAction(hotmailAct);
	share_menu->addAction(yahooAct);

	helpMenu->addMenu(share_menu);
	helpMenu->addSeparator();
#endif

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
#ifdef REMINDER_ACTIONS
	helpMenu->addAction(donateAct);
	helpMenu->addSeparator();
#endif
	helpMenu->addAction(aboutThisAct);

	// POPUP MENU
	if (!popup)
		popup = new QMenu(this);
	else
		popup->clear();

	popup->addMenu( openMenu );
	popup->addMenu( playMenu );
	popup->addMenu( videoMenu );
	popup->addMenu( audioMenu );
	popup->addMenu( subtitlesMenu );
	popup->addMenu( favorites );
	popup->addMenu( browseMenu );
	popup->addMenu( optionsMenu );
}

void TBase::closeEvent( QCloseEvent * e )  {
	qDebug("Gui::TBase::closeEvent");

	core->close();

	saveConfig("");
	e->accept();
}

void TBase::closeWindow() {
	qDebug("Gui::TBase::closeWindow");

	close();
}

void TBase::showPlaylist() {
	showPlaylist( !playlist->isVisible() );
}

void TBase::showPlaylist(bool b) {
	if ( !b ) {
		playlist->hide();
	} else {
		exitFullscreenIfNeeded();
		playlist->show();
	}
	//updateWidgets();
}

void TBase::showVideoEqualizer() {
	showVideoEqualizer( !video_equalizer->isVisible() );
}

void TBase::showVideoEqualizer(bool b) {
	if (!b) {
		video_equalizer->hide();
	} else {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		video_equalizer->show();
	}
	updateWidgets();
}

void TBase::showAudioEqualizer() {
	showAudioEqualizer( !audio_equalizer->isVisible() );
}

void TBase::showAudioEqualizer(bool b) {
	if (!b) {
		audio_equalizer->hide();
	} else {
		// Exit fullscreen, otherwise dialog is not visible
		exitFullscreenIfNeeded();
		audio_equalizer->show();
	}
	updateWidgets();
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
#if !DOCK_PLAYLIST
	pref_dialog->mod_input()->actions_editor->addActions(playlist);
#endif

	// Set playlist preferences
	PrefPlaylist * pl = pref_dialog->mod_playlist();
	pl->setDirectoryRecursion(playlist->directoryRecursion());
	pl->setAutoGetInfo(playlist->autoGetInfo());
	pl->setSavePlaylistOnExit(playlist->savePlaylistOnExit());
	pl->setPlayFilesFromStart(playlist->playFilesFromStart());
	pl->setIgnorePlayerErrors(playlist->ignorePlayerErrors());

	pref_dialog->show();
}

// The user has pressed OK in preferences dialog
void TBase::applyNewPreferences() {
	qDebug("Gui::TBase::applyNewPreferences");

	bool need_update_language = false;

	PlayerID::Player old_player_type = PlayerID::player(pref->mplayer_bin);

	pref_dialog->getData(pref);

	// Setup proxy
	setupNetworkProxy();

	// Change application font
	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString( pref->default_font );
		if (QApplication::font() != f) {
			qDebug("Gui::TBase::applyNewPreferences: setting new font: %s", pref->default_font.toLatin1().constData());
			QApplication::setFont(f);
		}
	}

#ifndef NO_USE_INI_FILES
	PrefGeneral *_general = pref_dialog->mod_general();
	if (_general->fileSettingsMethodChanged()) {
		core->changeFileSettingsMethod(pref->file_settings_method);
	}
#endif

	PrefInterface *_interface = pref_dialog->mod_interface();
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

	mplayerwindow->setDelayLeftClick(pref->delay_left_click);

#if ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
	if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
		resize( width(), height() + 200);
		panel->show();
	}
#endif

	PrefAdvanced *advanced = pref_dialog->mod_advanced();
#if REPAINT_BACKGROUND_OPTION
	if (advanced->repaintVideoBackgroundChanged()) {
		mplayerwindow->videoLayer()->setRepaintBackground(pref->repaint_video_background);
	}
#endif
#if USE_COLORKEY
	if (advanced->colorkeyChanged()) {
		mplayerwindow->setColorKey( pref->color_key );
	}
#endif
	if (advanced->monitorAspectChanged()) {
		mplayerwindow->setMonitorAspect( pref->monitor_aspect_double() );
	}
#if ALLOW_DEMUXER_CODEC_CHANGE
	if (advanced->lavfDemuxerChanged()) {
		core->mset.forced_demuxer = pref->use_lavf_demuxer ? "lavf" : "";
	}
#endif

	// Update playlist preferences
	PrefPlaylist * pl = pref_dialog->mod_playlist();
	playlist->setDirectoryRecursion(pl->directoryRecursion());
	playlist->setAutoGetInfo(pl->autoGetInfo());
	playlist->setSavePlaylistOnExit(pl->savePlaylistOnExit());
	playlist->setPlayFilesFromStart(pl->playFilesFromStart());
	playlist->setIgnorePlayerErrors(pl->ignorePlayerErrors());


	if (need_update_language) {
		translator->load(pref->language);
	}

	setJumpTexts(); // Update texts in menus
	updateWidgets(); // Update the screenshot action

#if STYLE_SWITCHING
	if (_interface->styleChanged()) {
		qDebug( "selected style: '%s'", pref->style.toUtf8().data() );
		if ( !pref->style.isEmpty()) {
			qApp->setStyle( pref->style );
		} else {
			qDebug("setting default style: '%s'", default_style.toUtf8().data() );
			qApp->setStyle( default_style );
		}
	}
#endif

	// Update actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
	saveActions();

#ifndef NO_USE_INI_FILES
	pref->save();
#endif

	// Any restarts needed?
	if (_interface->guiChanged()
		|| old_player_type != PlayerID::player(pref->mplayer_bin)) {
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
#if ALLOW_DEMUXER_CODEC_CHANGE
	InfoReader *i = InfoReader::obj();
	i->getInfo();
	file_dialog->setCodecs( i->vcList(), i->acList(), i->demuxerList() );

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
#endif

	file_dialog->setMplayerAdditionalArguments( core->mset.mplayer_additional_options );
	file_dialog->setMplayerAdditionalVideoFilters( core->mset.mplayer_additional_video_filters );
	file_dialog->setMplayerAdditionalAudioFilters( core->mset.mplayer_additional_audio_filters );

	file_dialog->setMediaData( core->mdat );
}

void TBase::applyFileProperties() {
	qDebug("Gui::TBase::applyFileProperties");

	bool need_restart = false;

#undef TEST_AND_SET
#define TEST_AND_SET( Pref, Dialog ) \
	if ( Pref != Dialog ) { Pref = Dialog; need_restart = true; }

#if ALLOW_DEMUXER_CODEC_CHANGE
	bool demuxer_changed = false;

	QString prev_demuxer = core->mset.forced_demuxer;

	QString demuxer = file_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	if (prev_demuxer != core->mset.forced_demuxer) {
		// Demuxer changed
		demuxer_changed = true;
		core->mset.current_audio_id = MediaSettings::NoneSelected;
		core->mset.current_sub_idx = MediaSettings::NoneSelected;
	}

	QString ac = file_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);
#endif

	TEST_AND_SET(core->mset.mplayer_additional_options, file_dialog->mplayerAdditionalArguments());
	TEST_AND_SET(core->mset.mplayer_additional_video_filters, file_dialog->mplayerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.mplayer_additional_audio_filters, file_dialog->mplayerAdditionalAudioFilters());

#if ALLOW_DEMUXER_CODEC_CHANGE
	// Restart the video to apply
	if (need_restart) {
		if (demuxer_changed) {
			core->reload();
		} else {
			core->restart();
		}
	}
#endif
}


void TBase::updateMediaInfo() {
	qDebug("Gui::TBase::updateMediaInfo");

	if (file_dialog) {
		if (file_dialog->isVisible()) setDataToFileProperties();
	}

	setWindowCaption( core->mdat.displayName(pref->show_tag_in_window_title) + " - SMPlayer" );

	emit videoInfoChanged(core->mdat.video_width, core->mdat.video_height, core->mdat.video_fps);
}

void TBase::newMediaLoaded() {
	qDebug("Gui::TBase::newMediaLoaded");

	QString filename = core->mdat.filename;
	QString stream_title = core->mdat.stream_title;
	if (!stream_title.isEmpty()) {
		pref->history_recents->addItem(filename, stream_title);
	} else {
		pref->history_recents->addItem(filename);
	}
	updateRecents();
}

void TBase::gotNoFileToPlay() {
	qDebug("Gui::TBase::gotNoFileToPlay");

	playlist->resumePlay();
}

#ifdef LOG_MPLAYER
void TBase::clearMplayerLog() {
	mplayer_log.clear();
	if (mplayer_log_window->isVisible()) mplayer_log_window->clear();
}

void TBase::recordMplayerLog(QString line) {
	if (pref->log_mplayer) {
		if ( (line.indexOf("A:")==-1) && (line.indexOf("V:")==-1) ) {
			line.append("\n");
			mplayer_log.append(line);
			if (mplayer_log_window->isVisible()) mplayer_log_window->appendText(line);
		}
	}
}

/*! 
	Save the mplayer log to a file, so it can be used by external
	applications.
*/
void TBase::autosaveMplayerLog() {
	qDebug("Gui::TBase::autosaveMplayerLog");

	if (pref->autosave_mplayer_log) {
		if (!pref->mplayer_log_saveto.isEmpty()) {
			QFile file( pref->mplayer_log_saveto );
			if ( file.open( QIODevice::WriteOnly ) ) {
				QTextStream strm( &file );
				strm << mplayer_log;
				file.close();
			}
		}
	}
}

void TBase::showMplayerLog() {
	qDebug("Gui::TBase::showMplayerLog");

	exitFullscreenIfNeeded();

	mplayer_log_window->setText( mplayer_log );
	mplayer_log_window->show();
}
#endif

#ifdef LOG_SMPLAYER
void TBase::recordSmplayerLog(QString line) {
	if (pref->log_smplayer) {
		line.append("\n");
		smplayer_log.append(line);
		if (smplayer_log_window->isVisible()) smplayer_log_window->appendText(line);
	}
}

void TBase::showLog() {
	qDebug("Gui::TBase::showLog");

	exitFullscreenIfNeeded();

	smplayer_log_window->setText( smplayer_log );
	smplayer_log_window->show();
}
#endif

void TBase::updateVideoTracks() {
	qDebug("Gui::TBase::updateVideoTracks");

	videoTrackGroup->clear(true);

	Maps::TTracks* videos = &core->mdat.videos;
	if (videos->count() == 0) {
		QAction * a = videoTrackGroup->addAction( tr("<empty>") );
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

	videotrack_menu->addActions( videoTrackGroup->actions() );
}

void TBase::updateAudioTracks() {
	qDebug("Gui::TBase::updateAudioTracks");

	audioTrackGroup->clear(true);

	Maps::TTracks* audios = &core->mdat.audios;
	if (audios->count() == 0) {
		QAction * a = audioTrackGroup->addAction( tr("<empty>") );
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

	QAction * subNoneAct = subtitleTrackGroup->addAction( tr("&None") );
	subNoneAct->setData(SubData::None);
	subNoneAct->setCheckable(true);
	if (core->mset.current_sub_idx < 0) {
		subNoneAct->setChecked(true);
	}
#ifdef MPV_SUPPORT
	subNoneAct = secondarySubtitleTrackGroup->addAction( tr("&None") );
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
	subFPSGroup->setEnabled(have_ext_subs);

	// Enable or disable subtitle options
	bool e = core->mset.current_sub_idx >= 0;

	if (core->mset.closed_caption_channel !=0 ) e = true; // Enable if using closed captions

	decSubDelayAct->setEnabled(e);
	incSubDelayAct->setEnabled(e);
	subDelayAct->setEnabled(e);
	decSubPosAct->setEnabled(e);
	incSubPosAct->setEnabled(e);
	decSubScaleAct->setEnabled(e);
	incSubScaleAct->setEnabled(e);
	decSubStepAct->setEnabled(e);
	incSubStepAct->setEnabled(e);
}

void TBase::updateTitles() {
	qDebug("Gui::TBase::updateTitles");

	titleGroup->clear(true);
	if (core->mdat.titles.count() == 0) {
		QAction * a = titleGroup->addAction( tr("<empty>") );
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

	// Clear selected. Core::gotCurrentSec will set it.
	core->mdat.chapters.setSelectedID(-1);
	chapterGroup->clear(true);
	if (core->mdat.chapters.count() > 0) {
		Maps::TChapters::TChapterIterator i = core->mdat.chapters.getIterator();
		do {
			i.next();
			const Maps::TChapterData chapter = i.value();
			QAction *a = new QAction(chapterGroup);
			a->setCheckable(true);
			a->setText(chapter.getDisplayName());
			a->setData(chapter.getID());
		} while (i.hasNext());
	} else {
		QAction * a = chapterGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	}

	chapters_menu->addActions( chapterGroup->actions() );
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
			a->setText( QString::number(n) );
			a->setData( n );
		}
	} else {
		QAction * a = angleGroup->addAction( tr("<empty>") );
		a->setEnabled(false);
	}
	angles_menu->addActions( angleGroup->actions() );
}

void TBase::updateRecents() {
	qDebug("Gui::TBase::updateRecents");

	recentfiles_menu->clear();

	int current_items = 0;

	if (pref->history_recents->count() > 0) {
		for (int n=0; n < pref->history_recents->count(); n++) {
			QString i = QString::number( n+1 );
			QString fullname = pref->history_recents->item(n);
			QString filename = fullname;
			QFileInfo fi(fullname);
			//if (fi.exists()) filename = fi.fileName(); // Can be slow

			// Let's see if it looks like a file (no dvd://1 or something)
			if (fullname.indexOf(QRegExp("^.*://.*")) == -1) filename = fi.fileName();

			if (filename.size() > 85) {
				filename = filename.left(80) + "...";
			}

			QString show_name = filename;
			QString title = pref->history_recents->title(n);
			if (!title.isEmpty()) show_name = title;

			QAction * a = recentfiles_menu->addAction( QString("%1. " + show_name ).arg( i.insert( i.size()-1, '&' ), 3, ' ' ));
			a->setStatusTip(fullname);
			a->setData(n);
			connect(a, SIGNAL(triggered()), this, SLOT(openRecent()));
			current_items++;
		}
	} else {
		QAction * a = recentfiles_menu->addAction( tr("<empty>") );
		a->setEnabled(false);
	}

	recentfiles_menu->menuAction()->setVisible( current_items > 0 );
	if (current_items  > 0) {
		recentfiles_menu->addSeparator();
		recentfiles_menu->addAction( clearRecentsAct );
	}
}

void TBase::clearRecentsList() {
	int ret = QMessageBox::question(this, tr("Confirm deletion - SMPlayer"),
				tr("Delete the list of recent files?"),
				QMessageBox::Cancel, QMessageBox::Ok);

	if (ret == QMessageBox::Ok) {
		// Delete items in menu
		pref->history_recents->clear();
		updateRecents();
	}
}

void TBase::updateWidgets() {
	qDebug("Gui::TBase::updateWidgets");

	// Closed caption menu
	ccGroup->setChecked( core->mset.closed_caption_channel );

	// Subfps menu
	subFPSGroup->setChecked( core->mset.external_subtitles_fps );

	// Audio menu
	channelsGroup->setChecked( core->mset.audio_use_channels );
	stereoGroup->setChecked( core->mset.stereo_mode );
	// Disable the unload audio file action if there's no external audio file
	unloadAudioAct->setEnabled( !core->mset.external_audio.isEmpty() );

#if PROGRAM_SWITCH
	// Program menu
	programTrackGroup->setChecked( core->mset.current_program_id );
#endif

	// Aspect ratio
	aspectGroup->setChecked( core->mset.aspect_ratio_id );

	// Rotate
	rotateGroup->setChecked( core->mset.rotate );

#if USE_ADAPTER
	screenGroup->setChecked( pref->adapter );
#endif

	// OSD
	osdGroup->setChecked( (int) pref->osd_level );

	// Deinterlace menu
	deinterlaceGroup->setChecked( core->mset.current_deinterlacer );

	// Auto phase
	phaseAct->setChecked( core->mset.phase_filter );

	// Deblock
	deblockAct->setChecked( core->mset.deblock_filter );

	// Dering
	deringAct->setChecked( core->mset.dering_filter );

	// Gradfun
	gradfunAct->setChecked( core->mset.gradfun_filter );

	// Add noise
	addNoiseAct->setChecked( core->mset.noise_filter );

	// Letterbox
	addLetterboxAct->setChecked( core->mset.add_letterbox );

	// Upscaling
	upscaleAct->setChecked( core->mset.upscaling_filter );


	// Postprocessing
	postProcessingAct->setChecked( core->mset.postprocessing_filter );

	// Denoise submenu
	denoiseGroup->setChecked( core->mset.current_denoiser );

	// Unsharp submenu
	unsharpGroup->setChecked( core->mset.current_unsharp );

	/*
	// Fullscreen button
	fullscreenbutton->setOn(pref->fullscreen); 

	// Mute button
	mutebutton->setOn(core->mset.mute);
	if (core->mset.mute) 
		mutebutton->setPixmap( Images::icon("mute_small") );
	else
		mutebutton->setPixmap( Images::icon("volume_small") );

	// Volume slider
	volumeslider->setValue( core->mset.volume );
	*/

	// Mute menu option
	muteAct->setChecked( (pref->global_volume ? pref->mute : core->mset.mute) );

	// Karaoke menu option
	karaokeAct->setChecked( core->mset.karaoke_filter );

	// Extrastereo menu option
	extrastereoAct->setChecked( core->mset.extrastereo_filter );

	// Volnorm menu option
	volnormAct->setChecked( core->mset.volnorm_filter );

	// Repeat menu option
	repeatAct->setChecked( core->mset.loop );

	// Fullscreen action
	fullscreenAct->setChecked( pref->fullscreen );

	// Time slider
	if (core->state()==Core::Stopped) {
		//FIXME
		//timeslider->setValue( (int) core->mset.current_sec );
	}

	// Video equalizer
	videoEqualizerAct->setChecked( video_equalizer->isVisible() );
	video_equalizer->setBySoftware( pref->use_soft_video_eq );

	// Audio equalizer
	audioEqualizerAct->setChecked( audio_equalizer->isVisible() );

	// TPlaylist
#if !DOCK_PLAYLIST
	//showPlaylistAct->setChecked( playlist->isVisible() );
#endif

#if DOCK_PLAYLIST
	showPlaylistAct->setChecked( playlist->isVisible() );
#endif

	// Compact mode
	compactAct->setChecked( pref->compact_mode );

	// Stay on top
	onTopActionGroup->setChecked( (int) pref->stay_on_top );

	// Flip
	flipAct->setChecked( core->mset.flip );

	// Mirror
	mirrorAct->setChecked( core->mset.mirror );

	// Use custom style
	useCustomSubStyleAct->setChecked( pref->enable_ass_styles );

	// Forced subs
	useForcedSubsOnlyAct->setChecked( pref->use_forced_subs_only );

}

void TBase::updateVideoEqualizer() {
	// Equalizer
	video_equalizer->setContrast( core->mset.contrast );
	video_equalizer->setBrightness( core->mset.brightness );
	video_equalizer->setHue( core->mset.hue );
	video_equalizer->setSaturation( core->mset.saturation );
	video_equalizer->setGamma( core->mset.gamma );
}

void TBase::updateAudioEqualizer() {
	// Audio Equalizer
	AudioEqualizerList l = pref->global_audio_equalizer ? pref->audio_equalizer : core->mset.audio_equalizer;
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
                                "used as default.") );
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
					pref->latest_dir );

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

	Extensions e;
	QString s = MyFileDialog::getOpenFileName(
					   this, tr("Choose a file"), pref->latest_dir,
					   tr("Multimedia") + e.allPlayable().forFilter()+";;" +
					   tr("Video") + e.video().forFilter()+";;" +
					   tr("Audio") + e.audio().forFilter()+";;" +
					   tr("TPlaylists") + e.playlist().forFilter()+";;" +
					   tr("All files") +" (*.*)" );

	if ( !s.isEmpty() ) {
		open(s);
	}
}

void TBase::openRecent() {
	qDebug("Gui::TBase::openRecent");

	QAction *a = qobject_cast<QAction *> (sender());
	if (a) {
		int item = a->data().toInt();
		QString filename = pref->history_recents->item(item);
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

	InputURL d(this);

	// Get url from clipboard
	QString clipboard_text = QApplication::clipboard()->text();
	if ((!clipboard_text.isEmpty()) && (clipboard_text.contains("://")) /*&& (QUrl(clipboard_text).isValid())*/) {
		d.setURL(clipboard_text);
	}

	for (int n=0; n < pref->history_urls->count(); n++) {
		d.setURL( pref->history_urls->url(n) );
	}

	if (d.exec() == QDialog::Accepted ) {
		QString url = d.url();
		if (!url.isEmpty()) {
			pref->history_urls->addUrl(url);
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
	QMessageBox::information( this, tr("SMPlayer - Information"),
			tr("The CDROM / DVD drives are not configured yet.\n"
			   "The configuration dialog will be shown now, "
               "so you can do it."), QMessageBox::Ok);
	
	showPreferencesDialog();
	pref_dialog->showSection( PreferencesDialog::Drives );
}

void TBase::openVCD() {
	qDebug("Gui::TBase::openVCD");

	if (pref->dvd_device.isEmpty()
		|| pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		// TODO: remove pref->vcd_initial_title?
		core->open(DiscName::join(DiscName::VCD, 0, pref->cdrom_device));
	}
}

void TBase::openAudioCD() {
	qDebug("Gui::TBase::openAudioCD");

	if ( (pref->dvd_device.isEmpty()) || 
         (pref->cdrom_device.isEmpty()) )
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
			core->open(DiscName::joinDVD(pref->dvd_device, pref->use_dvdnav));
		}
	}
}

void TBase::openDVDFromFolder() {
	qDebug("Gui::TBase::openDVDFromFolder");

	if (playlist->maybeSave()) {
		InputDVDDirectory *d = new InputDVDDirectory(this);
		d->setFolder( pref->last_dvd_directory );

		if (d->exec() == QDialog::Accepted) {
			qDebug("Gui::TBase::openDVDFromFolder: accepted");
			openDVDFromFolder( d->folder() );
		}

		delete d;
	}
}

void TBase::openDVDFromFolder(const QString &directory) {

	pref->last_dvd_directory = directory;
	core->open( DiscName::joinDVD(directory, pref->use_dvdnav) );
}

/**
 * Minimal TBase abstraction for calling openBluRay. It's called from
 * OpenBluRayFromFolder()
 */
void TBase::openBluRayFromFolder(QString directory) {

	pref->last_dvd_directory = directory;
	core->open(DiscName::join(DiscName::BLURAY, 0, directory));
}

/**
 * Attempts to open a bluray from pref->bluray_device. If not set, calls configureDiscDevices.
 * If successful, calls Core::OpenBluRay(QString)
 */
void TBase::openBluRay() {
	qDebug("Gui::TBase::openBluRay");

	if (pref->dvd_device.isEmpty()
		|| pref->cdrom_device.isEmpty()
		|| pref->bluray_device.isEmpty()) {
		configureDiscDevices();
	} else {
		core->open(DiscName::join(DiscName::BLURAY, 0, pref->bluray_device));
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

	Extensions e;
    QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Subtitles") + e.subtitles().forFilter()+ ";;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadSub(s);
}

void TBase::setInitialSubtitle(const QString & subtitle_file) {
	qDebug("Gui::TBase::setInitialSubtitle: '%s'", subtitle_file.toUtf8().constData());

	core->setInitialSubtitle(subtitle_file);
}

void TBase::loadAudioFile() {
	qDebug("Gui::TBase::loadAudioFile");

	exitFullscreenIfNeeded();

	Extensions e;
	QString s = MyFileDialog::getOpenFileName(
        this, tr("Choose a file"), 
	    pref->latest_dir, 
        tr("Audio") + e.audio().forFilter()+";;" +
        tr("All files") +" (*.*)" );

	if (!s.isEmpty()) core->loadAudioFile(s);
}

void TBase::helpFirstSteps() {
	QDesktopServices::openUrl(QString(URL_FIRST_STEPS "?version=%1").arg(Version::printable()));
}

void TBase::helpFAQ() {
	QString url = URL_FAQ;
	/* if (!pref->language.isEmpty()) url += QString("?tr_lang=%1").arg(pref->language); */
	QDesktopServices::openUrl( QUrl(url) );
}

void TBase::helpCLOptions() {
	if (clhelp_window == 0) {
		clhelp_window = new LogWindow(this);
	}
	clhelp_window->setWindowTitle( tr("SMPlayer command line options") );
	clhelp_window->setHtml(CLHelp::help(true));
	clhelp_window->show();
}

void TBase::helpCheckUpdates() {
#ifdef UPDATE_CHECKER
	update_checker->check();
#else
	QString url = QString(URL_CHANGES "?version=%1").arg(Version::with_revision());
	QDesktopServices::openUrl( QUrl(url) );
#endif
}

void TBase::helpShowConfig() {
	QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::configPath()));
}

#ifdef REMINDER_ACTIONS
void TBase::helpDonate() {
	ShareDialog d(this);
	d.showRemindCheck(false);

	#ifdef SHAREWIDGET
	d.setActions(sharewidget->actions());
	#endif

	d.exec();
	int action = d.actions();
	qDebug("Gui::TBase::helpDonate: action: %d", action);

	if (action > 0) {
		#ifdef SHAREWIDGET
		sharewidget->setActions(action);
		#else
		QSettings * set = Global::settings;
		set->beginGroup("reminder");
		set->setValue("action", action);
		set->endGroup();
		#endif
	}
}
#endif

void TBase::helpAbout() {
	About d(this);
	d.exec();
}

#ifdef SHARE_MENU
void TBase::shareSMPlayer() {
	QString text = QString("SMPlayer - Free Media Player with built-in codecs that can play and download Youtube videos").replace(" ","+");
	QString url = URL_HOMEPAGE;

	if (sender() == twitterAct) {
		QDesktopServices::openUrl(QUrl("http://twitter.com/intent/tweet?text=" + text + "&url=" + url + "/&via=smplayer_dev"));
	}
	else
	if (sender() == gmailAct) {
		QDesktopServices::openUrl(QUrl("https://mail.google.com/mail/?view=cm&fs=1&to&su=" + text + "&body=" + url + "&ui=2&tf=1&shva=1"));
	}
	else
	if (sender() == yahooAct) {
		QDesktopServices::openUrl(QUrl("http://compose.mail.yahoo.com/?To=&Subject=" + text + "&body=" + url));
	}
	else
	if (sender() == hotmailAct) {
		QDesktopServices::openUrl(QUrl("http://www.hotmail.msn.com/secure/start?action=compose&to=&subject=" + text + "&body=" + url));
	}
	else
	if (sender() == facebookAct) {
		QDesktopServices::openUrl(QUrl("http://www.facebook.com/sharer.php?u=" + url + "&t=" + text));

		#ifdef REMINDER_ACTIONS
		QSettings * set = Global::settings;
		set->beginGroup("reminder");
		set->setValue("action", 2);
		set->endGroup();
		#endif
	}
}
#endif

void TBase::showGotoDialog() {
	TimeDialog d(this);
	d.setLabel(tr("&Jump to:"));
	d.setWindowTitle(tr("SMPlayer - Seek"));
	d.setMaximumTime( (int) core->mdat.duration);
	d.setTime( (int) core->mset.current_sec);
	if (d.exec() == QDialog::Accepted) {
		core->goToSec( d.time() );
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
	Stereo3dDialog d(this);
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

void TBase::toggleFullscreen(bool b) {
	qDebug("Gui::TBase::toggleFullscreen: %d", b);

	if (b==pref->fullscreen) {
		// Nothing to do
		qDebug("Gui::TBase::toggleFullscreen: nothing to do, returning");
		return;
	}

	pref->fullscreen = b;

	// If using mplayer window
	if (pref->use_mplayer_window) {
		core->changeFullscreenMode(b);
		updateWidgets();
		return;
	}

	if (!panel->isVisible()) return; // mplayer window is not used.

	if (pref->fullscreen) {
		compactAct->setEnabled(false);

		if (pref->restore_pos_after_fullscreen) {
			win_pos = pos();
			win_size = size();
		}

		was_maximized = isMaximized();
		qDebug("Gui::TBase::toggleFullscreen: was_maximized: %d", was_maximized);

		aboutToEnterFullscreen();

		#ifdef Q_OS_WIN
		// Hack to avoid the windows taskbar to be visible on Windows XP
		if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA) {
			if (!pref->pause_when_hidden) hide();
		}
		#endif

		showFullScreen();

	} else {
		showNormal();

		if (was_maximized) showMaximized(); // It has to be called after showNormal()

		aboutToExitFullscreen();

		if (pref->restore_pos_after_fullscreen) {
			move( win_pos );
			resize( win_size );
		}

		compactAct->setEnabled(true);
	}

	updateWidgets();

	if (pref->add_blackborders_on_fullscreen &&  !core->mset.add_letterbox) {
		core->changeLetterboxOnFullscreen(b);
	}

	setFocus(); // Fixes bug #2493415
}

void TBase::aboutToEnterFullscreen() {
	//qDebug("Gui::TBase::aboutToEnterFullscreen");

	mplayerwindow->aboutToEnterFullscreen();

	if (!pref->compact_mode) {
		menuBar()->hide();
		statusBar()->hide();
	}
}

void TBase::aboutToExitFullscreen() {
	//qDebug("Gui::TBase::aboutToExitFullscreen");

	mplayerwindow->aboutToExitFullscreen();

	if (!pref->compact_mode) {
		menuBar()->show();
		statusBar()->show();
	}
	//qDebug("Gui::TBase::aboutToExitFullscreen done");
}


void TBase::leftClickFunction() {
	qDebug("Gui::TBase::leftClickFunction");

	if (core->mdat.detected_type == MediaData::TYPE_DVDNAV
		&& mplayerwindow->videoLayer()->underMouse()) {
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

void TBase::processFunction(QString function) {
	qDebug("Gui::TBase::processFunction: '%s'", function.toUtf8().data());

	//parse args for checkable actions
	QRegExp func_rx("(.*) (true|false)");
	bool value = false;
	bool checkableFunction = false;

	if(func_rx.indexIn(function) > -1){
		function = func_rx.cap(1);
		value = (func_rx.cap(2) == "true");
		checkableFunction = true;
	} //end if

	QAction * action = ActionsEditor::findAction(this, function);
	if (!action) action = ActionsEditor::findAction(playlist, function);

	if (action) {
		qDebug("Gui::TBase::processFunction: action found");

		if (!action->isEnabled()) {
			qDebug("Gui::TBase::processFunction: action is disabled, doing nothing");
			return;
		}

		if (action->isCheckable()){
			if(checkableFunction)
				action->setChecked(value);
			else
				//action->toggle();
				action->trigger();
		}else{
			action->trigger();
		}
	}
}

void TBase::runActions(QString actions) {
	qDebug("Gui::TBase::runActions");

	actions = actions.simplified(); // Remove white space

	QAction * action;
	QStringList actionsList = actions.split(" ");

	for (int n = 0; n < actionsList.count(); n++) {
		QString actionStr = actionsList[n];
		QString par = ""; //the parameter which the action takes

		//set par if the next word is a boolean value
		if ( (n+1) < actionsList.count() ) {
			if ( (actionsList[n+1].toLower() == "true") || (actionsList[n+1].toLower() == "false") ) {
				par = actionsList[n+1].toLower();
				n++;
			} //end if
		} //end if

		action = ActionsEditor::findAction(this, actionStr);
		if (!action) action = ActionsEditor::findAction(playlist, actionStr);

		if (action) {
			qDebug("Gui::TBase::runActions: running action: '%s' (par: '%s')",
				   actionStr.toUtf8().data(), par.toUtf8().data() );

			if (action->isCheckable()) {
				if (par.isEmpty()) {
					//action->toggle();
					action->trigger();
				} else {
					action->setChecked( (par == "true") );
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

#if REPORT_OLD_MPLAYER
void TBase::checkMplayerVersion() {
	qDebug("Gui::TBase::checkMplayerVersion");

	// Qt 4.3.5 is crazy, I can't popup a messagebox here, it calls 
	// this function once and again when the messagebox is shown

	//if ( (pref->mplayer_detected_version > 0) && (!MplayerVersion::isMplayerAtLeast(25158)) ) {
	//	QTimer::singleShot(1000, this, SLOT(displayWarningAboutOldMplayer()));
	//}
}

void TBase::displayWarningAboutOldMplayer() {
	qDebug("Gui::TBase::displayWarningAboutOldMplayer");

	if (!pref->reported_mplayer_is_old) {
		QMessageBox::warning(this, tr("Warning - Using old MPlayer"),
			tr("The version of MPlayer (%1) installed on your system "
               "is obsolete. SMPlayer can't work well with it: some "
               "options won't work, subtitle selection may fail...")
               .arg(MplayerVersion::toString(pref->mplayer_detected_version)) +
            "<br><br>" + 
            tr("Please, update your MPlayer.") +
            "<br><br>" + 
            tr("(This warning won't be displayed anymore)") );

		pref->reported_mplayer_is_old = true;
	}
	//else
	//statusBar()->showMessage( tr("Using an old MPlayer, please update it"), 10000 );
}
#endif

#ifdef CHECK_UPGRADED
void TBase::checkIfUpgraded() {
	qDebug("Gui::TBase::checkIfUpgraded");

	if ( (pref->check_if_upgraded) && (pref->smplayer_stable_version != Version::stable()) ) {
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

#if defined(REMINDER_ACTIONS) && !defined(SHAREWIDGET)
void TBase::checkReminder() {
	qDebug("Gui::TBase::checkReminder");

	if (core->state() == Core::Playing) return;

	QSettings * set = Global::settings;
	set->beginGroup("reminder");
	int count = set->value("count", 0).toInt();
	count++;
	set->setValue("count", count);
	int action = set->value("action", 0).toInt();
	bool dont_show = set->value("dont_show_anymore", false).toBool();
	set->endGroup();

#if 1
	if (dont_show) return;

	if (action != 0) return;
	if ((count != 25) && (count != 45)) return;
#endif

	ShareDialog d(this);
	//d.showRemindCheck(false);
	d.exec();
	action = d.actions();
	qDebug("Gui::TBase::checkReminder: action: %d", action);

	if (!d.isRemindChecked()) {
		set->beginGroup("reminder");
		set->setValue("dont_show_anymore", true);
		set->endGroup();
	}

	if (action > 0) {
		set->beginGroup("reminder");
		set->setValue("action", action);
		set->endGroup();
	}

	//qDebug() << "size:" << d.size();
}
#endif

#ifdef YOUTUBE_SUPPORT
void TBase::YTNoSslSupport() {
	qDebug("Gui::TBase::YTNoSslSupport");
	QMessageBox::warning(this, tr("Connection failed"),
		tr("The video you requested needs to open a HTTPS connection.") +"<br>"+
		tr("Unfortunately the OpenSSL component, required for it, is not available in your system.") +"<br>"+
		tr("Please, visit %1 to know how to fix this problem.")
			.arg("<a href=\"" URL_OPENSSL_INFO "\">" + tr("this link") + "</a>") );
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
	static CodeDownloader * downloader = 0;
	if (!downloader) downloader = new CodeDownloader(this);
	downloader->saveAs(Paths::configPath() + "/yt.js");
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

void TBase::dragEnterEvent( QDragEnterEvent *e ) {
	qDebug("Gui::TBase::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void TBase::dropEvent( QDropEvent *e ) {
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

void TBase::showPopupMenu() {
	showPopupMenu(QCursor::pos());
}

void TBase::showPopupMenu( QPoint p ) {
	//qDebug("Gui::TBase::showPopupMenu: %d, %d", p.x(), p.y());
	popup->move( p );
	popup->show();
}

/*
void TBase::mouseReleaseEvent( QMouseEvent * e ) {
	qDebug("Gui::TBase::mouseReleaseEvent");

	if (e->button() == Qt::LeftButton) {
		e->accept();
		emit leftClicked();
	}
	else
	if (e->button() == Qt::MidButton) {
		e->accept();
		emit middleClicked();
	}
	//
	//else
	//if (e->button() == Qt::RightButton) {
	//	showPopupMenu( e->globalPos() );
    //}
	//
	else 
		e->ignore();
}

void TBase::mouseDoubleClickEvent( QMouseEvent * e ) {
	e->accept();
	emit doubleClicked();
}
*/

/*
void TBase::wheelEvent( QWheelEvent * e ) {
	qDebug("Gui::TBase::wheelEvent: delta: %d", e->delta());
	e->accept();

	if (e->orientation() == Qt::Vertical) {
	    if (e->delta() >= 0)
	        emit wheelUp();
	    else
	        emit wheelDown();
	} else {
		qDebug("Gui::TBase::wheelEvent: horizontal event received, doing nothing");
	}
}
*/

// Called when a video has started to play
void TBase::enterFullscreenOnPlay() {
	qDebug("Gui::TBase::enterFullscreenOnPlay: arg_start_in_fullscreen: %d, pref->start_in_fullscreen: %d", arg_start_in_fullscreen, pref->start_in_fullscreen);

	if (arg_start_in_fullscreen != 0) {
		if ( (arg_start_in_fullscreen == 1) || (pref->start_in_fullscreen) ) {
			if (!pref->fullscreen) toggleFullscreen(true);
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

void TBase::displayState(Core::State state) {
	qDebug("Gui::TBase::displayState: %s", core->stateToString().toUtf8().data());
	switch (state) {
		case Core::Playing:	statusBar()->showMessage( tr("Playing %1").arg(core->mdat.filename), 2000); break;
		case Core::Paused:	statusBar()->showMessage( tr("Pause") ); break;
		case Core::Stopped:	statusBar()->showMessage( tr("Stop") , 2000); break;
	}
	if (state == Core::Stopped) setWindowCaption( "SMPlayer" );

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	just_stopped = false;
	
	if (state == Core::Stopped) {
		just_stopped = true;
		int time = 1000 * 60; // 1 minute
		QTimer::singleShot( time, this, SLOT(clear_just_stopped()) );
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
	//qDebug( "Gui::TBase::displayTime: %f", sec);

	QString time =
		Helper::formatTime((int) sec) + " / " +
		Helper::formatTime(qRound(core->mdat.duration));

	emit timeChanged( time );
}

void TBase::gotDuration(double duration) {
	Q_UNUSED(duration)

	// Uses duration in text
	gotCurrentTime( core->mset.current_sec );
}

void TBase::toggleDoubleSize() {
	if (pref->size_factor != 1.0)
		core->changeSize(100);
	else core->changeSize(200);
}

// Slot called by signal needResize
void TBase::resizeWindow(int w, int h) {
	qDebug("Gui::TBase::resizeWindow: %d, %d", w, h);

	// Set first time if pref->save_window_size_on_exit selected
	bool block = block_resize;
	block_resize = false;

	if (panel->isVisible()) {
		// Don't resize if any mouse buttons down, like when dragging.
		// Button state is synchronized to events, so can be old.
		if (block || (pref->resize_method == Preferences::Never)
			|| QApplication::mouseButtons()) {
			return;
		}
	} else {
		panel->show();
	}

	// If fullscreen, don't resize!
	if (!pref->fullscreen)
		resizeMainWindow(w, h);
}

void TBase::resizeMainWindow(int w, int h, bool try_twice) {
	qDebug("Gui::TBase::resizeMainWindow: size to scale: %d, %d", w, h);

	// Adjust for selected size and aspect.
	QSize video_size = mplayerwindow->getAdjustedSize(w, h, pref->size_factor);

	if (video_size == panel->size()) {
		qDebug("Gui::TBase::resizeMainWindow: the panel size is already the required size. Doing nothing.");
		return;
	}

	QSize new_size = size() + video_size - panel->size();

#if USE_MINIMUMSIZE
	int minimum_width = minimumSizeHint().width();
	if (pref->gui_minimum_width != 0) minimum_width = pref->gui_minimum_width;
	if (new_size.width() < minimum_width) {
		qDebug("Gui::TBase::resizeMainWindow: width is too small, setting width to %d", minimum_width);
		new_size = QSize(minimum_width, new_size.height());
		try_twice = false;
	}
#endif

	qDebug("Gui::TBase::resizeMainWindow resizing from %d x %d to %d x %d",
		   width(), height(), new_size.width(), new_size.height());
	resize(new_size);

	if (panel->size() == video_size) {
		qDebug("Gui::TBase::resizeMainWindow succeeded");
	} else {
		// TODO: Resizing the main window can change the height of the control
		// bar. On my system when the volume slider becomes visible, the  control
		// bar grows with two pixels in height. This changes the height of the
		// panel during resize. For now, resize once again, using the new panel
		// height.
		if (try_twice) {
			qDebug("Gui::TBase::resizeMainWindow panel size now %d x %d. Wanted size %d x %d. Trying a second time",
				   panel->size().width(), panel->size().height(),
				   video_size.width(), video_size.height());
			resizeMainWindow(w, h, false);
		} else {
			qWarning("Gui::TBase::resizeMainWindow failed. Panel size now %d x %d. Wanted size %d x %d",
					 panel->size().width(), panel->size().height(),
					 video_size.width(), video_size.height());
		}
	}
}

void TBase::hidePanel() {
	qDebug("Gui::TBase::hidePanel");

	if (panel->isVisible()) {
		// Exit from fullscreen mode 
	    if (pref->fullscreen) { toggleFullscreen(false); update(); }

		// Exit from compact mode first
		if (pref->compact_mode) toggleCompactMode(false);

		//resizeWindow( size().width(), 0 );
		int width = size().width();
		if (width > pref->default_size.width()) width = pref->default_size.width();
		resize( width, size().height() - panel->size().height() );
		panel->hide();

		// Disable compact mode
		//compactAct->setEnabled(false);
	}
}

void TBase::slotNoVideo() {
	qDebug("Gui::TBase::slotNoVideo");

	block_resize = false;

	// TODO: remove ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
#if ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
	if (pref->hide_video_window_on_audio_files) {
		hidePanel();
	} else {
		mplayerwindow->showLogo();
	}
#else
	hidePanel();
#endif

}

void TBase::displayGotoTime(int t) {

	int jump_time = qRound(core->mdat.duration * t / core->positionMax());
	QString s = tr("Jump to %1").arg( Helper::formatTime(jump_time) );
	statusBar()->showMessage( s, 1000 );

	if (pref->fullscreen) {
		core->displayTextOnOSD( s );
	}
}

void TBase::goToPosOnDragging(int t) {
	if (pref->update_while_seeking) {
#if ENABLE_DELAYED_DRAGGING
		core->goToPosition(t);
#else
		if ( ( t % 4 ) == 0 ) {
			qDebug("Gui::TBase::goToPosOnDragging: %d", t);
			core->goToPosition(t);
		}
#endif
	}
}

void TBase::toggleCompactMode() {
	toggleCompactMode( !pref->compact_mode );
}

void TBase::toggleCompactMode(bool b) {
	qDebug("Gui::TBase::toggleCompactMode: %d", b);

	if (b) 
		aboutToEnterCompactMode();
	else
		aboutToExitCompactMode();

	pref->compact_mode = b;
	updateWidgets();
}

void TBase::aboutToEnterCompactMode() {
	menuBar()->hide();
	statusBar()->hide();
}

void TBase::aboutToExitCompactMode() {
	menuBar()->show();
	statusBar()->show();
}

void TBase::setStayOnTop(bool b) {
	qDebug("Gui::TBase::setStayOnTop: %d", b);

	if ( (b && (windowFlags() & Qt::WindowStaysOnTopHint)) ||
         (!b && (!(windowFlags() & Qt::WindowStaysOnTopHint))) )
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
	}
	else {
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	}

	move(old_pos);

	if (visible) {
		show();
	}

	ignore_show_hide_events = false;
}

void TBase::changeStayOnTop(int stay_on_top) {
	switch (stay_on_top) {
		case Preferences::AlwaysOnTop : setStayOnTop(true); break;
		case Preferences::NeverOnTop  : setStayOnTop(false); break;
		case Preferences::WhilePlayingOnTop : setStayOnTop((core->state() == Core::Playing)); break;
	}

	pref->stay_on_top = (Preferences::OnTop) stay_on_top;
	updateWidgets();
}

void TBase::checkStayOnTop(Core::State state) {
	qDebug("Gui::TBase::checkStayOnTop");
    if ((!pref->fullscreen) && (pref->stay_on_top == Preferences::WhilePlayingOnTop)) {
		setStayOnTop((state == Core::Playing));
	}
}

void TBase::toggleStayOnTop() {
	if (pref->stay_on_top == Preferences::AlwaysOnTop) 
		changeStayOnTop(Preferences::NeverOnTop);
	else
	if (pref->stay_on_top == Preferences::NeverOnTop) 
		changeStayOnTop(Preferences::AlwaysOnTop);
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
	QFile file( filename );
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
		QString qss_file = Paths::configPath() + "/themes/" + pref->iconset + "/main.css";
		if (!QFile::exists(qss_file)) {
			qss_file = Paths::themesPath() +"/"+ pref->iconset + "/main.css";
		}

		// Check style.qss
		if (!QFile::exists(qss_file)) {
			qss_file = Paths::configPath() + "/themes/" + pref->iconset + "/style.qss";
			if (!QFile::exists(qss_file)) {
				qss_file = Paths::themesPath() +"/"+ pref->iconset + "/style.qss";
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

void TBase::loadActions() {
	qDebug("Gui::TBase::loadActions");
	ActionsEditor::loadFromConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::loadFromConfig(playlist, settings);
#endif

	actions_list = ActionsEditor::actionsNames(this);
#if !DOCK_PLAYLIST
	actions_list += ActionsEditor::actionsNames(playlist);
#endif
}

void TBase::saveActions() {
	qDebug("Gui::TBase::saveActions");

	ActionsEditor::saveToConfig(this, settings);
#if !DOCK_PLAYLIST
	ActionsEditor::saveToConfig(playlist, settings);
#endif
}

#if QT_VERSION < 0x050000
void TBase::showEvent( QShowEvent * ) {
	qDebug("Gui::TBase::showEvent");

	if (ignore_show_hide_events) return;

	//qDebug("Gui::TBase::showEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Paused)) {
		qDebug("Gui::TBase::showEvent: unpausing");
		core->play();
	}
}

void TBase::hideEvent( QHideEvent * ) {
	qDebug("Gui::TBase::hideEvent");

	if (ignore_show_hide_events) return;

	//qDebug("Gui::TBase::hideEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if ((pref->pause_when_hidden) && (core->state() == Core::Playing)) {
		qDebug("Gui::TBase::hideEvent: pausing");
		core->pause();
	}
}
#else
// Qt 5 doesn't call showEvent / hideEvent when the window is minimized or unminimized
bool TBase::event(QEvent * e) {
	//qDebug("Gui::TBase::event: %d", e->type());

	bool result = QWidget::event(e);
	if ((ignore_show_hide_events) || (!pref->pause_when_hidden)) return result;

	if (e->type() == QEvent::WindowStateChange) {
		qDebug("Gui::TBase::event: WindowStateChange");

		if (isMinimized()) {
			was_minimized = true;
			if (core->state() == Core::Playing) {
				qDebug("Gui::TBase::event: pausing");
				core->pause();
			}
		}
	}

	if ((e->type() == QEvent::ActivationChange) && (isActiveWindow())) {
		qDebug("Gui::TBase::event: ActivationChange: %d", was_minimized);

		if ((!isMinimized()) && (was_minimized)) {
			was_minimized = false;
			if (core->state() == Core::Paused) {
				qDebug("Gui::TBase::showEvent: unpausing");
				core->play();
			}
		}
	}

	return result;
}
#endif

void TBase::moveEvent(QMoveEvent *) {
	if (mplayerwindow)
		mplayerwindow->main_window_moved = true;
}

void TBase::askForMplayerVersion(QString line) {
	qDebug("Gui::TBase::askForMplayerVersion: %s", line.toUtf8().data());

	if (pref->mplayer_user_supplied_version <= 0) {
		InputMplayerVersion d(this);
		d.setVersion( pref->mplayer_user_supplied_version );
		d.setVersionFromOutput(line);
		if (d.exec() == QDialog::Accepted) {
			pref->mplayer_user_supplied_version = d.version();
			qDebug("Gui::TBase::askForMplayerVersion: user supplied version: %d", pref->mplayer_user_supplied_version);
		}
	} else {
		qDebug("Gui::TBase::askForMplayerVersion: already have a version supplied by user, so no asking");
	}
}

void TBase::showExitCodeFromPlayer(int exit_code) {
	qDebug("Gui::TBase::showExitCodeFromPlayer: %d", exit_code);

	QString msg = tr("%1 has finished unexpectedly.").arg(PLAYER_NAME)
			+ " " + tr("Exit code: %1").arg(exit_code);

	if (!pref->report_mplayer_crashes) {
		qDebug("Gui::TBase::showExitCodeFromPlayer: error reporting is turned off");
		displayMessage(msg, 6000);
		return;
	}

	if (exit_code != 255 ) {
		ErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(PLAYER_NAME));
		d.setText(msg);
#ifdef LOG_MPLAYER
		d.setLog( mplayer_log );
#endif
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
		ErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(PLAYER_NAME));
		if (e == QProcess::FailedToStart) {
			d.setText(tr("%1 failed to start.").arg(PLAYER_NAME) + " " + 
                         tr("Please check the %1 path in preferences.").arg(PLAYER_NAME));
		} else {
			d.setText(tr("%1 has crashed.").arg(PLAYER_NAME) + " " + 
                      tr("See the log for more info."));
		}
#ifdef LOG_MPLAYER
		d.setLog( mplayer_log );
#endif
		d.exec();
	}
}


#ifdef FIND_SUBTITLES
void TBase::showFindSubtitlesDialog() {
	qDebug("Gui::TBase::showFindSubtitlesDialog");

	if (!find_subs_dialog) {
		find_subs_dialog = new FindSubtitlesWindow(this, Qt::Window | Qt::WindowMinMaxButtonsHint);
		find_subs_dialog->setSettings(Global::settings);
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
	QDesktopServices::openUrl( QUrl("http://www.opensubtitles.org/upload") );
}
#endif

#ifdef VIDEOPREVIEW
void TBase::showVideoPreviewDialog() {
	qDebug("Gui::TBase::showVideoPreviewDialog");

	if (video_preview == 0) {
		video_preview = new VideoPreview( pref->mplayer_bin, this );
		video_preview->setSettings(Global::settings);
	}

	if (!core->mdat.filename.isEmpty()) {
		video_preview->setVideoFile(core->mdat.filename);

		// DVD
		if (MediaData::isDVD(core->mdat.selected_type)) {
			QString file = core->mdat.filename;
			DiscData disc_data = DiscName::split(file);
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

	if ( (video_preview->showConfigDialog(this)) && (video_preview->createThumbnails()) ) {
		video_preview->show();
		video_preview->adjustWindowSize();
	}
}
#endif

#ifdef YOUTUBE_SUPPORT
void TBase::showTubeBrowser() {
	qDebug("Gui::TBase::showTubeBrowser");
	QString exec = Paths::appPath() + "/smtube";
	qDebug("Gui::TBase::showTubeBrowser: '%s'", exec.toUtf8().constData());
	if (!QProcess::startDetached(exec, QStringList())) {
		QMessageBox::warning(this, "SMPlayer",
			tr("The YouTube Browser is not installed.") +"<br>"+ 
			tr("Visit %1 to get it.").arg("<a href=http://www.smtube.org>http://www.smtube.org</a>"));
	}
}
#endif

// Language change stuff
void TBase::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMainWindow::changeEvent(e);
	}
}

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
/* Disable screensaver by event */
bool TBase::winEvent ( MSG * m, long * result ) {
	//qDebug("Gui::TBase::winEvent");
	if (m->message==WM_SYSCOMMAND) {
		if ((m->wParam & 0xFFF0)==SC_SCREENSAVE || (m->wParam & 0xFFF0)==SC_MONITORPOWER) {
			qDebug("Gui::TBase::winEvent: received SC_SCREENSAVE or SC_MONITORPOWER");
			qDebug("Gui::TBase::winEvent: avoid_screensaver: %d", pref->avoid_screensaver);
			qDebug("Gui::TBase::winEvent: playing: %d", core->state()==Core::Playing);
			qDebug("Gui::TBase::winEvent: video: %d", !core->mdat.novideo);
			
			if ((pref->avoid_screensaver) && (core->state()==Core::Playing) && (!core->mdat.novideo)) {
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

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
#include "desktop.h"
#include "discname.h"
#include "extensions.h"
#include "log.h"
#include "colorutils.h"
#include "images.h"
#include "helper.h"
#include "mediadata.h"
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
#include "gui/action/menu.h"
#include "gui/action/menuopen.h"
#include "gui/action/menuplay.h"
#include "gui/action/menuvideo.h"
#include "gui/action/menuaudio.h"
#include "gui/action/menusubtitle.h"
#include "gui/action/menubrowse.h"
#include "gui/action/menuoptions.h"
#include "gui/action/menuhelp.h"

#include "gui/links.h"
#include "gui/errordialog.h"
#include "gui/logwindow.h"
#include "gui/playlist.h"
#include "gui/autohidetimer.h"
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
#include "gui/action/tvlist.h"

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
	, file_properties_dialog(0)
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

	setWindowTitle("SMPlayer");
	setAcceptDrops(true);

	// Reset size factor
	if (pref->save_window_size_on_exit) {
		force_resize = false;
	} else {
		force_resize = true;
		pref->size_factor = 1.0;
	}
	// Resize window to default size
	resize(pref->default_size);

	// Create objects:
	createPanel();
	setCentralWidget(panel);

	createPlayerWindow();
	createCore();
	createPlaylist();
	createVideoEqualizer();
	createAudioEqualizer();

	log_window = new TLogWindow(this, true);

	createActions();
	createToolbars();
	createMenus();

	setupNetworkProxy();
	changeStayOnTop(pref->stay_on_top);

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

TBase::~TBase() {
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

	// Pause messages before exiting fullscreen
	connect(this, SIGNAL(aboutToExitFullscreenSignal()),
			playerwindow, SLOT(pauseMessages()));
}

void TBase::createCore() {

	core = new TCore(this, playerwindow);

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

	connect(core, SIGNAL(showMessage(const QString&, int)),
			 this, SLOT(displayMessage(const QString&, int)));
	connect(core, SIGNAL(showMessage(const QString&)),
			 this, SLOT(displayMessage(const QString&)));

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
	connect(playerwindow, SIGNAL(moveOSD(const QPoint&)),
			 core, SLOT(setOSDPos(const QPoint&)));
	connect(playerwindow, SIGNAL(showMessage(const QString&, int, int)),
			 core, SLOT(displayMessage(const QString&, int, int)));
}

void TBase::createPlaylist() {

	playlist = new TPlaylist(this, core, 0);
	connect(playlist, SIGNAL(playlistEnded()),
			 this, SLOT(playlistHasFinished()));
	connect(playlist, SIGNAL(displayMessage(const QString&, int)),
			this, SLOT(displayMessage(const QString&, int)));
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

	showContextMenuAct = new TAction(this, "show_context_menu", QT_TR_NOOP("Show context menu"));
	connect(showContextMenuAct, SIGNAL(triggered()), this, SLOT(showContextMenu()));

	nextWheelFunctionAct = new TAction(this, "next_wheel_function", QT_TR_NOOP("Next wheel function"));
	connect(nextWheelFunctionAct, SIGNAL(triggered()), core, SLOT(nextWheelFunction()));

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
	viewMenuBarAct = new TAction(this, "toggle_menubar", QT_TR_NOOP("Me&nu bar"), "", Qt::Key_F2);
	viewMenuBarAct->setCheckable(true);
	viewMenuBarAct->setChecked(true);
	connect(viewMenuBarAct, SIGNAL(toggled(bool)), menuBar(), SLOT(setVisible(bool)));

	// Toolbars
	editToolbarAct = new TAction(this, "edit_toolbar1", QT_TR_NOOP("Edit main &toolbar"));
	editToolbar2Act = new TAction(this, "edit_toolbar2", QT_TR_NOOP("Edit extra t&oolbar"));

	// Control bar
	editControlBarAct = new TAction(this, "edit_controlbar", QT_TR_NOOP("Edit control &bar"));

	// Status bar
	viewStatusBarAct = new TAction(this, "toggle_statusbar", QT_TR_NOOP("&Status bar"), "", Qt::Key_F6);
	viewStatusBarAct->setCheckable(true);
	viewStatusBarAct->setChecked(true);
	connect(viewStatusBarAct, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
} // createActions

void TBase::createMenus() {

	// MENUS
	openMenu = new TMenuOpen(this, core, playlist);
	menuBar()->addMenu(openMenu);
	playMenu = new TMenuPlay(this, core, playlist);
	menuBar()->addMenu(playMenu);
	videoMenu = new TMenuVideo(this, core, playerwindow, video_equalizer);
	menuBar()->addMenu(videoMenu);
	audioMenu = new TMenuAudio(this, core, audio_equalizer);
	menuBar()->addMenu(audioMenu);
	subtitleMenu = new TMenuSubtitle(this, core);
	menuBar()->addMenu(subtitleMenu);
	browseMenu = new TMenuBrowse(this, core);
	menuBar()->addMenu(browseMenu);

	// statusbar_menu added to toolbar_menu by createToolbarMenu()
	// and filled by descendants::createMenus()
	statusbar_menu = new QMenu(this);
	toolbar_menu = createToolbarMenu();
	optionsMenu = new TMenuOptions(this, core, toolbar_menu, playlist, log_window);
	menuBar()->addMenu(optionsMenu);

	helpMenu = new TMenuHelp(this);
	menuBar()->addMenu(helpMenu);

	// POPUP MENU
	popup = new QMenu(this);
	popup->addMenu(openMenu);
	popup->addMenu(playMenu);
	popup->addMenu(videoMenu);
	popup->addMenu(audioMenu);
	popup->addMenu(subtitleMenu);
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
	actions << "audiotrack_menu" << "subtitlestrack_menu";
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
	//qDebug("Gui::TBase::setupNetworkProxy");

	QNetworkProxy proxy;

	if (pref->use_proxy && !pref->proxy_host.isEmpty()) {
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

	emit enableActions(!b, !core->mdat.noVideo(), core->mdat.audios.count() > 0);

	// Time slider
	timeslider_action->enable(b);
}

void TBase::enableActionsOnPlaying() {
	qDebug("Gui::TBase::enableActionsOnPlaying");
	setActionsEnabled(true);
}

void TBase::disableActionsOnStop() {
	qDebug("Gui::TBase::disableActionsOnStop");
	setActionsEnabled(false);
}

void TBase::retranslateStrings() {
	qDebug("Gui::TBase::retranslateStrings");

	setWindowIcon(Images::icon("logo", 64));

	// Menu Play
	playMenu->retranslateStrings();

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

	// PlayerWindow
	playerwindow->retranslateStrings();

	// Playlist
	playlist->retranslateStrings();

	// Log window
	log_window->retranslateStrings();

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
	file_properties_dialog = new TFilePropertiesDialog(this, &core->mdat);
	file_properties_dialog->setModal(false);
	connect(file_properties_dialog, SIGNAL(applied()),
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
		QAction* a = all_actions[i];
		if (a->objectName().isEmpty() || a->isSeparator()) {
			all_actions.removeAt(i);
		}
	}

	return all_actions;
}

TAction* TBase::findAction(const QString& name) {

	QList<TAction*> actions = findChildren<TAction*>(name);
	if (actions.count() == 0) {
		qWarning("Gui::TBase::findAction: action '%s' not found",
				 name.toUtf8().constData());
		return new TAction(this, name, name);
	}

	return actions[0];
}

void TBase::loadConfig() {
	qDebug("Gui::TBase::loadConfig");

	// Get all actions with a name
	TActionList all_actions = getAllNamedActions();
	// Load actions from outside group derived class
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
	viewMenuBarAct->setChecked(menubar_visible);
	fullscreen_menubar_visible = pref->value("fullscreen_menubar_visible", fullscreen_menubar_visible).toBool();

	statusbar_visible = pref->value("statusbar_visible", statusbar_visible).toBool();
	viewStatusBarAct->setChecked(statusbar_visible);
	fullscreen_statusbar_visible = pref->value("fullscreen_statusbar_visible", fullscreen_statusbar_visible).toBool();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();

	// Load playlist settings outside group
	playlist->loadSettings();

	// Disable actions
	setActionsEnabled(false);
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

// Overriden by TBasePlus
void TBase::showPlaylist(bool b) {
	playlist->setVisible(b);
}

void TBase::showPreferencesDialog() {
	qDebug("Gui::TBase::showPreferencesDialog");

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

	QString old_player_bin = pref->player_bin;

	// Update pref from dialog
	pref_dialog->getData(pref);

	// Update and save playlist preferences
	Pref::TPrefPlaylist* pl = pref_dialog->mod_playlist();
	playlist->setDirectoryRecursion(pl->directoryRecursion());
	playlist->setAutoGetInfo(pl->autoGetInfo());
	playlist->setSavePlaylistOnExit(pl->savePlaylistOnExit());
	playlist->setPlayFilesFromStart(pl->playFilesFromStart());
	playlist->saveSettings();

	// Update actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
	TActionsEditor::saveToConfig(this, pref);

	// Commit changes
	pref->save();

	// Update logging
	TLog::log->setEnabled(pref->log_enabled);
	TLog::log->setLogFileEnabled(pref->log_file);
	TLog::log->setFilter(pref->log_filter);

	// Interface tab first to check for needed restarts
	Pref::TInterface* _interface = pref_dialog->mod_interface();

	// Load translation if language changed
	if (_interface->languageChanged()) {
		emit loadTranslation();
	}

	// Style changes need recreation of main window
	if (_interface->styleChanged()) {
		// Request restart and optional reset of style to default
		emit requestRestart(pref->style.isEmpty());
		// Close and restart with the new settings
		close();
		return;
	}

	// Gui, icon or player change needs restart smplayer
	if (_interface->guiChanged()
		|| _interface->iconsetChanged()
		|| old_player_bin != pref->player_bin) {
		// Request restart
		emit requestRestart(false);
		// Close and restart with the new settings
		close();
		return;
	}

	// Keeping the current main window

	// Update application font
	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		if (QApplication::font() != f) {
			qDebug("Gui::TBase::applyNewPreferences: setting new font: %s",
				   pref->default_font.toLatin1().constData());
			QApplication::setFont(f);
		}
	}

	// Recents
	if (_interface->recentsChanged()) {
		openMenu->updateRecents();
	}

	// Show panel
	if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
		resize(width(), height() + 200);
		panel->show();
	}

	// Video equalizer
	video_equalizer->setBySoftware(pref->use_soft_video_eq);

	// Hide toolbars delay
	auto_hide_timer->setInterval(pref->floating_hide_delay);

	// Subtitles
	subtitleMenu->useForcedSubsOnlyAct->setChecked(pref->use_forced_subs_only);
	subtitleMenu->useCustomSubStyleAct->setChecked(pref->enable_ass_styles);

	// Advanced tab
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
	playerwindow->setDelayLeftClick(pref->delay_left_click);

	playMenu->setJumpTexts(); // Update texts in menus

	// Network
	setupNetworkProxy();

	// Reenable actions to reflect changes
	if (core->state() == TCore::Stopped) {
		disableActionsOnStop();
	} else {
		enableActionsOnPlaying();
	}

	// Restart video if needed
	if (pref_dialog->requiresRestart()) {
		core->restart();
	}
}

void TBase::showFilePropertiesDialog() {
	qDebug("Gui::TBase::showFilePropertiesDialog");

	if (!file_properties_dialog) {
		createFilePropertiesDialog();
	}
	setDataToFileProperties();
	file_properties_dialog->show();
}

void TBase::setDataToFileProperties() {
	qDebug("TBase::setDataToFileProperties");

	InfoReader *i = InfoReader::obj();
	i->getInfo();
	file_properties_dialog->setCodecs(i->vcList(), i->acList(), i->demuxerList());

	// Save a copy of the demuxer, video and audio codec
	if (core->mset.original_demuxer.isEmpty()) 
		core->mset.original_demuxer = core->mdat.demuxer;
	if (core->mset.original_video_codec.isEmpty()) 
		core->mset.original_video_codec = core->mdat.video_codec;
	if (core->mset.original_audio_codec.isEmpty()) 
		core->mset.original_audio_codec = core->mdat.audio_codec;

	// Set demuxer, video and audio codec
	QString demuxer = core->mset.forced_demuxer;
	if (demuxer.isEmpty())
		demuxer = core->mdat.demuxer;
	QString vc = core->mset.forced_video_codec;
	if (vc.isEmpty())
		vc = core->mdat.video_codec;
	QString ac = core->mset.forced_audio_codec;
	if (ac.isEmpty())
		ac = core->mdat.audio_codec;

	file_properties_dialog->setDemuxer(demuxer, core->mset.original_demuxer);
	file_properties_dialog->setAudioCodec(ac, core->mset.original_audio_codec);
	file_properties_dialog->setVideoCodec(vc, core->mset.original_video_codec);

	file_properties_dialog->setMplayerAdditionalArguments(core->mset.mplayer_additional_options);
	file_properties_dialog->setMplayerAdditionalVideoFilters(core->mset.mplayer_additional_video_filters);
	file_properties_dialog->setMplayerAdditionalAudioFilters(core->mset.mplayer_additional_audio_filters);

	file_properties_dialog->showInfo();
}

void TBase::applyFileProperties() {
	qDebug("Gui::TBase::applyFileProperties");

	bool need_restart = false;

#undef TEST_AND_SET
#define TEST_AND_SET(Pref, Dialog) \
	if (Pref != Dialog) { Pref = Dialog; need_restart = true; }

	bool demuxer_changed = false;

	QString prev_demuxer = core->mset.forced_demuxer;

	QString demuxer = file_properties_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	if (prev_demuxer != core->mset.forced_demuxer) {
		// Demuxer changed
		demuxer_changed = true;
		core->mset.current_audio_id = TMediaSettings::NoneSelected;
		core->mset.current_sub_idx = TMediaSettings::NoneSelected;
	}

	QString ac = file_properties_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_properties_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);

	TEST_AND_SET(core->mset.mplayer_additional_options, file_properties_dialog->mplayerAdditionalArguments());
	TEST_AND_SET(core->mset.mplayer_additional_video_filters, file_properties_dialog->mplayerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.mplayer_additional_audio_filters, file_properties_dialog->mplayerAdditionalAudioFilters());

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

	if (file_properties_dialog && file_properties_dialog->isVisible()) {
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
	openMenu->updateRecents();

	checkPendingActionsToRun();
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
			// Create playlist for a directory
			openDirectory(file);
			return;
		}
		// Set latest directory for existing file
		pref->latest_dir = fi.absolutePath();
	}

	core->open(file);

	qDebug("Gui::TBase::open: done");
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

	if (pref->dvd_device.isEmpty() || pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		core->open(TDiscName::join(TDiscName::VCD, pref->vcd_initial_title,
								   pref->cdrom_device));
	}
}

void TBase::openAudioCD() {
	qDebug("Gui::TBase::openAudioCD");

	if (pref->dvd_device.isEmpty() || pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		core->open("cdda://");
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
		clhelp_window = new TLogWindow(this, false);
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

void TBase::showConfigFolder() {
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
	//qDebug("Gui::TBase::unlockSizeFactor");
	block_update_size_factor--;
}

void TBase::toggleFullscreen(bool b) {
	qDebug("Gui::TBase::toggleFullscreen: %d", b);

	if (b == pref->fullscreen) {
		qDebug("Gui::TBase::toggleFullscreen: nothing to do, returning");
		return;
	}
	pref->fullscreen = b;

	block_update_size_factor++;

	// Update fullscreen actions
	videoMenu->fullscreenChanged(pref->fullscreen);

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

	// Risky?
	QTimer::singleShot(350, this, SLOT(unlockSizeFactor()));

	setFocus(); // Fixes bug #2493415
}

void TBase::aboutToEnterFullscreen() {
	//qDebug("Gui::TBase::aboutToEnterFullscreen");

	emit aboutToEnterFullscreenSignal();

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
	viewMenuBarAct->setChecked(fullscreen_menubar_visible);
	viewStatusBarAct->setChecked(fullscreen_statusbar_visible);

	pref->beginGroup(settingsGroupName());
	if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
					  Helper::qtVersion())) {
		// First time there is no fullscreen toolbar state
		qWarning("Gui::TBase::didEnterFullscreen: failed to restore fullscreen toolbar state");
		toolbar->hide();
		toolbar2->hide();
	}
	pref->endGroup();

	emit didEnterFullscreenSignal();

	auto_hide_timer->start();
}

void TBase::aboutToExitFullscreen() {
	//qDebug("Gui::TBase::aboutToExitFullscreen");

	auto_hide_timer->stop();

	emit aboutToExitFullscreenSignal();

	// Save fullscreen state
	fullscreen_menubar_visible = !menuBar()->isHidden();
	fullscreen_statusbar_visible = !statusBar()->isHidden();

	pref->beginGroup(settingsGroupName());
	pref->setValue("toolbars_state_fullscreen", saveState(Helper::qtVersion()));
	pref->endGroup();
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

	viewMenuBarAct->setChecked(menubar_visible);
	viewStatusBarAct->setChecked(statusbar_visible);

	pref->beginGroup(settingsGroupName());
	if (!restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion()))
		qWarning("Gui::TBase::didExitFullscreen: failed to restore toolbar state");
	pref->endGroup();

	emit didExitFullscreenSignal();
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

// Called by playerwindow when dragging main window
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
			displayMessage(tr("Playing %1").arg(core->mdat.filename), 3000);
			auto_hide_timer->startAutoHideMouse();
			break;
		case TCore::Paused:
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

void TBase::displayMessage(const QString& message, int time) {
	statusBar()->showMessage(message, time);
}

void TBase::displayMessage(const QString& message) {
	displayMessage(message, 3000);
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

void TBase::changeSize(int percentage) {
	qDebug("TBase::changeSize: %d", percentage);

	if (!pref->fullscreen) {
		bool center = false;
		if (isMaximized()) {
			showNormal();
			center = true;
		}

		pref->size_factor = (double) percentage / 100;
		resizeWindow(core->mset.win_width, core->mset.win_height);
		emit videoSizeFactorChanged();

		core->displayMessage(tr("Size %1%").arg(QString::number(percentage)));

		if (center) {
			centerWindow();
		}
	}
}

void TBase::toggleDoubleSize() {

	if (pref->size_factor != 1.0)
		changeSize(100);
	else changeSize(200);
}

void TBase::centerWindow() {

	QSize center_pos = (TDesktop::availableSize(this) - frameGeometry().size()) / 2;
	if (center_pos.isValid()) {
		move(center_pos.width(), center_pos.height());
	}
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
		// Have video
		// Show panel
		if (!panel->isVisible()) {
			panel->show();
		}
		if (!isMaximized()) {
			// Block_resize set if pref->save_window_size_on_exit selected
			if (!force_resize
				&& (block_resize || pref->resize_method == TPreferences::Never)) {
				// Adjust size factor to window size
				playerwindow->updateSizeFactor();
			} else {
				pref->size_factor = 1.0;
				resizeWindow(w, h);
			}
		}
	}

	if (center_window) {
		center_window = false;
		if (!isMaximized()) {
			centerWindow();
		}
	}

	force_resize = false;
	block_resize = false;
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
	//qDebug() << "TBase::resizeEvent: event spontaneous:" << event->spontaneous()
	//		 << "lock:" << block_update_size_factor;

	QMainWindow::resizeEvent(event);

	// Update size factor after window resized by user.
	// In TPlayerWindow::resizeEvent() event->spontaneous() does not become
	// true during an user induces resize, so its needs to be here.
	if (event->spontaneous() && block_update_size_factor == 0) {
		playerwindow->updateSizeFactor();
		emit videoSizeFactorChanged();
	}
}

// Slot called when media settings reset or loaded
void TBase::onMediaSettingsChanged() {
	qDebug("Gui::TBase::onMediaSettingsChanged");

	emit mediaSettingsChanged(&core->mset);

	updateVideoEqualizer();
	updateAudioEqualizer();
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

	bool stay_on_top = windowFlags() & Qt::WindowStaysOnTopHint;
	if (b == stay_on_top) {
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
	emit stayOnTopChanged(stay_on_top);
}

void TBase::checkStayOnTop(TCore::State state) {
	//qDebug("Gui::TBase::checkStayOnTop");

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

void TBase::setFloatingToolbarsVisible(bool visible) {

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

	setFloatingToolbarsVisible(true);
}

void TBase::hideEvent(QHideEvent* event) {
	qDebug("Gui::TBase::hideEvent");

	if (event) {
		QMainWindow::hideEvent(event);
	}

	if (pref->pause_when_hidden && core->state() == TCore::Playing && !ignore_show_hide_events) {
		qDebug("Gui::TBase::hideEvent: pausing");
		core->pause();
	}

	setFloatingToolbarsVisible(false);
}

#if QT_VERSION >= 0x050000
// Qt 5 doesn't call showEvent / hideEvent when the window is minimized or unminimized
// TODO: handle with changeEvent instead
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

QString TBase::exitCodeToMessage(int exit_code) {

	// TODO: use Player/Qt/sys msgs
	QString msg;
	switch (exit_code) {
		case 2: msg = tr("File not found: '%1'").arg(core->mdat.filename); break;
		case 159: msg = tr("No disk in device for '%1'").arg(core->mdat.filename); break;
		default: msg = tr("%1 has finished unexpectedly.").arg(pref->playerName())
					   + " " + tr("Exit code: %1").arg(exit_code);
	}

	qDebug() << "Gui::TBase::exitCodeToMessage:" << msg;
	return msg;
}

void TBase::showExitCodeFromPlayer(int exit_code) {
	qDebug("Gui::TBase::showExitCodeFromPlayer: %d", exit_code);

	block_resize = false;

	QString msg = exitCodeToMessage(exit_code);
	displayMessage(msg, 0);

	if (pref->report_mplayer_crashes) {
		TErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(pref->playerName()));
		d.setText(msg);
		d.setLog(TLog::log->getLogLines());
		d.exec();
	} else {
		qDebug("Gui::TBase::showExitCodeFromPlayer: error reporting is turned off");
	}
}

void TBase::showErrorFromPlayer(QProcess::ProcessError e) {
	qDebug("Gui::TBase::showErrorFromPlayer");

	block_resize = false;

	if (!pref->report_mplayer_crashes) {
		qDebug("Gui::TBase::showErrorFromPlayer: error reporting is turned off");
		displayMessage(tr("Player crashed or quit with errors."), 6000);
		return;
	}

	if (e == QProcess::FailedToStart || e == QProcess::Crashed) {
		TErrorDialog d(this);
		d.setWindowTitle(tr("%1 Error").arg(pref->playerName()));
		if (e == QProcess::FailedToStart) {
			d.setText(tr("%1 failed to start.").arg(pref->playerName()) + " " + 
					  tr("Please check the %1 path in preferences.").arg(pref->playerName()));
		} else {
			d.setText(tr("%1 has crashed.").arg(pref->playerName()) + " " + 
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
		video_preview = new VideoPreview(this);
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

	if (video_preview->showConfigDialog(this) && video_preview->createThumbnails()) {
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

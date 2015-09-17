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

#include "skingui.h"
#include "helper.h"
#include "colorutils.h"
#include "core.h"
#include "global.h"
#include "widgetactions.h"
#include "playlist.h"
#include "mplayerwindow.h"
#include "myaction.h"
#include "images.h"
#include "autohidewidget.h"
#include "desktopinfo.h"
#include "editabletoolbar.h"
#include "mediabarpanel.h"
#include "actionseditor.h"

#if DOCK_PLAYLIST
#include "playlistdock.h"
#endif

#include <QMenu>
#include <QSettings>
#include <QLabel>
#include <QStatusBar>
#include <QPushButton>
#include <QToolButton>
#include <QMenuBar>
#include <QLayout>
#include <QApplication>
#include <QDir>

#define TOOLBAR_VERSION 1

using namespace Global;

SkinGui::SkinGui( QWidget * parent, Qt::WindowFlags flags )
	: Gui::TBasePlus( parent, flags )
	, was_muted(false)
{
	connect( this, SIGNAL(timeChanged(QString)),
             this, SLOT(displayTime(QString)) );

	createActions();
	createMainToolBars();
	createControlWidget();
	createFloatingControl();
	createMenus();

	connect( editToolbar1Act, SIGNAL(triggered()),
             toolbar1, SLOT(edit()) );
	#if defined(SKIN_EDITABLE_CONTROL)
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	iw->takeAvailableActionsFrom(this);
	connect( editFloatingControlAct, SIGNAL(triggered()), iw, SLOT(edit()) );
	#endif
}

SkinGui::~SkinGui() {
}

void SkinGui::createActions() {
	qDebug("SkinGui::createActions");

	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();

	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();

	// Create the time label
	time_label_action = new TimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

#if MINI_ARROW_BUTTONS
	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new SeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new SeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");
#endif

	editToolbar1Act = new MyAction( this, "edit_main_toolbar" );
	#if defined(SKIN_EDITABLE_CONTROL)
	editFloatingControlAct = new MyAction( this, "edit_floating_control" );
	#endif

	playOrPauseAct->setCheckable(true);

	viewVideoInfoAct = new MyAction(this, "toggle_video_info_skingui" );
	viewVideoInfoAct->setCheckable(true);

	scrollTitleAct = new MyAction(this, "toggle_scroll_title_skingui" );
	scrollTitleAct->setCheckable(true);
}

#if AUTODISABLE_ACTIONS
void SkinGui::enableActionsOnPlaying() {
	qDebug("SkinGui::enableActionsOnPlaying");
	Gui::TBasePlus::enableActionsOnPlaying();

	timeslider_action->enable();
	volumeslider_action->enable();
}

void SkinGui::disableActionsOnStop() {
	qDebug("SkinGui::disableActionsOnStop");
	Gui::TBasePlus::disableActionsOnStop();

	timeslider_action->disable();
	volumeslider_action->disable();
}
#endif // AUTODISABLE_ACTIONS

void SkinGui::togglePlayAction(Core::State state) {
	qDebug("SkinGui::togglePlayAction");
	Gui::TBasePlus::togglePlayAction(state);

	if (state == Core::Playing) {
		playOrPauseAct->setChecked(true);
	}
	else {
		playOrPauseAct->setChecked(false);
	}
}

void SkinGui::createMenus() {
	menuBar()->setObjectName("menubar");
	QFont font = menuBar()->font();
	font.setPixelSize(11);
	menuBar()->setFont(font);
	/*menuBar()->setFixedHeight(21);*/

	toolbar_menu = new QMenu(this);
	toolbar_menu->addAction(toolbar1->toggleViewAction());

	toolbar_menu->addSeparator();
	toolbar_menu->addAction(editToolbar1Act);
#if defined(SKIN_EDITABLE_CONTROL)
	toolbar_menu->addAction(editFloatingControlAct);
#endif

	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);

	statusbar_menu = new QMenu(this);
	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(scrollTitleAct);
	optionsMenu->addMenu(statusbar_menu);
}

QMenu * SkinGui::createPopupMenu() {
	QMenu * m = new QMenu(this);
	m->addAction(editToolbar1Act);
#if defined(SKIN_EDITABLE_CONTROL)
	m->addAction(editFloatingControlAct);
#endif
	return m;
}

void SkinGui::createMainToolBars() {
	toolbar1 = new EditableToolbar( this );
	toolbar1->setObjectName("toolbar");
	toolbar1->setMovable(false);
	//toolbar1->setFixedHeight(35);
	addToolBar(Qt::TopToolBarArea, toolbar1);

	QStringList toolbar1_actions;
	toolbar1_actions << "open_file" << "open_url" << "favorites_menu" << "separator"
                     << "screenshot" << "separator" << "show_file_properties" 
                     << "show_find_sub_dialog" << "show_tube_browser" << "show_preferences";
	toolbar1->setDefaultActions(toolbar1_actions);

	// Modify toolbars' actions
	QAction *tba;
	tba = toolbar1->toggleViewAction();
	tba->setObjectName("show_main_toolbar");
	tba->setShortcut(Qt::Key_F5);
}


void SkinGui::createControlWidget() {
	qDebug("SkinGui::createControlWidget");

	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);
	controlwidget->setStyleSheet("QToolBar { spacing: 0px; }");
	controlwidget->setMovable(false);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

	mediaBarPanel = new MediaBarPanel(panel);
	mediaBarPanel->setObjectName("mediabar-panel");
	mediaBarPanel->setCore(core);
	/* panel->layout()->addWidget(mediaBarPanel); */

	QList<QAction*> actions;
	//actions << halveSpeedAct << playPrevAct << playOrPauseAct << stopAct << recordAct << playNextAct << doubleSpeedAct;
	actions << rewind1Act << playPrevAct << playOrPauseAct << stopAct << playNextAct << forward1Act;
	mediaBarPanel->setPlayControlActionCollection(actions);

	actions.clear();
	//actions << timeslider_action << shuffleAct << repeatPlaylistAct;
	QAction * shuffleAct = ActionsEditor::findAction(playlist, "pl_shuffle");
	QAction * repeatPlaylistAct = ActionsEditor::findAction(playlist, "pl_repeat");
	if (shuffleAct) actions << shuffleAct;
	if (repeatPlaylistAct) actions << repeatPlaylistAct;
	mediaBarPanel->setMediaPanelActionCollection(actions);
	connect(core, SIGNAL(stateChanged(Core::State)), mediaBarPanel, SLOT(setMplayerState(Core::State)));

	actions.clear();
	//actions << volumeslider_action << showPlaylistAct << fullscreenAct << equalizerAct;
	actions << volumeslider_action << showPlaylistAct << fullscreenAct << videoEqualizerAct;
	mediaBarPanel->setVolumeControlActionCollection(actions);

	actions.clear();
	actions << openFileAct << openDirectoryAct << openDVDAct << openURLAct << screenshotAct << showPropertiesAct;
#ifdef FIND_SUBTITLES
	actions << showFindSubtitlesDialogAct;
#endif
	actions << showPreferencesAct;
	mediaBarPanel->setToolbarActionCollection(actions);

	connect(mediaBarPanel, SIGNAL(volumeChanged(int)), core, SLOT(setVolume(int)));
	connect(mediaBarPanel, SIGNAL(volumeSliderMoved(int)), core, SLOT(setVolume(int)));
	connect(core, SIGNAL(volumeChanged(int)), mediaBarPanel, SLOT(setVolume(int)));

	connect(mediaBarPanel, SIGNAL(seekerChanged(int)), core, SLOT(goToPosition(int)));
	connect(core, SIGNAL(positionChanged(int)), mediaBarPanel, SLOT(setSeeker(int)));

	connect( viewVideoInfoAct, SIGNAL(toggled(bool)),
             mediaBarPanel, SLOT(setResolutionVisible(bool)) );

	connect( scrollTitleAct, SIGNAL(toggled(bool)),
             mediaBarPanel, SLOT(setScrollingEnabled(bool)) );

	mediaBarPanelAction = controlwidget->addWidget(mediaBarPanel);
}

void SkinGui::createFloatingControl() {
	// Floating control
	floating_control = new AutohideWidget(panel, mplayerwindow);
	floating_control->setAutoHide(true);

#ifdef SKIN_EDITABLE_CONTROL
	EditableToolbar * iw = new EditableToolbar(floating_control);

	QStringList floatingcontrol_actions;
	floatingcontrol_actions << "play" << "pause" << "stop" << "separator";
	#if MINI_ARROW_BUTTONS
	floatingcontrol_actions << "rewindbutton_action";
	#else
	floatingcontrol_actions << "rewind3" << "rewind2" << "rewind1";
	#endif
	floatingcontrol_actions << "timeslider_action";
	#if MINI_ARROW_BUTTONS
	floatingcontrol_actions << "forwardbutton_action";
	#else
	floatingcontrol_actions << "forward1" << "forward2" << "forward3";
	#endif
	floatingcontrol_actions << "separator" << "fullscreen" << "mute" << "volumeslider_action" << "separator" << "timelabel_action";

	iw->setDefaultActions(floatingcontrol_actions);

	floating_control->setInternalWidget(iw);
#endif // SKIN_EDITABLE_CONTROL

/*
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	// To make work the ESC key (exit fullscreen) and Ctrl-X (close) in Windows and OS2
	floating_control->addAction(exitFullscreenAct);
	floating_control->addAction(exitAct);
#endif
*/

	floating_control->hide();
}

void SkinGui::retranslateStrings() {
	Gui::TBasePlus::retranslateStrings();

	toolbar_menu->menuAction()->setText( tr("&Toolbars") );
	toolbar_menu->menuAction()->setIcon( Images::icon("toolbars") );

	statusbar_menu->menuAction()->setText( tr("Status&bar") );
	statusbar_menu->menuAction()->setIcon( Images::icon("statusbar") );

	toolbar1->setWindowTitle( tr("&Main toolbar") );
	toolbar1->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

	editToolbar1Act->change( tr("Edit main &toolbar") );
#if defined(SKIN_EDITABLE_CONTROL)
	editFloatingControlAct->change( tr("Edit &floating control") );
#endif

	viewVideoInfoAct->change(Images::icon("view_video_info"), tr("&Video info") );
	scrollTitleAct->change(Images::icon("scroll_title"), tr("&Scroll title") );
}

void SkinGui::displayTime(QString text) {
	time_label_action->setText(text);
}

void SkinGui::displayState(Core::State state) {
	Gui::TBasePlus::displayState(state);

	switch (state) {
		case Core::Playing:		mediaBarPanel->displayMessage( tr("Playing %1").arg(core->mdat.filename)); break;
		case Core::Paused:		mediaBarPanel->displayMessage( tr("Pause") ); break;
		case Core::Stopped:		mediaBarPanel->displayMessage( tr("Stop") ); break;
	}
}

void SkinGui::displayMessage(QString message, int time) {
	Gui::TBasePlus::displayMessage(message, time);
	mediaBarPanel->displayMessage(message, time);
}

void SkinGui::displayMessage(QString message) {
	Gui::TBasePlus::displayMessage(message);
	mediaBarPanel->displayMessage(message);
}

void SkinGui::updateWidgets() {
	qDebug("SkinGui::updateWidgets");

	Gui::TBasePlus::updateWidgets();

	panel->setFocus();

	bool muted = (pref->global_volume ? pref->mute : core->mset.mute);
	if (was_muted != muted) {
		was_muted = muted;
		if (muted) {
			mediaBarPanel->setVolume(0);
		} else {
			mediaBarPanel->setVolume(core->mset.volume);
		}
	}
}

void SkinGui::aboutToEnterFullscreen() {
	qDebug("SkinGui::aboutToEnterFullscreen");

	Gui::TBasePlus::aboutToEnterFullscreen();

	#ifndef SKIN_EDITABLE_CONTROL
	controlwidget->removeAction(mediaBarPanelAction);
	floating_control->layout()->addWidget(mediaBarPanel);
	mediaBarPanel->show();
	floating_control->adjustSize();
	#endif
	floating_control->setMargin(pref->floating_control_margin);
	floating_control->setPercWidth(pref->floating_control_width);
	floating_control->setAnimated(pref->floating_control_animated);
	floating_control->setActivationArea( (AutohideWidget::Activation) pref->floating_activation_area);
	floating_control->setHideDelay(pref->floating_hide_delay);
	QTimer::singleShot(100, floating_control, SLOT(activate()));


	// Save visibility of toolbars
	fullscreen_toolbar1_was_visible = toolbar1->isVisible();

	if (!pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
	}
}

void SkinGui::aboutToExitFullscreen() {
	qDebug("SkinGui::aboutToExitFullscreen");

	Gui::TBasePlus::aboutToExitFullscreen();

	floating_control->deactivate();
	#ifndef SKIN_EDITABLE_CONTROL
	floating_control->layout()->removeWidget(mediaBarPanel);
	mediaBarPanelAction = controlwidget->addWidget(mediaBarPanel);
	#endif

	if (!pref->compact_mode) {
		statusBar()->hide();
		controlwidget->show();
		toolbar1->setVisible( fullscreen_toolbar1_was_visible );
	}
}

void SkinGui::aboutToEnterCompactMode() {

	Gui::TBasePlus::aboutToEnterCompactMode();

	// Save visibility of toolbars
	compact_toolbar1_was_visible = toolbar1->isVisible();

	controlwidget->hide();
	toolbar1->hide();
}

void SkinGui::aboutToExitCompactMode() {
	Gui::TBasePlus::aboutToExitCompactMode();

	statusBar()->hide();
	controlwidget->show();
	toolbar1->setVisible( compact_toolbar1_was_visible );

	// Recheck size of controlwidget
	/* resizeEvent( new QResizeEvent( size(), size() ) ); */
}

void SkinGui::saveConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("SkinGui::saveConfig");

	Gui::TBasePlus::saveConfig("skin_gui");

	QSettings * set = settings;
	set->beginGroup("skin_gui");

	set->setValue("video_info", viewVideoInfoAct->isChecked());
	set->setValue("scroll_title", scrollTitleAct->isChecked());

	set->setValue("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible);
	set->setValue("compact_toolbar1_was_visible", compact_toolbar1_was_visible);

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

	set->beginGroup( "actions" );
	set->setValue("toolbar1", toolbar1->actionsToStringList() );
#if defined(SKIN_EDITABLE_CONTROL)
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	set->setValue("floating_control", iw->actionsToStringList() );
#endif
	set->setValue("toolbar1_version", TOOLBAR_VERSION);
	set->endGroup();

	set->beginGroup("toolbars_icon_size");
	set->setValue("toolbar1", toolbar1->iconSize());
#if defined(SKIN_EDITABLE_CONTROL)
	set->setValue("floating_control", iw->iconSize());
#endif
	set->endGroup();

	set->endGroup();
}

void SkinGui::loadConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("SkinGui::loadConfig");

	Gui::TBasePlus::loadConfig("skin_gui");

	QSettings * set = settings;
	set->beginGroup("skin_gui");

	viewVideoInfoAct->setChecked(set->value("video_info", false).toBool());
	scrollTitleAct->setChecked(set->value("scroll_title", false).toBool());

	fullscreen_toolbar1_was_visible = set->value("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible).toBool();
	compact_toolbar1_was_visible = set->value("compact_toolbar1_was_visible", compact_toolbar1_was_visible).toBool();

	set->beginGroup( "actions" );
	int toolbar_version = set->value("toolbar1_version", 0).toInt();
	if (toolbar_version >= TOOLBAR_VERSION) {
		toolbar1->setActionsFromStringList( set->value("toolbar1", toolbar1->defaultActions()).toStringList() );
	} else {
		qWarning("SkinGui::loadConfig: toolbar too old, loading default one");
		toolbar1->setActionsFromStringList( toolbar1->defaultActions() );
	}
#if defined(SKIN_EDITABLE_CONTROL)
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	iw->setActionsFromStringList( set->value("floating_control", iw->defaultActions()).toStringList() );
	floating_control->adjustSize();
#endif
	set->endGroup();

	set->beginGroup("toolbars_icon_size");
	toolbar1->setIconSize(set->value("toolbar1", toolbar1->iconSize()).toSize());
#if defined(SKIN_EDITABLE_CONTROL)
	iw->setIconSize(set->value("floating_control", iw->iconSize()).toSize());
#endif
	set->endGroup();

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	set->endGroup();

	updateWidgets();

	if (pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
	}

	statusBar()->hide();

	mediaBarPanel->setVolume(50);
}

#include "moc_skingui.cpp"

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

#include "default.h"

#include <QDebug>
#include <QMenu>
#include <QSettings>
#include <QLabel>
#include <QStatusBar>
#include <QPushButton>
#include <QToolButton>
#include <QMenuBar>
#include <QTimer>

#include "helper.h"
#include "colorutils.h"
#include "core.h"
#include "widgetactions.h"
#include "playlist.h"
#include "playerwindow.h"
#include "gui/action.h"
#include "images.h"
#include "autohidewidget.h"
#include "desktopinfo.h"
#include "editabletoolbar.h"

#if DOCK_PLAYLIST
#include "playlistdock.h"
#endif

#define TOOLBAR_VERSION 1

using namespace Settings;

namespace Gui {

TDefault::TDefault()
	: TBasePlus() {

	createStatusBar();

	connect(this, SIGNAL(timeChanged(QString)),
			 this, SLOT(displayTime(QString)));
	connect(this, SIGNAL(frameChanged(int)),
			 this, SLOT(displayFrame(int)));
	connect(this, SIGNAL(ABMarkersChanged(int,int)),
			 this, SLOT(displayABSection(int,int)));
	connect(this, SIGNAL(videoInfoChanged(int,int,double)),
			 this, SLOT(displayVideoInfo(int,int,double)));

	createActions();
	createMainToolBars();
    createControlWidget();
    createControlWidgetMini();
	createFloatingControl();
	createMenus();

	connect(editToolbar1Act, SIGNAL(triggered()),
			 toolbar1, SLOT(edit()));
	connect(editControl1Act, SIGNAL(triggered()),
			 controlwidget, SLOT(edit()));
	connect(editControl2Act, SIGNAL(triggered()),
			 controlwidget_mini, SLOT(edit()));
	TEditableToolbar* iw = static_cast<TEditableToolbar*>(floating_control->internalWidget());
	iw->takeAvailableActionsFrom(this);
	connect(editFloatingControlAct, SIGNAL(triggered()),
			 iw, SLOT(edit()));

	menuBar()->setObjectName("menubar");
}

TDefault::~TDefault() {
}

void TDefault::createActions() {
	qDebug("Gui::TDefault::createActions");

	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new TSeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new TSeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");

	// Statusbar
	viewVideoInfoAct = new TAction(this, "toggle_video_info");
	viewVideoInfoAct->setCheckable(true);
	connect(viewVideoInfoAct, SIGNAL(toggled(bool)),
			 video_info_display, SLOT(setVisible(bool)));

	viewFrameCounterAct = new TAction(this, "toggle_frame_counter");
	viewFrameCounterAct->setCheckable(true);
	connect(viewFrameCounterAct, SIGNAL(toggled(bool)),
			 frame_display, SLOT(setVisible(bool)));

	editToolbar1Act = new TAction(this, "edit_main_toolbar");
	editControl1Act = new TAction(this, "edit_control1");
	editControl2Act = new TAction(this, "edit_control2");
	editFloatingControlAct = new TAction(this, "edit_floating_control");
}

void TDefault::togglePlayAction(TCore::State state) {
	qDebug("Gui::TDefault::togglePlayAction");
	TBasePlus::togglePlayAction(state);

	if (state == TCore::Playing) {
		playOrPauseAct->setIcon(Images::icon("pause"));
	} else {
		playOrPauseAct->setIcon(Images::icon("play"));
	}
}

void TDefault::createMenus() {
	toolbar_menu = new QMenu(this);
	toolbar_menu->addAction(toolbar1->toggleViewAction());
	toolbar_menu->addAction(toolbar2->toggleViewAction());

	toolbar_menu->addSeparator();
	toolbar_menu->addAction(editToolbar1Act);
	toolbar_menu->addAction(editControl1Act);
	toolbar_menu->addAction(editControl2Act);
	toolbar_menu->addAction(editFloatingControlAct);

	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);

	statusbar_menu = new QMenu(this);
	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(viewFrameCounterAct);

	optionsMenu->addMenu(statusbar_menu);
}

QMenu* TDefault::createPopupMenu() {
	QMenu* m = new QMenu(this);
	m->addAction(editToolbar1Act);
	m->addAction(editControl1Act);
	m->addAction(editControl2Act);
	m->addAction(editFloatingControlAct);
	return m;
}

void TDefault::createMainToolBars() {
	toolbar1 = new TEditableToolbar(this);
	toolbar1->setObjectName("toolbar1");
	//toolbar1->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolbar1);

	QStringList toolbar1_actions;
	toolbar1_actions << "open_file" << "open_url" << "favorites_menu" << "separator"
                     << "screenshot" << "separator" << "show_file_properties" << "show_playlist"
                     << "show_tube_browser" << "separator" << "show_preferences"
                     << "separator" << "play_prev" << "play_next";

	toolbar1->setDefaultActions(toolbar1_actions);

	toolbar2 = new QToolBar(this);
	toolbar2->setObjectName("toolbar2");
	//toolbar2->setMovable(false);
	addToolBar(Qt::TopToolBarArea, toolbar2);

	select_audio = new QPushButton(this);
	select_audio->setMenu(audiotrack_menu);
	toolbar2->addWidget(select_audio);

	select_subtitle = new QPushButton(this);
	select_subtitle->setMenu(subtitles_track_menu);
	toolbar2->addWidget(select_subtitle);

	/*
	toolbar1->show();
	toolbar2->show();
	*/

	// Modify toolbars' actions
	QAction *tba;
	tba = toolbar1->toggleViewAction();
	tba->setObjectName("show_main_toolbar");
	tba->setShortcut(Qt::Key_F5);

	tba = toolbar2->toggleViewAction();
	tba->setObjectName("show_language_toolbar");
	tba->setShortcut(Qt::Key_F6);
}


void TDefault::createControlWidgetMini() {
	qDebug("Gui::TDefault::createControlWidgetMini");

	controlwidget_mini = new TEditableToolbar(this);
	controlwidget_mini->setObjectName("controlwidget_mini");
	controlwidget_mini->setLayoutDirection(Qt::LeftToRight);
	//controlwidget_mini->setResizeEnabled(false);
	controlwidget_mini->setMovable(false);
	//addDockWindow(controlwidget_mini, Qt::DockBottom);
	addToolBar(Qt::BottomToolBarArea, controlwidget_mini);

	QStringList controlwidget_mini_actions;
	controlwidget_mini_actions << "play_or_pause" << "stop" << "separator" << "rewind1" << "timeslider_action" 
                               << "forward1" << "separator" << "mute" << "volumeslider_action";
	controlwidget_mini->setDefaultActions(controlwidget_mini_actions);

	controlwidget_mini->hide();
}

void TDefault::createControlWidget() {
	qDebug("Gui::TDefault::createControlWidget");

	controlwidget = new TEditableToolbar(this);
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);
	//controlwidget->setResizeEnabled(false);
	controlwidget->setMovable(false);
	//addDockWindow(controlwidget, Qt::DockBottom);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

	QStringList controlwidget_actions;
	controlwidget_actions << "play_or_pause" << "stop" << "separator";
	controlwidget_actions << "rewindbutton_action";
	controlwidget_actions << "timeslider_action";
	controlwidget_actions << "forwardbutton_action";
	controlwidget_actions << "separator" << "fullscreen" << "mute" << "volumeslider_action";
	controlwidget->setDefaultActions(controlwidget_actions);
}

void TDefault::createFloatingControl() {

	TEditableToolbar* iw = new TEditableToolbar(floating_control);
	iw->setObjectName("floating_control");
	connect(iw, SIGNAL(iconSizeChanged(const QSize &)), this, SLOT(adjustFloatingControlSize()));

	QStringList floatingcontrol_actions;
	floatingcontrol_actions << "play" << "pause" << "stop" << "separator";
	floatingcontrol_actions << "rewindbutton_action";
	floatingcontrol_actions << "timeslider_action";
	floatingcontrol_actions << "forwardbutton_action";
	floatingcontrol_actions << "separator" << "fullscreen" << "mute" << "volumeslider_action" << "separator" << "timelabel_action";
	iw->setDefaultActions(floatingcontrol_actions);

	floating_control->setInternalWidget(iw);
}

void TDefault::createStatusBar() {
	qDebug("Gui::TDefault::createStatusBar");

	time_display = new QLabel(statusBar());
	time_display->setObjectName("time_display");
	time_display->setAlignment(Qt::AlignRight);
	time_display->setFrameShape(QFrame::NoFrame);
	time_display->setText(" 88:88:88 / 88:88:88 ");
	time_display->setMinimumSize(time_display->sizeHint());

	frame_display = new QLabel(statusBar());
	frame_display->setObjectName("frame_display");
	frame_display->setAlignment(Qt::AlignRight);
	frame_display->setFrameShape(QFrame::NoFrame);
	frame_display->setText("88888888");
	frame_display->setMinimumSize(frame_display->sizeHint());

	ab_section_display = new QLabel(statusBar());
	ab_section_display->setObjectName("ab_section_display");
	ab_section_display->setAlignment(Qt::AlignRight);
	ab_section_display->setFrameShape(QFrame::NoFrame);
//	ab_section_display->setText("A:0:00:00 B:0:00:00");
//	ab_section_display->setMinimumSize(ab_section_display->sizeHint());

	video_info_display = new QLabel(statusBar());
	video_info_display->setObjectName("video_info_display");
	video_info_display->setAlignment(Qt::AlignRight);
	video_info_display->setFrameShape(QFrame::NoFrame);

	statusBar()->setAutoFillBackground(true);

	ColorUtils::setBackgroundColor(statusBar(), QColor(0,0,0));
	ColorUtils::setForegroundColor(statusBar(), QColor(255,255,255));
	ColorUtils::setBackgroundColor(time_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(time_display, QColor(255,255,255));
	ColorUtils::setBackgroundColor(frame_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(frame_display, QColor(255,255,255));
	ColorUtils::setBackgroundColor(ab_section_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(ab_section_display, QColor(255,255,255));
	ColorUtils::setBackgroundColor(video_info_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(video_info_display, QColor(255,255,255));
	statusBar()->setSizeGripEnabled(false);

	statusBar()->addPermanentWidget(video_info_display);
	statusBar()->addPermanentWidget(ab_section_display);

	statusBar()->showMessage(tr("Welcome to SMPlayer"));
	statusBar()->addPermanentWidget(frame_display, 0);
	frame_display->setText("0");

	statusBar()->addPermanentWidget(time_display, 0);
	time_display->setText(" 00:00:00 / 00:00:00 ");

	time_display->show();
	frame_display->hide();
	ab_section_display->show();
	video_info_display->hide();
}

void TDefault::retranslateStrings() {
	qDebug("Gui::TDefault::retranslateStrings");

	TBasePlus::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));

	toolbar_menu->menuAction()->setText(tr("&Toolbars"));
	toolbar_menu->menuAction()->setIcon(Images::icon("toolbars"));

	statusbar_menu->menuAction()->setText(tr("Status&bar"));
	statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));

	toolbar1->setWindowTitle(tr("&Main toolbar"));
	toolbar1->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

	toolbar2->setWindowTitle(tr("&Language toolbar"));
	toolbar2->toggleViewAction()->setIcon(Images::icon("lang_toolbar"));

	select_audio->setText(tr("Audio"));
	select_subtitle->setText(tr("Subtitle"));

	viewVideoInfoAct->change(Images::icon("view_video_info"), tr("&Video info"));
	viewFrameCounterAct->change(Images::icon("frame_counter"), tr("&Frame counter"));

	editToolbar1Act->change(tr("Edit main &toolbar"));
	editControl1Act->change(tr("Edit &control bar"));
	editControl2Act->change(tr("Edit m&ini control bar"));
	editFloatingControlAct->change(tr("Edit &floating control"));
}


void TDefault::displayTime(QString text) {

	time_display->setText(text);
}

void TDefault::displayFrame(int frame) {
	if (frame_display->isVisible()) {
		frame_display->setNum(frame);
	}
}

void TDefault::displayABSection(int secs_a, int secs_b) {
	QString s;
	if (secs_a > -1) s = tr("A:%1").arg(Helper::formatTime(secs_a));

	if (secs_b > -1) {
		if (!s.isEmpty()) s += " ";
		s += tr("B:%1").arg(Helper::formatTime(secs_b));
	}

	ab_section_display->setText(s);

	ab_section_display->setVisible(!s.isEmpty());
}

void TDefault::displayVideoInfo(int width, int height, double fps) {
	if ((width != 0) && (height != 0)) {
		video_info_display->setText(tr("%1x%2 %3 fps", "width + height + fps").arg(width).arg(height).arg(fps));
	} else {
		video_info_display->setText(" ");
	}
}

void TDefault::aboutToEnterFullscreen() {
	//qDebug("Gui::TDefault::aboutToEnterFullscreen");

	TBasePlus::aboutToEnterFullscreen();

	// Save visibility of toolbars
	fullscreen_toolbar1_was_visible = toolbar1->isVisible();
	fullscreen_toolbar2_was_visible = toolbar2->isVisible();

	if (!pref->compact_mode) {
		controlwidget->hide();
		controlwidget_mini->hide();
		toolbar1->hide();
		toolbar2->hide();
	}
}

void TDefault::aboutToExitFullscreen() {
	//qDebug("Gui::TDefault::aboutToExitFullscreen");

	TBasePlus::aboutToExitFullscreen();

	if (!pref->compact_mode) {
		controlwidget->show();
		toolbar1->setVisible(fullscreen_toolbar1_was_visible);
		toolbar2->setVisible(fullscreen_toolbar2_was_visible);
	}
}

void TDefault::aboutToEnterCompactMode() {

	TBasePlus::aboutToEnterCompactMode();

	// Save visibility of toolbars
	compact_toolbar1_was_visible = toolbar1->isVisible();
	compact_toolbar2_was_visible = toolbar2->isVisible();

	//menuBar()->hide();
	//statusBar()->hide();
	controlwidget->hide();
	controlwidget_mini->hide();
	toolbar1->hide();
	toolbar2->hide();
}

void TDefault::aboutToExitCompactMode() {

	TBasePlus::aboutToExitCompactMode();

	controlwidget->show();
	toolbar1->setVisible(compact_toolbar1_was_visible);
	toolbar2->setVisible(compact_toolbar2_was_visible);

	// Recheck size of controlwidget
	resizeEvent(new QResizeEvent(size(), size()));
}

void TDefault::resizeEvent(QResizeEvent* e) {
	/*
	qDebug("Gui::TDefault::resizeEvent %d x %d", width(), height());
	qDebug(" controlwidget width: %d", controlwidget->width());
	qDebug(" controlwidget_mini width: %d", controlwidget_mini->width());
	*/

	TBasePlus::resizeEvent(e);

#if QT_VERSION < 0x040000
#define LIMIT 470
#else
#define LIMIT 570
#endif

	if ((controlwidget->isVisible()) && (width() < LIMIT)) {
		controlwidget->hide();
		controlwidget_mini->show();
	}
	else
	if ((controlwidget_mini->isVisible()) && (width() > LIMIT)) {
		controlwidget_mini->hide();
		controlwidget->show();
	}
}

void TDefault::adjustFloatingControlSize() {
	qDebug("Gui::TDefault::adjustFloatingControlSize");

	QWidget *iw = floating_control->internalWidget();
	QSize iws = iw->size();
	QMargins m = floating_control->contentsMargins();
	int new_height = iws.height() + m.top() + m.bottom();
	if (new_height < 32) new_height = 32;
	floating_control->resize(floating_control->width(), new_height);
}

void TDefault::saveConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("Gui::TDefault::saveConfig");

	TBasePlus::saveConfig("default_gui");

	pref->beginGroup("default_gui");

	pref->setValue("video_info", viewVideoInfoAct->isChecked());
	pref->setValue("frame_counter", viewFrameCounterAct->isChecked());

	pref->setValue("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible);
	pref->setValue("fullscreen_toolbar2_was_visible", fullscreen_toolbar2_was_visible);
	pref->setValue("compact_toolbar1_was_visible", compact_toolbar1_was_visible);
	pref->setValue("compact_toolbar2_was_visible", compact_toolbar2_was_visible);

	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

	pref->beginGroup("actions");
	pref->setValue("toolbar1", toolbar1->actionsToStringList());
	pref->setValue("controlwidget", controlwidget->actionsToStringList());
	pref->setValue("controlwidget_mini", controlwidget_mini->actionsToStringList());
	TEditableToolbar* iw = static_cast<TEditableToolbar*>(floating_control->internalWidget());
	pref->setValue("floating_control", iw->actionsToStringList());
	pref->setValue("toolbar1_version", TOOLBAR_VERSION);
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("toolbar1", toolbar1->iconSize());
	pref->setValue("controlwidget", controlwidget->iconSize());
	pref->setValue("controlwidget_mini", controlwidget_mini->iconSize());
	pref->setValue("floating_control", iw->iconSize());
	pref->endGroup();

	pref->endGroup();
}

void TDefault::loadConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("Gui::TDefault::loadConfig");

	TBasePlus::loadConfig("default_gui");

	pref->beginGroup("default_gui");

	viewVideoInfoAct->setChecked(pref->value("video_info", false).toBool());
	viewFrameCounterAct->setChecked(pref->value("frame_counter", false).toBool());

	fullscreen_toolbar1_was_visible = pref->value("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible).toBool();
	fullscreen_toolbar2_was_visible = pref->value("fullscreen_toolbar2_was_visible", fullscreen_toolbar2_was_visible).toBool();
	compact_toolbar1_was_visible = pref->value("compact_toolbar1_was_visible", compact_toolbar1_was_visible).toBool();
	compact_toolbar2_was_visible = pref->value("compact_toolbar2_was_visible", compact_toolbar2_was_visible).toBool();

	pref->beginGroup("actions");
	int toolbar_version = pref->value("toolbar1_version", 0).toInt();
	if (toolbar_version >= TOOLBAR_VERSION) {
		toolbar1->setActionsFromStringList(pref->value("toolbar1", toolbar1->defaultActions()).toStringList());
	} else {
		qWarning("Gui::TDefault::loadConfig: toolbar too old, loading default one");
		toolbar1->setActionsFromStringList(toolbar1->defaultActions());
	}
	controlwidget->setActionsFromStringList(pref->value("controlwidget", controlwidget->defaultActions()).toStringList());
	controlwidget_mini->setActionsFromStringList(pref->value("controlwidget_mini", controlwidget_mini->defaultActions()).toStringList());
	TEditableToolbar* iw = static_cast<TEditableToolbar*>(floating_control->internalWidget());
	iw->setActionsFromStringList(pref->value("floating_control", iw->defaultActions()).toStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	toolbar1->setIconSize(pref->value("toolbar1", toolbar1->iconSize()).toSize());
	controlwidget->setIconSize(pref->value("controlwidget", controlwidget->iconSize()).toSize());
	controlwidget_mini->setIconSize(pref->value("controlwidget_mini", controlwidget_mini->iconSize()).toSize());
	iw->setIconSize(pref->value("floating_control", iw->iconSize()).toSize());
	pref->endGroup();

	floating_control->adjustSize();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();

	updateWidgets();

	if (pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
		toolbar2->hide();
	}
}

} // namespace Gui

#include "moc_default.cpp"

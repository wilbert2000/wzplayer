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
#include "gui/autohidetoolbar.h"
#include "desktopinfo.h"
#include "editabletoolbar.h"

#if DOCK_PLAYLIST
#include "playlistdock.h"
#endif

#define TOOLBAR_VERSION 1

using namespace Settings;

namespace Gui {

TDefault::TDefault()
	: TBasePlus()
	, statusbar_menu(0) {

	createStatusBar();
	createActions();
	createMainToolBars();
	createMenus();
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

QMenu* TDefault::createToolbarMenu() {

	QMenu* menu = new QMenu(this);
	menu->addAction(viewMenuBarAct);
	menu->addAction(toolbar->toggleViewAction());
	menu->addAction(toolbar2->toggleViewAction());
	menu->addAction(controlbar->toggleViewAction());
	menu->addAction(viewStatusBarAct);

	menu->addSeparator();
	menu->addAction(editToolbarAct);
	menu->addAction(editControlBarAct);

	if (!statusbar_menu) {
		statusbar_menu = new QMenu(this);
		statusbar_menu->addAction(viewVideoInfoAct);
		statusbar_menu->addAction(viewFrameCounterAct);
	}
	menu->addMenu(statusbar_menu);

	return menu;
}

void TDefault::createMenus() {

	toolbar_menu = createToolbarMenu();
	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);
}

QMenu* TDefault::createPopupMenu() {
	return createToolbarMenu();
}

void TDefault::createMainToolBars() {

	toolbar2 = new QToolBar(this);
	toolbar2->setObjectName("toolbar2");
	addToolBar(Qt::TopToolBarArea, toolbar2);

	select_audio = new QPushButton(this);
	select_audio->setMenu(audiotrack_menu);
	toolbar2->addWidget(select_audio);

	select_subtitle = new QPushButton(this);
	select_subtitle->setMenu(subtitles_track_menu);
	toolbar2->addWidget(select_subtitle);

	QAction* tba = toolbar2->toggleViewAction();
	tba->setObjectName("show_language_toolbar");
	tba->setShortcut(Qt::Key_F6);
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

	statusBar()->showMessage(tr("Ready"));
	statusBar()->addPermanentWidget(frame_display, 0);
	frame_display->setText("0");

	statusBar()->addPermanentWidget(time_display, 0);
	time_display->setText(" 00:00:00 / 00:00:00 ");

	time_display->show();
	frame_display->hide();
	ab_section_display->show();
	video_info_display->hide();

	connect(this, SIGNAL(timeChanged(QString)),
			 this, SLOT(displayTime(QString)));
	connect(this, SIGNAL(frameChanged(int)),
			 this, SLOT(displayFrame(int)));
	connect(this, SIGNAL(ABMarkersChanged(int,int)),
			 this, SLOT(displayABSection(int,int)));
	connect(this, SIGNAL(videoInfoChanged(int,int,double)),
			 this, SLOT(displayVideoInfo(int,int,double)));

}

void TDefault::retranslateStrings() {
	qDebug("Gui::TDefault::retranslateStrings");

	TBasePlus::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));

	toolbar_menu->menuAction()->setText(tr("&Toolbars"));
	toolbar_menu->menuAction()->setIcon(Images::icon("toolbars"));

	toolbar2->setWindowTitle(tr("&Language toolbar"));
	toolbar2->toggleViewAction()->setIcon(Images::icon("lang_toolbar"));

	select_audio->setText(tr("Audio"));
	select_subtitle->setText(tr("Subtitle"));

	statusbar_menu->menuAction()->setText(tr("Status&bar"));
	statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));

	viewVideoInfoAct->change(Images::icon("view_video_info"), tr("&Video info"));
	viewFrameCounterAct->change(Images::icon("frame_counter"), tr("&Frame counter"));
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
	if (secs_a >= 0)
		s = tr("A:%1").arg(Helper::formatTime(secs_a));

	if (secs_b >= 0) {
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

	pref->beginGroup("default_gui");
	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));
	if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
					  Helper::qtVersion())) {
		toolbar->hide();
		toolbar2->hide();
	}
	pref->endGroup();
}

void TDefault::aboutToExitFullscreen() {
	//qDebug("Gui::TDefault::aboutToExitFullscreen");

	TBasePlus::aboutToExitFullscreen();

	pref->beginGroup("default_gui");
	pref->setValue("toolbars_state_fullscreen", saveState(Helper::qtVersion()));
	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());
	pref->endGroup();
}

void TDefault::saveConfig(const QString&) {
	qDebug("Gui::TDefault::saveConfig");

	TBasePlus::saveConfig("default_gui");

	pref->beginGroup("default_gui");
	pref->setValue("video_info", viewVideoInfoAct->isChecked());
	pref->setValue("frame_counter", viewFrameCounterAct->isChecked());
	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));
	pref->endGroup();
}

void TDefault::loadConfig(const QString&) {
	qDebug("Gui::TDefault::loadConfig");

	TBasePlus::loadConfig("default_gui");

	pref->beginGroup("default_gui");
	viewVideoInfoAct->setChecked(pref->value("video_info", false).toBool());
	viewFrameCounterAct->setChecked(pref->value("frame_counter", false).toBool());
	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());
	pref->endGroup();

	viewMenuBarAct->setChecked(!menuBar()->isHidden());
	viewStatusBarAct->setChecked(!statusBar()->isHidden());

	updateWidgets();
}

} // namespace Gui

#include "moc_default.cpp"

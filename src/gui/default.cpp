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

#include "gui/default.h"

#include <QDebug>
#include <QMenu>
#include <QLabel>
#include <QStatusBar>
#include <QList>

#include "gui/action/action.h"
#include "colorutils.h"
#include "images.h"


using namespace Settings;

namespace Gui {

TDefault::TDefault() : TBasePlus() {

	createStatusBar();
	createActions();
	createMenus();
}

TDefault::~TDefault() {
}

void TDefault::createActions() {
	qDebug("Gui::TDefault::createActions");

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

void TDefault::createMenus() {

	statusbar_menu = new QMenu(this);
	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(viewFrameCounterAct);
	toolbar_menu->addSeparator();
	toolbar_menu->addMenu(statusbar_menu);
}

void TDefault::onStateChanged(TCore::State state) {
	qDebug("Gui::TDefault::onStateChanged");

	TBasePlus::onStateChanged(state);

	if (state == TCore::Playing) {
		playOrPauseAct->setIcon(Images::icon("pause"));
	} else {
		playOrPauseAct->setIcon(Images::icon("play"));
	}
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

	connect(core, SIGNAL(ABMarkersChanged()),
			 this, SLOT(displayABSection()));
	connect(core, SIGNAL(mediaLoaded()),
			 this, SLOT(displayABSection()));

	connect(this, SIGNAL(videoInfoChanged(int,int,double)),
			 this, SLOT(displayVideoInfo(int,int,double)));

}

void TDefault::retranslateStrings() {
	//qDebug("Gui::TDefault::retranslateStrings");

	TBasePlus::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));

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

void TDefault::displayABSection() {

	QString s;
	int secs = core->mset.A_marker;
	if (secs >= 0)
		s = tr("A:%1").arg(Helper::formatTime(secs));

	secs = core->mset.B_marker;
	if (secs >= 0) {
		if (!s.isEmpty()) s += " ";
		s += tr("B:%1").arg(Helper::formatTime(secs));
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

void TDefault::saveConfig() {
	qDebug("Gui::TDefault::saveConfig");

	TBasePlus::saveConfig();

	pref->beginGroup(settingsGroupName());
	pref->setValue("video_info", viewVideoInfoAct->isChecked());
	pref->setValue("frame_counter", viewFrameCounterAct->isChecked());
	pref->endGroup();
}

void TDefault::loadConfig() {
	qDebug("Gui::TDefault::loadConfig");

	TBasePlus::loadConfig();

	pref->beginGroup(settingsGroupName());
	viewVideoInfoAct->setChecked(pref->value("video_info", false).toBool());
	viewFrameCounterAct->setChecked(pref->value("frame_counter", false).toBool());
	pref->endGroup();

	updateWidgets();
}

} // namespace Gui

#include "moc_default.cpp"

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

#include "gui/default.h"

#include <QDebug>
#include <QMenu>
#include <QLabel>
#include <QStatusBar>
#include <QList>

#include "colorutils.h"
#include "images.h"
#include "core.h"
#include "gui/action/action.h"
#include "playerwindow.h"


using namespace Settings;

namespace Gui {

TDefault::TDefault() : TBasePlus() {

	createStatusBar();
	createActions();
}

TDefault::~TDefault() {
}

void TDefault::createActions() {
	qDebug("Gui::TDefault::createActions");

	// Statusbar
	viewVideoInfoAct = new Action::TAction(this, "toggle_video_info", tr("&Video info"), "view_video_info");
	viewVideoInfoAct->setCheckable(true);
	statusbar_menu->addAction(viewVideoInfoAct);
	connect(viewVideoInfoAct, SIGNAL(toggled(bool)), video_info_display, SLOT(setVisible(bool)));

    viewVideoTimeAct = new Action::TAction(this, "toggle_video_time", tr("&Video time"), "view_video_time");
    viewVideoTimeAct->setCheckable(true);
    statusbar_menu->addAction(viewVideoTimeAct);
    connect(viewVideoTimeAct, SIGNAL(toggled(bool)), time_display, SLOT(setVisible(bool)));

    viewFrameCounterAct = new Action::TAction(this, "toggle_frame_counter", tr("&Frame counter"), "frame_counter");
	viewFrameCounterAct->setCheckable(true);
	statusbar_menu->addAction(viewFrameCounterAct);
    connect(viewFrameCounterAct, SIGNAL(toggled(bool)),
            frame_display, SLOT(setVisible(bool)));
}

void TDefault::createStatusBar() {
	qDebug("Gui::TDefault::createStatusBar");

    const int margin = 1;

    frame_display = new QLabel(statusBar());
    frame_display->setObjectName("frame_display");
    frame_display->setAlignment(Qt::AlignRight);
    frame_display->setFrameShape(QFrame::NoFrame);
    frame_display->setMargin(margin);
    frame_display->setIndent(margin);
    frame_display->setText("0");
    frame_display->hide();

    time_display = new QLabel(statusBar());
	time_display->setObjectName("time_display");
    time_display->setAlignment(Qt::AlignRight);
	time_display->setFrameShape(QFrame::NoFrame);
    time_display->setMargin(margin);
    time_display->setText(" 00:00:00 / 00:00:00 ");
    time_display->hide();

    // Controls its own visibility, so no hide()
    in_out_points_label = new QLabel(statusBar());
    in_out_points_label->setObjectName("in_out_points_label");
    in_out_points_label->setAlignment(Qt::AlignRight);
    in_out_points_label->setFrameShape(QFrame::NoFrame);
    in_out_points_label->setMargin(margin);

	video_info_display = new QLabel(statusBar());
	video_info_display->setObjectName("video_info_display");
	video_info_display->setFrameShape(QFrame::NoFrame);
    video_info_display->setMargin(margin);
    video_info_display->setIndent(margin);
    video_info_display->hide();

	statusBar()->setAutoFillBackground(true);

	ColorUtils::setBackgroundColor(statusBar(), QColor(0,0,0));
	ColorUtils::setForegroundColor(statusBar(), QColor(255,255,255));
	ColorUtils::setBackgroundColor(time_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(time_display, QColor(255,255,255));
	ColorUtils::setBackgroundColor(frame_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(frame_display, QColor(255,255,255));
    ColorUtils::setBackgroundColor(in_out_points_label, QColor(0,0,0));
    ColorUtils::setForegroundColor(in_out_points_label, QColor(255,255,255));
	ColorUtils::setBackgroundColor(video_info_display, QColor(0,0,0));
	ColorUtils::setForegroundColor(video_info_display, QColor(255,255,255));
	statusBar()->setSizeGripEnabled(false);

    statusBar()->addWidget(video_info_display);
    statusBar()->addPermanentWidget(in_out_points_label);
    statusBar()->addPermanentWidget(time_display, 0);
    statusBar()->addPermanentWidget(frame_display, 0);

    connect(this, SIGNAL(timeChanged(QString)),
			this, SLOT(displayTime(QString)));
	connect(this, SIGNAL(frameChanged(int)),
			this, SLOT(displayFrame(int)));

	connect(core, SIGNAL(InOutPointsChanged()),
            this, SLOT(displayInOutPoints()));
	connect(core, SIGNAL(mediaLoaded()),
            this, SLOT(displayInOutPoints()));

	connect(playerwindow, SIGNAL(videoOutChanged(const QSize&)),
			this, SLOT(displayVideoInfo()), Qt::QueuedConnection);
}

void TDefault::displayTime(QString text) {
    time_display->setText(text);
}

void TDefault::displayFrame(int frame) {

	if (frame_display->isVisible()) {
		frame_display->setNum(frame);
	}
}

void TDefault::displayInOutPoints() {

	QString s;
	int secs = core->mset.in_point;
	if (secs >= 0)
        s = tr("I: %1", "In point in statusbar").arg(Helper::formatTime(secs));

	secs = core->mset.out_point;
	if (secs >= 0) {
		if (!s.isEmpty()) s += " ";
        s += " " + tr("O: %1", "Out point in statusbar").arg(Helper::formatTime(secs));
	}

    if (core->mset.loop) {
        if (!s.isEmpty()) s += " ";
        s += tr("R", "Repeat in-out in statusbar");
    }

    in_out_points_label->setVisible(!s.isEmpty());
    in_out_points_label->setText(s);
}

void TDefault::displayVideoInfo() {

	if (core->mdat.noVideo()) {
        video_info_display->setText("");
	} else {
		QSize video_out_size = playerwindow->lastVideoOutSize();
		video_info_display->setText(tr("%1x%2", "video source width x height")
			.arg(core->mdat.video_width)
			.arg(core->mdat.video_height)
			+ " " + QString::fromUtf8("\u279F") + " "
			+ tr("%1x%2 %3 fps", "video out width x height + fps")
			.arg(video_out_size.width())
			.arg(video_out_size.height())
			.arg(core->mdat.video_fps));
	}
}

// Slot called when media settings reset or loaded
void TDefault::onMediaSettingsChanged() {
    qDebug("Gui::TDefault::onMediaSettingsChanged");

    TBase::onMediaSettingsChanged();
    displayInOutPoints();
}

void TDefault::onMediaInfoChanged() {
	qDebug("Gui::TDefault::onMediaInfoChanged");

	TBasePlus::onMediaInfoChanged();
    displayVideoInfo();
}

void TDefault::saveConfig() {
	qDebug("Gui::TDefault::saveConfig");

	TBasePlus::saveConfig();

	pref->beginGroup(settingsGroupName());
	pref->setValue("video_info", viewVideoInfoAct->isChecked());
    pref->setValue("video_time", viewVideoTimeAct->isChecked());
    pref->setValue("frame_counter", viewFrameCounterAct->isChecked());
	pref->endGroup();
}

void TDefault::loadConfig() {
	qDebug("Gui::TDefault::loadConfig");

	TBasePlus::loadConfig();

	pref->beginGroup(settingsGroupName());
    viewVideoInfoAct->setChecked(pref->value("video_info", true).toBool());
    viewVideoTimeAct->setChecked(pref->value("video_time", true).toBool());
    viewFrameCounterAct->setChecked(pref->value("frame_counter", true).toBool());
	pref->endGroup();
}

} // namespace Gui

#include "moc_default.cpp"

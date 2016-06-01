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
#include <QLayout>

#include "colorutils.h"
#include "images.h"
#include "core.h"
#include "gui/action/action.h"
#include "playerwindow.h"


using namespace Settings;

namespace Gui {

TDefault::TDefault() :
    TBasePlus(),
    debug(logger()) {

	createStatusBar();
	createActions();
}

TDefault::~TDefault() {
}

void TDefault::createActions() {
    logger()->debug("createActions");

	// Statusbar
    viewVideoInfoAct = new Action::TAction(this, "toggle_video_info",
        tr("&Video info"), "view_video_info");
	viewVideoInfoAct->setCheckable(true);
    viewVideoInfoAct->setChecked(true);
    statusbar_menu->addAction(viewVideoInfoAct);
    connect(viewVideoInfoAct, SIGNAL(toggled(bool)),
            video_info_label, SLOT(setVisible(bool)));

    viewInOutPointsAct = new Action::TAction(this, "toggle_in_out_points",
        tr("&In-out points"), "view_in_out_points");
    viewInOutPointsAct->setCheckable(true);
    viewInOutPointsAct->setChecked(true);
    statusbar_menu->addAction(viewInOutPointsAct);
    connect(viewInOutPointsAct, SIGNAL(toggled(bool)),
            in_out_points_label, SLOT(setVisible(bool)));

    viewVideoTimeAct = new Action::TAction(this, "toggle_video_time",
        tr("&Video time"), "view_video_time");
    viewVideoTimeAct->setCheckable(true);
    viewVideoTimeAct->setChecked(true);
    statusbar_menu->addAction(viewVideoTimeAct);
    connect(viewVideoTimeAct, SIGNAL(toggled(bool)),
            time_label, SLOT(setVisible(bool)));

    viewFrameCounterAct = new Action::TAction(this, "toggle_frame_counter",
        tr("&Frame counter"), "frame_counter");
	viewFrameCounterAct->setCheckable(true);
    viewFrameCounterAct->setChecked(false);
    statusbar_menu->addAction(viewFrameCounterAct);
    connect(viewFrameCounterAct, SIGNAL(toggled(bool)),
            frame_label, SLOT(setVisible(bool)));
}

void TDefault::createStatusBar() {
    logger()->debug("createStatusBar");

    QColor bgc(0, 0, 0);
    QColor fgc(255, 255, 255);
    int margin = 3;
    QMargins margins(margin, 0, margin, 0);

    statusBar()->setSizeGripEnabled(false);
    ColorUtils::setBackgroundColor(statusBar(), bgc);
    ColorUtils::setForegroundColor(statusBar(), fgc);
    statusBar()->setContentsMargins(1, 1, 1, 1);

    video_info_label = new QLabel(statusBar());
    video_info_label->setObjectName("video_info_label");
    ColorUtils::setBackgroundColor(video_info_label, bgc);
    ColorUtils::setForegroundColor(video_info_label, fgc);
    video_info_label->setFrameShape(QFrame::NoFrame);
    video_info_label->setContentsMargins(margins);
    statusBar()->addWidget(video_info_label);
    connect(playerwindow, SIGNAL(videoOutChanged(const QSize&)),
            this, SLOT(displayVideoInfo()), Qt::QueuedConnection);

    in_out_points_label = new QLabel(statusBar());
    in_out_points_label->setObjectName("in_out_points_label");
    ColorUtils::setBackgroundColor(in_out_points_label, bgc);
    ColorUtils::setForegroundColor(in_out_points_label, fgc);
    in_out_points_label->setFrameShape(QFrame::NoFrame);
    in_out_points_label->setContentsMargins(margins);
    statusBar()->addPermanentWidget(in_out_points_label, 0);
    connect(core, SIGNAL(InOutPointsChanged()),
            this, SLOT(displayInOutPoints()));
    connect(core, SIGNAL(mediaSettingsChanged()),
            this, SLOT(displayInOutPoints()));

    time_label = new QLabel(statusBar());
    time_label->setObjectName("time_label");
    ColorUtils::setBackgroundColor(time_label, bgc);
    ColorUtils::setForegroundColor(time_label, fgc);
    time_label->setFrameShape(QFrame::NoFrame);
    time_label->setContentsMargins(margins);
    time_label->setText("00:00 / 00:00");
    statusBar()->addPermanentWidget(time_label, 0);
    connect(this, SIGNAL(timeChanged(QString)),
            this, SLOT(displayTime(QString)));

    frame_label = new QLabel(statusBar());
    frame_label->setObjectName("frame_label");
    ColorUtils::setBackgroundColor(frame_label, bgc);
    ColorUtils::setForegroundColor(frame_label, fgc);
    frame_label->setFrameShape(QFrame::NoFrame);
    margins.setLeft(0);
    frame_label->setContentsMargins(margins);
    frame_label->setText("00");
    frame_label->hide();
    statusBar()->addPermanentWidget(frame_label, 0);
	connect(this, SIGNAL(frameChanged(int)),
			this, SLOT(displayFrame(int)));
}

void TDefault::displayTime(QString text) {
    time_label->setText(text);
}

void TDefault::displayFrame(int frame) {

    if (frame_label->isVisible()) {
        if (core->mdat.video_fps > 0) {
            QString s = QString::number(frame % qRound(core->mdat.video_fps));
            if (s.length() < 2) {
                s = "0" + s;
            }
            frame_label->setText(s);
        } else {
            frame_label->setNum(frame);
        }
	}
}

void TDefault::displayInOutPoints() {

	QString s;
	int secs = core->mset.in_point;
    if (secs > 0)
        s = tr("I: %1", "In point in statusbar").arg(Helper::formatTime(secs));

	secs = core->mset.out_point;
    if (secs > 0) {
		if (!s.isEmpty()) s += " ";
        s += tr("O: %1", "Out point in statusbar").arg(Helper::formatTime(secs));
	}

    if (core->mset.loop) {
        if (!s.isEmpty()) s += " ";
        s += tr("R", "Repeat in-out in statusbar");
    }

    in_out_points_label->setText(s);
}

void TDefault::displayVideoInfo() {

	if (core->mdat.noVideo()) {
        video_info_label->setText("");
	} else {
		QSize video_out_size = playerwindow->lastVideoOutSize();
        video_info_label->setText(tr("%1x%2", "video source width x height")
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
    logger()->debug("onMediaSettingsChanged");

    TBase::onMediaSettingsChanged();
    displayInOutPoints();
}

void TDefault::onMediaInfoChanged() {
    logger()->debug("onMediaInfoChanged");

	TBasePlus::onMediaInfoChanged();
    displayVideoInfo();
}

void TDefault::saveConfig() {
    logger()->debug("saveConfig");

	TBasePlus::saveConfig();

	pref->beginGroup(settingsGroupName());
	pref->setValue("video_info", viewVideoInfoAct->isChecked());
    pref->setValue("in_out_points", viewInOutPointsAct->isChecked());
    pref->setValue("video_time", viewVideoTimeAct->isChecked());
    pref->setValue("frame_counter", viewFrameCounterAct->isChecked());
	pref->endGroup();
}

void TDefault::loadConfig() {
    logger()->debug("loadConfig");

	TBasePlus::loadConfig();

	pref->beginGroup(settingsGroupName());
    viewVideoInfoAct->setChecked(pref->value("video_info", true).toBool());
    viewInOutPointsAct->setChecked(pref->value("in_out_points", true).toBool());
    viewVideoTimeAct->setChecked(pref->value("video_time", true).toBool());
    viewFrameCounterAct->setChecked(pref->value("frame_counter", false).toBool());
	pref->endGroup();
}

} // namespace Gui

#include "moc_default.cpp"

/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

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

#include "gui/skin/mediabarpanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

#include "gui/skin/iconsetter.h"
#include "colorutils.h"

namespace Gui {
namespace Skin {


TMediaBarPanel::TMediaBarPanel(QWidget *parent) :
	QWidget(parent),ui(new Ui::TMediaBarPanel),core(0)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(53);
    QHBoxLayout* layout = new QHBoxLayout;
	playControlPanel = new TPlayControl(this);
	TIconSetter::instance()->playControl = playControlPanel;
	volumeControlPanel = new TVolumeControlPanel(this);
    volumeControlPanel->setObjectName("volume-control-panel");
	mediaPanel = new TMediaPanel(this);
    mediaPanel->setObjectName("media-panel");
	TIconSetter::instance()->mediaPanel = mediaPanel;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(playControlPanel);
    layout->addWidget(mediaPanel);
    layout->addWidget(volumeControlPanel);            
    setLayout(layout);

	connect(volumeControlPanel, SIGNAL(volumeChanged(int)), this, SIGNAL(volumeChanged(int)));
	connect(volumeControlPanel, SIGNAL(volumeSliderMoved(int)), this, SIGNAL(volumeSliderMoved(int)));
	connect(mediaPanel, SIGNAL(seekerChanged(int)), this, SIGNAL(seekerChanged(int)));
}

TMediaBarPanel::~TMediaBarPanel()
{
    delete ui;
}

void TMediaBarPanel::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void TMediaBarPanel::setToolbarActionCollection(QList<QAction *>actions) {
	TIconSetter::instance()->setToolbarActions(actions);
}

void TMediaBarPanel::setPlayControlActionCollection(QList<QAction *>actions)
{
    playControlPanel->setActionCollection(actions);
}

void TMediaBarPanel::setMediaPanelActionCollection(QList<QAction *>actions)
{
    mediaPanel->setActionCollection(actions);
}

void TMediaBarPanel::setMplayerState(Core::State state)
{
    mediaPanel->setMplayerState((int)state);
}

void TMediaBarPanel::setCore(Core *c)
{
    core = c;
    connect(core, SIGNAL(newDuration(double)), this, SLOT(setDuration()));
    connect(core, SIGNAL(showTime(double)), this, SLOT(gotCurrentTime(double)));
    connect(core, SIGNAL(mediaInfoChanged()), this, SLOT(updateMediaInfo()));
    connect(core, SIGNAL(buffering()), this, SLOT(setBuffering()));
	connect(mediaPanel, SIGNAL(seekerWheelUp()), core, SLOT(wheelUp()));
	connect(mediaPanel, SIGNAL(seekerWheelDown()), core, SLOT(wheelDown()));
}

void TMediaBarPanel::setDuration()
{
	qDebug("Gui::Skin::TMediaBarPanel::setDuration");

	mediaPanel->setDuration(core->mdat.duration);
}

void TMediaBarPanel::setVolumeControlActionCollection(QList<QAction *>actions)
{
    volumeControlPanel->setActionCollection(actions);
}

void TMediaBarPanel::gotCurrentTime(double time)
{
    mediaPanel->setElapsedText(Helper::formatTime((int)time));    
}

void TMediaBarPanel::updateMediaInfo()
{
    //QString s = QString("%1 (%2x%3)").arg(core->mdat.displayName()).arg(core->mdat.video_width).arg(core->mdat.video_height);
    mediaPanel->setMediaLabelText(core->mdat.displayName());

    if ((core->mdat.video_width != 0) && (core->mdat.video_height != 0)) {
        QString s = QString("%1x%2").arg(core->mdat.video_width).arg(core->mdat.video_height);
        mediaPanel->setResolutionLabelText(s);
    } else {
        mediaPanel->setResolutionLabelText(" ");
    }
}

void TMediaBarPanel::displayMessage(QString status, int time)
{
    mediaPanel->setStatusText(status, time);
}

void TMediaBarPanel::displayMessage(QString status)
{
    mediaPanel->setStatusText(status);
}

void TMediaBarPanel::displayPermanentMessage(QString status)
{
    mediaPanel->setStatusText(status, 0);
}

void TMediaBarPanel::setRecordAvailable(bool av)
{
    playControlPanel->setRecordEnabled(av);
}

void TMediaBarPanel::setBuffering()
{
    mediaPanel->setBuffering(true);
}

void TMediaBarPanel::setVolume(int v) {
	volumeControlPanel->setVolume(v); 
}

void TMediaBarPanel::setSeeker(int v) {
	mediaPanel->setSeeker(v);
}

void TMediaBarPanel::setResolutionVisible(bool b) {
	qDebug("Gui::Skin::TMediaBarPanel::setResolutionVisible: %d", b);
	mediaPanel->setResolutionVisible(b); 
}

void TMediaBarPanel::setScrollingEnabled(bool b) {
	qDebug("Gui::Skin::TMediaBarPanel::setScrollingEnabled: %d", b);
	mediaPanel->setScrollingEnabled(b);
}

} // namesapce Skin
} // namespace Gui

#include "moc_mediabarpanel.cpp"


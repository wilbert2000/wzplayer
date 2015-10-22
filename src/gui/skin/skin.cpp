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

#include "gui/skin/skin.h"
#include <QMenu>
#include <QMenuBar>
#include "gui/action.h"
#include "gui/playlist.h"
#include "gui/skin/mediabarpanel.h"
#include "gui/actionseditor.h"
#include "images.h"


using namespace Settings;

namespace Gui {

TSkin::TSkin() : TBasePlus() {

	createActions();
	createMenus();
	createControlWidget();
}

TSkin::~TSkin() {
}

void TSkin::createActions() {
	qDebug("Gui::TSkin::createActions");

	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new TSeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new TSeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");

	playOrPauseAct->setCheckable(true);

	viewVideoInfoAct = new TAction(this, "toggle_video_info_skingui");
	viewVideoInfoAct->setCheckable(true);

	scrollTitleAct = new TAction(this, "toggle_scroll_title_skingui");
	scrollTitleAct->setCheckable(true);
}

void TSkin::createMenus() {

	QFont font = menuBar()->font();
	font.setPixelSize(11);
	menuBar()->setFont(font);

	statusbar_menu = new QMenu(this);
	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(scrollTitleAct);
	toolbar_menu->addSeparator();
	toolbar_menu->addMenu(statusbar_menu);
}

void TSkin::togglePlayAction(TCore::State state) {
	qDebug("Gui::TSkin::togglePlayAction");
	TBasePlus::togglePlayAction(state);

	if (state == TCore::Playing) {
		playOrPauseAct->setChecked(true);
	}
	else {
		playOrPauseAct->setChecked(false);
	}
}

void TSkin::createControlWidget() {
	qDebug("Gui::TSkin::createControlWidget");

	mediaBarPanel = new Skin::TMediaBarPanel(panel, core);
	mediaBarPanel->setObjectName("mediabar-panel");

	QList<QAction*> actions;
	actions << rewind1Act << playPrevAct << playOrPauseAct << stopAct << playNextAct << forward1Act;
	mediaBarPanel->setPlayControlActionCollection(actions);

	actions.clear();
	QAction* shuffleAct = TActionsEditor::findAction(playlist, "pl_shuffle");
	QAction* repeatPlaylistAct = TActionsEditor::findAction(playlist, "pl_repeat");
	if (shuffleAct) actions << shuffleAct;
	if (repeatPlaylistAct) actions << repeatPlaylistAct;
	mediaBarPanel->setMediaPanelActionCollection(actions);

	actions.clear();
	actions << volumeslider_action << showPlaylistAct << fullscreenAct << videoEqualizerAct;
	mediaBarPanel->setVolumeControlActionCollection(actions);

	actions.clear();
	actions << openFileAct << openDirectoryAct << openDVDAct << openURLAct << screenshotAct << showPropertiesAct;
#ifdef FIND_SUBTITLES
	actions << showFindSubtitlesDialogAct;
#endif
	actions << showPreferencesAct;
	mediaBarPanel->setToolbarActionCollection(actions);

	connect(viewVideoInfoAct, SIGNAL(toggled(bool)),
			mediaBarPanel, SLOT(setResolutionVisible(bool)));
	connect(scrollTitleAct, SIGNAL(toggled(bool)),
			mediaBarPanel, SLOT(setScrollingEnabled(bool)));

	mediaBarPanelAction = controlbar->addWidget(mediaBarPanel);
}

void TSkin::retranslateStrings() {

	TBasePlus::retranslateStrings();

	viewVideoInfoAct->change(Images::icon("view_video_info"), tr("&Video info"));
	scrollTitleAct->change(Images::icon("scroll_title"), tr("&Scroll title"));
}

void TSkin::displayState(TCore::State state) {
	TBasePlus::displayState(state);

	switch (state) {
		case TCore::Playing: mediaBarPanel->displayMessage(tr("Playing %1").arg(core->mdat.filename)); break;
		case TCore::Paused: mediaBarPanel->displayMessage(tr("Pause")); break;
		case TCore::Stopped: mediaBarPanel->displayMessage(tr("Stop")); break;
	}
}

void TSkin::displayMessage(QString message, int time) {
	TBasePlus::displayMessage(message, time);
	mediaBarPanel->displayMessage(message, time);
}

void TSkin::displayMessage(QString message) {
	TBasePlus::displayMessage(message);
	mediaBarPanel->displayMessage(message);
}

void TSkin::saveConfig() {
	qDebug("Gui::TSkin::saveConfig");

	TBasePlus::saveConfig();

	pref->beginGroup(settingsGroupName());
	pref->setValue("video_info", viewVideoInfoAct->isChecked());
	pref->setValue("scroll_title", scrollTitleAct->isChecked());
	pref->endGroup();
}

void TSkin::loadConfig() {
	qDebug("Gui::TSkin::loadConfig");

	TBasePlus::loadConfig();

	pref->beginGroup(settingsGroupName());
	viewVideoInfoAct->setChecked(pref->value("video_info", false).toBool());
	scrollTitleAct->setChecked(pref->value("scroll_title", false).toBool());
	pref->endGroup();

	updateWidgets();
}

} // namespace Gui

#include "moc_skin.cpp"

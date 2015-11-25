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
#include "gui/action/action.h"
#include "gui/action/widgetactions.h"
#include "gui/action/editabletoolbar.h"
#include "gui/playlist.h"
#include "gui/skin/mediabarpanel.h"
#include "gui/action/actionseditor.h"
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

	viewVideoInfoAct = new TAction(this, "toggle_video_info_skingui", QT_TR_NOOP("&Video info"), "view_video_info");
	viewVideoInfoAct->setCheckable(true);

	scrollTitleAct = new TAction(this, "toggle_scroll_title_skingui", QT_TR_NOOP("&Scroll title"), "scroll_title");
	scrollTitleAct->setCheckable(true);
}

void TSkin::createMenus() {

	QFont font = menuBar()->font();
	font.setPixelSize(11);
	menuBar()->setFont(font);

	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(scrollTitleAct);
}

void TSkin::createControlWidget() {
	qDebug("Gui::TSkin::createControlWidget");

	mediaBarPanel = new Skin::TMediaBarPanel(panel, core);
	mediaBarPanel->setObjectName("mediabar-panel");

	QList<QAction*> actions;
	actions << findAction("rewind1") << findAction("play_prev")
			<< findAction("play_or_pause") << findAction("stop")
			<< findAction("play_next") << findAction("forward1");
	mediaBarPanel->setPlayControlActionCollection(actions);

	actions.clear();
	QAction* shuffleAct = TActionsEditor::findAction(playlist, "pl_shuffle");
	QAction* repeatPlaylistAct = TActionsEditor::findAction(playlist, "pl_repeat");
	if (shuffleAct) actions << shuffleAct;
	if (repeatPlaylistAct) actions << repeatPlaylistAct;
	mediaBarPanel->setMediaPanelActionCollection(actions);

	actions.clear();
	actions << volumeslider_action << findAction("show_playlist")
			<< findAction("fullscreen") << findAction("video_equalizer");
	mediaBarPanel->setVolumeControlActionCollection(actions);

	actions.clear();
	actions << findAction("open_file") << findAction("open_directory")
			<< findAction("open_url") << findAction("screenshot")
			<< findAction("show_file_properties")
			<< findAction("show_preferences");
	mediaBarPanel->setToolbarActionCollection(actions);

	connect(viewVideoInfoAct, SIGNAL(toggled(bool)),
			mediaBarPanel, SLOT(setResolutionVisible(bool)));
	connect(scrollTitleAct, SIGNAL(toggled(bool)),
			mediaBarPanel, SLOT(setScrollingEnabled(bool)));

	mediaBarPanelAction = controlbar->addWidget(mediaBarPanel);
}

void TSkin::displayMessage(const QString& message, int time) {
	TBasePlus::displayMessage(message, time);
	mediaBarPanel->displayMessage(message, time);
}

void TSkin::displayMessage(const QString& message) {
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
}

} // namespace Gui

#include "moc_skin.cpp"

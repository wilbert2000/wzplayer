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
#include "gui/action.h"
#include "gui/widgetactions.h"
#include "gui/editabletoolbar.h"
#include "gui/autohidewidget.h"
#include "gui/playlist.h"
#include "helper.h"
#include "colorutils.h"
#include "core.h"
#include "playerwindow.h"
#include "images.h"
#include "desktopinfo.h"
#include "mediabarpanel.h"
#include "gui/actionseditor.h"

#if DOCK_PLAYLIST
#include "gui/playlistdock.h"
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

using namespace Settings;

namespace Gui {

TSkin::TSkin()
	: TBasePlus() {

	createActions();
	createMainToolBars();
	createControlWidget();
	createMenus();

	connect(editToolbar1Act, SIGNAL(triggered()), toolbar1, SLOT(edit()));
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

	editToolbar1Act = new TAction(this, "edit_main_toolbar");

	playOrPauseAct->setCheckable(true);

	viewVideoInfoAct = new TAction(this, "toggle_video_info_skingui");
	viewVideoInfoAct->setCheckable(true);

	scrollTitleAct = new TAction(this, "toggle_scroll_title_skingui");
	scrollTitleAct->setCheckable(true);
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

void TSkin::createMenus() {
	menuBar()->setObjectName("menubar");
	QFont font = menuBar()->font();
	font.setPixelSize(11);
	menuBar()->setFont(font);
	/*menuBar()->setFixedHeight(21);*/

	toolbar_menu = new QMenu(this);
	toolbar_menu->addAction(toolbar1->toggleViewAction());

	toolbar_menu->addSeparator();
	toolbar_menu->addAction(editToolbar1Act);
	toolbar_menu->addAction(editFloatingControlAct);

	optionsMenu->addSeparator();
	optionsMenu->addMenu(toolbar_menu);

	statusbar_menu = new QMenu(this);
	statusbar_menu->addAction(viewVideoInfoAct);
	statusbar_menu->addAction(scrollTitleAct);
	optionsMenu->addMenu(statusbar_menu);
}

QMenu* TSkin::createPopupMenu() {
	QMenu* m = new QMenu(this);
	m->addAction(editToolbar1Act);
	m->addAction(editFloatingControlAct);
	return m;
}

void TSkin::createMainToolBars() {
	toolbar1 = new TEditableToolbar(this);
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


void TSkin::createControlWidget() {
	qDebug("Gui::TSkin::createControlWidget");

	controlwidget = new QToolBar(this);
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);
	controlwidget->setStyleSheet("QToolBar { spacing: 0px; }");
	controlwidget->setMovable(false);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

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

	mediaBarPanelAction = controlwidget->addWidget(mediaBarPanel);
}

void TSkin::retranslateStrings() {
	TBasePlus::retranslateStrings();

	toolbar_menu->menuAction()->setText(tr("&Toolbars"));
	toolbar_menu->menuAction()->setIcon(Images::icon("toolbars"));

	statusbar_menu->menuAction()->setText(tr("Status&bar"));
	statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));

	toolbar1->setWindowTitle(tr("&Main toolbar"));
	toolbar1->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

	editToolbar1Act->change(tr("Edit main &toolbar"));

	viewVideoInfoAct->change(Images::icon("view_video_info"), tr("&Video info"));
	scrollTitleAct->change(Images::icon("scroll_title"), tr("&Scroll title"));
}

void TSkin::displayState(TCore::State state) {
	TBasePlus::displayState(state);

	switch (state) {
		case TCore::Playing:		mediaBarPanel->displayMessage(tr("Playing %1").arg(core->mdat.filename)); break;
		case TCore::Paused:		mediaBarPanel->displayMessage(tr("Pause")); break;
		case TCore::Stopped:		mediaBarPanel->displayMessage(tr("Stop")); break;
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

void TSkin::aboutToEnterFullscreen() {
	qDebug("Gui::TSkin::aboutToEnterFullscreen");

	TBasePlus::aboutToEnterFullscreen();

	controlwidget->removeAction(mediaBarPanelAction);
	floating_control->layout()->addWidget(mediaBarPanel);
	mediaBarPanel->show();
	floating_control->adjustSize();

	// Save visibility of toolbars
	fullscreen_toolbar1_was_visible = toolbar1->isVisible();

	if (!pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
	}
}

void TSkin::aboutToExitFullscreen() {
	qDebug("Gui::TSkin::aboutToExitFullscreen");

	TBasePlus::aboutToExitFullscreen();

	floating_control->layout()->removeWidget(mediaBarPanel);
	mediaBarPanelAction = controlwidget->addWidget(mediaBarPanel);

	if (!pref->compact_mode) {
		statusBar()->hide();
		controlwidget->show();
		toolbar1->setVisible(fullscreen_toolbar1_was_visible);
	}
}

void TSkin::aboutToEnterCompactMode() {

	TBasePlus::aboutToEnterCompactMode();

	// Save visibility of toolbars
	compact_toolbar1_was_visible = toolbar1->isVisible();

	controlwidget->hide();
	toolbar1->hide();
}

void TSkin::aboutToExitCompactMode() {
	TBasePlus::aboutToExitCompactMode();

	statusBar()->hide();
	controlwidget->show();
	toolbar1->setVisible(compact_toolbar1_was_visible);

	// Recheck size of controlwidget
	/* resizeEvent(new QResizeEvent(size(), size())); */
}

void TSkin::saveConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("Gui::TSkin::saveConfig");

	TBasePlus::saveConfig("skin_gui");

	pref->beginGroup("skin_gui");

	pref->setValue("video_info", viewVideoInfoAct->isChecked());
	pref->setValue("scroll_title", scrollTitleAct->isChecked());

	pref->setValue("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible);
	pref->setValue("compact_toolbar1_was_visible", compact_toolbar1_was_visible);

	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

	pref->beginGroup("actions");
	pref->setValue("toolbar1", toolbar1->actionsToStringList());
	pref->setValue("toolbar1_version", TOOLBAR_VERSION);
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("toolbar1", toolbar1->iconSize());
	pref->endGroup();

	pref->endGroup();
}

void TSkin::loadConfig(const QString &group) {
	Q_UNUSED(group)
	qDebug("Gui::TSkin::loadConfig");

	TBasePlus::loadConfig("skin_gui");

	pref->beginGroup("skin_gui");

	viewVideoInfoAct->setChecked(pref->value("video_info", false).toBool());
	scrollTitleAct->setChecked(pref->value("scroll_title", false).toBool());

	fullscreen_toolbar1_was_visible = pref->value("fullscreen_toolbar1_was_visible", fullscreen_toolbar1_was_visible).toBool();
	compact_toolbar1_was_visible = pref->value("compact_toolbar1_was_visible", compact_toolbar1_was_visible).toBool();

	pref->beginGroup("actions");
	int toolbar_version = pref->value("toolbar1_version", 0).toInt();
	if (toolbar_version >= TOOLBAR_VERSION) {
		toolbar1->setActionsFromStringList(pref->value("toolbar1", toolbar1->defaultActions()).toStringList());
	} else {
		qWarning("Gui::TSkin::loadConfig: toolbar too old, loading default one");
		toolbar1->setActionsFromStringList(toolbar1->defaultActions());
	}
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	toolbar1->setIconSize(pref->value("toolbar1", toolbar1->iconSize()).toSize());
	pref->endGroup();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();

	if (pref->compact_mode) {
		controlwidget->hide();
		toolbar1->hide();
	}
	statusBar()->hide();

	updateWidgets();
}

} // namespace Gui

#include "moc_skin.cpp"

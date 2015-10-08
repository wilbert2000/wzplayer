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

#include "mini.h"

#include <QStatusBar>
#include <QMenu>
#include <QTimer>

#include "widgetactions.h"
#include "autohidewidget.h"
#include "gui/action.h"
#include "playerwindow.h"
#include "helper.h"
#include "desktopinfo.h"
#include "editabletoolbar.h"
#include "images.h"

using namespace Settings;

namespace Gui {

TMini::TMini()
	: TBaseEdit	 ()
{
	statusBar()->hide();
}

TMini::~TMini() {
}

QMenu* TMini::createPopupMenu() {
	QMenu* m = new QMenu(this);
	m->addAction(editControlWidgetAct);
	m->addAction(editFloatingControlAct);
	return m;
}

void TMini::retranslateStrings() {
	qDebug("Gui::TMini::retranslateStrings");

	TBaseEdit::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));
}

void TMini::togglePlayAction(TCore::State state) {
	qDebug("Gui::TMini::togglePlayAction");
	TBaseEdit::togglePlayAction(state);

	if (state == TCore::Playing) {
		playOrPauseAct->setIcon(Images::icon("pause"));
	} else {
		playOrPauseAct->setIcon(Images::icon("play"));
	}
}

void TMini::aboutToEnterFullscreen() {

	TBaseEdit::aboutToEnterFullscreen();
}

void TMini::aboutToExitFullscreen() {

	TBaseEdit::aboutToExitFullscreen();

	floating_control->deactivate();

	if (!pref->compact_mode) {
		statusBar()->hide();
	}
}

void TMini::aboutToEnterCompactMode() {

	TBaseEdit::aboutToEnterCompactMode();
}

void TMini::aboutToExitCompactMode() {

	TBaseEdit::aboutToExitCompactMode();
	statusBar()->hide();
}

void TMini::saveConfig(const QString &gui_group) {
	Q_UNUSED(gui_group)

	TBaseEdit::saveConfig("mini_gui");

	pref->beginGroup("mini_gui");
	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

	pref->beginGroup("actions");
	TEditableToolbar* iw = static_cast<TEditableToolbar *>(floating_control->internalWidget());
	pref->setValue("floating_control", iw->actionsToStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("floating_control", iw->iconSize());
	pref->endGroup();

	pref->endGroup();
}

void TMini::loadConfig(const QString &gui_group) {
	Q_UNUSED(gui_group)

	TBaseEdit::loadConfig("mini_gui");

	pref->beginGroup("mini_gui");

	pref->beginGroup("actions");
	TEditableToolbar* iw = static_cast<TEditableToolbar *>(floating_control->internalWidget());
	iw->setActionsFromStringList(pref->value("floating_control", iw->defaultActions()).toStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	iw->setIconSize(pref->value("floating_control", iw->iconSize()).toSize());
	pref->endGroup();

	floating_control->adjustSize();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();
} // loadConfig

} // namespace Gui

#include "moc_mini.cpp"


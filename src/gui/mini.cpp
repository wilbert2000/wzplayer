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
	: TBasePlus()
{
	createActions();
	createControlWidget();
	createFloatingControl();

	connect( editControlAct, SIGNAL(triggered()), controlwidget, SLOT(edit()) );
	TEditableToolbar * iw = static_cast<TEditableToolbar *>(floating_control->internalWidget());
	iw->takeAvailableActionsFrom(this);
	connect( editFloatingControlAct, SIGNAL(triggered()), iw, SLOT(edit()) );

	statusBar()->hide();
}

TMini::~TMini() {
}

QMenu * TMini::createPopupMenu() {
	QMenu * m = new QMenu(this);
	m->addAction(editControlAct);
	m->addAction(editFloatingControlAct);
	return m;
}

void TMini::createActions() {
	timeslider_action = createTimeSliderAction(this);

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
#endif

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif

	time_label_action = new TTimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	connect( this, SIGNAL(timeChanged(QString)),
             time_label_action, SLOT(setText(QString)) );

	editControlAct = new TAction( this, "edit_control_minigui" );
	editFloatingControlAct = new TAction( this, "edit_floating_control_minigui" );
}


void TMini::createControlWidget() {
	controlwidget = new TEditableToolbar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);
	controlwidget->setMovable(true);
	controlwidget->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, controlwidget);

	QStringList controlwidget_actions;
	controlwidget_actions << "play_or_pause" << "stop" << "separator" << "timeslider_action" << "separator"
                          << "fullscreen" << "mute" << "volumeslider_action";
	controlwidget->setDefaultActions(controlwidget_actions);
}

void TMini::createFloatingControl() {
	// Floating control
	floating_control = new TAutohideWidget(panel, playerwindow);
	floating_control->setAutoHide(true);

	TEditableToolbar * iw = new TEditableToolbar(floating_control);
	iw->setObjectName("floating_control");
	connect(iw, SIGNAL(iconSizeChanged(const QSize &)), this, SLOT(adjustFloatingControlSize()));

	QStringList floatingcontrol_actions;
	floatingcontrol_actions << "play_or_pause" << "stop" << "separator" << "timeslider_action" << "separator"
                            << "fullscreen" << "mute";
	#if USE_VOLUME_BAR
	floatingcontrol_actions << "volumeslider_action";
	#endif
	floatingcontrol_actions << "separator" << "timelabel_action";
	iw->setDefaultActions(floatingcontrol_actions);

	floating_control->setInternalWidget(iw);

	floating_control->hide();
}

void TMini::retranslateStrings() {
	qDebug("Gui::TMini::retranslateStrings");

	TBasePlus::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));

	controlwidget->setWindowTitle( tr("Control bar") );

	editControlAct->change( tr("Edit &control bar") );
	editFloatingControlAct->change( tr("Edit &floating control") );
}

void TMini::enableActionsOnPlaying() {
	TBasePlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void TMini::disableActionsOnStop() {
	TBasePlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}

void TMini::togglePlayAction(TCore::State state) {
	qDebug("Gui::TMini::togglePlayAction");
	TBasePlus::togglePlayAction(state);

	if (state == TCore::Playing) {
		playOrPauseAct->setIcon(Images::icon("pause"));
	} else {
		playOrPauseAct->setIcon(Images::icon("play"));
	}
}

void TMini::aboutToEnterFullscreen() {
	TBasePlus::aboutToEnterFullscreen();

	floating_control->setMargin(pref->floating_control_margin);
	floating_control->setPercWidth(pref->floating_control_width);
	floating_control->setAnimated(pref->floating_control_animated);
	floating_control->setActivationArea( (TAutohideWidget::Activation) pref->floating_activation_area);
	floating_control->setHideDelay(pref->floating_hide_delay);
	QTimer::singleShot(100, floating_control, SLOT(activate()));

	if (!pref->compact_mode) {
		controlwidget->hide();
	}
}

void TMini::aboutToExitFullscreen() {
	TBasePlus::aboutToExitFullscreen();

	floating_control->deactivate();
	//floating_control->hide();

	if (!pref->compact_mode) {
		statusBar()->hide();
		controlwidget->show();
	}
}

void TMini::aboutToEnterCompactMode() {
	TBasePlus::aboutToEnterCompactMode();

	controlwidget->hide();
}

void TMini::aboutToExitCompactMode() {
	TBasePlus::aboutToExitCompactMode();

	statusBar()->hide();

	controlwidget->show();
}

void TMini::adjustFloatingControlSize() {
	qDebug("Gui::TMini::adjustFloatingControlSize");

	QWidget *iw = floating_control->internalWidget();
	QSize iws = iw->size();
	QMargins m = floating_control->contentsMargins();
	int new_height = iws.height() + m.top() + m.bottom();
	if (new_height < 32) new_height = 32;
	floating_control->resize(floating_control->width(), new_height);
}

void TMini::saveConfig(const QString &group) {
	Q_UNUSED(group)

	TBasePlus::saveConfig("mini_gui");

	pref->beginGroup("mini_gui");

	pref->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

	pref->beginGroup( "actions" );
	pref->setValue("controlwidget", controlwidget->actionsToStringList() );
	TEditableToolbar * iw = static_cast<TEditableToolbar *>(floating_control->internalWidget());
	pref->setValue("floating_control", iw->actionsToStringList() );
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("controlwidget", controlwidget->iconSize());
	pref->setValue("floating_control", iw->iconSize());
	pref->endGroup();

	pref->endGroup();
}

void TMini::loadConfig(const QString &group) {
	Q_UNUSED(group)

	TBasePlus::loadConfig("mini_gui");

	pref->beginGroup("mini_gui");

	pref->beginGroup( "actions" );
	controlwidget->setActionsFromStringList( pref->value("controlwidget", controlwidget->defaultActions()).toStringList() );
	TEditableToolbar * iw = static_cast<TEditableToolbar *>(floating_control->internalWidget());
	iw->setActionsFromStringList( pref->value("floating_control", iw->defaultActions()).toStringList() );
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	controlwidget->setIconSize(pref->value("controlwidget", controlwidget->iconSize()).toSize());
	iw->setIconSize(pref->value("floating_control", iw->iconSize()).toSize());
	pref->endGroup();

	floating_control->adjustSize();

	restoreState( pref->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	pref->endGroup();

	if (pref->compact_mode) {
		controlwidget->hide();
	}
}

} // namespace Gui

#include "moc_mini.cpp"


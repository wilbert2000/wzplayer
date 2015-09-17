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

#include "minigui.h"
#include "widgetactions.h"
#include "autohidewidget.h"
#include "myaction.h"
#include "mplayerwindow.h"
#include "global.h"
#include "helper.h"
#include "desktopinfo.h"
#include "editabletoolbar.h"
#include "images.h"
#include <QStatusBar>
#include <QMenu>

using namespace Global;

MiniGui::MiniGui( QWidget * parent, Qt::WindowFlags flags )
	: Gui::TBasePlus( parent, flags )
{
	createActions();
	createControlWidget();
	createFloatingControl();

	connect( editControlAct, SIGNAL(triggered()), controlwidget, SLOT(edit()) );
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	iw->takeAvailableActionsFrom(this);
	connect( editFloatingControlAct, SIGNAL(triggered()), iw, SLOT(edit()) );

	statusBar()->hide();
}

MiniGui::~MiniGui() {
}

QMenu * MiniGui::createPopupMenu() {
	QMenu * m = new QMenu(this);
	m->addAction(editControlAct);
	m->addAction(editFloatingControlAct);
	return m;
}

void MiniGui::createActions() {
	timeslider_action = createTimeSliderAction(this);

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
#endif

#if AUTODISABLE_ACTIONS
	timeslider_action->disable();
	#if USE_VOLUME_BAR
	volumeslider_action->disable();
	#endif
#endif

	time_label_action = new TimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	connect( this, SIGNAL(timeChanged(QString)),
             time_label_action, SLOT(setText(QString)) );

	editControlAct = new MyAction( this, "edit_control_minigui" );
	editFloatingControlAct = new MyAction( this, "edit_floating_control_minigui" );
}


void MiniGui::createControlWidget() {
	controlwidget = new EditableToolbar( this );
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

void MiniGui::createFloatingControl() {
	// Floating control
	floating_control = new AutohideWidget(panel, mplayerwindow);
	floating_control->setAutoHide(true);

	EditableToolbar * iw = new EditableToolbar(floating_control);
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

void MiniGui::retranslateStrings() {
	qDebug("MiniGui::retranslateStrings");

	Gui::TBasePlus::retranslateStrings();

	// Change the icon of the play/pause action
	playOrPauseAct->setIcon(Images::icon("play"));

	controlwidget->setWindowTitle( tr("Control bar") );

	editControlAct->change( tr("Edit &control bar") );
	editFloatingControlAct->change( tr("Edit &floating control") );
}

#if AUTODISABLE_ACTIONS
void MiniGui::enableActionsOnPlaying() {
	Gui::TBasePlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void MiniGui::disableActionsOnStop() {
	Gui::TBasePlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}
#endif // AUTODISABLE_ACTIONS

void MiniGui::togglePlayAction(Core::State state) {
	qDebug("MiniGui::togglePlayAction");
	TBasePlus::togglePlayAction(state);

	if (state == Core::Playing) {
		playOrPauseAct->setIcon(Images::icon("pause"));
	} else {
		playOrPauseAct->setIcon(Images::icon("play"));
	}
}

void MiniGui::aboutToEnterFullscreen() {
	Gui::TBasePlus::aboutToEnterFullscreen();

	floating_control->setMargin(pref->floating_control_margin);
	floating_control->setPercWidth(pref->floating_control_width);
	floating_control->setAnimated(pref->floating_control_animated);
	floating_control->setActivationArea( (AutohideWidget::Activation) pref->floating_activation_area);
	floating_control->setHideDelay(pref->floating_hide_delay);
	QTimer::singleShot(100, floating_control, SLOT(activate()));

	if (!pref->compact_mode) {
		controlwidget->hide();
	}
}

void MiniGui::aboutToExitFullscreen() {
	Gui::TBasePlus::aboutToExitFullscreen();

	floating_control->deactivate();
	//floating_control->hide();

	if (!pref->compact_mode) {
		statusBar()->hide();
		controlwidget->show();
	}
}

void MiniGui::aboutToEnterCompactMode() {
	Gui::TBasePlus::aboutToEnterCompactMode();

	controlwidget->hide();
}

void MiniGui::aboutToExitCompactMode() {
	Gui::TBasePlus::aboutToExitCompactMode();

	statusBar()->hide();

	controlwidget->show();
}

#if USE_MINIMUMSIZE
QSize MiniGui::minimumSizeHint() const {
	return QSize(controlwidget->sizeHint().width(), 0);
}
#endif

void MiniGui::adjustFloatingControlSize() {
	qDebug("MiniGui::adjustFloatingControlSize");
	//floating_control->adjustSize();
	QWidget *iw = floating_control->internalWidget();
	QSize iws = iw->size();
	QMargins m = floating_control->contentsMargins();
	int new_height = iws.height() + m.top() + m.bottom();
	if (new_height < 32) new_height = 32;
	floating_control->resize(floating_control->width(), new_height);
}

void MiniGui::saveConfig(const QString &group) {
	Q_UNUSED(group)

	Gui::TBasePlus::saveConfig("mini_gui");

	QSettings * set = settings;
	set->beginGroup("mini_gui");

	set->setValue( "toolbars_state", saveState(Helper::qtVersion()) );

	set->beginGroup( "actions" );
	set->setValue("controlwidget", controlwidget->actionsToStringList() );
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	set->setValue("floating_control", iw->actionsToStringList() );
	set->endGroup();

	set->beginGroup("toolbars_icon_size");
	set->setValue("controlwidget", controlwidget->iconSize());
	set->setValue("floating_control", iw->iconSize());
	set->endGroup();

	set->endGroup();
}

void MiniGui::loadConfig(const QString &group) {
	Q_UNUSED(group)

	Gui::TBasePlus::loadConfig("mini_gui");

	QSettings * set = settings;
	set->beginGroup("mini_gui");

	set->beginGroup( "actions" );
	controlwidget->setActionsFromStringList( set->value("controlwidget", controlwidget->defaultActions()).toStringList() );
	EditableToolbar * iw = static_cast<EditableToolbar *>(floating_control->internalWidget());
	iw->setActionsFromStringList( set->value("floating_control", iw->defaultActions()).toStringList() );
	set->endGroup();

	set->beginGroup("toolbars_icon_size");
	controlwidget->setIconSize(set->value("controlwidget", controlwidget->iconSize()).toSize());
	iw->setIconSize(set->value("floating_control", iw->iconSize()).toSize());
	set->endGroup();

	floating_control->adjustSize();

	restoreState( set->value( "toolbars_state" ).toByteArray(), Helper::qtVersion() );

	set->endGroup();

	if (pref->compact_mode) {
		controlwidget->hide();
	}
}

#include "moc_minigui.cpp"


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

#include "widgetactions.h"
#include "colorutils.h"
#include <QLabel>

#if MINI_ARROW_BUTTONS
#include <QToolButton>
#endif

namespace Gui {

TWidgetAction::TWidgetAction( QWidget * parent )
	: QWidgetAction(parent)
{
	custom_style = 0;
	custom_stylesheet = "";
}

TWidgetAction::~TWidgetAction() {
}

void TWidgetAction::enable() {
	propagate_enabled(true);
}

void TWidgetAction::disable() {
	propagate_enabled(false);
}

void TWidgetAction::propagate_enabled(bool b) {
	QList<QWidget *> l = createdWidgets();
	for (int n=0; n < l.count(); n++) {
		TTimeSlider *s = (TTimeSlider*) l[n];
		s->setEnabled(b);;
	}
	setEnabled(b);
}


TTimeSliderAction::TTimeSliderAction( QWidget * parent )
	: TWidgetAction(parent), drag_delay(200) {
}

TTimeSliderAction::~TTimeSliderAction() {
}

void TTimeSliderAction::setPos(int v) {
	QList<QWidget *> l = createdWidgets();
	for (int n=0; n < l.count(); n++) {
		TTimeSlider *s = (TTimeSlider*) l[n];
		bool was_blocked= s->blockSignals(true);
		s->setPos(v);
		s->blockSignals(was_blocked);
	}
}

int TTimeSliderAction::pos() {
	QList<QWidget *> l = createdWidgets();
	if (l.count() >= 1) {
		TTimeSlider *s = (TTimeSlider*) l[0];
		return s->pos();
	} else {
		return -1;
	}
}

void TTimeSliderAction::setDragDelay(int d) {
	drag_delay = d;

	QList<QWidget *> l = createdWidgets();
	for (int n=0; n < l.count(); n++) {
		TTimeSlider *s = (TTimeSlider*) l[n];
		s->setDragDelay(drag_delay);
	}
}

int TTimeSliderAction::dragDelay() {
	return drag_delay;
}

QWidget * TTimeSliderAction::createWidget ( QWidget * parent ) {
	TTimeSlider *t = new TTimeSlider(parent);
	t->setEnabled( isEnabled() );

	if (custom_style) t->setStyle(custom_style);
	if (!custom_stylesheet.isEmpty()) t->setStyleSheet(custom_stylesheet);

	connect( t,    SIGNAL(posChanged(int)), 
             this, SIGNAL(posChanged(int)) );
	connect( t,    SIGNAL(draggingPos(int)),
             this, SIGNAL(draggingPos(int)) );

	t->setDragDelay(drag_delay);
	connect( t,    SIGNAL(delayedDraggingPos(int)),
	         this, SIGNAL(delayedDraggingPos(int)) );

	connect(t, SIGNAL(wheelUp()), this, SIGNAL(wheelUp()));
	connect(t, SIGNAL(wheelDown()), this, SIGNAL(wheelDown()));

	return t;
}


TVolumeSliderAction::TVolumeSliderAction( QWidget * parent )
	: TWidgetAction(parent)
{
	tick_position = QSlider::TicksBelow;
}

TVolumeSliderAction::~TVolumeSliderAction() {
}

void TVolumeSliderAction::setValue(int v) {
	QList<QWidget *> l = createdWidgets();
	for (int n=0; n < l.count(); n++) {
		TSlider *s = (TSlider*) l[n];
		bool was_blocked = s->blockSignals(true);
		s->setValue(v);
		s->blockSignals(was_blocked);
	}
}

int TVolumeSliderAction::value() {
	QList<QWidget *> l = createdWidgets();
	if (l.count() >= 1) {
		TSlider *s = (TSlider*) l[0];
		return s->value();
	} else {
		return -1;
	}
}

void TVolumeSliderAction::setTickPosition(QSlider::TickPosition position) {
	// For new widgets
	tick_position = position; 

	// Propagate changes to all existing widgets
	QList<QWidget *> l = createdWidgets();
	for (int n=0; n < l.count(); n++) {
		TSlider *s = (TSlider*) l[n];
		s->setTickPosition(tick_position);
	}
}

QWidget * TVolumeSliderAction::createWidget ( QWidget * parent ) {
	TSlider *t = new TSlider(parent);

	if (custom_style) t->setStyle(custom_style);
	if (!custom_stylesheet.isEmpty()) t->setStyleSheet(custom_stylesheet);
	if (fixed_size.isValid()) t->setFixedSize(fixed_size);

	t->setMinimum(0);
	t->setMaximum(100);
	t->setValue(50);
	t->setOrientation( Qt::Horizontal );
	t->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	t->setFocusPolicy( Qt::NoFocus );
	t->setTickPosition( tick_position );
	t->setTickInterval( 10 );
	t->setSingleStep( 1 );
	t->setPageStep( 10 );
	t->setToolTip( tr("Volume") );
	t->setEnabled( isEnabled() );
	t->setAttribute(Qt::WA_NoMousePropagation);

	connect( t,    SIGNAL(valueChanged(int)), 
             this, SIGNAL(valueChanged(int)) );
	return t;
}


TTimeLabelAction::TTimeLabelAction( QWidget * parent )
	: TWidgetAction(parent)
{
}

TTimeLabelAction::~TTimeLabelAction() {
}

void TTimeLabelAction::setText(QString s) {
	_text = s;
	emit newText(s);
}

QWidget * TTimeLabelAction::createWidget ( QWidget * parent ) {
	QLabel * time_label = new QLabel(parent);
	time_label->setObjectName("time_label");
    time_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    time_label->setAutoFillBackground(true);

    ColorUtils::setBackgroundColor( time_label, QColor(0,0,0) );
    ColorUtils::setForegroundColor( time_label, QColor(255,255,255) );
    time_label->setText( "00:00:00 / 00:00:00" );
    time_label->setFrameShape( QFrame::Panel );
    time_label->setFrameShadow( QFrame::Sunken );

	connect( this, SIGNAL(newText(QString)), 
             time_label, SLOT(setText(QString)) );

	return time_label;
}

#if MINI_ARROW_BUTTONS
TSeekingButton::TSeekingButton( QList<QAction*> actions, QWidget * parent )
	: QWidgetAction(parent)
{
	_actions = actions;
}

TSeekingButton::~TSeekingButton() {
}

QWidget * TSeekingButton::createWidget( QWidget * parent ) {
	QToolButton * button = new QToolButton(parent);
	button->setPopupMode(QToolButton::MenuButtonPopup);

	if (_actions.count() > 0 ) {
		button->setDefaultAction( _actions[0] );
	}
	for (int n = 1; n < _actions.count(); n++) {
		button->addAction( _actions[n] );
	}

	return button;
}
#endif

} // namespace Gui

#include "moc_widgetactions.cpp"

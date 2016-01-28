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

#include "gui/action/widgetactions.h"
#include <QDebug>
#include <QLabel>
#include <QToolButton>
#include <QToolBar>
#include "colorutils.h"
#include "gui/action/timeslider.h"

namespace Gui {
namespace Action {

TWidgetAction::TWidgetAction(QWidget* parent)
	: QWidgetAction(parent)
	, custom_style(0) {
}

TWidgetAction::~TWidgetAction() {
}

void TWidgetAction::enable(bool e) {
	propagate_enabled(e);
}

void TWidgetAction::disable() {
	propagate_enabled(false);
}

void TWidgetAction::propagate_enabled(bool b) {

	QList<QWidget*> l = createdWidgets();
	for (int n = 0; n < l.count(); n++) {
		l[n]->setEnabled(b);;
	}
	setEnabled(b);
}


TTimeSliderAction::TTimeSliderAction(QWidget* parent , int max_position, int delay)
	: TWidgetAction(parent),
	  max_pos(max_position),
	  drag_delay(delay) {
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

void TTimeSliderAction::setDuration(double t) {
	//qDebug() << "Gui::Action::TTimeSliderAction::setDuration:" << t;

	total_time = t;
	QList<QWidget*> l = createdWidgets();
	for (int n = 0; n < l.count(); n++) {
		TTimeSlider* s = (TTimeSlider*) l[n];
		s->setDuration(t);
	}
}

QWidget* TTimeSliderAction::createWidget(QWidget* parent) {

	TTimeSlider* slider = new TTimeSlider(parent, max_pos, drag_delay);
	slider->setEnabled(isEnabled());
	slider->setDuration(total_time);

	if (custom_style)
		slider->setStyle(custom_style);
	if (!custom_stylesheet.isEmpty())
		slider->setStyleSheet(custom_stylesheet);

	connect(slider, SIGNAL(posChanged(int)),
			this, SIGNAL(posChanged(int)));
	connect(slider, SIGNAL(draggingPos(int)),
			this, SIGNAL(draggingPos(int)));
	connect(slider, SIGNAL(delayedDraggingPos(int)),
			this, SIGNAL(delayedDraggingPos(int)));

	connect(slider, SIGNAL(wheelUp()),
			this, SIGNAL(wheelUp()));
	connect(slider, SIGNAL(wheelDown()),
			this, SIGNAL(wheelDown()));

	return slider;
}


TVolumeSliderAction::TVolumeSliderAction(QWidget* parent, int vol)
	: TWidgetAction(parent)
	, volume(vol)
	, tick_position(QSlider::TicksBelow) {
}

TVolumeSliderAction::~TVolumeSliderAction() {
}

void TVolumeSliderAction::setValue(int v) {
	//qDebug("Gui::Action::TVolumeSliderAction::setValue: %d", v);

	volume = v;
	QList<QWidget*> l = createdWidgets();
	for (int n = 0; n < l.count(); n++) {
		TSlider* s = (TSlider*) l[n];
		bool was_blocked = s->blockSignals(true);
		s->setValue(v);
		s->blockSignals(was_blocked);
	}
}

int TVolumeSliderAction::value() {
	return volume;
}

void TVolumeSliderAction::setTickPosition(QSlider::TickPosition position) {

	// For new widgets
	tick_position = position; 

	// Propagate changes to all existing widgets
	QList<QWidget*> l = createdWidgets();
	for (int n = 0; n < l.count(); n++) {
		TSlider* s = (TSlider*) l[n];
		s->setTickPosition(tick_position);
	}
}

void TVolumeSliderAction::valueSliderChanged(int value) {
	//qDebug("Gui::Action::TVolumeSliderAction::valueSliderChanged: %d", value);

	volume = value;
	emit valueChanged(value);
}

QWidget* TVolumeSliderAction::createWidget(QWidget* parent) {

	TSlider* slider = new TSlider(parent);

	if (custom_style) slider->setStyle(custom_style);
	if (!custom_stylesheet.isEmpty()) slider->setStyleSheet(custom_stylesheet);
	if (fixed_size.isValid()) slider->setFixedSize(fixed_size);

	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setValue(volume);
	slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	slider->setFocusPolicy(Qt::NoFocus);
	slider->setTickPosition(tick_position);
	slider->setTickInterval(10);
	slider->setSingleStep(1);
	slider->setPageStep(10);
	slider->setToolTip(tr("Volume"));
	slider->setEnabled(isEnabled());
	slider->setAttribute(Qt::WA_NoMousePropagation);

	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(valueSliderChanged(int)));

	return slider;
}


TTimeLabelAction::TTimeLabelAction(QWidget* parent)
	: TWidgetAction(parent) {
}

TTimeLabelAction::~TTimeLabelAction() {
}

void TTimeLabelAction::setText(QString s) {
	_text = s;
	emit newText(s);
}

QWidget* TTimeLabelAction::createWidget (QWidget* parent) {

	QLabel* time_label = new QLabel(parent);
	time_label->setObjectName("time_label");
	time_label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	time_label->setAutoFillBackground(true);

	ColorUtils::setBackgroundColor(time_label, QColor(0,0,0));
	ColorUtils::setForegroundColor(time_label, QColor(255,255,255));
	time_label->setText("00:00:00 / 00:00:00");
	time_label->setFrameShape(QFrame::Panel);
	time_label->setFrameShadow(QFrame::Sunken);

	connect(this, SIGNAL(newText(QString)), 
			time_label, SLOT(setText(QString)));

	return time_label;
}


TSeekingButton::TSeekingButton(const TActionList& actions, QWidget* parent)
	: QWidgetAction(parent),
	  _actions(actions) {
}

TSeekingButton::~TSeekingButton() {
}

QWidget* TSeekingButton::createWidget(QWidget* parent) {

	QToolButton* button = new QToolButton(parent);
	button->setPopupMode(QToolButton::MenuButtonPopup);

	if (_actions.count() > 0) {
		button->setDefaultAction(_actions[0]);
	}
	for (int n = 1; n < _actions.count(); n++) {
		button->addAction(_actions[n]);
	}

	return button;
}

} // namespace Action
} // namespace Gui

#include "moc_widgetactions.cpp"

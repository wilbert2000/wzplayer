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

#include "gui/timeslider.h"
#include <QDebug>
#include <QWheelEvent>
#include <QTimer>
#include <QToolTip>
#include <QStyleOptionSlider>
#include "helper.h"


namespace Gui {

TTimeSlider::TTimeSlider(QWidget* parent, int max_pos, int drag_delay)
	: TSlider(parent)
	, dont_update(false)
	, position(0)
	, total_time(0)
	, last_pos_to_send(-1)
{
	setMinimum(0);
	setMaximum(max_pos);
	setFocusPolicy(Qt::NoFocus);
	setSizePol();

	connect(this, SIGNAL(sliderPressed()), this, SLOT(stopUpdate()));
	connect(this, SIGNAL(sliderReleased()), this, SLOT(resumeUpdate()));
	connect(this, SIGNAL(sliderReleased()), this, SLOT(mouseReleased()));
	connect(this, SIGNAL(valueChanged(int)), this, SLOT(valueChanged_slot(int)));
	connect(this, SIGNAL(draggingPos(int)), this, SLOT(checkDragging(int)));
	
	timer = new QTimer(this);
	timer->setInterval(drag_delay);
	connect(timer, SIGNAL(timeout()), this, SLOT(sendDelayedPos()));
}

TTimeSlider::~TTimeSlider() {
}

QSize TTimeSlider::sizeHint() const {

	QSize s = TSlider::sizeHint();
	if (orientation() == Qt::Horizontal)
		s.rwidth() = 84;
	else
		s.rheight() = 84;
	return s;
}

void TTimeSlider::setSizePol() {

	if (orientation() == Qt::Horizontal)
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	else setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
}

void TTimeSlider::onOrientationChanged(Qt::Orientation orientation) {
	//qDebug() << "TTimeSlider::onOrientationChanged"
	//		 << (orientation == Qt::Horizontal);

	if (orientation != this->orientation()) {
		setOrientation(orientation);
		setSizePol();
		// TODO: does not work. See Teditable toolbar::resizeEvent for details
		// updateGeometry();
	}
}

void TTimeSlider::setPos(int v) {

	if (v != pos() && !dont_update) {
		position = v;
		setValue(v);
	}
}

int TTimeSlider::pos() {
	return position;
}

void TTimeSlider::setDuration(double t) {
	total_time = t;
}

double TTimeSlider::duration() {
	return total_time;
}

void TTimeSlider::stopUpdate() {

	dont_update = true;
	last_pos_to_send = -1;
	timer->start();
}

void TTimeSlider::resumeUpdate() {

	dont_update = false;
	timer->stop();
	last_pos_to_send = -1;
}

void TTimeSlider::mouseReleased() {
	emit posChanged(value());
}

void TTimeSlider::valueChanged_slot(int v) {

	bool dragging = dont_update;
	if (!dragging) {
		if (v != position) {
			emit posChanged(v);
		}
	} else {
		emit draggingPos(v);
	}
}

void TTimeSlider::checkDragging(int v) {
	last_pos_to_send = v;
}

void TTimeSlider::sendDelayedPos() {

	if (last_pos_to_send != -1) {
		emit delayedDraggingPos(last_pos_to_send);
		last_pos_to_send = -1;
	}
}

void TTimeSlider::wheelEvent(QWheelEvent* e) {
	qDebug("Gui::TTimeSlider::wheelEvent: delta: %d", e->delta());

	e->accept();
	if (e->orientation() == Qt::Vertical) {
		if (e->delta() >= 0)
			emit wheelUp();
		else
			emit wheelDown();
	} else {
		qDebug("Gui::TTimeslider::wheelEvent: horizontal event received, doing nothing");
	}
}

bool TTimeSlider::event(QEvent* event) {

	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* help_event = static_cast<QHelpEvent*>(event);
		QStyleOptionSlider opt;
		initStyleOption(&opt);
		const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
		const QPoint center = sliderRect.center() - sliderRect.topLeft();
		int val = pixelPosToRangeValue(pick(help_event->pos() - center));
		int time = val * total_time / maximum();
		QToolTip::showText(help_event->globalPos(), Helper::formatTime(time), this);
		event->accept();
		return true;
	}

	return QWidget::event(event);
}

} // namespace Gui

#include "moc_timeslider.cpp"

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
#include "helper.h"

#define DEBUG 0

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
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Fixed);

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

void TTimeSlider::stopUpdate() {
	#if DEBUG
	qDebug("Gui::TTimeSlider::stopUpdate");
	#endif
	dont_update = true;
	last_pos_to_send = -1;
	timer->start();
}

void TTimeSlider::resumeUpdate() {
	#if DEBUG
	qDebug("Gui::TTimeSlider::resumeUpdate");
	#endif
	dont_update = false;
	timer->stop();
	last_pos_to_send = -1;
}

void TTimeSlider::mouseReleased() {
	#if DEBUG
	qDebug("Gui::TTimeSlider::mouseReleased");
	#endif
	emit posChanged(value());
}

void TTimeSlider::valueChanged_slot(int v) {
	#if DEBUG
	qDebug("Gui::TTimeSlider::changedValue_slot: %d", v);
	#endif

	bool dragging = dont_update;
	if (!dragging) {
		if (v != position) {
			#if DEBUG
			qDebug(" emitting posChanged");
			#endif
			emit posChanged(v);
		}
	} else {
		#if DEBUG
		qDebug(" emitting draggingPos");
		#endif
		emit draggingPos(v);
	}
}

void TTimeSlider::setDragDelay(int d) {
	qDebug("Gui::TTimeSlider::setDragDelay: %d", d);
	timer->setInterval(d);
}

int TTimeSlider::dragDelay() {
	return timer->interval();
}

void TTimeSlider::checkDragging(int v) {
	qDebug("Gui::TTimeSlider::checkDragging: %d", v);
	last_pos_to_send = v;
}

void TTimeSlider::sendDelayedPos() {

	if (last_pos_to_send != -1) {
		qDebug("Gui::TTimeSlider::sendDelayedPos: %d", last_pos_to_send);
		emit delayedDraggingPos(last_pos_to_send);
		last_pos_to_send = -1;
	}
}

void TTimeSlider::setPos(int v) {
	#if DEBUG
	qDebug("Gui::TTimeSlider::setPos: %d", v);
	qDebug(" dont_update: %d", dont_update);
	#endif

	if (v != pos() && !dont_update) {
		position = v;
		setValue(v);
	}
}

int TTimeSlider::pos() {
	return position;
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

	/* TODO: connect duration slot
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* help_event = static_cast<QHelpEvent*>(event);
		int pos_in_slider = help_event->x() * maximum() / width();
		int time = pos_in_slider * total_time / maximum();
		if (time >= 0 && time <= total_time) {
			QToolTip::showText(help_event->globalPos(), Helper::formatTime(time), this);
		} else {
			QToolTip::hideText();
			event->ignore();
		}
		return true;
	}
	*/

	return QWidget::event(event);
}


} // namespace Gui

#include "moc_timeslider.cpp"

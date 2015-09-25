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

#include <QWheelEvent>
#include <QTimer>

#define DEBUG 0

namespace Gui {

TTimeSlider::TTimeSlider( QWidget * parent ) : TSlider(parent)
{
	dont_update = false;
	setMinimum(0);
	setMaximum(SEEKBAR_RESOLUTION);

	setFocusPolicy( Qt::NoFocus );
	setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Fixed );

	connect( this, SIGNAL( sliderPressed() ), this, SLOT( stopUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( resumeUpdate() ) );
	connect( this, SIGNAL( sliderReleased() ), this, SLOT( mouseReleased() ) );
	connect( this, SIGNAL( valueChanged(int) ), this, SLOT( valueChanged_slot(int) ) );
#if ENABLE_DELAYED_DRAGGING
	connect( this, SIGNAL(draggingPos(int) ), this, SLOT(checkDragging(int)) );
	
	last_pos_to_send = -1;
	timer = new QTimer(this);
	connect( timer, SIGNAL(timeout()), this, SLOT(sendDelayedPos()) );
	timer->start(200);
#endif
}

TTimeSlider::~TTimeSlider() {
}

void TTimeSlider::stopUpdate() {
	#if DEBUG
	qDebug("TTimeSlider::stopUpdate");
	#endif
	dont_update = true;
}

void TTimeSlider::resumeUpdate() {
	#if DEBUG
	qDebug("TTimeSlider::resumeUpdate");
	#endif
	dont_update = false;
}

void TTimeSlider::mouseReleased() {
	#if DEBUG
	qDebug("TTimeSlider::mouseReleased");
	#endif
	emit posChanged( value() );
}

void TTimeSlider::valueChanged_slot(int v) {
	#if DEBUG
	qDebug("TTimeSlider::changedValue_slot: %d", v);
	#endif

	// Only to make things clear:
	bool dragging = dont_update;
	if (!dragging) {
		if (v!=position) {
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

#if ENABLE_DELAYED_DRAGGING
void TTimeSlider::setDragDelay(int d) {
	qDebug("TTimeSlider::setDragDelay: %d", d);
	timer->setInterval(d);
}

int TTimeSlider::dragDelay() {
	return timer->interval();
}

void TTimeSlider::checkDragging(int v) {
	qDebug("TTimeSlider::checkDragging: %d", v);
	last_pos_to_send = v;
}

void TTimeSlider::sendDelayedPos() {
	if (last_pos_to_send != -1) {
		qDebug("TTimeSlider::sendDelayedPos: %d", last_pos_to_send);
		emit delayedDraggingPos(last_pos_to_send);
		last_pos_to_send = -1;
	}
}
#endif

void TTimeSlider::setPos(int v) {
	#if DEBUG
	qDebug("TTimeSlider::setPos: %d", v);
	qDebug(" dont_update: %d", dont_update);
	#endif

	if (v!=pos()) {
		if (!dont_update) {
			position = v;
			setValue(v);
		}
	}
}

int TTimeSlider::pos() {
	return position;
}

void TTimeSlider::wheelEvent(QWheelEvent * e) {
	//e->ignore();

	qDebug("TTimeSlider::wheelEvent: delta: %d", e->delta());
	e->accept();

	if (e->orientation() == Qt::Vertical) {
	    if (e->delta() >= 0)
	        emit wheelUp();
	    else
	        emit wheelDown();
	} else {
		qDebug("Timeslider::wheelEvent: horizontal event received, doing nothing");
	}
}

} // namespace Gui

#include "moc_timeslider.cpp"

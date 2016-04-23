/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/action/timeslider.h"
#include <QDebug>
#include <QWheelEvent>
#include <QTimer>
#include <QToolTip>
#include <QStyleOptionSlider>
#include "helper.h"


namespace Gui {
namespace Action {


TTimeSlider::TTimeSlider(QWidget* parent,
                         int pos,
                         int max_pos,
                         double duration,
                         int drag_delay)
	: TSlider(parent)
	, dont_update(false)
    , position(pos)
    , _duration(duration)
	, last_pos_to_send(-1)
	, savedSize(256)
	, getInitialSize(true) {

	setMinimum(0);
    setMaximum(max_pos);
    setValue(position);
    setFocusPolicy(Qt::NoFocus);

	connect(this, SIGNAL(sliderPressed()), this, SLOT(stopUpdate()));
	connect(this, SIGNAL(sliderReleased()), this, SLOT(resumeUpdate()));
	connect(this, SIGNAL(sliderReleased()), this, SLOT(mouseReleased()));
	connect(this, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
    connect(this, SIGNAL(draggingPosChanged(int)), this, SLOT(checkDragging(int)));
	
	timer = new QTimer(this);
	timer->setInterval(drag_delay);
	connect(timer, SIGNAL(timeout()), this, SLOT(sendDelayedPos()));
}

TTimeSlider::~TTimeSlider() {
}

QSize TTimeSlider::sizeHint() const {

	QSize s = TSlider::sizeHint();
	if (orientation() == Qt::Horizontal)
		s.rwidth() = savedSize;
	else
		s.rheight() = savedSize;
	//qDebug() << "Gui::Action::TTimeSlider::sizeHint: size" << size() << "hint" << s;
	return s;
}

QSize TTimeSlider::minimumSizeHint() const {

	QSize s = TSlider::sizeHint();
	if (orientation() == Qt::Horizontal)
		s.rwidth() = SLIDER_MIN_SIZE;
	else
		s.rheight() = SLIDER_MIN_SIZE;
	//qDebug() << "Gui::Action::TTimeSlider::minimumSizeHint: size" << size() << "hint" << s;
	return s;
}

// Called by TSizeGrip signal saveSizeHint when user resized toolbar
void TTimeSlider::saveSizeHint() {

	savedSize = orientation() == Qt::Horizontal ? width() : height();
	if (savedSize < SLIDER_MIN_SIZE)
		savedSize = SLIDER_MIN_SIZE;
	//qDebug() << "Gui::Action::TTimeSlider::saveSizeHint: size" << size() << "saved size" << savedSize;
}

void TTimeSlider::resizeEvent(QResizeEvent* event) {
	//qDebug() << "Gui::Action::TTimeSlider::resizeEvent:"
	//		 << "from" << event->oldSize() << "to" << size();

	TSlider::resizeEvent(event);
	// Save initial size as size hint
	if (getInitialSize) {
		getInitialSize = false;
		saveSizeHint();
	}
}

void TTimeSlider::setPos(int v) {

    if (v != position && !dont_update) {
		position = v;
		setValue(v);
	}
}

int TTimeSlider::pos() {
	return position;
}

void TTimeSlider::setDuration(double t) {
	_duration = t;
}

double TTimeSlider::duration() {
	return _duration;
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

void TTimeSlider::onValueChanged(int v) {

	bool dragging = dont_update;
	if (!dragging) {
		if (v != position) {
			emit posChanged(v);
		}
	} else {
        emit draggingPosChanged(v);
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
	//qDebug("Gui::Action::TTimeSlider::wheelEvent: delta: %d", e->delta());

	e->accept();
	if (e->orientation() == Qt::Vertical) {
		if (e->delta() >= 0)
			emit wheelUp();
		else
			emit wheelDown();
	} else {
		qDebug("Gui::Action::TTimeslider::wheelEvent: horizontal event received, doing nothing");
	}
}

bool TTimeSlider::event(QEvent* event) {

	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* help_event = static_cast<QHelpEvent*>(event);
		QStyleOptionSlider opt;
		initStyleOption(&opt);
        // Rect of handle/knob
        const QRect sliderRect = style()->subControlRect(
            QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        // Center of handle
		const QPoint center = sliderRect.center() - sliderRect.topLeft();

		int val = pixelPosToRangeValue(pick(help_event->pos() - center));
		int time = val * _duration / maximum();
		QToolTip::showText(help_event->globalPos(), Helper::formatTime(time), this);
		event->accept();
		return true;
	}

	return QWidget::event(event);
}

} // namespace Action
} // namespace Gui

#include "moc_timeslider.cpp"

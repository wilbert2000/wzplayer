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
#include <QWheelEvent>
#include <QTimer>
#include <QStyleOptionSlider>


namespace Gui {
namespace Action {


TTimeSlider::TTimeSlider(QWidget* parent,
                         int pos,
                         int max_pos,
                         double duration,
                         int drag_delay) :
    TSlider(parent),
    dont_update(false),
    position(pos),
    _duration(duration),
    last_pos_to_send(-1) {

    setMinimum(0);
    setMaximum(max_pos);
    setValue(position);

    connect(this, &TTimeSlider::sliderPressed,
            this, &TTimeSlider::stopUpdate);
    connect(this, &TTimeSlider::sliderReleased,
            this, &TTimeSlider::resumeUpdate);
    connect(this, &TTimeSlider::sliderReleased,
            this, &TTimeSlider::mouseReleased);
    connect(this, &TTimeSlider::valueChanged,
            this, &TTimeSlider::onValueChanged);
    connect(this, &TTimeSlider::draggingPosChanged,
            this, &TTimeSlider::checkDragging);

    timer = new QTimer(this);
    timer->setInterval(drag_delay);
    connect(timer, &QTimer::timeout, this, &TTimeSlider::sendDelayedPos);
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

    e->accept();
    if (e->orientation() == Qt::Vertical) {
        if (e->delta() >= 0)
            emit wheelUp();
        else
            emit wheelDown();
    } else {
        WZDEBUG("ignoring horizontal wheel event");
    }
}

int TTimeSlider::getTime(const QPoint& pos) {

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    // Rect of handle/knob
    const QRect sliderRect = style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    // Center of handle
    const QPoint center = sliderRect.center() - sliderRect.topLeft();

    int val = pixelPosToRangeValue(pick(pos - center));
    return qRound(val * _duration / maximum());
}

bool TTimeSlider::onToolTipEvent(QHelpEvent* event) {

    emit toolTipEvent(this, event->globalPos(), getTime(event->pos()));
    event->accept();
    return true;
}

bool TTimeSlider::event(QEvent* event) {

    if (event->type() == QEvent::ToolTip) {
        return onToolTipEvent(static_cast<QHelpEvent*>(event));
    }
    return QWidget::event(event);
}

} // namespace Action
} // namespace Gui

#include "moc_timeslider.cpp"

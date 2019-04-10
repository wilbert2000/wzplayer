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
#include "player/player.h"

#include <QWheelEvent>
#include <QStyleOptionSlider>


namespace Gui {
namespace Action {


TTimeSlider::TTimeSlider(QWidget* parent, int posMS, int durationMS) :
    TSlider(parent),
    dragging(false) {

    setMinimum(0);
    setMaximum(durationMS);
    setValue(posMS);

    connect(this, &TTimeSlider::sliderPressed,
            this, &TTimeSlider::startDragging);
    connect(this, &TTimeSlider::sliderReleased,
            this, &TTimeSlider::stopDragging);
    connect(this, &TTimeSlider::valueChanged,
            this, &TTimeSlider::onValueChanged);
}

void TTimeSlider::setPosMS(int ms) {

    if (!dragging) {
        setValue(ms);
    }
}

void TTimeSlider::setDurationMS(int ms) {
    setMaximum(ms);
}

void TTimeSlider::startDragging() {

    dragging = true;

    pausedPlayer = player->state() == Player::STATE_PLAYING;
    if (pausedPlayer) {
        player->pause();
    }
}

void TTimeSlider::stopDragging() {

    dragging = false;
    emit posChanged(value());

    if (pausedPlayer && player->state() == Player::STATE_PAUSED) {
        player->play();
    }
}

void TTimeSlider::onValueChanged(int v) {

    if (dragging) {
        emit draggingPosChanged(v);
    } else {
        emit posChanged(v);
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
        WZDEBUG("Ignoring horizontal wheel event");
    }
}

int TTimeSlider::getTimeMS(const QPoint& pos) {

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    // Rect of handle/knob
    const QRect sliderRect = style()->subControlRect(
                QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    // Center of handle
    const QPoint center = sliderRect.center() - sliderRect.topLeft();

    return pixelPosToRangeValue(pick(pos - center));
}

bool TTimeSlider::onToolTipEvent(QHelpEvent* event) {

    emit toolTipEvent(this, event->globalPos(), getTimeMS(event->pos()));
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

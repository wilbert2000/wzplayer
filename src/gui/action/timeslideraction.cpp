#include "gui/action/timeslideraction.h"
#include "gui/action/timeslider.h"
#include "settings/preferences.h"


namespace Gui {
namespace Action {


TTimeSliderAction::TTimeSliderAction(QWidget* parent) :
    TWidgetAction(parent),
    pos(0),
    maxPos(1000),
    duration(0) {
}

QWidget* TTimeSliderAction::createWidget(QWidget* parent) {

    TTimeSlider* slider = new TTimeSlider(parent, pos, maxPos, duration,
        Settings::pref->time_slider_drag_delay);
    slider->setEnabled(isEnabled());

    connect(slider, &TTimeSlider::posChanged,
            this, &TTimeSliderAction::onPosChanged);
    connect(slider, &TTimeSlider::draggingPosChanged,
            this, &TTimeSliderAction::onDraggingPosChanged);
    connect(slider, &TTimeSlider::delayedDraggingPos,
            this, &TTimeSliderAction::onDelayedDraggingPos);

    connect(slider, &TTimeSlider::wheelUp,
            this, &TTimeSliderAction::wheelUp);
    connect(slider, &TTimeSlider::wheelDown,
            this, &TTimeSliderAction::wheelDown);

    return slider;
}

void TTimeSliderAction::setPos() {

    QList<QWidget*> widgets = createdWidgets();
    for(int i = 0; i < widgets.count(); i++) {
        TTimeSlider* widget = static_cast<TTimeSlider*>(widgets.at(i));
        bool was_blocked = widget->blockSignals(true);
        widget->setPos(pos);
        widget->blockSignals(was_blocked);
    }
}

void TTimeSliderAction::setPosition(double sec) {

    int p = 0;
    if (sec > 0 && duration > 0.1) {
        p = qRound((sec * maxPos) / duration);
        if (p > maxPos) {
            p = maxPos;
        }
    }

    if (p != pos) {
        pos = p;
        setPos();
    }
}

void TTimeSliderAction::setDuration(double t) {

    duration = t;
    QList<QWidget*> l = createdWidgets();
    for (int n = 0; n < l.count(); n++) {
        TTimeSlider* s = (TTimeSlider*) l[n];
        s->setDuration(t);
    }
}

// Slider pos changed
void TTimeSliderAction::onPosChanged(int value) {

    pos = value;
    if (Settings::pref->relative_seeking || duration <= 0) {
        emit percentageChanged((double) (pos * 100) / maxPos);
    } else {
        emit positionChanged(duration * pos / maxPos);
    }
}

// Slider pos changed while dragging
void TTimeSliderAction::onDraggingPosChanged(int value) {

    pos = value;
    emit dragPositionChanged(duration * pos / maxPos);
}

// Delayed slider pos while dragging
void TTimeSliderAction::onDelayedDraggingPos(int value) {

    pos = value;
    if (Settings::pref->update_while_seeking) {
        onPosChanged(pos);
    }
}

} // namespace Action
} // namespace Gui

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

#include "gui/action/widgetactions.h"
#include <QDebug>
#include <QLabel>
#include <QToolButton>
#include <QToolBar>
#include "colorutils.h"
#include "gui/action/timeslider.h"
#include "settings/preferences.h"

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


TTimeSliderAction::TTimeSliderAction(QWidget* parent) :
    TWidgetAction(parent),
    pos(0),
    max_pos(1000),
    duration(0) {
}

TTimeSliderAction::~TTimeSliderAction() {
}

void TTimeSliderAction::setPos() {

    QList<QWidget*> l = createdWidgets();
    for (int n = 0; n < l.count(); n++) {
        TTimeSlider* s = (TTimeSlider*) l[n];
        bool was_blocked = s->blockSignals(true);
        s->setPos(pos);
        s->blockSignals(was_blocked);
    }
}

void TTimeSliderAction::setPosition(double sec) {

    int p = 0;
    if (sec > 0 && duration > 0.1) {
        p = qRound((sec * max_pos) / duration);
        if (p > max_pos) {
            p = max_pos;
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
        emit percentageChanged((double) (pos * 100) / max_pos);
    } else {
        emit positionChanged(duration * pos / max_pos);
    }
}

// Slider pos changed while dragging
void TTimeSliderAction::onDraggingPosChanged(int value) {

    pos = value;
    emit dragPositionChanged(duration * pos / max_pos);
}

// Delayed slider pos while dragging
void TTimeSliderAction::onDelayedDraggingPos(int value) {

    pos = value;
    if (Settings::pref->update_while_seeking) {
        onPosChanged(pos);
    }
}

QWidget* TTimeSliderAction::createWidget(QWidget* parent) {

    TTimeSlider* slider = new TTimeSlider(parent, pos, max_pos, duration,
        Settings::pref->time_slider_drag_delay);
    slider->setEnabled(isEnabled());

    if (custom_style)
        slider->setStyle(custom_style);
    if (!custom_stylesheet.isEmpty())
        slider->setStyleSheet(custom_stylesheet);

    connect(slider, SIGNAL(posChanged(int)),
            this, SLOT(onPosChanged(int)));
    connect(slider, SIGNAL(draggingPosChanged(int)),
            this, SLOT(onDraggingPosChanged(int)));
    connect(slider, SIGNAL(delayedDraggingPos(int)),
            this, SLOT(onDelayedDraggingPos(int)));

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

} // namespace Action
} // namespace Gui

#include "moc_widgetactions.cpp"

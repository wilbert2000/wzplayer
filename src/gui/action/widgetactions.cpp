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
#include "gui/action/slider.h"
#include "settings/preferences.h"
#include "colorutils.h"

#include <QLabel>
#include <QToolButton>
#include <QToolBar>


namespace Gui {
namespace Action {

TWidgetAction::TWidgetAction(QWidget* parent)
    : QWidgetAction(parent) {
}

void TWidgetAction::enable(bool e) {
    propagate_enabled(e);
}

void TWidgetAction::disable() {
    propagate_enabled(false);
}

void TWidgetAction::propagate_enabled(bool b) {

    QList<QWidget*> l = createdWidgets();
    for (int i = 0; i < l.count(); i++) {
        l.at(i)->setEnabled(b);;
    }
    setEnabled(b);
}




TVolumeSliderAction::TVolumeSliderAction(QWidget* parent, int vol)
    : TWidgetAction(parent)
    , volume(vol)
    , tick_position(QSlider::TicksBelow) {
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

    slider->setMinimum(0);
    slider->setMaximum(100);
    slider->setValue(volume);
    slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    slider->setTickPosition(tick_position);
    slider->setTickInterval(10);
    slider->setSingleStep(1);
    slider->setPageStep(10);
    slider->setToolTip(tr("Volume"));
    slider->setEnabled(isEnabled());
    slider->setAttribute(Qt::WA_NoMousePropagation);

    connect(slider, &TSlider::valueChanged,
            this, &TVolumeSliderAction::valueSliderChanged);

    return slider;
}

} // namespace Action
} // namespace Gui

#include "moc_widgetactions.cpp"

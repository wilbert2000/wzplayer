/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Trolltech ASA
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/action/slider.h"
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOption>
#include <QToolBar>
#include <QLayout>


namespace Gui {
namespace Action {


TSlider::TSlider(QWidget* parent) : QSlider(parent) {

    setFocusPolicy(Qt::TabFocus);
    // Copy orientation toolbar
    QToolBar* toolbar = qobject_cast<QToolBar*>(parent);
    if (toolbar) {
        setOrientation(toolbar->orientation());
        connect(toolbar, &QToolBar::orientationChanged,
                this, &TSlider::onToolbarOrientationChanged);
    } else {
        setOrientation(Qt::Horizontal);
    }
}

void TSlider::setAlign() {

    if (orientation() == Qt::Horizontal) {
        parentWidget()->layout()->setAlignment(this, Qt::AlignVCenter);
    } else {
        parentWidget()->layout()->setAlignment(this, Qt::AlignHCenter);
    }
}

void TSlider::onToolbarOrientationChanged(Qt::Orientation orientation) {

    setOrientation(orientation);
    setAlign();
}

// Copied from qslider.cpp and modified to make it compile
int TSlider::pixelPosToRangeValue(int pos) const
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QRect grooveRect = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderGroove, this);
    QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt,
                                               QStyle::SC_SliderHandle, this);

    int sliderMin, sliderMax;
    if (orientation() == Qt::Horizontal) {
        sliderMin = grooveRect.x();
        sliderMax = grooveRect.x() + grooveRect.width() - sliderRect.width();
    } else {
        sliderMin = grooveRect.y();
        sliderMax = grooveRect.y() + grooveRect.height() - sliderRect.height();
    }
    return QStyle::sliderValueFromPosition(
                minimum(), maximum(),
                pos - sliderMin, sliderMax - sliderMin,
                opt.upsideDown);
}

// Based on code from qslider.cpp
void TSlider::mousePressEvent(QMouseEvent* event) {

    // Change QSlider PgUp/PgDown behaviour to directly jumping to clicked pos
    if (event->button() == Qt::LeftButton) {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const QRect sliderRect = style()->subControlRect(
                    QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        if (sliderRect.contains(event->pos())) {
            QSlider::mousePressEvent(event);
        } else {
            event->accept();
            const QPoint center = sliderRect.center() - sliderRect.topLeft();
            setSliderPosition(pixelPosToRangeValue(pick(event->pos() - center)));
            triggerAction(SliderMove);
            setRepeatAction(SliderNoAction);
        }
    } else {
        QSlider::mousePressEvent(event);
    }
}

} // namespace Action
} // namespace Gui

#include "moc_slider.cpp"


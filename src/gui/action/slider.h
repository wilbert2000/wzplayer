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

#ifndef GUI_SLIDER_H
#define GUI_SLIDER_H

#include <QSlider>


namespace Gui {
namespace Action {


class TSlider : public QSlider {
    Q_OBJECT
public:
    TSlider(QWidget* parent);

protected:
    void mousePressEvent(QMouseEvent* event);
    // Copied from qslider.cpp
    inline int pick(const QPoint &pt) const {
        return orientation() == Qt::Horizontal ? pt.x() : pt.y();
    }
    int pixelPosToRangeValue(int pos) const;

private:
    void setAlign();

private slots:
    void onToolbarOrientationChanged(Qt::Orientation orientation);
}; // class TSlider


} // namespace Action
} // namespace Gui

#endif // GUI_SLIDER_H


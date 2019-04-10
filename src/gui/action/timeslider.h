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

#ifndef GUI_TIMESLIDER_H
#define GUI_TIMESLIDER_H

#include "gui/action/slider.h"
#include "wzdebug.h"


class QHelpEvent;

namespace Gui {
namespace Action {

class TTimeSlider : public TSlider {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TTimeSlider(QWidget* parent, int posMS, int durationMS);

    void setPosMS(int ms); // Don't use setValue!
    void setDurationMS(int ms);

    int getTimeMS(const QPoint& pos);

signals:
    void posChanged(int ms);
    void draggingPosChanged(int ms);

    void wheelUp();
    void wheelDown();

    void toolTipEvent(TTimeSlider* slider, QPoint pos, int ms);

protected slots:
    void startDragging();
    void stopDragging();
    void onValueChanged(int);

protected:
    virtual void wheelEvent(QWheelEvent* e);
    virtual bool event(QEvent* event);

private:
    bool dragging;
    bool pausedPlayer;

    bool onToolTipEvent(QHelpEvent* event);
}; // class TTimeSlider

} // namespace Action
} // namespace Gui

#endif // GUI_TIMESLIDER_H


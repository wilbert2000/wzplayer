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

#ifndef _GUI_TIMESLIDER_H_
#define _GUI_TIMESLIDER_H_

#include "config.h"
#include "gui/slider.h"

namespace Gui {

class TTimeSlider : public TSlider
{
	Q_OBJECT

public:
	TTimeSlider(QWidget* parent, int max_pos, int drag_delay);
	virtual ~TTimeSlider();

public slots:
	virtual void setPos(int); // Don't use setValue!
	virtual int pos();
	void setDragDelay(int);
	int dragDelay();

signals:
	void posChanged(int);
	void draggingPos(int);
	//! Emitted with a few ms of delay
	void delayedDraggingPos(int);

	void wheelUp();
	void wheelDown();

protected slots:
	void stopUpdate();
	void resumeUpdate();
	void mouseReleased();
	void valueChanged_slot(int);
	void checkDragging(int);
	void sendDelayedPos();

	virtual void wheelEvent(QWheelEvent* e);

private:
	bool dont_update;
	int position;
	
	int last_pos_to_send;
	QTimer* timer;
};

} // namespace Gui

#endif // _GUI_TIMESLIDER_H_


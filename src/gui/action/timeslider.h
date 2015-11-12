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

#ifndef GUI_TIMESLIDER_H
#define GUI_TIMESLIDER_H

#include "config.h"
#include "gui/action/slider.h"

namespace Gui {

class TTimeSlider : public TSlider
{
	Q_OBJECT

public:
	TTimeSlider(QWidget* parent, int max_pos, int drag_delay);
	virtual ~TTimeSlider();

	virtual int pos();
	virtual double duration();

public slots:
	virtual void setPos(int); // Don't use setValue!
	virtual void setDuration(double t);
	void onOrientationChanged(Qt::Orientation orientation);

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

protected:
	virtual void wheelEvent(QWheelEvent* e);
	virtual bool event(QEvent* event);
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

private:
	bool dont_update;
	int position;
	double total_time;
	
	int last_pos_to_send;
	QTimer* timer;
};

} // namespace Gui

#endif // GUI_TIMESLIDER_H

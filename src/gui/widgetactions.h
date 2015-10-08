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

#ifndef _GUI_WIDGETACTIONS_H_
#define _GUI_WIDGETACTIONS_H_

#include <QWidgetAction>
#include "config.h"
#include "gui/guiconfig.h"
#include "timeslider.h"
#include "settings/preferences.h"

class QStyle;

namespace Gui {

class TWidgetAction : public QWidgetAction
{
	Q_OBJECT

public:
	TWidgetAction(QWidget* parent);
	~TWidgetAction();

	void setCustomStyle(QStyle* style) { custom_style = style; }
	QStyle* customStyle() { return custom_style; }

	void setStyleSheet(QString style) { custom_stylesheet = style; }
	QString styleSheet() { return custom_stylesheet; }

public slots:
	virtual void enable(bool e = true); // setEnabled in QAction is not virtual :(
	virtual void disable();

protected:
	virtual void propagate_enabled(bool);

protected:
	QStyle* custom_style;
	QString custom_stylesheet;
};


class TTimeSliderAction : public TWidgetAction
{
	Q_OBJECT

public:
	TTimeSliderAction(QWidget* parent, int max_position, int delay);
	virtual ~TTimeSliderAction();

public slots:
	virtual void setPos(int);
	virtual int pos();
	virtual void setDuration(double);
	virtual double duration() { return total_time; }
	void setDragDelay(int);
	int dragDelay();

signals:
	void posChanged(int value);
	void draggingPos(int value);
	void delayedDraggingPos(int);

	void wheelUp(Settings::TPreferences::WheelFunction function = Settings::TPreferences::Seeking);
	void wheelDown(Settings::TPreferences::WheelFunction function = Settings::TPreferences::Seeking);

protected:
	virtual QWidget* createWidget (QWidget* parent);

private:
	int max_pos;
	int drag_delay;
	double total_time;
};


class TVolumeSliderAction : public TWidgetAction
{
	Q_OBJECT

public:
	TVolumeSliderAction(QWidget* parent);
	~TVolumeSliderAction();

	void setFixedSize(QSize size) { fixed_size = size; }
	QSize fixedSize() { return fixed_size; }

	void setTickPosition(QSlider::TickPosition position);
	QSlider::TickPosition tickPosition() { return tick_position; }

public slots:
	virtual void setValue(int);
	virtual int value();

signals:
	void valueChanged(int value);

protected:
	virtual QWidget* createWidget (QWidget* parent);

private:
	QSize fixed_size;
	QSlider::TickPosition tick_position;
};


class TTimeLabelAction : public TWidgetAction
{
	Q_OBJECT

public:
	TTimeLabelAction(QWidget* parent);
	~TTimeLabelAction();

	virtual QString text() { return _text; }

public slots:
	virtual void setText(QString s);

signals:
	void newText(QString s);

protected:
	virtual QWidget* createWidget (QWidget* parent);

private:
	QString _text;
};


class TSeekingButton : public QWidgetAction
{
	Q_OBJECT

public:
	TSeekingButton(QList<QAction*> actions, QWidget* parent);
	~TSeekingButton();

protected:
	virtual QWidget* createWidget (QWidget* parent);

	QList<QAction*> _actions;
};

} // namespace Gui

#endif // _GUI_WIDGETACTIONS_H_


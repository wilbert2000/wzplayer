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

#ifndef GUI_WIDGETACTIONS_H
#define GUI_WIDGETACTIONS_H

#include <QWidgetAction>
#include <QSlider>
#include "settings/preferences.h"


class QStyle;

namespace Gui {
namespace Action {


class TWidgetAction : public QWidgetAction {
	Q_OBJECT

public:
	TWidgetAction(QWidget* parent);
	~TWidgetAction();

	void setCustomStyle(QStyle* style) { custom_style = style; }
	QStyle* customStyle() const { return custom_style; }

	void setStyleSheet(const QString& style) { custom_stylesheet = style; }
	QString styleSheet() const { return custom_stylesheet; }

public slots:
	virtual void enable(bool e = true); // setEnabled in QAction is not virtual :(
	virtual void disable();

protected:
	virtual void propagate_enabled(bool);

protected:
	QStyle* custom_style;
	QString custom_stylesheet;
};


class TTimeSliderAction : public TWidgetAction {
	Q_OBJECT

public:
	TTimeSliderAction(QWidget* parent, int max_position, int delay);
	virtual ~TTimeSliderAction();

public slots:
	virtual void setPos(int);
	virtual void setDuration(double);

signals:
	void posChanged(int value);
	void draggingPos(int value);
	void delayedDraggingPos(int);

	void wheelUp(Settings::TPreferences::WheelFunction function = Settings::TPreferences::Seeking);
	void wheelDown(Settings::TPreferences::WheelFunction function = Settings::TPreferences::Seeking);

protected:
	virtual QWidget* createWidget(QWidget* parent);

private:
	int max_pos;
	int drag_delay;
	double total_time;
};


class TVolumeSliderAction : public TWidgetAction {
	Q_OBJECT

public:
	TVolumeSliderAction(QWidget* parent, int vol);
	~TVolumeSliderAction();

	void setFixedSize(QSize size) { fixed_size = size; }
	QSize fixedSize() const { return fixed_size; }

	void setTickPosition(QSlider::TickPosition position);
	QSlider::TickPosition tickPosition() const { return tick_position; }

	virtual int value();

public slots:
	virtual void setValue(int);

signals:
	void valueChanged(int value);

protected:
	virtual QWidget* createWidget(QWidget* parent);

private:
	int volume;
	QSize fixed_size;
	QSlider::TickPosition tick_position;

private slots:
	void valueSliderChanged(int value);
};


class TTimeLabelAction : public TWidgetAction {
	Q_OBJECT

public:
	TTimeLabelAction(QWidget* parent);
	~TTimeLabelAction();

	virtual QString text() const { return _text; }

public slots:
	virtual void setText(QString s);

signals:
	void newText(QString s);

protected:
	virtual QWidget* createWidget(QWidget* parent);

private:
	QString _text;
};


typedef QList<QAction*> TActionList;

class TSeekingButton : public QWidgetAction {
	Q_OBJECT

public:
	TSeekingButton(const TActionList& actions, QWidget* parent);
	~TSeekingButton();

protected:
	virtual QWidget* createWidget(QWidget* parent);

	TActionList _actions;
};

} // namespace Action
} // namespace Gui

#endif // GUI_WIDGETACTIONS_H


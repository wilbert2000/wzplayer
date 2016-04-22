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

#ifndef GUI_ACTION_WIDGETACTIONS_H
#define GUI_ACTION_WIDGETACTIONS_H

#include <QWidgetAction>
#include <QSlider>
#include "settings/preferences.h"
#include "gui/action/actionlist.h"


class QStyle;

namespace Gui {
namespace Action {


class TWidgetAction : public QWidgetAction {
	Q_OBJECT

public:
	TWidgetAction(QWidget* parent);
    virtual ~TWidgetAction();

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
    TTimeSliderAction(QWidget* parent);
	virtual ~TTimeSliderAction();

public slots:
    virtual void setPosition(double sec);
    virtual void setDuration(double);

signals:
    void positionChanged(double sec);
    void percentageChanged(double percentage);
    void dragPositionChanged(double sec);

	void wheelUp(Settings::TPreferences::TWheelFunction function = Settings::TPreferences::Seeking);
	void wheelDown(Settings::TPreferences::TWheelFunction function = Settings::TPreferences::Seeking);

protected:
	virtual QWidget* createWidget(QWidget* parent);

private:
    int pos;
	int max_pos;
    double duration;

    void setPos();
private slots:
    void onPosChanged(int value);
    void onDraggingPosChanged(int value);
    void onDelayedDraggingPos(int value);
};


class TVolumeSliderAction : public TWidgetAction {
	Q_OBJECT

public:
	TVolumeSliderAction(QWidget* parent, int vol);
    virtual ~TVolumeSliderAction();

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
    virtual ~TTimeLabelAction();

	virtual QString text() const { return _text; }

public slots:
    virtual void setText(const QString& s);

signals:
    void newText(const QString& s);

protected:
	virtual QWidget* createWidget(QWidget* parent);

private:
	QString _text;
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_WIDGETACTIONS_H


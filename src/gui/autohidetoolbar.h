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

#ifndef GUI_AUTOHIDETOOLBAR_H
#define GUI_AUTOHIDETOOLBAR_H

#include "gui/editabletoolbar.h"

class QTimer;

namespace Gui {

class TAutohideToolbar : public TEditableToolbar {
	Q_OBJECT

public:
	enum Activation { Anywhere = 1, Bottom = 2 };

	TAutohideToolbar(QWidget* parent, QWidget* playerwindow);
	virtual ~TAutohideToolbar();

	int margin() const { return spacing; }
	int percWidth() const { return perc_width; }
	Activation activationArea() const { return activation_area; }
	int hideDelay() const;

	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();

public slots:
	void setMargin(int margin) { spacing = margin; }
	void setPercWidth(int s) { perc_width = s;}
	void setActivationArea(Activation m) { activation_area = m; }
	void setHideDelay(int ms);

protected:
	bool eventFilter(QObject* obj, QEvent* event);

private slots:
	void checkUnderMouse();

private:
	bool auto_hide;
	int spacing;
	int perc_width;
	Activation activation_area;
	QTimer* timer;

	void showAuto();
	bool insideShowArea(const QPoint& p) const;
	void resizeAndMove();
};

} // namespace Gui

#endif // GUI_AUTOHIDETOOLBAR_H


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

	TAutohideToolbar(QMainWindow* mainwindow, QWidget* playerwindow);
	virtual ~TAutohideToolbar();

	int percWidth() const { return perc_width; }
	Activation activationArea() const { return activation_area; }
	int hideDelay() const;

	void setPercWidth(int s) { perc_width = s;}
	void setActivationArea(Activation m) { activation_area = m; }
	void setHideDelay(int ms);
	void resizeToolbar();
	void resetPosition() { reset_pos = true; }

	void didEnterFullscreen();
	void aboutToExitFullscreen();

	virtual void setVisible(bool visible);

protected:
	bool eventFilter(QObject* obj, QEvent* event);
	virtual void moveEvent(QMoveEvent* event);

private slots:
	void checkUnderMouse();
	void startAutoHide();

private:
	bool auto_hide;
	bool reset_pos;
	bool fullscreen;

	int perc_width;
	Activation activation_area;
	QTimer* timer;

	bool insideShowArea(const QPoint& p) const;
	bool allowGeometryChanges() const;
};

} // namespace Gui

#endif // GUI_AUTOHIDETOOLBAR_H


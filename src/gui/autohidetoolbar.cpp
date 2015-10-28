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

#include "gui/autohidetoolbar.h"
#include <QDebug>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include "gui/base.h"

namespace Gui {

TAutohideToolbar::TAutohideToolbar(TBase* mainwindow, QWidget* playerwindow)
	: TEditableToolbar(mainwindow)
	, auto_hide(false)
	, reset_pos(false)
	, perc_width(100)
	, activation_area(Anywhere) {

	timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->setInterval(3000);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkUnderMouse()));

	playerwindow->installEventFilter(this);
}

TAutohideToolbar::~TAutohideToolbar() {
}

void TAutohideToolbar::setHideDelay(int ms) {
	timer->setInterval(ms);
}

int TAutohideToolbar::hideDelay() const {
	return timer->interval();
}

// Slot called by timer
void TAutohideToolbar::checkUnderMouse() {

	if (auto_hide) {
		if (isVisible()) {
			if (underMouse()) {
				timer->start();
			} else {
				hide();
			}
		}
	}
}

bool TAutohideToolbar::allowGeometryChanges() const {

	if (auto_hide && isFloating()) {
		int current_screen = QApplication::desktop()->screenNumber(this);
		int main_window_screen = QApplication::desktop()->screenNumber(main_window);
		return current_screen == main_window_screen;
	}
	return false;
}

void TAutohideToolbar::moveEvent(QMoveEvent* event) {
	//qDebug() << "TAutohideToolbar::moveEvent" << event->oldPos() << event->pos();

	TEditableToolbar::moveEvent(event);

	if (!allowGeometryChanges()) {
		return;
	}

	QWidget* panel = main_window->centralWidget();
	QPoint origin = main_window->mapToGlobal(panel->pos());
	//qDebug() << "TAutohideToolbar::moveEvent: panel" << origin << panel->size();

	int x = event->pos().x();
	if (x < origin.x()) {
		x = origin.x();
	} else if (x > panel->width() - width()) {
		x = panel->width() - width();
	}

	int y = event->pos().y();
	if (y < origin.y()) {
		//qDebug("TAutohideToolbar::moveEvent: moving to (%d, %d)", x, origin.y());
		move(x, origin.y());
		return;
	}

	int max_y = origin.y() + panel->height() - height();
	if (y > max_y) {
		//qDebug("TAutohideToolbar::moveEvent: moving to (%d, %d)", x, max_y);
		move(x, max_y);
		return;
	}

	static bool block_move = false;
	if (block_move) {
		block_move = false;
		//qDebug("TAutohideToolbar::moveEvent: blocked");
		return;
	}

	// Qt centers the toolbar when it is outside its allowed area. On my system
	// (KDE 4.14.9 / Qt 4.8) when in fullscreen mode the floating toolbar seems
	// not to be allowed into the docking area at the bottom of the screen.
	// This hack tries to detect that behaviour and puts the bar in a more
	// reasonable place.
	int center = max_y / 2;
	int old_y = event->oldPos().y();
	if (qAbs(old_y - y) >= center && qAbs(y - center) < height()) {
		// Moved almost half a screen to the center, assume centered
		qDebug("TAutohideToolbar::moveEvent: undo center control bar");
		block_move = true;
		// Need to use old x, otherwise still centered horizontally
		int x = event->oldPos().x();
		if (x < origin.x()) {
			x = origin.x();
		} else if (x > panel->width() - width()) {
			x = panel->width() - width();
		}
		if (old_y < center) {
			move(x, origin.y());
		} else {
			move(x, max_y);
		}
	}
}

void TAutohideToolbar::resizeToolbar() {
	//qDebug("TAutohideToolbar::resizeToolbar");

	if (!allowGeometryChanges()) {
		return;
	}

	QWidget* panel = main_window->centralWidget();
	QPoint origin = main_window->mapToGlobal(panel->pos());
	//qDebug() << "TAutohideToolbar::resizeToolbar: panel" << origin << panel->size();

	int w = panel->width() * perc_width / 100;
	int h = height();
	int max_y = origin.y() + panel->height() - h;
	int x, y;
	if (reset_pos) {
		reset_pos = false;
		x = origin.x() + (panel->width() - w) / 2;
		y = max_y;
	} else {
		x = pos().x();
		if (x < origin.x()) {
			x = origin.x();
		} else {
			int max_x = origin.x() + panel->width() - w;
			if (x > max_x) {
				x = max_x;
			}
		}
		y = pos().y();
		if (y < origin.y()) {
			y = origin.y();
		} else if (y > max_y) {
			y = max_y;
		}
	}
	//qDebug() << "TAutohideToolbar::resizeToolbar: resizing to" << x << y << w << h;
	setGeometry(x, y, w, h);
}

void TAutohideToolbar::startAutoHide() {
	//qDebug("TAutohideToolbar::startAutoHide");

	// Start when still in fullscreen
	if (fullscreen) {
		auto_hide = true;
		resizeToolbar();
		show();
	}
}

void TAutohideToolbar::didEnterFullscreen() {
	//qDebug("TAutohideToolbar::didEnterFullscreen");

	TEditableToolbar::didEnterFullscreen();

	setAllowedAreas(Qt::NoToolBarArea);
	// The panel will not yet have its fullscreen size by now, so need to post
	// enable auto_hide to prevent using the wrong screen size in
	// resizeToolbar() and moveEvent().
	fullscreen = true;
	QTimer::singleShot(300, this, SLOT(startAutoHide()));
}

void TAutohideToolbar::aboutToExitFullscreen() {
	//qDebug("TAutohideToolbar::aboutToExitFullscreen");

	timer->stop();
	auto_hide = false;
	fullscreen = false;
	setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
}

void TAutohideToolbar::setVisible(bool visible) {
	//qDebug("TAutohideToolbar::setVisible: %d", visible);

	TEditableToolbar::setVisible(visible);
	if (auto_hide) {
		if (visible) {
			timer->start();
		} else {
			timer->stop();
		}
	}
}

bool TAutohideToolbar::insideShowArea(const QPoint& p) const {

	QPoint origin = mapToGlobal(QPoint(0, 0));
	int margin = 4 * height();
	return p.y() > origin.y() - margin
			&& p.y() < origin.y() + height() + margin;
}

bool TAutohideToolbar::eventFilter(QObject* obj, QEvent* event) {

	if (auto_hide) {
		if (event->type() == QEvent::MouseMove) {
			if (!isVisible()) {
				if (activation_area == Anywhere) {
					show();
				} else {
					QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
					if (insideShowArea(mouse_event->globalPos())) {
						show();
					}
				}
			}
		} else if (event->type() == QEvent::Resize) {
			resizeToolbar();
		}
	}

	return TEditableToolbar::eventFilter(obj, event);
}

} // namespace Gui

#include "moc_autohidetoolbar.cpp"


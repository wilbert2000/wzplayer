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

namespace Gui {

TAutohideToolbar::TAutohideToolbar(QWidget* parent, QWidget* playerwindow)
	: TEditableToolbar(parent)
	, auto_hide(false)
	, spacing(0)
	, perc_width(100)
	, activation_area(Anywhere)
{
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

void TAutohideToolbar::resizeAndMove() {

/*
	QWidget* widget = main_window;
	int w = widget->width() * perc_width / 100;
	int h = height();
	resize(w, h);

	int x = (widget->width() - width()) / 2;
	int y = widget->height() - height() - spacing;
	move(x, y);
*/
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

void TAutohideToolbar::aboutToEnterFullscreen() {

	auto_hide = true;
	timer->start();
}

void TAutohideToolbar::aboutToExitFullscreen() {

	timer->stop();
	if (auto_hide && isHidden()) {
		show();
	}
	auto_hide = false;
}

void TAutohideToolbar::showAuto() {

	show();
	timer->start();
}

bool TAutohideToolbar::insideShowArea(const QPoint& p) const {

	QPoint origin = mapToGlobal(QPoint(0, 0));
	return p.y() > origin.y() - 64 && p.y() < origin.y() + height() + 64;
}

bool TAutohideToolbar::eventFilter(QObject* obj, QEvent* event) {

	if (auto_hide
		&& event->type() == QEvent::MouseMove
		&& !isVisible()) {

		if (activation_area == Anywhere) {
			showAuto();
		} else {
			QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
			if (insideShowArea(mouse_event->globalPos())) {
				showAuto();
			}
		}

	}

	return QWidget::eventFilter(obj, event);
}

} // namespace Gui

#include "moc_autohidetoolbar.cpp"


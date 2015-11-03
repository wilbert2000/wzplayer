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
#include "settings/preferences.h"
#include "gui/base.h"

namespace Gui {

TAutohideToolbar::TAutohideToolbar(TBase* mainwindow, QWidget* playerwindow)
	: TEditableToolbar(mainwindow)
	, auto_hide(false) {

	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkUnderMouse()));

	playerwindow->installEventFilter(this);
}

TAutohideToolbar::~TAutohideToolbar() {
}

void TAutohideToolbar::startUnderMouseTimer() {

	timer->setInterval(pref->floating_hide_delay);
	timer->start();
}

// Slot called by timer
void TAutohideToolbar::checkUnderMouse() {

	if (auto_hide) {
		if (isVisible()) {
			if (underMouse() || QApplication::mouseButtons()) {
				startUnderMouseTimer();
			} else {
				hide();
			}
		}
	}
}

void TAutohideToolbar::didEnterFullscreen() {
	//qDebug("TAutohideToolbar::didEnterFullscreen");

	TEditableToolbar::didEnterFullscreen();
	auto_hide = true;
}

void TAutohideToolbar::aboutToExitFullscreen() {
	//qDebug("TAutohideToolbar::aboutToExitFullscreen");

	timer->stop();
	auto_hide = false;
}

void TAutohideToolbar::setVisible(bool visible) {
	//qDebug("TAutohideToolbar::setVisible: %d", visible);

	TEditableToolbar::setVisible(visible);
	if (auto_hide) {
		if (visible) {
			startUnderMouseTimer();
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
				if (pref->floating_activation_area == Settings::TPreferences::Anywhere) {
					show();
				} else {
					QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
					if (insideShowArea(mouse_event->globalPos())) {
						show();
					}
				}
			}
		}
	}

	return TEditableToolbar::eventFilter(obj, event);
}

} // namespace Gui

#include "moc_autohidetoolbar.cpp"


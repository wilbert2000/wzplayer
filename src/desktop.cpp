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

#include "desktop.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

QSize TDesktop::size(const QWidget* w) {
	return QApplication::desktop()->screenGeometry(w).size();
}

QSize TDesktop::availableSize(const QWidget* w) {
	return QApplication::desktop()->availableGeometry(w).size();
}

double TDesktop::aspectRatio(const QWidget* w) {

	QSize s = size(w);
	return  (double) s.width() / s.height() ;
}

void TDesktop::keepInsideDesktop(QWidget* w) {

	if (w->isMaximized())
		return;

	QSize desktop_size = availableSize(w);
	QPoint p = w->pos();
	QSize s = w->frameGeometry().size();
	int max = desktop_size.width() - s.width();
	if (p.x() > max)
		p.rx() = max;
	if (p.x() < 0)
		p.rx() = 0;

	max = desktop_size.height() - s.height();
	if (p.y() > max)
		p.ry() = max;
	if (p.y() < 0)
		p.ry() = 0;

	if (p != w->pos()) {
		qDebug() << "Gui::TDesktop::keepInsideDesktop: moving window from"
				 << w->pos() << "to" << p;
		w->move(p);
	}
}



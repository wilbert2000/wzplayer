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

#include "gui/desktop.h"
#include "wzdebug.h"
#include <QApplication>
#include <QDesktopWidget>


LOG4QT_DECLARE_STATIC_LOGGER(logger, TDesktop)


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

void TDesktop::centerWindow(QWidget* w) {

    if (!w->isMaximized()) {
        QRect available = QApplication::desktop()->availableGeometry(w);
        QSize center = (available.size() - w->frameGeometry().size()) / 2;
        if (center.isValid()) {
            w->move(available.x() + center.width(),
                    available.y() + center.height());
        }
    }
}

void TDesktop::keepInsideDesktop(QWidget* w) {

    if (w->isMaximized()) {
        return;
    }

    QRect available = QApplication::desktop()->availableGeometry(w);
    QPoint p = w->pos();
    QSize s = w->frameGeometry().size();
    int max = available.x() + available.width() - s.width();
    if (p.x() > max) {
        p.rx() = max;
    }
    if (p.x() < available.x()) {
        p.rx() = available.x();
    }

    max = available.y() + available.height() - s.height();
    if (p.y() > max) {
        p.ry() = max;
    }
    if (p.y() < available.y()) {
        p.ry() = available.y();
    }

    if (p != w->pos()) {
        WZDEBUG("Moving from " + QString::number(w->pos().x())
                + ", " + QString::number(w->pos().y())
                + " to " + QString::number(p.x())
                + ", " + QString::number(p.y()));
        w->move(p);
    }
}



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
        } else {
            w->move(available.topLeft());
        }
    }
}

bool TDesktop::keepInsideDesktop(QWidget* w) {

    if (w->isMaximized()) {
        return false;
    }

    QRect available = QApplication::desktop()->availableGeometry(w);
    QPoint pos = w->pos();
    QSize size = w->frameGeometry().size();
    int max = available.x() + available.width() - size.width();
    if (pos.x() > max) {
        pos.rx() = max;
    }
    if (pos.x() < available.x()) {
        pos.rx() = available.x();
    }

    max = available.y() + available.height() - size.height();
    if (pos.y() > max) {
        pos.ry() = max;
    }
    if (pos.y() < available.y()) {
        pos.ry() = available.y();
    }

    if (pos == w->pos()) {
        return false;
    }

    WZD << "Moving from" << w->pos() << "to" << pos;
    w->move(pos);
    return true;
}

bool TDesktop::windowIsSnapped(QWidget* w) {

    QRect avail = QApplication::desktop()->availableGeometry(w);
    QRect frame = w->frameGeometry();
    if (frame.height() == avail.height()) {
        if (frame.width() == avail.width() / 2) {
            if (frame.x() == avail.x()
                    || frame.x() == avail.x() + avail.width() / 2 + 1) {
                return true;
            }
        }
    } else if (frame.height() == avail.height() / 2) {
        if (frame.width() == avail.width()) {
            if (frame.y() == avail.y()
                    || frame.y() == avail.y() + avail.height() / 2 + 1) {
                return true;
            }
        } else if (frame.width() == avail.width() / 2) {
            if (frame.x() == avail.x()) {
                if (frame.y() == avail.y()
                        || frame.y() == avail.y() + avail.height() / 2 + 1) {
                    return true;
                }
            } else if (frame.x() == avail.x() + avail.width() / 2 + 1) {
                if (frame.y() == avail.y()
                        || frame.y() == avail.y() + avail.height() / 2 + 1) {
                    return true;
                }
            }
        }
    }

    return false;
}

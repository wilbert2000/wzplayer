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

#include "gui/pref/seekwidget.h"
#include <QLabel>
#include <QDateTimeEdit>

namespace Gui { namespace Pref {

TSeekWidget::TSeekWidget( QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	setupUi(this);
	time_edit->setDisplayFormat("mm:ss");
}

TSeekWidget::~TSeekWidget() {
}

void TSeekWidget::setIcon(QPixmap icon) {
	_image->setText("");
	_image->setPixmap(icon);
}

const QPixmap * TSeekWidget::icon() const {
	return _image->pixmap();
}

void TSeekWidget::setLabel(QString text) {
	_label->setText(text);
}

QString TSeekWidget::label() const {
	return _label->text();
}

void TSeekWidget::setTime(int secs) {
	QTime t(0,0);
	time_edit->setTime(t.addSecs(secs));
}

int TSeekWidget::time() const {
	QTime t = time_edit->time();
	return (t.minute() * 60) + t.second();
}

}} // namespace Gui::Pref

#include "moc_seekwidget.cpp"

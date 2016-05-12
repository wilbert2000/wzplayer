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

#include "gui/pref/selectcolorbutton.h"
#include "colorutils.h"
#include <QColorDialog>
#include <QApplication>
#include <QStyle>

namespace Gui { namespace Pref {

TSelectColorButton::TSelectColorButton(QWidget* parent) 
	: QPushButton(parent)
{
	connect(this, SIGNAL(clicked()), this, SLOT(selectColor()));

	ignore_change_event = false;
}

TSelectColorButton::~TSelectColorButton() {
}

void TSelectColorButton::setColor(QColor c) {

	_color = c;

	ignore_change_event = true;

	setStyleSheet(
		QString("QPushButton { background-color: #%1; border-style: outset; "
				"border-width: 2px; border-radius: 5px; "
				"border-color: grey; padding: 3px; min-width: 4ex; min-height: 1.2ex; } "
				"QPushButton:pressed { border-style: inset; }"
				).arg(ColorUtils::colorToRRGGBB(_color.rgb())));

	ignore_change_event = false;
}

void TSelectColorButton::selectColor() {
	QColor c = QColorDialog::getColor(_color, 0);
	if (c.isValid()) {
		setColor(c);
	}
}

void TSelectColorButton::changeEvent(QEvent *e) {

	QPushButton::changeEvent(e);
	
	if ((e->type() == QEvent::StyleChange) && (!ignore_change_event)) {
		setColor(color());
	}

}

}} // namespace Gui::Pref

#include "moc_selectcolorbutton.cpp"

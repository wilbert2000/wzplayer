/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

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

#include "gui/skin/iconsetter.h"
#include "gui/skin/icon.h"
#include "gui/skin/actiontools.h"
#include "images.h"


namespace Gui { namespace Skin {

TIconSetter* TIconSetter::m_instance = 0;

TIconSetter::TIconSetter() : QWidget() {
}

TIconSetter::~TIconSetter() {
}

void TIconSetter::setToolbarIcon(QPixmap) {
}

TIconSetter* TIconSetter::instance()
{
	if(m_instance == 0) {
		m_instance = new TIconSetter();
	}
	return m_instance;
}

void TIconSetter::removeInstance() {

	if(m_instance)
		delete m_instance;
	m_instance = 0;
}

void TIconSetter::buttonIcon(int buttonNo, const QPixmap& pix) {

	TIcon icon;
	int w = pix.width();
	int h = pix.height();
	icon.setPixmap(pix.copy(0, 0, w, h/4), TIcon::Normal, TIcon::Off);
	icon.setPixmap(pix.copy(0, h/4, w, h/4), TIcon::MouseOver, TIcon::Off);
	icon.setPixmap(pix.copy(0, h/2, w, h/4), TIcon::MouseDown, TIcon::Off);
	icon.setPixmap(pix.copy(0, 3*h/4, w, h/4), TIcon::Disabled, TIcon::Off);

	switch(buttonNo) {
		case 1: playControl->setBackwardIcon(icon); break;
		case 2: playControl->setPreviousIcon(icon);break;
		case 3: {
			TIcon icon2;
			icon2.setPixmap(pix.copy(0, 0, w/2, h/4), TIcon::Normal, TIcon::Off);
			icon2.setPixmap(pix.copy(0, h/4, w/2, h/4), TIcon::MouseOver, TIcon::Off);
			icon2.setPixmap(pix.copy(0, h/2, w/2, h/4), TIcon::MouseDown, TIcon::Off);
			icon2.setPixmap(pix.copy(0, 3*h/4, w/2, h/4), TIcon::Disabled, TIcon::Off);

			icon2.setPixmap(pix.copy(w/2, 0, w/2, h/4), TIcon::Normal, TIcon::On);
			icon2.setPixmap(pix.copy(w/2, h/4, w/2, h/4), TIcon::MouseOver, TIcon::On);
			icon2.setPixmap(pix.copy(w/2, h/2, w/2, h/4), TIcon::MouseDown, TIcon::On);
			icon2.setPixmap(pix.copy(w/2, 3*h/4, w/2, h/4), TIcon::Disabled, TIcon::On);

			playControl->setPlayPauseIcon(icon2);
		} break;
		case 4: playControl->setStopIcon(icon); break;
		case 5: playControl->setRecordIcon(icon); break;
		case 6: playControl->setNextIcon(icon); break;
		case 7: playControl->setForwardIcon(icon); break;
	}
}

void TIconSetter::mediaPanelButtonIcon(int n, const QPixmap& pix) {

	if(pix.isNull()) return;
	TIcon icon;
	int w = pix.width();
	int h = pix.height();
	icon.setPixmap(pix.copy(0, 0, w/2, h/4), TIcon::Normal, TIcon::Off);
	icon.setPixmap(pix.copy(0, h/4, w/2, h/4), TIcon::MouseOver, TIcon::Off);
	icon.setPixmap(pix.copy(0, h/2, w/2, h/4), TIcon::MouseDown, TIcon::Off);
	icon.setPixmap(pix.copy(0, 3*h/4, w/2, h/4), TIcon::Disabled, TIcon::Off);

	icon.setPixmap(pix.copy(w/2, 0, w/2, h/4), TIcon::Normal, TIcon::On);
	icon.setPixmap(pix.copy(w/2, h/4, w/2, h/4), TIcon::MouseOver, TIcon::On);
	icon.setPixmap(pix.copy(w/2, h/2, w/2, h/4), TIcon::MouseDown, TIcon::On);
	icon.setPixmap(pix.copy(w/2, 3*h/4, w/2, h/4), TIcon::Disabled, TIcon::On);

	switch(n) {
		case 1: mediaPanel->setShuffleIcon(icon); break;
		case 2: mediaPanel->setRepeatIcon(icon); break;
	}
}

} // namesapce Skin
} // namespace Gui

#include "moc_iconsetter.cpp"

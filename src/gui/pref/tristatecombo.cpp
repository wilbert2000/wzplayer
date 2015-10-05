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

#include "pref/tristatecombo.h"
#include <QEvent>

namespace Pref {

TTristateCombo::TTristateCombo( QWidget * parent ) : QComboBox(parent) 
{
	retranslateStrings();
}

TTristateCombo::~TTristateCombo() {
}

void TTristateCombo::retranslateStrings() {
	int i = currentIndex();

	clear();
	addItem( tr("Auto"), TPreferences::Detect );
	addItem( tr("Yes"), TPreferences::Enabled );
	addItem( tr("No"), TPreferences::Disabled );

	setCurrentIndex(i);
}

void TTristateCombo::setState( TPreferences::OptionState v ) {
	setCurrentIndex( findData(v) );
}

TPreferences::OptionState TTristateCombo::state() {
	return (TPreferences::OptionState) itemData( currentIndex() ).toInt();
}

// Language change stuff
void TTristateCombo::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QComboBox::changeEvent(e);
	}
}

} // namespace Pref

#include "moc_tristatecombo.cpp"

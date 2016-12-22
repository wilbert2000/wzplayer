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

#include "gui/pref/tristatecombo.h"
#include <QEvent>

namespace Gui { namespace Pref {

TTristateCombo::TTristateCombo(QWidget* parent) : QComboBox(parent) 
{
    retranslateStrings();
}

TTristateCombo::~TTristateCombo() {
}

void TTristateCombo::retranslateStrings() {
    int i = currentIndex();

    clear();
    addItem(tr("Auto"), Settings::TPreferences::Detect);
    addItem(tr("Yes"), Settings::TPreferences::Enabled);
    addItem(tr("No"), Settings::TPreferences::Disabled);

    setCurrentIndex(i);
}

void TTristateCombo::setState(Settings::TPreferences::TOptionState v) {
    setCurrentIndex(findData(v));
}

Settings::TPreferences::TOptionState TTristateCombo::state() {
    return (Settings::TPreferences::TOptionState) itemData(currentIndex()).toInt();
}

// Language change stuff
void TTristateCombo::changeEvent(QEvent *e) {
    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QComboBox::changeEvent(e);
    }
}

}} // namespace Gui::Pref

#include "moc_tristatecombo.cpp"

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

/*! 
    This class is to replace some QCheckBox with a combo with three possible
    values: true, false or autodetect
*/

#ifndef GUI_PREF_TRISTATE_COMBO_H
#define GUI_PREF_TRISTATE_COMBO_H

#include <QComboBox>
#include "settings/preferences.h"


namespace Gui {
namespace Pref {

class TTristateCombo : public QComboBox {
    Q_OBJECT

public:
    TTristateCombo(QWidget* parent = 0);

    void setState(Settings::TPreferences::TOptionState v);
    Settings::TPreferences::TOptionState state();

protected:
    virtual void retranslateStrings();
    virtual void changeEvent(QEvent* event);

};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_TRISTATE_COMBO_H

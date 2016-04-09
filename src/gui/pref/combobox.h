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

#ifndef GUI_PREF_COMBOBOX_H
#define GUI_PREF_COMBOBOX_H

#include <QComboBox>
#include <QFontComboBox>

//! This class adds some Qt 3 compatibility functions which don't have a
//! direct equivalent in Qt 4.

namespace Gui {
namespace Pref {

class TComboBox : public QComboBox {
public:
	TComboBox(QWidget* parent = 0);
	virtual ~TComboBox();

	void setCurrentText (const QString& text);
	void insertStringList (const QStringList& list, int index = -1);
};

class TFontComboBox : public QFontComboBox {
public:
	TFontComboBox(QWidget* parent = 0);
	~TFontComboBox();

	void setCurrentText (const QString& text);
	void setFontsFromDir(const QString& fontdir);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_COMBOBOX_H

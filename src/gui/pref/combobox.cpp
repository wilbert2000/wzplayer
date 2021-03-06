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

#include "gui/pref/combobox.h"
#include <QDir>
#include <QStringListModel>


namespace Gui { namespace Pref {

TComboBox::TComboBox(QWidget* parent) : QComboBox(parent) {
}

void TComboBox::setCurrentText(const QString & text) {
    int i = findText(text);
    if (i != -1)
        setCurrentIndex(i);
    else if (isEditable())
        setEditText(text);
    else
        setItemText(currentIndex(), text);
}

void TComboBox::insertStringList(const QStringList & list, int index) {
    insertItems((index < 0 ? count() : index), list);
}



TFontComboBox::TFontComboBox(QWidget* parent) : QFontComboBox(parent)
{
}

void TFontComboBox::setCurrentText(const QString & text) {
    int i = findText(text);
    if (i != -1)
        setCurrentIndex(i);
    else if (isEditable())
        setEditText(text);
    else
        setItemText(currentIndex(), text);
}

void TFontComboBox::setFontsFromDir(const QString & fontdir) {
    QString current_text = currentText();

    if (fontdir.isEmpty()) {
        QFontDatabase::removeAllApplicationFonts();
        clear();
        setWritingSystem(QFontDatabase::Any);
    } else {
        QFontDatabase fdb;
        QStringList fontnames;
        QStringList fontfiles = QDir(fontdir).entryList(QStringList() << "*.ttf" << "*.otf", QDir::Files);
        for (int n=0; n < fontfiles.count(); n++) {
            int id = fdb.addApplicationFont(fontdir +"/"+ fontfiles[n]);
            fontnames << fdb.applicationFontFamilies(id);
        }
        //fdb.removeAllApplicationFonts();
        fontnames.removeDuplicates();
        clear();
        QStringListModel *m = qobject_cast<QStringListModel *>(model());
        if (m) m->setStringList(fontnames);
    }

    setCurrentText(current_text);
}

}} // namespace Gui::Pref

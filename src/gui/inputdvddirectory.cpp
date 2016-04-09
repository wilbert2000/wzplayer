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

#include "gui/inputdvddirectory.h"

#include <QLineEdit>
#include "filedialog.h"

namespace Gui {

TInputDVDDirectory::TInputDVDDirectory(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setupUi(this);
}

TInputDVDDirectory::~TInputDVDDirectory() {
}

void TInputDVDDirectory::setFolder(QString folder) {
	dvd_directory_edit->setText(folder);
}

QString TInputDVDDirectory::folder() {
	return dvd_directory_edit->text();
}

void TInputDVDDirectory::on_searchButton_clicked() {
	QString s = MyFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"),
                    dvd_directory_edit->text());
	/*
	QString s = QFileDialog::getOpenFileName(
                    dvd_directory_edit->text(),
                    "*.*", this,
                    "select_dvd_device_dialog",
                    tr("Choose a directory or iso file"));
	*/

	if (!s.isEmpty()) {
		dvd_directory_edit->setText(s);
	}
}

} // namespace Gui

#include "moc_inputdvddirectory.cpp"

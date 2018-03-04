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

#include "gui/filechooser.h"
#include <QToolButton>
#include <QStyle>

#include "gui/filedialog.h"
#include "images.h"


namespace Gui {

QString TFileChooser::last_dir;

TFileChooser::TFileChooser(QWidget* parent) : Gui::TLineEditWithIcon(parent) {

    setDialogType(GetFileName);
    setOptions(0);

    setupButton();
    button->setCursor(Qt::PointingHandCursor);

    connect(button, &QToolButton::clicked, this, &TFileChooser::openFileDialog);
    connect(this, &TFileChooser::editingFinished,
            this, &TFileChooser::onEditingFinished);
}

TFileChooser::~TFileChooser() {
}

void TFileChooser::setupButton() {

    setIcon(Images::icon("folder_open"));
    button->setToolTip(tr("Click to select a file or folder"));
}

void TFileChooser::openFileDialog() {

    QString result;
    QString f;

    if (dialogType() == GetFileName) {
        QFileDialog::Options opts = _options;
        if (opts == 0)
            opts = QFileDialog::DontResolveSymlinks;

        QString dir = QFileInfo(text()).absolutePath();
        if (dir.isEmpty())
            dir = last_dir;
        if (dir.isEmpty())
            dir = QDir::homePath();

        result = TFileDialog::getOpenFileName(
                     this,
                     _caption,
                     dir,
                     _filter,
                     &f,
                     opts);
        if (!result.isEmpty()) {
            last_dir = QFileInfo(result).absolutePath();
        }
    } else if (dialogType() == GetDirectory) {
        QFileDialog::Options opts = options();
        if (opts == 0)
            opts = QFileDialog::ShowDirsOnly;

        QString dir = text();
        if (dir.isEmpty())
            dir = last_dir;
        if (dir.isEmpty())
            dir = QDir::homePath();

        result = TFileDialog::getExistingDirectory(
                     this,
                     _caption,
                     dir,
                     opts);
        if (!result.isEmpty()) {
            last_dir = result;
        }
    }

    if (!result.isEmpty()) {
        QString old_file = text();
        setText(result);
        if (old_file != result)
            emit fileChanged(result);
    }
}

void TFileChooser::onEditingFinished() {

    if (isModified()) {
        emit fileChanged(text());
    }
}

} // namespace Gui

#include "moc_filechooser.cpp"

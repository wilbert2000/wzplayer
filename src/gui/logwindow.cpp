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

#include "gui/logwindow.h"
#include "gui/logwindowappender.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>

#include "wzdebug.h"
#include "config.h"
#include "gui/desktop.h"
#include "images.h"
#include "gui/filedialog.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {

TLogWindowAppender* TLogWindow::appender = 0;

TLogWindow::TLogWindow(QWidget* parent)
    : QWidget(parent) {

    setupUi(this);
    setObjectName("logwindow");

    connect(saveButton, SIGNAL(clicked()), this, SLOT(onSaveButtonClicked()));
    connect(copyButton, SIGNAL(clicked()), this, SLOT(onCopyButtonClicked()));
    connect(findEdit, SIGNAL(returnPressed()),
            this, SLOT(onFindNextButtonClicked()));
    connect(findPreviousButton, SIGNAL(clicked()),
            this, SLOT(onFindPreviousButtonClicked()));
    connect(findNextButton, SIGNAL(clicked()),
            this, SLOT(onFindNextButtonClicked()));

    edit->setMaximumBlockCount(pref->log_window_max_events);
    edit->setFont(QFont("Monospace"));

    retranslateStrings();
}

TLogWindow::~TLogWindow() {
    WZDEBUG("");
    appender->setEdit(0);
}

void TLogWindow::retranslateStrings() {

    retranslateUi(this);

    saveButton->setText("");
    saveButton->setIcon(Images::icon("save"));
    copyButton->setText("");
    copyButton->setIcon(Images::icon("copy"));
}

void TLogWindow::showEvent(QShowEvent*) {

    appender->setEdit(edit);
}

void TLogWindow::hideEvent(QShowEvent*) {

    appender->setEdit(0);
}

// Call hideEvent() and accept close
void TLogWindow::closeEvent(QCloseEvent* event) {
    WZDEBUG("");

    hideEvent(0);
    event->accept();
}

void TLogWindow::onSaveButtonClicked() {

    QString s = TFileDialog::getSaveFileName(
                    this, tr("Choose a filename to save under"), 
                    "", tr("Logs") +" (*.log *.txt)");

    if (!s.isEmpty()) {
        if (QFileInfo(s).exists()) {
            int res =QMessageBox::question(this,
                tr("Confirm overwrite?"),
                tr("The file already exists.\n"
                   "Do you want to overwrite?"),
                QMessageBox::Yes,
                QMessageBox::No,
                QMessageBox::NoButton);
            if (res == QMessageBox::No) {
                return;
            }
        }

        QFile file(s);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << edit->toPlainText();
            file.close();
        } else {
            WZERROR("failed to save file '" + s + "'");
            QMessageBox::warning (this,
                tr("Error saving file"),
                tr("The log couldn't be saved"),
                QMessageBox::Ok,
                QMessageBox::NoButton,
                QMessageBox::NoButton);

        }
    }
}

void TLogWindow::onCopyButtonClicked() {

    edit->selectAll();
    edit->copy();
}

void TLogWindow::onFindPreviousButtonClicked() {

    edit->find(findEdit->text(), QTextDocument::FindBackward);
}

void TLogWindow::onFindNextButtonClicked() {

    if (edit->textCursor().atEnd()) {
        edit->moveCursor(QTextCursor::Start);
    }
    edit->find(findEdit->text());
}

} // namespace Gui

#include "moc_logwindow.cpp"

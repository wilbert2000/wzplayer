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

#include "gui/helpwindow.h"
#include "config.h"
#include "gui/desktop.h"
#include "images.h"

#include <QCloseEvent>
#include <QTextEdit>
#include <QSettings>


namespace Gui {

THelpWindow::THelpWindow(QWidget* parent, const QString& name)
    : QWidget(parent, Qt::Window) {

    setupUi(this);
    setObjectName(name);
    retranslateStrings();
}

THelpWindow::~THelpWindow() {
}

void THelpWindow::retranslateStrings() {

    retranslateUi(this);

    saveButton->setText("");
    copyButton->setText("");

    saveButton->setIcon(Images::icon("save"));
    copyButton->setIcon(Images::icon("copy"));

    // Title changed by TMainWindow::helpCLOptions()
    setWindowTitle(tr("%1 help").arg(TConfig::PROGRAM_NAME));
    setWindowIcon(Images::icon("logo"));
}

void THelpWindow::loadSettings(QSettings* pref) {

    pref->beginGroup(objectName());
    QPoint p = pref->value("pos", QPoint()).toPoint();
    QSize s = pref->value("size", QPoint()).toSize();
    int state = pref->value("state", 0).toInt();
    pref->endGroup();

    if (s.width() > 200 && s.height() > 200) {
        move(p);
        resize(s);
        setWindowState((Qt::WindowStates) state);
        TDesktop::keepInsideDesktop(this);
    }
}

void THelpWindow::saveSettings(QSettings* pref) {

    pref->beginGroup(objectName());
    pref->setValue("pos", pos());
    pref->setValue("size", size());
    pref->setValue("state", (int) windowState());
    pref->endGroup();
}

void THelpWindow::showEvent(QShowEvent*) {
    emit visibilityChanged(true);
}

void THelpWindow::hideEvent(QShowEvent*) {

    clear();
    emit visibilityChanged(false);
}

// Fix hideEvent() not called on close
void THelpWindow::closeEvent(QCloseEvent* event) {

    hideEvent(0);
    event->accept();
}

void THelpWindow::setText(const QString& log) {
    browser->setPlainText(log);
    browser->moveCursor(QTextCursor::End);
}

QString THelpWindow::text() {
    return browser->toPlainText();
}

void THelpWindow::setHtml(const QString& text) {
    browser->setHtml(text);
}

QString THelpWindow::html() {
    return browser->toHtml();
}

void THelpWindow::clear() {
    browser->clear();
}

void THelpWindow::appendText(const QString& text) {
    browser->moveCursor(QTextCursor::End);
    browser->insertPlainText(text);
}

void THelpWindow::appendHtml(const QString& text) {
    browser->moveCursor(QTextCursor::End);
    browser->insertHtml(text);
}

void THelpWindow::onCopyButtonClicked() {
    browser->selectAll();
    browser->copy();
}

} // namespace Gui

#include "moc_helpwindow.cpp"

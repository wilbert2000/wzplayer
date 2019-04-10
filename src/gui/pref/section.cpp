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

#include "gui/pref/section.h"
#include <QEvent>
#include "settings/assstyles.h"
#include "wzdebug.h"


namespace Gui {
namespace Pref {

TSection::TSection(QWidget* parent, Qt::WindowFlags f) :
    QWidget(parent, f),
    _requiresRestartApp(false),
    _requiresRestartPlayer(false),
    iconSize(32) {
}

void TSection::getData(Settings::TPreferences* pref) {
    Q_UNUSED(pref)

    _requiresRestartApp = false;
    _requiresRestartPlayer = false;
}

QString TSection::sectionName() {
    return QString();
}

QPixmap TSection::sectionIcon() {
    return QPixmap();
}

void TSection::addSectionTitle(const QString& title) {
    help_message += "<h2>" + title + "</h2>";
}

void TSection::addSectionGroup(const QString& title) {
    help_message += "<h3>" + title + "</h3>";
}

void TSection::restartIfBoolChanged(bool& old_value, bool new_value,
                                   const QString& name) {

    if (old_value != new_value) {
        WZDEBUG(QString("need restart, %1 changed from %2 to %3")
                .arg(name).arg(old_value).arg(new_value));
        old_value = new_value;
        _requiresRestartPlayer = true;
    }
}

void TSection::restartIfIntChanged(int& old_value,
                                  int new_value,
                                  const QString& name) {

    if (old_value != new_value) {
        WZDEBUG(QString("need restart, %1 changed from %2 to %3")
                .arg(name).arg(old_value).arg(new_value));
        old_value = new_value;
        _requiresRestartPlayer = true;
    }
}

void TSection::restartIfUIntChanged(unsigned int& old_value,
                                   unsigned int new_value,
                                   const QString& name) {

    if (old_value != new_value) {
        WZDEBUG(QString("need restart, %1 changed from %2 to %3")
                .arg(name).arg(old_value).arg(new_value));
        old_value = new_value;
        _requiresRestartPlayer = true;
    }
}

void TSection::restartIfDoubleChanged(double& old_value, const double& new_value,
                                     const QString& name) {

    if (old_value != new_value) {
        WZDEBUG(QString("need restart, %1 changed from %2 to %3")
                .arg(name).arg(old_value).arg(new_value));
        old_value = new_value;
        _requiresRestartPlayer = true;
    }
}

void TSection::restartIfStringChanged(QString& old_value,
                                     const QString& new_value,
                                     const QString& name) {

    if (old_value != new_value) {
        WZDEBUG(QString("need restart, %1 changed from %2 to %3")
                .arg(name).arg(old_value).arg(new_value));
        old_value = new_value;
        _requiresRestartPlayer = true;
    }
}

void TSection::setWhatsThis(QWidget* w,
                           const QString& title,
                           const QString& text,
                           bool set_tooltip,
                           bool set_help) {

    w->setWhatsThis(text);
    if (set_help) {
        help_message += "<b>"+title+"</b><br>"+text+"<br><br>";
    }
    if (set_tooltip) {
        w->setToolTip("<qt>"+ text +"</qt>");
    }
}

void TSection::clearHelp() {
    help_message = "<h1>" + sectionName() + "</h1>";
}

// Language change stuff
void TSection::changeEvent(QEvent *e) {
    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QWidget::changeEvent(e);
    }
}

void TSection::retranslateStrings() {
}

} // namespace Pref
} // namespace Gui

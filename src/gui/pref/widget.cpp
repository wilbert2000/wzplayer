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

#include "gui/pref/widget.h"
#include <QEvent>
#include "settings/assstyles.h"

namespace Gui {
namespace Pref {

TWidget::TWidget(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
	, requires_restart(false)
	, icon_size(32) {
}

TWidget::~TWidget() {
}

QString TWidget::sectionName() {
	return QString();
}

QPixmap TWidget::sectionIcon() {
	return QPixmap();
}

void TWidget::addSectionTitle(const QString& title) {
	help_message += "<h2>" + title + "</h2>";
}

void TWidget::addSectionGroup(const QString& title) {
	help_message += "<h3>" + title + "</h3>";
}

void TWidget::restartIfBoolChanged(bool& old_value, bool new_value) {

	if (old_value != new_value) {
		old_value = new_value;
		requires_restart = true;
	}
}

void TWidget::restartIfIntChanged(int& old_value, int new_value) {

	if (old_value != new_value) {
		old_value = new_value;
		requires_restart = true;
	}
}

void TWidget::restartIfUIntChanged(unsigned int& old_value, unsigned int new_value) {

	if (old_value != new_value) {
		old_value = new_value;
		requires_restart = true;
	}
}

void TWidget::restartIfDoubleChanged(double& old_value, const double& new_value) {

	if (old_value != new_value) {
		old_value = new_value;
		requires_restart = true;
	}
}

void TWidget::restartIfStringChanged(QString& old_value, const QString& new_value) {

	if (old_value != new_value) {
		old_value = new_value;
		requires_restart = true;
	}
}

void TWidget::setWhatsThis(QWidget* w,
                           const QString& title,
                           const QString& text,
                           bool set_tooltip) {

	w->setWhatsThis(text);
	help_message += "<b>"+title+"</b><br>"+text+"<br><br>";
    if (set_tooltip) {
        w->setToolTip("<qt>"+ text +"</qt>");
    }
}

void TWidget::clearHelp() {
	help_message = "<h1>" + sectionName() + "</h1>";
}

// Language change stuff
void TWidget::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

void TWidget::retranslateStrings() {
}

} // namespace Pref
} // namespace Gui

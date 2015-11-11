/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/action/action.h"
#include <QDebug>
#include <QEvent>
#include <QWidget>
#include "images.h"


namespace Gui {

TAction::TAction (QObject* parent,
				  const QString& name,
				  const QString& text,
				  bool autoadd)
	: QAction(parent)
	, text_en(text) {

	setObjectName(name);
	retranslateStrings();
	if (autoadd)
		addActionToParent();
}

TAction::TAction (QObject* parent,
				  const QString& name,
				  const QString& text,
				  const QString& icon,
				  bool autoadd)
	: QAction(parent)
	, text_en(text) {

	setObjectName(name);
	setIcon(Images::icon(icon));
	retranslateStrings();
	if (autoadd)
		addActionToParent();
}

TAction::TAction (QObject* parent,
				  const QString& name,
				  const QString& text,
				  const QString& icon,
				  QKeySequence accel,
				  bool autoadd)
	: QAction(parent)
	, text_en(text) {

	setObjectName(name);
	setIcon(Images::icon(icon));
	setShortcut(accel);
	retranslateStrings();
	if (autoadd)
		addActionToParent();
}

TAction::TAction(QObject* parent,
				 const QString& name,
				 const QString& text,
				 QKeySequence accel,
				 bool autoadd)
	: QAction(parent)
	, text_en(text) {

	setObjectName(name);
	setShortcut(accel);
	retranslateStrings();
	if (autoadd)
		addActionToParent();
}

TAction::~TAction() {
}

void TAction::retranslateStrings() {
	// Translate with parent
	if (!text_en.isEmpty())
		change(parent()->tr(text_en.toUtf8().constData()));
}

bool TAction::event(QEvent* e) {

	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	}
	return QAction::event(e);
}

void TAction::addShortcut(QKeySequence key) {
	setShortcuts(shortcuts() << key);
}

void TAction::addActionToParent() {

	if (parent()) {
		if (parent()->inherits("QWidget")) {
			QWidget* w = static_cast<QWidget*> (parent());
			w->addAction(this);
		}
	}
}

void TAction::change(const QString& text) {

	setText(text);

	QString accel_text = shortcut().toString();
	if (!accel_text.isEmpty()) {
		QString s = text;
		s.replace("&", "");
		setToolTip(s + " (" + accel_text + ")");
	}
}

void TAction::update(bool check) {

	if (check != isChecked())
		trigger();
}

} // namespace Gui

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

#ifndef _GUI_ACTION_H_
#define _GUI_ACTION_H_

#include <QAction>
#include <QString>
#include <QIcon>
#include <QKeySequence>

namespace Gui {

class TAction : public QAction {
public:
	//! Creates a new TAction with name \a name. If \a autoadd is true
	//! the action will be added to the parent
	TAction(QObject* parent,
			const QString& name,
			const QString& text,
			const QString& iconName = QString(),
			bool autoadd = true);

	TAction(QObject* parent,
			const QString& name,
			const QString& text,
			const QString& iconName,
			QKeySequence accel,
			bool autoadd = true);

	virtual ~TAction();

	void addShortcut(QKeySequence key);

	//! Change the text of the action.
	void setTextAndTip(const QString& text);

	void update(bool check);

protected:
	virtual bool event(QEvent* event);

	//! Checks if the parent is a QWidget and adds the action to it.
	void addActionToParent();

private:
	QString text_en;

	void init(const QString& name, QString iconName, bool autoadd);
	void retranslateStrings();
};

} // namespace Gui

#endif // _GUI_ACTION_H_


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

class TAction : public QAction
{

public:
	//! Creates a new TAction with name \a name. If \a autoadd is true
	//! the action will be added to the parent
	TAction ( QObject * parent, const char * name, bool autoadd = true );

	//! Creates a new TAction. If \a autoadd is true
	//! the action will be added to the parent
	TAction ( QObject * parent, bool autoadd = true );

	TAction ( const QString & text, QKeySequence accel,
               QObject * parent, const char * name = "",
               bool autoadd = true );

	TAction ( QKeySequence accel, QObject * parent,
               const char * name = "", bool autoadd = true );

	virtual ~TAction();

	void addShortcut(QKeySequence key);

	//! Change the icon and text of the action.
	void change(const QIcon & icon, const QString & text );

	//! Change the text of the action.
	void change(const QString & text);

protected:
	//! Checks if the parent is a QWidget and adds the action to it.
	void addActionToParent();
};

} // namespace Gui

#endif // _GUI_ACTION_H_


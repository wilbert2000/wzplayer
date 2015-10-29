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

#ifndef GUI_EDITABLETOOLBAR_H
#define GUI_EDITABLETOOLBAR_H

#include <QToolBar>
#include <QList>
#include <QStringList>

class QAction;

namespace Gui {

class TBase;

typedef QList<QAction*> TActionList;

class TEditableToolbar : public QToolBar {
	Q_OBJECT

public:
	TEditableToolbar(TBase* mainwindow);
	virtual ~TEditableToolbar();

	void setActionsFromStringList(const QStringList& acts, const TActionList& all_actions);
	QStringList actionsToStringList() const { return actions; }

	void setDefaultActions(const QStringList& action_names) { default_actions = action_names; }
	QStringList defaultActions() const { return default_actions; }

	virtual void didEnterFullscreen();
	virtual void didExitFullscreen();

public slots:
	void edit();

protected:
	TBase* main_window;

private:
	QStringList actions;
	QStringList default_actions;

	void reload();

private slots:
	void showPopup(const QPoint& pos);
};

} // namesapce Gui

#endif // GUI_EDITABLETOOLBAR_H

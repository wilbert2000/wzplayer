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

#ifndef _GUI_TOOLBAR_EDITOR_H_
#define _GUI_TOOLBAR_EDITOR_H_

#include <QStringList>
#include <QWidget>
#include <QList>
#include <QAction>
#include "ui_toolbareditor.h"

class QListWidget;

namespace Gui {

typedef QList<QAction*> TActionList;

class TToolbarEditor : public QDialog, public Ui::TToolbarEditor {
	Q_OBJECT

public:
	TToolbarEditor(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TToolbarEditor();

	void setAllActions(const TActionList& actions_list);
	void setActiveActions(const TActionList& actions_list);

	QStringList activeActionsToStringList() const;

	void setDefaultActions(const QStringList& action_names) { default_actions = action_names; }
	QStringList defaultActions() const { return default_actions; }

	void setIconSize(int size);
	int iconSize() const;

	//! Save the widget's list of actions into a QStringList 
	static QStringList save(QWidget *w);

	//! Add to the widget the actions specified in actions.
	//! all_actions is the list of all available actions
	static void load(QWidget* w, const QStringList& actions, const TActionList& all_actions);

protected slots:
	void on_up_button_clicked();
	void on_down_button_clicked();
	void on_right_button_clicked();
	void on_left_button_clicked();
	void on_separator_button_clicked();
	void restoreDefaults();
	void checkRowsAllList(int currentRow);
	void checkRowsActiveList(int currentRow);

protected:
	static QAction* findAction(const QString& action_name, const TActionList& actions_list);

	static void populateList(QListWidget* w, const TActionList& actions_list, bool add_separators = false);
	static int findItem(const QString& action_name, QListWidget* w);

	static QString fixname(const QString& name, const QString& action_name);

	TActionList all_actions_copy;
	QStringList default_actions;
};

} // namespace Gui

#endif // _GUI_TOOLBAR_EDITOR_H_


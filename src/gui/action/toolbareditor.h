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

#ifndef GUI_ACTION_TOOLBAR_EDITOR_H
#define GUI_ACTION_TOOLBAR_EDITOR_H

#include <QStringList>
#include "ui_toolbareditor.h"
#include "gui/action/actionlist.h"


class QWidget;
class QListWidget;

namespace Gui {
namespace Action {


class TToolbarEditor : public QDialog, public Ui::TToolbarEditor {
	Q_OBJECT

public:
	TToolbarEditor(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TToolbarEditor();

	void setAllActions(const TActionList& actions_list);
	void setActiveActions(const QStringList& actions);
	void setDefaultActions(const QStringList& action_names) { default_actions = action_names; }
	void setIconSize(int size);
	int iconSize() const;

	QStringList saveActions();
	static QAction* findAction(const QString& action_name, const TActionList& actions_list);
	static QAction* newSeparator(QWidget* parent);
	static void stringToAction(const QString& s, QString& action_name, bool& ns, bool&fs);

protected:
	static void populateList(QListWidget* w, const TActionList& actions_list);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void changeEvent(QEvent* event);

protected slots:
	void on_up_button_clicked();
	void on_down_button_clicked();
	void on_right_button_clicked();
	void on_left_button_clicked();
	void on_separator_button_clicked();
	void restoreDefaults();
	void checkRowsAllList(int currentRow);
	void onCurrentCellChanged(int currentRow, int currentColumn,
							  int previousRow, int previousColumn);

private:
	const TActionList* all_actions;
	QStringList default_actions;

	bool getVis(int row, int col);
	void insertRowFromAction(int row, QAction* action, bool ns, bool fs);
	void insertSeparator(int row, bool ns, bool fs);
	void swapRows(int row1, int row2);
	void setCurrentRow(int row);
	void resizeColumns();
	void retranslateStrings();
}; // class TToolbarEditor

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_TOOLBAR_EDITOR_H


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

/* This is based on qq14-actioneditor-code.zip from Qt */

#ifndef GUI_ACTION_ACTIONSEDITOR_H
#define GUI_ACTION_ACTIONSEDITOR_H

#include <QWidget>
#include <QStringList>
#include <gui/action/actionlist.h>

class QTableWidget;
class QTableWidgetItem;
class QSettings;
class QPushButton;

namespace Gui {
namespace Action {

typedef QList<QKeySequence> TShortCutList;

class TActionsEditor : public QWidget {
	Q_OBJECT

public:
	TActionsEditor(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TActionsEditor();

	// Clear the actionlist
	void clear();

	// There are no actions yet?
	bool isEmpty();

	void addActions(QWidget* widget);

	// Static functions
	static QString actionTextToDescription(const QString& text, const QString& action_name);
	static QAction* findAction(QObject* o, const QString& name);
	static QStringList actionsNames(QObject* o);

	static void saveToConfig(QObject* o, QSettings* set);
	static void loadFromConfig(const TActionList& all_actions, QSettings* set);

	static QString shortcutsToString(const TShortCutList& shortcuts);
	static TShortCutList stringToShortcuts(const QString& shortcuts);

public slots:
	void applyChanges();
	void saveActionsTable();
	bool saveActionsTable(const QString& filename);
	void loadActionsTable();
	bool loadActionsTable(const QString& filename);

	void updateView();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent(QEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

	// Find in table, not in actionslist
	int findActionName(const QString& name);
	int findActionAccel(const QString& accel, int ignoreRow = -1);
	bool hasConflicts();
	static bool containsShortcut(const QString& accel, const QString& shortcut);

protected slots:
	void editShortcut();

private:

	enum TActionCols {
		COL_CONFLICTS = 0,
		COL_ACTION = 1,
		COL_DESC = 2,
		COL_SHORTCUT = 3,
		COL_COUNT = 4
	};

	QTableWidget* actionsTable;
	TActionList actionsList;
	QPushButton* saveButton;
	QPushButton* loadButton;
	QPushButton* editButton;
	QString latest_dir;

	static QString actionToString(const QAction& action);
	static void setActionFromString(QAction& action, const QString& s, const TActionList& actions);
	static void removeShortcuts(const TActionList& actions, const TShortCutList& shortcuts, QAction* skip_action);
	void resizeColumns();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_ACTIONSEDITOR_H

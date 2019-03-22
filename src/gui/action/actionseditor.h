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


class QTableWidget;
class QTableWidgetItem;
class QSettings;
class QPushButton;

namespace Gui {

namespace Action {

class TActionsEditor : public QWidget {
    Q_OBJECT
public:
    // Static functions
    static QString cleanActionText(const QString& text,
                                   const QString& actionName);

    static void saveSettings(QSettings* set);
    static void loadSettings(QSettings* set);
    static QStringList shortcutsToStringList(const QString& s);

    TActionsEditor(QWidget* parent);

    void setActionsTable();
    void applyChanges();

    void findShortcutActionAndLabel(const QString& shortcut,
                                    const QString& actionOwner,
                                    QString& actionName,
                                    QString& label);

private:
    enum TActionCols {
        COL_CONFLICTS = 0,
        COL_ACTION = 1,
        COL_DESC = 2,
        COL_FOR = 3,
        COL_SHORTCUTS = 4,
        COL_COUNT = 5
    };

    QTableWidget* actionsTable;
    QPushButton* saveButton;
    QPushButton* loadButton;
    QPushButton* editButton;
    QString last_dir;

    QString getWindowForAction(QAction* action) const;
    int findShortcutsInTable(const QString& aShortCuts,
                             const QString& actionOwner,
                             int startRow,
                             int skipRow);
    int findActionNameInTable(const QString& name);

    static QString keySequnceToString(QKeySequence key);
    static QKeySequence stringToKeySequence(QString s);
    static QString shortcutsToString(const QList<QKeySequence>& shortcuts);
    static QList<QKeySequence> stringToShortcutList(const QString& shortcuts);

    static QString actionToString(QAction *action);
    static void setActionFromString(QAction* action, const QString& s);
    static bool removeShortcutsFromList(const QList<QKeySequence>& remove,
                                        QList<QKeySequence>& from,
                                        const QString& toName,
                                        const QString& fromName);

    static void removeShortcutsFromActions(const QList<QKeySequence>& shortcuts,
                                           QAction* skip_action);

    bool loadActionsTableFromFile(const QString& filename);
    bool saveActionsTableAsFile(const QString& filename);

    void setConflictTextModified(int row);
    void removeConflictingShortcuts(int row, int conflictRow);
    bool updateConflictCell(int row, bool takeShortcuts, int rowToKeep);
    bool updateConflicts(bool takeShortcuts, int rowToKeep);

private slots:
    void editShortcut();
    void loadActionsTable();
    void saveActionsTable();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_ACTIONSEDITOR_H

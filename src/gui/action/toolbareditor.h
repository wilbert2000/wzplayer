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

#include "ui_toolbareditor.h"
#include "wzdebug.h"
#include <QStringList>


class QWidget;
class QListWidget;

namespace Gui {
namespace Action {

class TActionItem;

class TToolbarEditor : public QDialog, public Ui::TToolbarEditor {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    static QAction* findAction(const QString& actionName,
                               const QList<QAction*>& actionList);
    static void stringToAction(const QString& s, QString& actionName,
                               bool& ns, bool&fs);

    TToolbarEditor(QWidget* parent, const QString& tbarName);
    virtual ~TToolbarEditor();

    void setAllActions(const QList<QAction*>& actions);
    void setActiveActions(const QStringList& actions);
    void setDefaultActions(const QStringList& actionNames) {
        defaultActions = actionNames;
    }
    void setIconSize(int size);
    int iconSize() const;

    QStringList saveActions();

private:
    QList<QAction*> allActions;
    QString toolbarName;
    QStringList defaultActions;
    QAction* separatorAction;
    int savedFirst, savedLast;

    QString getToolTipForItem(QListWidgetItem* item);
    Qt::CheckState getCheckStateItem(bool ns, bool fs);
    TActionItem* createActiveActionItem(QAction* action, bool ns, bool fs);
    bool isActionEditable(QAction* action);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onCurrentRowChanged(int currentRow);
    void onRowsInsertedActiveList();
    void onRowsInserted(const QModelIndex&, int first, int last);
    void onUpButtonClicked();
    void onDownButtonClicked();
    void onRightButtonClicked();
    void onLeftButtonClicked();
    void onSeperatorButtonClicked();
    void restoreDefaults();
}; // class TToolbarEditor

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_TOOLBAR_EDITOR_H


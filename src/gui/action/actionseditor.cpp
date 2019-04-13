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


#include "gui/action/actionseditor.h"
#include "gui/action/shortcutgetter.h"
#include "gui/action/action.h"
#include "gui/filedialog.h"
#include "settings/paths.h"
#include "images.h"
#include "iconprovider.h"
#include "wzdebug.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QString>
#include <QAction>
#include <QLayout>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegExp>


/*
#include <QLineEdit>
#include <QItemDelegate>

class MyDelegate : public QItemDelegate 
{
public:
    MyDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;
        virtual void setModelData(QWidget* editor, QAbstractItemModel* model,
                              const QModelIndex & index) const;
};

MyDelegate::MyDelegate(QObject *parent) : QItemDelegate(parent)
{
}

static QString old_accel_text;

QWidget* MyDelegate::createEditor(QWidget *parent, 
                                   const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const
{
    logger()->debug("MyDelegate::createEditor");

    old_accel_text = index.model()->data(index, Qt::DisplayRole).toString();

    return QItemDelegate::createEditor(parent, option, index);
}

void MyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const
{
    QLineEdit *line_edit = static_cast<QLineEdit*>(editor);

    QString accelText = QKeySequence(line_edit->text()).toString();
    if (accelText.isEmpty() && !line_edit->text().isEmpty()) {
        model->setData(index, old_accel_text);
    }
    else {
        model->setData(index, accelText);
    }
}
*/

namespace Gui {
namespace Action {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Action::TActionsEditor)

const int WIDTH_CONFLICT_ICON = 16;
const int MARGINS = 2;

TActionsEditor::TActionsEditor(QWidget* parent) :
    QWidget(parent) {

    last_dir = Settings::TPaths::shortcutsPath();

    actionsTable = new QTableWidget(0, COL_COUNT, this);
    actionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    actionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    actionsTable->setIconSize(iconProvider.iconSize);

    actionsTable->verticalHeader()->hide();
    QHeaderView* h = actionsTable->horizontalHeader();
    h->setHighlightSections(false);
    h->setMinimumSectionSize(0);
    actionsTable->setHorizontalHeaderLabels(QStringList()
        << "" << tr("Action") << tr("Description") << tr("For")
        << tr("Shortcuts"));

    h->setSectionResizeMode(COL_CONFLICTS, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_ACTION, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_DESC, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_FOR, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_SHORTCUTS, QHeaderView::Stretch);
    h->setStretchLastSection(true);

    connect(actionsTable, &QTableWidget::itemActivated,
            this, &TActionsEditor::editShortcut);

    saveButton = new QPushButton(this);
    saveButton->setText(tr("Save"));
    saveButton->setIcon(iconProvider.saveIcon);

    loadButton = new QPushButton(this);
    loadButton->setText(tr("Load"));
    loadButton->setIcon(iconProvider.openIcon);

    connect(saveButton, &QPushButton::clicked,
            this, &TActionsEditor::saveActionsTable);
    connect(loadButton, &QPushButton::clicked,
            this, &TActionsEditor::loadActionsTable);

    editButton = new QPushButton(this);
    editButton->setText(tr("Change shortcut..."));
    connect(editButton, &QPushButton::clicked,
            this, &TActionsEditor::editShortcut);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(16, 8, 16, 0);
    buttonLayout->setSpacing(6);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(editButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(actionsTable);
    mainLayout->addLayout(buttonLayout);
}

QString TActionsEditor::getWindowForAction(QAction* action) const {

    QString window;
    QObject* parent = action->parent();
    while (parent && window.isEmpty()) {
        window = parent->objectName();
        parent = parent->parent();
    }

    if (window.endsWith("widget")) {
        window = window.left(window.length() - 6);
    } else if (window.endsWith("group")
               || window.endsWith("dock")
               || window.startsWith("toolbar")
               || window == "controlbar") {
        window = "mainwindow";
    }

    if (window == "mainwindow") {
        return tr("All");
    }
    if (window == "playlist" || window.startsWith("pl_")) {
        return tr("Playlist");
    }
    if (window == "favlist" || window.startsWith("fav_")) {
        return tr("Favorites");
    }

    WZERROR(QString("Found unexpected owner '%1' for action '%2'")
            .arg(window).arg(action->objectName()));
    return window;
}

void TActionsEditor::setActionsTable() {

    bool sortEnabled = actionsTable->isSortingEnabled();
    actionsTable->setSortingEnabled(false);
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    for (int i = 0; i < TAction::allActions.count(); i++) {
        // Add one row at the time to speed up the call to updateConflictCell(),
        // which will iterates over every row of the action table to check
        // for conflicts
        actionsTable->setRowCount(i + 1);

        QAction* action = TAction::allActions.at(i);

        // Conflict column
        QTableWidgetItem* conflictItem = new QTableWidgetItem();
        conflictItem->setFlags(flags);
        if (action->property("modified").toBool()) {
            conflictItem->setText(tr("m"));
        }
        if (action->property("modifiedicontext").toBool()) {
            conflictItem->setText(conflictItem->text() + tr("i"));
        }
        actionsTable->setItem(i, COL_CONFLICTS, conflictItem);

        // Action column
        QTableWidgetItem* item = new QTableWidgetItem(action->objectName());
        item->setFlags(flags);
        actionsTable->setItem(i, COL_ACTION, item);

        // Desc column
        item = new QTableWidgetItem(
                    cleanActionText(action->text(), action->objectName()));
        item->setFlags(flags);
        QIcon icon = action->icon();
        if (icon.isNull()) {
            icon = Images::icon("empty_icon");
        }
        item->setIcon(icon);
        actionsTable->setItem(i, COL_DESC, item);

        // For column
        item = new QTableWidgetItem(getWindowForAction(action));
        item->setFlags(flags);
        actionsTable->setItem(i, COL_FOR, item);

        // Shortcut column
        item = new QTableWidgetItem(shortcutsToString(action->shortcuts()));
        item->setFlags(flags);
        actionsTable->setItem(i, COL_SHORTCUTS, item);

        // Set conflict column. Is expensive, runs over every row of table
        updateConflictCell(i, false, -1);
    } // for

    if (sortEnabled) {
        actionsTable->setSortingEnabled(true);
    }

    actionsTable->selectRow(0);
}

void TActionsEditor::applyChanges() {
    WZDEBUG("");

    for (int row = 0; row < actionsTable->rowCount(); row++) {
        QTableWidgetItem* item = actionsTable->item(row, COL_SHORTCUTS);
        if (item) {
            QString actionName = actionsTable->item(row, COL_ACTION)->text();
            QAction* action = findAction(actionName);
            if (action) {
                QString shortcuts = item->text();
                QString oldShortcuts = shortcutsToString(action->shortcuts());
                if (shortcuts != oldShortcuts) {
                    action->setShortcuts(stringToShortcutList(shortcuts));
                    action->setProperty("modified", true);
                    WZINFO(QString("Updated shortcut of action '%1'"
                                    " from '%2' to '%3'")
                           .arg(actionName).arg(oldShortcuts).arg(shortcuts));
                }
            }
        }
    }
}

void TActionsEditor::editShortcut() {

    int row = actionsTable->currentRow();
    QTableWidgetItem* item = actionsTable->item(row, COL_SHORTCUTS);
    if (item) {
        QString actionName = actionsTable->item(row, COL_ACTION)->text();
        QString actionOwner = actionsTable->item(row, COL_FOR)->text();
        TShortcutGetter shortcutGetter(this, actionName, actionOwner);
        QString result = shortcutGetter.exec(item->text());
        if (shortcutGetter.result() == QDialog::Accepted) {
            // Make sure TShortcutGetter and TActionsEditor speak the
            // same language
            result = shortcutsToString(stringToShortcutList(result));
            item->setText(result);
            updateConflicts(true, row);
        }
    }
}

int TActionsEditor::findActionNameInTable(const QString& name) {

    for (int row = 0; row < actionsTable->rowCount(); row++) {
        if (actionsTable->item(row, COL_ACTION)->text() == name)
            return row;
    }
    return -1;
}

int TActionsEditor::findShortcutsInTable(const QString& aShortCuts,
                                         const QString& actionOwner,
                                         int startRow,
                                         int skipRow) {

    QStringList find = shortcutsToStringList(aShortCuts);
    if (find.count() > 0) {
        for (int row = startRow; row < actionsTable->rowCount(); row++) {
            if (row != skipRow) {
                QTableWidgetItem* item = actionsTable->item(row, COL_SHORTCUTS);
                if (item) {
                    QString shortcuts = item->text();
                    if (!shortcuts.isEmpty()) {
                        QString rowOwner = actionsTable->item(row, COL_FOR)->text();
                        if (rowOwner == actionOwner) {
                            QStringList found = shortcutsToStringList(shortcuts);
                            for(int i = find.count() - 1; i >= 0; i--) {
                                const QString& findShortcut = find.at(i);
                                for(int j = found.count() - 1; j >= 0; j--) {
                                    if (found.at(j) == findShortcut) {
                                        WZTRACE(QString("Found row %1").arg(row));
                                        return row;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return -1;
}

void TActionsEditor::findShortcutActionAndLabel(const QString& shortcut,
                                                const QString& actionOwner,
                                                QString& actionName,
                                                QString& label) {
    WZDEBUG(QString("%1 %2").arg(shortcut).arg(actionOwner));

    if (shortcut.isEmpty()) {
        label = "\n";
        actionName = "";
        return;
    }

    int row = findShortcutsInTable(shortcut, actionOwner, 0, -1);
    if (row >= 0) {
        actionName = actionsTable->item(row, COL_ACTION)->text();
        label = tr("Shortcut currently assigned to:");
        do {
            label += QString("\n%1 (%2)")
                .arg(actionsTable->item(row, COL_DESC)->text())
                .arg(actionsTable->item(row, COL_ACTION)->text());
            row = findShortcutsInTable(shortcut, actionOwner, row + 1, -1);
        } while (row >= 0);
    } else {
        label = tr("Shortcut not assigned\n");
        actionName = "";
    }
}

void TActionsEditor::setConflictTextModified(int row) {

    QString tran = tr("m");
    QTableWidgetItem* item = actionsTable->item(row, COL_CONFLICTS);
    QString txt = item->text();;
    if (txt.isEmpty()) {
        item->setText(tran);
    } else if (txt != tran) {
        tran = tran + tr("i");
        if (tran != txt) {
            item->setText(tran);
        }
    }
}

void TActionsEditor::removeConflictingShortcuts(int row, int conflictRow) {

    QList<QKeySequence> remove = stringToShortcutList(
                actionsTable->item(row, COL_SHORTCUTS)->text());
    QTableWidgetItem* conflictItem = actionsTable->item(conflictRow,
                                                        COL_SHORTCUTS);
    QList<QKeySequence> from = stringToShortcutList(conflictItem->text());
    if (removeShortcutsFromList(remove, from,
            actionsTable->item(row, COL_ACTION)->text(),
            actionsTable->item(conflictRow, COL_ACTION)->text())) {

        // Update shortcuts conflictRow with the remaining shortcuts
        conflictItem->setText(shortcutsToString(from));
        // Update conflict text of conflictRow
        setConflictTextModified(conflictRow);
        // Update conflict text of row that took the shortcut
        setConflictTextModified(row);
    }
}

bool TActionsEditor::updateConflictCell(int row,
                                        bool takeShortcuts,
                                        int rowToKeep) {

    bool conflict = false;
    QTableWidgetItem* conflictItem = actionsTable->item(row, COL_SHORTCUTS);
    QString shortcuts = conflictItem->text();
    if (!shortcuts.isEmpty()) {
        QString actionOwner = actionsTable->item(row, COL_FOR)->text();
        int startRow = 0;
        do {
            int conflictRow = findShortcutsInTable(shortcuts, actionOwner,
                                                   startRow, row);
            if (conflictRow >= 0) {
                if (takeShortcuts) {
                    if (conflictRow == rowToKeep) {
                        removeConflictingShortcuts(conflictRow, row);
                    } else {
                        removeConflictingShortcuts(row, conflictRow);
                    }
                } else {
                    conflict = true;
                    QTableWidgetItem* item = actionsTable->item(
                                conflictRow, COL_CONFLICTS);
                    item->setIcon(iconProvider.conflictItem);
                }
                // Find next conflict
                startRow = conflictRow + 1;
                continue;
            }
        } while (false);
    }

    if (conflict) {
        conflictItem->setIcon(iconProvider.conflictItem);
    } else {
        conflictItem->setIcon(QPixmap());
    }

    return conflict;
}

bool TActionsEditor::updateConflicts(bool takeShortcuts, int rowToKeep) {

    bool conflict = false;

    for (int i = 0; i < actionsTable->rowCount(); i++) {
        if (updateConflictCell(i, takeShortcuts, rowToKeep)) {
            conflict = true;
        }
    }

    return conflict;
}

bool TActionsEditor::saveActionsTableAsFile(const QString& filename) {
    WZDEBUG("'" + filename + "'");

    QFile f(filename);
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");

        for (int row = 0; row < actionsTable->rowCount(); row++) {
            stream << actionsTable->item(row, COL_ACTION)->text() << "\t"
                   << actionsTable->item(row, COL_SHORTCUTS)->text() << "\n";
        }
        f.close();
        return true;
    }
    return false;
}

void TActionsEditor::saveActionsTable() {

    QString s = TFileDialog::getSaveFileName(
                    this, tr("Choose a filename"),
                    last_dir,
                    tr("Key files") +" (*.keys)");

    if (!s.isEmpty()) {
        // If filename has no extension, add it
        if (QFileInfo(s).suffix().isEmpty()) {
            s = s + ".keys";
        }
        if (QFileInfo(s).exists()) {
            int res = QMessageBox::question(this,
                    tr("Confirm overwrite"),
                    tr("The file %1 already exists.\n"
                       "Do you want to overwrite it?").arg(s),
                    QMessageBox::Yes,
                    QMessageBox::No,
                    Qt::NoButton);
            if (res == QMessageBox::No) {
                return;
            }
        }

        last_dir = QFileInfo(s).absolutePath();
        bool r = saveActionsTableAsFile(s);
        if (!r) {
            QMessageBox::warning(this, tr("Error"),
                tr("The file couldn't be saved"),
                QMessageBox::Ok, Qt::NoButton);
        }
    }
}

bool TActionsEditor::loadActionsTableFromFile(const QString& filename) {

    QRegExp rx("^([^\\t]*)\\t(.*)");

    QFile f(filename);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream stream(&f);
        stream.setCodec("UTF-8");
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (rx.indexIn(line) >= 0) {
                QString name = rx.cap(1).trimmed();
                int row = findActionNameInTable(name);
                if (row >= 0) {
                    QString shortcuts = shortcutsToString(
                                stringToShortcutList(rx.cap(2)));
                    actionsTable->item(row, COL_SHORTCUTS)->setText(shortcuts);
                } else {
                    WZWARN("Action '" + name + "' not found");
                }
            } else {
                WZDEBUG("Skipped line '" + line + "'");
            }
        }
        f.close();
        updateConflicts(false, -1); // Check for conflicts
        return true;
    } else {
        return false;
    }
}

void TActionsEditor::loadActionsTable() {

    QString s = TFileDialog::getOpenFileName(
                    this, tr("Choose a file"),
                    last_dir, tr("Key files") +" (*.keys)");

    if (!s.isEmpty()) {
        last_dir = QFileInfo(s).absolutePath();
        if (!loadActionsTableFromFile(s)) {
            QMessageBox::warning(this, tr("Error"),
                tr("The file couldn't be loaded"),
                QMessageBox::Ok, Qt::NoButton);
        }
    }
}


// Static functions

QString TActionsEditor::keySequnceToString(QKeySequence key) {
    return key.toString(QKeySequence::PortableText);
}

QKeySequence TActionsEditor::stringToKeySequence(QString s) {
    return QKeySequence(s, QKeySequence::PortableText);
}

QString TActionsEditor::shortcutsToString(const QList<QKeySequence>& shortcuts) {

    QString s = "";
    for (int i = 0; i < shortcuts.count(); i++) {
        s += keySequnceToString(shortcuts.at(i));
        if (i < shortcuts.count() - 1)
            s += ", ";
    }

    return s;
}

QStringList TActionsEditor::shortcutsToStringList(const QString& s) {
    return s.split(", ", QString::SkipEmptyParts);
}

QList<QKeySequence> TActionsEditor::stringToShortcutList(const QString& shortcuts) {

    QList<QKeySequence> shortcutList;
    QStringList stringList = shortcutsToStringList(shortcuts);
    for (int i = 0; i < stringList.count(); i++) {
        shortcutList.append(stringToKeySequence(stringList.at(i)));
    }

    return shortcutList;
}

QString TActionsEditor::cleanActionText(const QString& text,
                                        const QString& actionName) {

    // Actions modifying their text
    if (actionName == "play_or_pause") {
        return tr("Play or pause");
    }
    if (actionName == "aspect_detect") {
        return tr("Auto");
    }
    if (actionName == "aspect_none") {
        return tr("Disabled");
    }
    if (actionName == "size_optimize") {
        return tr("Optimize size");
    }
    if (actionName == "view_playlist") {
        return tr("Playlist");
    }

    QString s = text;
    s = s.replace("&", ""); // Remove ampersand
    s = s.replace("...", ""); // Remove ...
    s = s.replace("\t", " "); // Replace tabs used by aspectratio menu
    s = s.simplified();

    if (actionName.startsWith("seek_rewind")
            && actionName != "seek_rewind_frame") {
        return tr("Rewind %1").arg(s);
    }
    if (actionName.startsWith("seek_forward")
            && actionName != "seek_forward_frame") {
        return tr("Forward %1").arg(s);
    }

    return s;
}

QString TActionsEditor::actionToString(QAction *action) {

    QString s = shortcutsToString(action->shortcuts());
    QString actionName = action->objectName();

    // Check if iconText changed
    QString iconText = cleanActionText(action->iconText(), actionName);
    if (!iconText.isEmpty()
            && iconText != cleanActionText(action->text(), actionName)) {
        s += "\t" + action->iconText();
    }

    return s;
}

void TActionsEditor::saveSettings(QSettings* set) {
    WZTRACE("");

    set->beginGroup("actions");
    // Clear group to remove actions no longer modified
    set->remove("");

    for (int i = 0; i < TAction::allActions.count(); i++) {
        QAction* action = TAction::allActions.at(i);
        if (action->property("modified").toBool()
                || action->property("modifiedicontext").toBool()) {
            set->setValue(action->objectName(), actionToString(action));
        }
    }

    set->endGroup();
}

bool TActionsEditor::removeShortcutsFromList(
        const QList<QKeySequence>& remove,
        QList<QKeySequence>& from,
        const QString& toName,
        const QString& fromName) {

    bool removed = false;

    for(int i = 0; i < remove.size(); i++) {
        const QKeySequence& key = remove.at(i);
        if (from.removeAll(key)) {
            WZINFO(QString("Removed shortcut %1 from %2 to assign it to %3")
                   .arg(keySequnceToString(key))
                   .arg(fromName)
                   .arg(toName));
            removed = true;
        }
    }

    return removed;
}

void TActionsEditor::removeShortcutsFromActions(
        const QList<QKeySequence>& shortcuts,
        QAction* skipAction) {

    if (!shortcuts.isEmpty()) {
        for(int i = 0; i < TAction::allActions.size(); i++) {
            QAction* action = TAction::allActions.at(i);
            if (action->parent() == skipAction->parent()
                    && action != skipAction) {
                QList<QKeySequence> shortcutsToClean = action->shortcuts();
                if (removeShortcutsFromList(shortcuts,
                                            shortcutsToClean,
                                            skipAction->objectName(),
                                            action->objectName())) {
                    action->setShortcuts(shortcutsToClean);
                    action->setProperty("modified", true);
                }
            }
        }
    }
}

void TActionsEditor::setActionFromString(QAction* action, const QString& s) {

    static QRegExp rx("^([^\\t]*)(\\t(.*))?");

    QString actionName = action->objectName();
    if (s.isEmpty()) {
        QList<QKeySequence> shortcutList = action->shortcuts();
        if (shortcutList.count() > 0) {
            WZINFO(QString("Removing shortcuts %1 from %2")
                   .arg(shortcutsToString(shortcutList)).arg(actionName));
            shortcutList.clear();
            action->setShortcuts(shortcutList);
            action->setProperty("modified", true);
        }
    } else if (rx.indexIn(s) >= 0) {
        QString shortcuts = rx.cap(1);
        QString oldShortcuts = shortcutsToString(action->shortcuts());
        if (shortcuts != oldShortcuts) {
            QList<QKeySequence> shortcutList = stringToShortcutList(shortcuts);
            removeShortcutsFromActions(shortcutList, action);
            action->setShortcuts(shortcutList);
            action->setProperty("modified", true);
            WZINFO(QString("Changed shortcuts %1 from '%2' to '%3'")
                   .arg(actionName).arg(oldShortcuts)
                   .arg(shortcutsToString(shortcutList)));
        }

        QString iconText = rx.cap(3).trimmed();
        if (!iconText.isEmpty()) {
            WZINFO(QString("Updating icon text %1 from '%2' to '%3'")
                   .arg(actionName).arg(action->iconText()).arg(iconText));
            action->setIconText(iconText);
            action->setProperty("modifiedicontext", true);
        }
    }
}

void TActionsEditor::loadSettings(QSettings* pref) {

    pref->beginGroup("actions");
    QStringList actions = pref->childKeys();
    WZTRACE(QString("Loading %1 actions").arg(actions.count()));

    for (int i = 0; i < actions.count(); i++) {
        const QString& name = actions.at(i);
        QAction* action = findAction(name);
        if (action) {
            setActionFromString(action, pref->value(name, "").toString());
        } else {
            WZWARN("Action '" + name + "' not found");
        }
    }

    pref->endGroup();
}

} // namespace Action
} // namespace Gui

#include "moc_actionseditor.cpp"

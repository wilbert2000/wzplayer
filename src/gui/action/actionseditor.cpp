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
#include "gui/filedialog.h"
#include "settings/paths.h"
#include "images.h"
#include "log4qt/logger.h"

#include <QDebug>
#include <QTableWidget>
#include <QHeaderView>

#include <QString>
#include <QAction>
#include <QLayout>
#include <QScrollBar>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegExp>
#include <QApplication>
#include <QResizeEvent>


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


const int WIDTH_CONFLICT_ICON = 16;
const int MARGINS = 2;

TActionsEditor::TActionsEditor(QWidget* parent) :
    QWidget(parent),
    debug(logger()) {

    last_dir = Settings::TPaths::shortcutsPath();

    actionsTable = new QTableWidget(0, COL_COUNT, this);
    actionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    actionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    actionsTable->verticalHeader()->hide();

    QHeaderView* h = actionsTable->horizontalHeader();
    h->setHighlightSections(false);
    h->setMinimumSectionSize(22);
    actionsTable->setHorizontalHeaderLabels(QStringList()
        << "" << tr("Action") << tr("Description") << tr("Shortcuts"));

    h->setSectionResizeMode(COL_CONFLICTS, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_ACTION, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_DESC, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_SHORTCUT, QHeaderView::Stretch);
    h->setStretchLastSection(true);

    actionsTable->setIconSize(QSize(22, 22));

    connect(actionsTable, &QTableWidget::itemActivated,
            this, &TActionsEditor::editShortcut);

    saveButton = new QPushButton(this);
    saveButton->setText(tr("&Save"));
    saveButton->setIcon(Images::icon("save"));

    loadButton = new QPushButton(this);
    loadButton->setText(tr("&Load"));
    loadButton->setIcon(Images::icon("open"));

    connect(saveButton, &QPushButton::clicked,
            this, &TActionsEditor::saveActionsTable);
    connect(loadButton, &QPushButton::clicked,
            this, &TActionsEditor::loadActionsTable);

    editButton = new QPushButton(this);
    editButton->setText(tr("&Change shortcut..."));
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

TActionsEditor::~TActionsEditor() {
}

void TActionsEditor::setActionsTable(const QList<QAction*>& allActions) {

    bool sortEnabled = actionsTable->isSortingEnabled();
    actionsTable->setSortingEnabled(false);
    actionsTable->setRowCount(allActions.count());
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    for (int i = 0; i < allActions.count(); i++) {
        QAction* action = allActions.at(i);
        WZTRACE(QString("Adding action '%1' '%2'")
                .arg(action->objectName()).arg(action->text()));

        // Conflict column
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setFlags(flags);
        bool mod = action->property("modified").toBool();
        item->setText(mod ? tr("m") : "");
        // Set conflict icon. Is expensive, runs over every row of table
        updateConflict(item, i);
        actionsTable->setItem(i, COL_CONFLICTS, item);

        // Action column
        item = new QTableWidgetItem(action->objectName());
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

        // Shortcut column
        item = new QTableWidgetItem(shortcutsToString(action->shortcuts()));
        item->setFlags(flags);
        actionsTable->setItem(i, COL_SHORTCUT, item);
    } // for

    if (sortEnabled) {
        actionsTable->setSortingEnabled(true);
    }

    actionsTable->selectRow(0);
}

void TActionsEditor::applyChanges(const QList<QAction*>& allActions) {
    WZDEBUG("");

    for (int row = 0; row < actionsTable->rowCount(); row++) {
        QTableWidgetItem* item = actionsTable->item(row, COL_SHORTCUT);
        if (item) {
            QString actionName = actionsTable->item(row, COL_ACTION)->text();
            QAction* action = findAction(allActions, actionName);
            if (action) {
                QString shortcuts = item->text();
                QString old_shortcuts = shortcutsToString(action->shortcuts());
                if (shortcuts != old_shortcuts) {
                    action->setShortcuts(stringToShortcuts(shortcuts));
                    action->setProperty("modified", true);
                    WZDEBUG(QString("Updated shortcut action '%1'"
                                    " from %2 to %3")
                            .arg(actionName).arg(old_shortcuts).arg(shortcuts));
                }
            }
        }
    }
}

void TActionsEditor::editShortcut() {

    QTableWidgetItem* i = actionsTable->item(actionsTable->currentRow(),
                                             COL_SHORTCUT);
    if (i) {
        TShortcutGetter d(this);
        QString result = d.exec(i->text());
        if (d.result() == QDialog::Accepted) {
            // Just to make sure shortcutgetter and actionseditor
            // speak the same language...
            result = shortcutsToString(stringToShortcuts(result));
            i->setText(result);
            if (updateConflicts())
                qApp->beep();
        }
    }
}

int TActionsEditor::findActionName(const QString& name) {

    for (int row = 0; row < actionsTable->rowCount(); row++) {
        if (actionsTable->item(row, COL_ACTION)->text() == name)
            return row;
    }
    return -1;
}

bool TActionsEditor::containsShortcut(const QString& accel, const QString& shortcut) {

    QStringList shortcut_list = accel.split(", ");
    QString s;
    foreach(s, shortcut_list) {
        s = s.trimmed();
        if (s == shortcut)
            return true;
    }
    return false;
}

int TActionsEditor::findShortcuts(const QString& accel, int ignoreRow) {

    QStringList shortcutList = accel.split(", ");
    for(int i = 0; i < shortcutList.count(); i++) {
        shortcutList[i] = shortcutList[i].trimmed();
    }

    for (int row = 0; row < actionsTable->rowCount(); row++) {
        if (row != ignoreRow) {
            QTableWidgetItem* item = actionsTable->item(row, COL_SHORTCUT);
            if (item) {
                QString txt = item->text();
                if (!txt.isEmpty()) {
                    foreach(const QString& shortcut, shortcutList) {
                        if (containsShortcut(item->text(), shortcut)) {
                            return row;
                        }
                    }
                }
            }
        }
    }

    return -1;
}

QString TActionsEditor::findShortcutsAction(const QString& shortcuts) {

    if (shortcuts.isEmpty()) {
        return "\n";
    }

    int row = findShortcuts(shortcuts, -1);
    if (row >= 0) {
        return tr("Shortcut currently assigned to:\n%1 (%2)")
                .arg(actionsTable->item(row, COL_DESC)->text())
                .arg(actionsTable->item(row, COL_ACTION)->text());
    }

    return tr("Shortcut not assigned\n");
}

bool TActionsEditor::updateConflict(QTableWidgetItem* item, int ignoreRow) {

    bool conflict;

    QString shortcuts = item->text();
    if (shortcuts.isEmpty() || findShortcuts(shortcuts, ignoreRow) < 0) {
        conflict = false;
        item->setIcon(QPixmap());
    } else {
        conflict = true;
        item->setIcon(Images::icon("conflict"));
    }

    return conflict;
}

bool TActionsEditor::updateConflicts() {

    bool conflict = false;

    for (int i = 0; i < actionsTable->rowCount(); i++) {
        QTableWidgetItem* item = actionsTable->item(i, COL_SHORTCUT);
        if (updateConflict(item, i)) {
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
                   << actionsTable->item(row, COL_SHORTCUT)->text() << "\n";
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
                    tr("Confirm overwrite?"),
                    tr("The file %1 already exists.\n"
                       "Do you want to overwrite?").arg(s),
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
                int row = findActionName(name);
                if (row >= 0) {
                    QString shortcuts = shortcutsToString(stringToShortcuts(rx.cap(2)));
                    actionsTable->item(row, COL_SHORTCUT)->setText(shortcuts);
                } else {
                    WZWARN("action '" + name + "' not found");
                }
            } else {
                WZDEBUG("skipped line '" + line + "'");
            }
        }
        f.close();
        updateConflicts(); // Check for conflicts
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

QString TActionsEditor::keyToString(QKeySequence key) {
    return key.toString(QKeySequence::PortableText);
}

QKeySequence TActionsEditor::stringToKey(QString s) {

    s = s.simplified();

    // Qt 4.3 and 4.4 (at least on linux) seems to have a problem when
    // using Simplified-Chinese. QKeysequence deletes the arrow key names
    // from the shortcut, so this is a work-around.
    s.replace(QString::fromUtf8("左"), "Left");
    s.replace(QString::fromUtf8("下"), "Down");
    s.replace(QString::fromUtf8("右"), "Right");
    s.replace(QString::fromUtf8("上"), "Up");

    return QKeySequence(s, QKeySequence::PortableText);
}

QString TActionsEditor::shortcutsToString(const TShortCutList& shortcuts) {

    QString s = "";
    for (int n = 0; n < shortcuts.count(); n++) {
        s += keyToString(shortcuts.at(n));
        if (n < shortcuts.count() - 1)
            s += ", ";
    }

    return s;
}

TShortCutList TActionsEditor::stringToShortcuts(const QString& shortcuts) {

    TShortCutList shortcut_list;
    QStringList l = shortcuts.split(", ", QString::SkipEmptyParts);

    for (int n = 0; n < l.count(); n++) {
        shortcut_list.append(stringToKey(l.at(n)));
    }

    return shortcut_list;
}

QString TActionsEditor::cleanActionText(const QString& text,
                                                const QString& action_name) {

    // Actions modifying the text property themselves
    if (action_name == "play_or_pause") {
        return tr("Play or pause");
    }
    if (action_name == "aspect_detect") {
        return tr("Auto");
    }
    if (action_name == "aspect_none") {
        return tr("Disabled");
    }
    if (action_name == "video_size") {
        return tr("Optimize size");
    }
    if (action_name == "view_playlist") {
        return tr("Playlist");
    }

    QString s = text;
    s = s.replace("&", "");
    s = s.replace("...", "");
    s = s.replace("\t", " "); // Aspect menu uses tabs
    return s;
}

QString TActionsEditor::actionToString(QAction *action) {

    QString s = shortcutsToString(action->shortcuts());
    QString actionName = action->objectName();
    QString iconText = cleanActionText(action->iconText(), actionName);
    if (!iconText.isEmpty()) {
        if (iconText != cleanActionText(action->text(), actionName)) {
            s += "\t" + action->iconText();
        }
    }

    return s;
}

void TActionsEditor::saveSettings(QSettings* set,
                                  const QList<QAction*>& allActions) {
    Log4Qt::Logger::logger("Gui::Action::TActionsEditor")
            ->debug("saveToConfig");

    set->beginGroup("actions");
    // Clear group to remove actions no longer modified
    set->remove("");

    for (int i = 0; i < allActions.count(); i++) {
        QAction* action = allActions.at(i);
        if (action->property("modified").toBool()) {
            set->setValue(action->objectName(), actionToString(action));
        }
    }

    set->endGroup();
}

void TActionsEditor::removeShortcuts(const TActionList& actions,
                                     const TShortCutList& shortcuts,
                                     QAction* skip_action) {

    if (shortcuts.isEmpty()) {
        TShortCutList remove = skip_action->shortcuts();
        if (!remove.isEmpty()) {
            qDebug() << "Gui::Action::TActionsEditor::removeShortcuts removing"
                        " shortcuts" << remove << "from"
                     << skip_action->objectName();
        }
        return;
    }

    for(int i = 0; i < actions.size(); i++) {
        QAction* action = actions.at(i);
        if (action != skip_action) {
            TShortCutList l = action->shortcuts();
            bool removed = false;
            for(int n = 0; n < shortcuts.size(); n++) {
                if (l.removeOne(shortcuts.at(n))) {
                    qDebug() << "Gui::Action::TActionsEditor::removeShortcuts"
                                " removing shortcut" << shortcuts.at(n)
                             << "from" << action->objectName()
                             << "to assign it to" << skip_action->objectName();
                    removed = true;
                }
            }
            if (removed) {
                action->setShortcuts(l);
            }
        }
    }
}

void TActionsEditor::setActionFromString(QAction* action,
                                         const QString& s,
                                         const TActionList& actions) {

    static QRegExp rx("^([^\\t]*)(\\t(.*))?");

    if (rx.indexIn(s) >= 0) {
        QString shortcuts = rx.cap(1);
        QString old_shortcuts = shortcutsToString(action->shortcuts());
        if (shortcuts != old_shortcuts) {
            TShortCutList l = stringToShortcuts(shortcuts);
            removeShortcuts(actions, l, action);
            action->setShortcuts(l);
            action->setProperty("modified", true);
        }

        QString icon_text = rx.cap(3).trimmed();
        if (!icon_text.isEmpty()) {
            action->setIconText(icon_text);
            action->setProperty("modified", true);
        }
    }
}

QAction* TActionsEditor::findAction(const TActionList& actions,
                                    const QString& name) {

    for (int n = 0; n < actions.count(); n++) {
        QAction* action = actions.at(n);
        if (name == action->objectName())
            return action;
    }

    return 0;
}

void TActionsEditor::loadSettings(QSettings* pref,
                                    const QList<QAction*>& allActions) {

    Log4Qt::Logger* l = Log4Qt::Logger::logger("Gui::Action::TActionsEditor");
    l->debug("loadSettings");

    pref->beginGroup("actions");
    QStringList actions = pref->childKeys();

    for (int i = 0; i < actions.count(); i++) {
        QString name = actions.at(i);
        QAction* action = findAction(allActions, name);
        if (action) {
            setActionFromString(action, pref->value(name, "").toString(),
                                allActions);
        } else {
            l->error("loadSettings action '" + name + "' not found");
        }
    }

    pref->endGroup();
}

} // namespace Action
} // namespace Gui

#include "moc_actionseditor.cpp"

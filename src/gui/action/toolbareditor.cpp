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

#include "gui/action/toolbareditor.h"

#include <QDebug>
#include <QToolBar>
#include <QToolButton>
#include <QScrollBar>
#include <QMatrix>

#include "log4qt/logger.h"
#include "gui/action/actionlist.h"
#include "gui/action/actionseditor.h"
#include "images.h"

namespace Gui {
namespace Action {


enum TCols {
    COL_NS = 0,
    COL_FS = 1,
    COL_ICON = 2,
    COL_NAME = 3,
    COL_DESC = 4,
    COL_COUNT = 5
};

TToolbarEditor::TToolbarEditor(QWidget* parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    debug(logger()) {

    setupUi(this);

    up_button->setIcon(Images::icon("up"));
    down_button->setIcon(Images::icon("down"));

    QMatrix matrix;
    matrix.rotate(90);

    right_button->setIcon(Images::icon("up").transformed(matrix));
    left_button->setIcon(Images::icon("down").transformed(matrix));
    connect(up_button, SIGNAL(clicked()),
            this, SLOT(onUpButtonClicked()));
    connect(down_button, SIGNAL(clicked()),
            this, SLOT(onDownButtonClicked()));
    connect(left_button, SIGNAL(clicked()),
            this, SLOT(onLeftButtonClicked()));
    connect(right_button, SIGNAL(clicked()),
            this, SLOT(onRightButtonClicked()));

    connect(separator_button, SIGNAL(clicked()),
            this, SLOT(onSeperatorButtonClicked()));

    QPushButton* restore = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    connect(restore, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

    connect(all_actions_list, SIGNAL(currentRowChanged(int)),
            this, SLOT(checkRowsAllList(int)));
    connect(active_actions_table, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(onCurrentCellChanged(int, int, int, int)));

    all_actions_list->setSelectionMode(QAbstractItemView::SingleSelection);
    all_actions_list->setDragEnabled(true);
    all_actions_list->viewport()->setAcceptDrops(true);
    all_actions_list->setDropIndicatorShown(true);
    all_actions_list->setDefaultDropAction(Qt::MoveAction); // Qt 4.6
    //all_actions_list->setDragDropMode(QAbstractItemView::InternalMove);

    // TODO:
    //active_actions_list->setDragEnabled(true);
    //active_actions_list->viewport()->setAcceptDrops(true);
    //active_actions_list->setDropIndicatorShown(true);
    //active_actions_list->setDefaultDropAction(Qt::MoveAction); // Qt 4.6
    // //active_actions_list->setDragDropMode(QAbstractItemView::InternalMove);

    active_actions_table->setColumnCount(COL_COUNT);

#if QT_VERSION_MAJOR >= 5
    active_actions_table->horizontalHeader()->setSectionResizeMode(COL_NS, QHeaderView::Fixed);
    active_actions_table->horizontalHeader()->setSectionResizeMode(COL_FS, QHeaderView::Fixed);
    active_actions_table->horizontalHeader()->setSectionResizeMode(COL_ICON, QHeaderView::Fixed);
#else
    active_actions_table->horizontalHeader()->setResizeMode(COL_NS, QHeaderView::Fixed);
    active_actions_table->horizontalHeader()->setResizeMode(COL_FS, QHeaderView::Fixed);
    active_actions_table->horizontalHeader()->setResizeMode(COL_ICON, QHeaderView::Fixed);
#endif

    active_actions_table->horizontalHeader()->setStretchLastSection(true);
    retranslateStrings();
}

void TToolbarEditor::retranslateStrings() {

    retranslateUi(this);

    QTableWidgetItem* header = new QTableWidgetItem(
        QIcon(Images::icon("normalscreen", 24)), "");
    header->setToolTip(tr("Show button in normal window"));
    active_actions_table->setHorizontalHeaderItem(COL_NS, header);

    header = new QTableWidgetItem(QIcon(Images::icon("fullscreen", 24)), "");
    header->setToolTip(tr("Show button in full screen"));
    active_actions_table->setHorizontalHeaderItem(COL_FS, header);

    header = new QTableWidgetItem("");
    header->setToolTip(tr("The icon to put on the button"));
    active_actions_table->setHorizontalHeaderItem(COL_ICON, header);

    header = new QTableWidgetItem(tr("Action"));
    header->setToolTip(tr("The action to execute when the button is clicked"));
    active_actions_table->setHorizontalHeaderItem(COL_NAME, header);

    header = new QTableWidgetItem(tr("Text"));
    header->setToolTip(tr("The text to use as hint and when there is no icon"));
    active_actions_table->setHorizontalHeaderItem(COL_DESC, header);
}

TToolbarEditor::~TToolbarEditor() {
}

void TToolbarEditor::setIconSize(int size) {
    iconsize_spin->setValue(size);
}

int TToolbarEditor::iconSize() const {
    return iconsize_spin->value();
}

void TToolbarEditor::populateList(QListWidget* w, const TActionList& actions_list) {

    w->clear();

    for (int n = 0; n < actions_list.count(); n++) {
        QAction* action = actions_list[n];
        if (action && !action->objectName().isEmpty() && !action->isSeparator()) {
            QListWidgetItem* i = new QListWidgetItem;
            QString text = TActionsEditor::actionTextToDescription(action->text(), action->objectName());
            i->setText(text + " ("+ action->objectName() +")");
            QIcon icon = action->icon();
            if (icon.isNull()) {
                icon = Images::icon("empty_icon");
            }
            i->setIcon(icon);
            i->setData(Qt::UserRole, action->objectName());
            w->addItem(i);
        }
    }
}

void TToolbarEditor::setAllActions(const TActionList& actions_list) {

    all_actions = &actions_list;
    populateList(all_actions_list, actions_list);
}

void TToolbarEditor::insertRowFromAction(int row, QAction* action, bool ns, bool fs) {

    Qt::ItemFlags item_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    active_actions_table->insertRow(row);

    // Icon
    QIcon icon = action->icon();
    if (icon.isNull()) {
        icon = Images::icon("empty_icon");
    }
    QTableWidgetItem* cell = new QTableWidgetItem(icon, "");
    cell->setFlags(item_flags);
    active_actions_table->setItem(row, COL_ICON, cell);

    // Name
    cell = new QTableWidgetItem(action->objectName());
    cell->setFlags(item_flags);
    active_actions_table->setItem(row, COL_NAME, cell);

    // Description
    QString text = TActionsEditor::actionTextToDescription(
        action->iconText(), action->objectName());
    cell = new QTableWidgetItem(text);
    if (action->isSeparator() || action->inherits("QWidgetAction")) {
        cell->setFlags(item_flags);
    } else {
        cell->setFlags(item_flags | Qt::ItemIsEditable);
    }
    active_actions_table->setItem(row, COL_DESC, cell);

    // Normal screen
    cell = new QTableWidgetItem();
    cell->setFlags(item_flags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    cell->setCheckState(ns ? Qt::Checked : Qt::Unchecked);
    active_actions_table->setItem(row, COL_NS, cell);

    // Full screen
    cell = new QTableWidgetItem();
    cell->setFlags(item_flags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    cell->setCheckState(fs ? Qt::Checked : Qt::Unchecked);
    active_actions_table->setItem(row, COL_FS, cell);
}

QAction* TToolbarEditor::newSeparator(QWidget* parent) {

    static int sep_number = 0;

    QAction* action = new QAction(parent);
    action->setSeparator(true);
    sep_number++;
    action->setObjectName("separator" + QString::number(sep_number));
    return action;
}

void TToolbarEditor::insertSeparator(int row, bool ns, bool fs) {

    QAction* action = newSeparator(0);
    insertRowFromAction(row, action, ns, fs);
    delete action;
}


void TToolbarEditor::stringToAction(const QString& s, QString& action_name, bool& ns, bool&fs) {

    // name|0|1
    static QRegExp rx("^([^|]+)(\\|(\\d+)\\|(\\d+))?");

    if (rx.indexIn(s) >= 0) {
        action_name = rx.cap(1);
        ns = rx.cap(3).trimmed() != "0";
        fs = rx.cap(4).trimmed() != "0";
    } else {
        action_name = "";
    }
}

void TToolbarEditor::setActiveActions(const QStringList& actions) {

    active_actions_table->clearContents();

    QString action_name;
    bool ns, fs;
    int row = 0;
    for(int i = 0; i < actions.count(); i++) {
        stringToAction(actions[i], action_name, ns, fs);
        if (action_name == "separator") {
            insertSeparator(row, ns, fs);
            row++;
        } else {
            QAction* action = findAction(action_name, *all_actions);
            if (action) {
                insertRowFromAction(row, action, ns, fs);
                row++;
            }
        }
    }

    if (active_actions_table->rowCount() > 0) {
        active_actions_table->setCurrentCell(0, COL_DESC);
    }
}

void TToolbarEditor::resizeColumns() {

    active_actions_table->resizeColumnsToContents();

    int w = active_actions_table->width()
            - 2 * active_actions_table->columnWidth(COL_NS)
            - active_actions_table->columnWidth(COL_ICON)
            - 2 * active_actions_table->columnCount();

    if (active_actions_table->verticalScrollBar()->isVisible()) {
        w -= active_actions_table->verticalScrollBar()->width();
    }

    active_actions_table->setColumnWidth(COL_NAME, w / 2);
    active_actions_table->resizeRowsToContents();
}

void TToolbarEditor::resizeEvent(QResizeEvent* event) {

    QDialog::resizeEvent(event);
    resizeColumns();
}

void TToolbarEditor::setCurrentRow(int row) {

    int col = active_actions_table->currentColumn();
    if (col < 0) {
        col = COL_DESC;
    }

    int row_count = active_actions_table->rowCount();
    if (row < 0) {
        if (row_count > 0) {
            row = 0;
        }
    } else if (row >= row_count) {
        row = row_count - 1;
    }

    if (row >= 0) {
        active_actions_table->setCurrentCell(row, col);
    }
}

void TToolbarEditor::swapRows(int row1, int row2) {

    for(int col = 0; col < active_actions_table->columnCount(); col++) {
        QTableWidgetItem* cell1 = active_actions_table->takeItem(row1, col);
        QTableWidgetItem* cell2 = active_actions_table->takeItem(row2, col);
        active_actions_table->setItem(row1, col, cell2);
        active_actions_table->setItem(row2, col, cell1);
    }
}

void TToolbarEditor::onUpButtonClicked() {

    int row = active_actions_table->currentRow();
    if (row > 0) {
        swapRows(row - 1, row);
        setCurrentRow(row - 1);
    }
}

void TToolbarEditor::onDownButtonClicked() {

    int row = active_actions_table->currentRow();
    if (row >= 0 && row < active_actions_table->rowCount() - 1) {
        swapRows(row, row + 1);
        setCurrentRow(row + 1);
    }
}

void TToolbarEditor::onRightButtonClicked() {

    int row = all_actions_list->currentRow();
    if (row >= 0) {
        int dest_row = active_actions_table->currentRow();
        if (dest_row < 0)
            dest_row = 0;
        QListWidgetItem* item = all_actions_list->item(row);
        if (item) {
            QString action_name = item->data(Qt::UserRole).toString();
            QAction* action = findAction(action_name, *all_actions);
            if (action) {
                insertRowFromAction(dest_row, action, true, true);
                active_actions_table->setCurrentCell(dest_row, COL_DESC);
            }
        }
    }
}

void TToolbarEditor::onLeftButtonClicked() {

    int row = active_actions_table->currentRow();
    if (row >= 0) {
        active_actions_table->removeRow(row);
        setCurrentRow(row);
    }
}

void TToolbarEditor::onSeperatorButtonClicked() {

    int row = active_actions_table->currentRow();
    if (row < 0)
        row = 0;
    insertSeparator(row, true, true);
}

void TToolbarEditor::restoreDefaults() {

    setActiveActions(default_actions);
}

bool TToolbarEditor::getVis(int row, int col) {

    QTableWidgetItem* item = active_actions_table->item(row, col);
    if (item)
        return item->checkState() != Qt::Unchecked;
    return true;
}

QStringList TToolbarEditor::saveActions() {
    WZDEBUG("");

    QStringList list;

    for (int row = 0; row < active_actions_table->rowCount(); row++) {
        QTableWidgetItem* item = active_actions_table->item(row, COL_NAME);
        if (item) {
            QString action_name = item->text();
            if (action_name.startsWith("separator")) {
                action_name = "separator";
            }
            QString s = action_name;

            bool ns = getVis(row, COL_NS);
            bool fs = getVis(row, COL_FS);
            if (ns) {
                if (!fs) {
                    s += "|1|0";
                }
            } else if (fs) {
                s += "|0|1";
            } else {
                s += "|0|0";
            }
            list << s;

            if (action_name != "separator") {
                // Update icon text
                QAction* action = findAction(action_name, *all_actions);
                if (action) {
                    item = active_actions_table->item(row, COL_DESC);
                    if (item) {
                        QString action_icon_text = TActionsEditor::actionTextToDescription(
                                                       item->text(), action_name).trimmed();
                        if (!action_icon_text.isEmpty()) {
                            QString action_text = TActionsEditor::actionTextToDescription(action->text(), action_name);
                            if (action_text != action_icon_text) {
                                action->setIconText(action_icon_text);
                                action->setProperty("modified", true);
                                WZDEBUG("updated icon text '" + action_name
                                        + "' to '" + action_icon_text + "'");
                            } else {
                                action_icon_text = TActionsEditor::actionTextToDescription(action->iconText(), action_name);
                                if (action_icon_text != action_text) {
                                    action->setIconText(action_text);
                                    WZDEBUG("cleared icon text " + action_name);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return list;
}

void TToolbarEditor::checkRowsAllList(int currentRow) {
    right_button->setEnabled(currentRow > -1);
}

void TToolbarEditor::onCurrentCellChanged(int currentRow, int currentColumn,
                          int previousRow, int previousColumn) {
    Q_UNUSED(currentColumn)
    Q_UNUSED(previousRow)
    Q_UNUSED(previousColumn)

    left_button->setEnabled(currentRow >= 0);
    if (currentRow < 0) {
        up_button->setEnabled(false);
        down_button->setEnabled(false);
    } else {
        up_button->setEnabled((currentRow > 0));
        down_button->setEnabled(currentRow < active_actions_table->rowCount() - 1);
    }
}

QAction* TToolbarEditor::findAction(const QString& action_name, const TActionList& actions_list) {

    for (int n = 0; n < actions_list.count(); n++) {
        QAction* action = actions_list[n];
        if (action->objectName() == action_name)
            return action;
    }

    return 0;
}

void TToolbarEditor::changeEvent(QEvent* e) {

    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QDialog::changeEvent(e);
    }
}

} // namespace Action
} // namespace Gui

#include "moc_toolbareditor.cpp"

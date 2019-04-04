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
#include "gui/action/actionseditor.h"
#include "gui/action/actionitem.h"
#include "gui/action/action.h"
#include "images.h"
#include "config.h"

#include <QToolBar>
#include <QToolButton>
#include <QMatrix>
#include <QTimer>
#include <QMessageBox>


namespace Gui {
namespace Action {

TToolbarEditor::TToolbarEditor(QWidget* parent,
                               const QString& tbarName,
                               bool mainToolbar) :
    QDialog(parent, TConfig::DIALOG_FLAGS),
    toolbarName(tbarName.toLower()),
    isMainToolbar(mainToolbar) {

    setupUi(this);
    retranslateUi(this);
    setWindowTitle(tr("Edit %1").arg(toolbarName));

    all_actions_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    all_actions_list->setDragEnabled(true);
    all_actions_list->setAcceptDrops(false);
    all_actions_list->viewport()->setAcceptDrops(false);
    all_actions_list->setDragDropMode(QAbstractItemView::DragOnly);

    active_actions_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    active_actions_list->setDragEnabled(true);
    active_actions_list->setAcceptDrops(true);
    active_actions_list->viewport()->setAcceptDrops(true);
    active_actions_list->setDragDropMode(QAbstractItemView::DragDrop);
    active_actions_list->setDefaultDropAction(Qt::MoveAction);
    active_actions_list->setDropIndicatorShown(true);

    up_button->setIcon(Images::icon("up"));
    down_button->setIcon(Images::icon("down"));
    QMatrix matrix;
    matrix.rotate(90);
    right_button->setIcon(Images::icon("up").transformed(matrix));
    left_button->setIcon(Images::icon("down").transformed(matrix));

    connect(up_button, &QToolButton::clicked,
            this, &TToolbarEditor::onUpButtonClicked);
    connect(down_button, &QToolButton::clicked,
            this, &TToolbarEditor::onDownButtonClicked);
    connect(left_button, &QToolButton::clicked,
            this, &TToolbarEditor::onLeftButtonClicked);
    connect(right_button, &QToolButton::clicked,
            this, &TToolbarEditor::onRightButtonClicked);

    connect(separator_button, &QToolButton::clicked,
            this, &TToolbarEditor::onSeperatorButtonClicked);

    QPushButton* restore = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    connect(restore, &QPushButton::clicked,
            this, &TToolbarEditor::restoreDefaults);

    connect(active_actions_list, &QListWidget::itemSelectionChanged,
            this, &TToolbarEditor::onActiveActionsItemSelectionChanged);

    separatorAction = new QAction(tr("Separator"), this);
    separatorAction->setObjectName("separator");
    separatorAction->setSeparator(true);

    QStyleOptionButton styleOption;
    QRect r = QApplication::style()->subElementRect(
                QStyle::SE_CheckBoxClickRect, &styleOption);
    checkBoxWidth = r.width() + 2 * 6; // TODO: get 6 from style
}

void TToolbarEditor::setIconSize(int size) {
    iconsize_spin->setValue(size);
}

int TToolbarEditor::iconSize() const {
    return iconsize_spin->value();
}

void TToolbarEditor::setAllActions(const QList<QAction*>& actions) {

    // Add actions to QListWidget all_actions_list
    for (int i = 0; i < actions.count(); i++) {
        all_actions_list->addItem(new TActionItem(actions.at(i)));
    }

    all_actions_list->setCurrentRow(0);
}

bool TToolbarEditor::isActionEditable(QAction* action) {

    if (!action || action->isSeparator() || action->inherits("QWidgetAction")) {
        return false;
    }

    // No use trying to edit items that won't change
    QString iconText = action->iconText();
    if (iconText.isEmpty()) {
        iconText = action->text();
    }
    QString name = action->objectName();
    if (TActionsEditor::cleanActionText(iconText, name)
            == TActionsEditor::cleanActionText("w", name)) {
        return false;
    }
    return true;
}

TActionItem* TToolbarEditor::createActiveActionItem(QAction* action,
                                                    bool ns, bool fs) {

    TActionItem* i = new TActionItem(action);
    Qt::ItemFlags flags = i->flags();
    if (isMainToolbar) {
        flags = flags | Qt::ItemIsUserTristate;
    }
    if (isActionEditable(action)) {
        flags = flags | Qt::ItemIsEditable;
    }
    i->setFlags(flags);

    if (isMainToolbar) {
        Qt::CheckState state;
        if (ns) {
            if (fs) {
                state = Qt::Checked;
            } else {
                state = Qt::Unchecked;
            }
        } else {
            state = Qt::PartiallyChecked;
        }
        i->setCheckState(state);
    }

    i->setToolTip(getToolTipForItem(i));
    return i;
}

QString TToolbarEditor::getToolTipForItem(QListWidgetItem* item) {

    QString tip;
    if (isMainToolbar) {
        switch (item->checkState()) {
            case Qt::Unchecked:
                tip = tr("Display on normal screen and not on full screen. ");
                break;
            case Qt::PartiallyChecked:
                tip = tr("Display on full screen and not on normal screen. ");
                break;
            case Qt::Checked:
                tip = tr("Display on normal and full screen. ");
        }
    }

    if (item->flags() & Qt::ItemIsEditable) {
        tip += tr("Double click to edit text.");
    } else {
        tip += tr("Text is not editable.");
    }
    return tip;
}

void TToolbarEditor::onItemClicked(QListWidgetItem* item) {

    if (isMainToolbar) {
        // Convert cursor to viewport coords
        QPoint cursorPos = active_actions_list->viewport()->mapFromGlobal(
                    QCursor::pos());

        if (cursorPos.x() < checkBoxWidth) {
            Qt::CheckState state = item->checkState();
            if (state == Qt::Unchecked) {
                state = Qt::Checked;
            } else if (state == Qt::Checked) {
                state = Qt::PartiallyChecked;
            } else {
                state = Qt::Unchecked;
            }
            item->setCheckState(state);
            item->setToolTip(getToolTipForItem(item));
        }
    }
}

void TToolbarEditor::onItemDoubleClicked(QListWidgetItem *item) {

    if (isMainToolbar) {
        // Convert cursor to viewport coords
        QPoint cursorPos = active_actions_list->viewport()->mapFromGlobal(
                    QCursor::pos());

        if (cursorPos.x() < checkBoxWidth) {
            // Undo the first click
            Qt::CheckState state = item->checkState();
            if (state == Qt::Unchecked) {
                state = Qt::PartiallyChecked;
            } else if (state == Qt::PartiallyChecked) {
                state = Qt::Checked;
            } else {
                state = Qt::Unchecked;
            }
            item->setCheckState(state);
        }
    }
}

void TToolbarEditor::stringToAction(const QString& s,
                                    QString& actionName,
                                    bool& ns,
                                    bool& fs) {

    // name|0|1
    static QRegExp rx("^([^|]+)(\\|(\\d+)\\|(\\d+))?");

    if (rx.indexIn(s) >= 0) {
        actionName = rx.cap(1);
        ns = rx.cap(3).trimmed() != "0";
        fs = rx.cap(4).trimmed() != "0";
    } else {
        actionName = "";
    }
}

void TToolbarEditor::onRowsInsertedActiveList() {
    // Only called after drop from all actions widget

    int first = savedFirst;
    int last = savedLast;
    WZDEBUG(QString("%1 %2").arg(first).arg(last));

    active_actions_list->clearSelection();
    for(int i = last; i >= first; i--) {
        TActionItem* item = dynamic_cast<TActionItem*>(
                    active_actions_list->item(i));
        if (item) {
            WZDEBUG(QString("Updating '%1'").arg(item->text()));
            QAction* action = item->getAction();
            Qt::ItemFlags flags = item->flags();
            if (isMainToolbar) {
                flags = flags | Qt::ItemIsUserTristate;
            }
            if (isActionEditable(action)) {
                flags = flags | Qt::ItemIsEditable;
            }
            item->setFlags(flags);
            if (isMainToolbar) {
                item->setCheckState(Qt::Checked);
            }
            item->setToolTip(getToolTipForItem(item));
            item->setSelected(true);
        } else {
            // TODO: Fix drop and remove this
            // Replace QListWidgetItem with a TActionItem
            QListWidgetItem* item = dynamic_cast<QListWidgetItem*>(
                        active_actions_list->item(i));
            QString actionName =
                        TActionItem::actionNameFromListWidgetItem(item);
            WZDEBUG(QString("Found dropped item '%1'").arg(actionName));
            delete active_actions_list->takeItem(i);
            TActionItem* new_item = new TActionItem(actionName);
            active_actions_list->insertItem(i, new_item);
        }
    }
}

void TToolbarEditor::onRowsInserted(const QModelIndex&,
                                         int first, int last) {

    // Post the event for onRowsInsertedActiveList()
    // TODO: check model vs view versions of first last
    savedFirst = first;
    savedLast = last;
    QTimer::singleShot(0, this, SLOT(onRowsInsertedActiveList()));
}

void TToolbarEditor::setActiveActions(const QStringList& actions) {

    active_actions_list->clear();

    QString actionName;
    bool ns, fs;
    for(int i = 0; i < actions.count(); i++) {
        stringToAction(actions.at(i), actionName, ns, fs);
        if (actionName.isEmpty()) {
            QString msg = tr("Failed to parse action '%1' for toolbar '%2'")
                    .arg(actions.at(i)).arg(toolbarName);
            WZERROR(msg);
            QMessageBox::warning(this, tr("Error"), msg);
        } else {
            QAction* action;
            if (actionName == "separator") {
                action = separatorAction;
            } else {
                action = findAction(actionName);
            }
            if (action) {
                active_actions_list->addItem(
                            createActiveActionItem(action, ns, fs));
            } else {
                QString msg = tr("Failed to find action '%1' for toolbar '%2'")
                        .arg(actions.at(i)).arg(toolbarName);
                WZERROR(msg);
                QMessageBox::warning(this, tr("Error"), msg);
            }
        }
    }

    connect(active_actions_list->model(),
            &QAbstractItemModel::rowsInserted,
            this, &TToolbarEditor::onRowsInserted);
    connect(active_actions_list, &QListWidget::itemClicked,
            this, &TToolbarEditor::onItemClicked);
    connect(active_actions_list, &QListWidget::itemDoubleClicked,
            this, &TToolbarEditor::onItemDoubleClicked);

    if (active_actions_list->count()) {
        active_actions_list->setCurrentRow(0);
    }
}

void TToolbarEditor::onUpButtonClicked() {

    const QList<QListWidgetItem*> sel = active_actions_list->selectedItems();
    active_actions_list->clearSelection();
    for(int i = 0; i <= sel.count() - 1; i++) {
        QListWidgetItem* item = sel.at(i);
        int row = active_actions_list->row(item);
        if (row > 0) {
            item = active_actions_list->takeItem(row);
            active_actions_list->insertItem(row - 1, item);
        }
        item->setSelected(true);
    }
}

void TToolbarEditor::onDownButtonClicked() {
    WZDEBUG("");

    const QList<QListWidgetItem*> sel = active_actions_list->selectedItems();
    active_actions_list->clearSelection();
    for(int i = sel.count() - 1; i >= 0; i--) {
        QListWidgetItem* item = sel.at(i);
        int row = active_actions_list->row(item);
        if (row < active_actions_list->count() - 1) {
            item = active_actions_list->takeItem(row);
            active_actions_list->insertItem(row + 1, item);
        }
        item->setSelected(true);
    }
}

void TToolbarEditor::onRightButtonClicked() {
    WZDEBUG("");

    int destRow;
    QList<QListWidgetItem*> sel = active_actions_list->selectedItems();
    if (sel.count() > 0) {
        destRow = active_actions_list->row(sel.at(0));
    } else {
        destRow = active_actions_list->count();
    }
    sel = all_actions_list->selectedItems();
    active_actions_list->clearSelection();
    for(int i = 0; i <= sel.count() - 1; i++) {
        QListWidgetItem* item = sel.at(i)->clone();
        active_actions_list->insertItem(destRow++, item);
        item->setSelected(true);
    }
}

void TToolbarEditor::onLeftButtonClicked() {
    WZDEBUG("");

    const QList<QListWidgetItem*> sel = active_actions_list->selectedItems();
    if (sel.count() > 0) {
        active_actions_list->clearSelection();
        int row = active_actions_list->row(sel.at(0));
        for(int i = sel.count() - 1; i >= 0; i--) {
            QListWidgetItem* item = sel.at(i);
            int idx = active_actions_list->row(item);
            delete active_actions_list->takeItem(idx);
        }
        if (row >= active_actions_list->count()) {
            row = active_actions_list->count() - 1;
        }
        active_actions_list->setCurrentRow(row);
    }
}

void TToolbarEditor::onSeperatorButtonClicked() {

    int row = active_actions_list->currentRow();
    if (row < 0) row = active_actions_list->count();
    active_actions_list->insertItem(
                row, createActiveActionItem(separatorAction, true, true));
    active_actions_list->setCurrentRow(row);
}

void TToolbarEditor::restoreDefaults() {
    setActiveActions(defaultActions);
}

QStringList TToolbarEditor::saveActions() {
    WZDEBUG("");

    QStringList list;

    for (int i = 0; i < active_actions_list->count(); i++) {
        TActionItem* item = static_cast<TActionItem*>(
                    active_actions_list->item(i));
        QAction* action = item->getAction();
        QString actionName = action->objectName();

        QString s = actionName;
        bool ns = item->checkState() == Qt::Checked
                || item->checkState() == Qt::Unchecked;
        bool fs =  item->checkState() == Qt::Checked
                || item->checkState() == Qt::PartiallyChecked;
        if (ns) {
            if (!fs) {
                s += "|1|0";
            }
        } else {
            s += "|0|1";
        }
        list.append(s);

        // Update icon text
        if (!action->isSeparator()) {
            QString iconText = item->getIconText();
            QString iText = TActionsEditor::cleanActionText(iconText,
                                                            actionName);
            QString actionText = TActionItem::iconDisplayText(action);
            if (actionText != iText) {
                WZINFO(QString("Updating icon text for action '%1' from '%2'"
                               " to '%3'")
                        .arg(actionName)
                        .arg(action->iconText())
                        .arg(iconText));
                action->setIconText(iconText);
                action->setProperty("modifiedicontext", true);
            } else {
                // Clear old icon text
                iconText = action->iconText();
                if (!iconText.isEmpty()) {
                    iText = TActionsEditor::cleanActionText(
                                iconText, actionName);
                    if (iText != actionText) {
                        WZINFO(QString("Cleared icon text '%1' of action '%2'")
                               .arg(iconText).arg(actionName));
                        action->setIconText("");
                        action->setProperty("modifiedicontext", false);
                    }
                }
            }
        }
    }

    return list;
}

void TToolbarEditor::onActiveActionsItemSelectionChanged() {
    WZDEBUG("");

    QList<QListWidgetItem*> sel = active_actions_list->selectedItems();
    bool e = sel.count() > 0;
    left_button->setEnabled(e);
    up_button->setEnabled(e && active_actions_list->row(sel.at(0)) > 0);
    down_button->setEnabled(e &&
        active_actions_list->row(sel.at(sel.count() - 1))
                            < active_actions_list->count() - 1);
    right_button->setEnabled(true);
}

} // namespace Action
} // namespace Gui

#include "moc_toolbareditor.cpp"

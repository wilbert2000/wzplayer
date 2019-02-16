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
#include "gui/action/actionlist.h"
#include "gui/action/actionseditor.h"
#include "images.h"
#include "config.h"

#include <QToolBar>
#include <QToolButton>
#include <QMatrix>
#include <QTimer>
#include <QMessageBox>


namespace Gui {
namespace Action {

class TActionItem : public QListWidgetItem {
public:
    TActionItem(QAction* action);

    virtual QVariant data(int role) const override;
    virtual void setData(int role, const QVariant& value) override;

    QAction* getAction() const;
    void setAction(QAction* action);
    QString getActionName() const;

    QString getIconText() const;
    void setIconText(const QString& s);
};

QString iconDisplayText(QAction* action) {

    QString s = action->iconText();
    if (s.isEmpty()) {
        s = action->text();
    }
    return TActionsEditor::cleanActionText(s, action->objectName());
}

TActionItem::TActionItem(QAction* action) :
    QListWidgetItem() {

    setAction(action);

    setFlags(Qt::ItemIsEnabled
             | Qt::ItemIsSelectable
             | Qt::ItemIsDragEnabled);

    if (action->isSeparator()) {
        setText(action->text());
        setTextAlignment(Qt::AlignHCenter);
    } else {
        setIconText(iconDisplayText(action));
    }

    QIcon icon = action->icon();
    if (icon.isNull()) {
        icon = Images::icon("empty_icon");
    }
    setIcon(icon);
}

QAction* TActionItem::getAction() const {
    return data(Qt::UserRole).value<QAction*>();
}

void TActionItem::setAction(QAction* action) {
    setData(Qt::UserRole, QVariant::fromValue(action));
}

QString TActionItem::getActionName() const {

    QAction* action = getAction();
    if (action) {
        return action->objectName();
    }
    return QString();
}

QString TActionItem::getIconText() const {

    QString s = text();
    return s.left(s.length() - getActionName().length() - 3);
}

void TActionItem::setIconText(const QString& s) {
    setText(s + " ("+ getActionName() + ")");
}

QVariant TActionItem::data(int role) const {

    if (role == Qt::EditRole) {
        return QVariant(getIconText());
    }
    return QListWidgetItem::data(role);
}

void TActionItem::setData(int role, const QVariant &value) {

    if (role == Qt::EditRole) {
        setIconText(value.toString());
    } else {
        QListWidgetItem::setData(role, value);
    }
}


TToolbarEditor::TToolbarEditor(QWidget* parent, const QString& tbarName) :
    QDialog(parent, TConfig::DIALOG_FLAGS),
    toolbarName(tbarName.toLower()) {

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

    connect(active_actions_list, &QListWidget::currentRowChanged,
            this, &TToolbarEditor::onCurrentRowChanged);

    separatorAction = new QAction(tr("Separator"), this);
    separatorAction->setObjectName("separator");
    separatorAction->setSeparator(true);
}

TToolbarEditor::~TToolbarEditor() {
}

void TToolbarEditor::setIconSize(int size) {
    iconsize_spin->setValue(size);
}

int TToolbarEditor::iconSize() const {
    return iconsize_spin->value();
}

void TToolbarEditor::setAllActions(const QList<QAction*>& actions) {

    allActions = actions;

    // Add actions to QListWidget all_actions_list
    for (int i = 0; i < allActions.count(); i++) {
        all_actions_list->addItem(new TActionItem(allActions.at(i)));
    }

    all_actions_list->setCurrentRow(0);
}

bool TToolbarEditor::isActionEditable(QAction* action) {

    if (!action || action->isSeparator() || action->inherits("QWidgetAction")) {
        return false;
    }

    // No use editing items that won't change
    QString iconText = action->iconText();
    if (iconText.isEmpty()) {
        iconText = action->text();
    }
    QString name = action->objectName();
    if (TActionsEditor::cleanActionText(iconText, name).trimmed()
            == TActionsEditor::cleanActionText("w", name).trimmed()) {
        return false;
    }
    return true;
}

TActionItem* TToolbarEditor::createActiveActionItem(QAction* action,
                                                    bool ns, bool fs) {

    TActionItem* i = new TActionItem(action);
    Qt::ItemFlags flags = i->flags() | Qt::ItemIsUserTristate;
    if (isActionEditable(action)) {
        flags = flags | Qt::ItemIsEditable;
    }
    i->setFlags(flags);

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

    i->setToolTip(getToolTipForItem(i));
    return i;
}

QString TToolbarEditor::getToolTipForItem(QListWidgetItem* item) {

    QString tip;
    switch (item->checkState()) {
        case Qt::Unchecked:
            tip = tr("Display on normal screen and not on full screen");
            break;
        case Qt::PartiallyChecked:
            tip = tr("Display on full screen and not on normal screen");
            break;
        case Qt::Checked:
            tip = tr("Display on normal and full screen");
    }

    if (item->flags() & Qt::ItemIsEditable) {
        tip += tr(". Text is editable.");
    } else {
        tip += tr(". Text is not editable.");
    }
    return tip;
}

void TToolbarEditor::onItemClicked(QListWidgetItem* item) {

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

void TToolbarEditor::onItemDoubleClicked(QListWidgetItem *item) {

    Qt::CheckState state = item->checkState();
    WZDEBUG(QString("'%1' %2").arg(item->text()).arg(state));
    // Undo the first click
    if (state == Qt::Unchecked) {
        state = Qt::PartiallyChecked;
    } else if (state == Qt::PartiallyChecked) {
        state = Qt::Checked;
    } else {
        state = Qt::Unchecked;
    }
    item->setCheckState(state);
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

    int first = savedFirst;
    int last = savedLast;
    WZDEBUG(QString("%1 %2").arg(first).arg(last));

    active_actions_list->clearSelection();
    for(int i = first; i <= last; i++) {
        TActionItem* item = static_cast<TActionItem*>(
                    active_actions_list->item(i));
        QAction* action = item->getAction();
        Qt::ItemFlags flags = item->flags() | Qt::ItemIsUserTristate;
        if (isActionEditable(action)) {
            flags = flags | Qt::ItemIsEditable;
        }
        item->setFlags(flags);
        item->setCheckState(Qt::Checked);
        item->setToolTip(getToolTipForItem(item));
        item->setSelected(true);
    }
}

void TToolbarEditor::onRowsInserted(const QModelIndex&,
                                         int first, int last) {

    // Post the event for onRowsInsertedActiveList()
    // TODO: check first last on hussled list
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
                action = findAction(actionName, allActions);
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

    int row = active_actions_list->currentRow();
    WZDEBUG(QString("%1").arg(row));
    if (row > 0) {
        QListWidgetItem* i = active_actions_list->takeItem(row);
        active_actions_list->insertItem(row - 1, i);
        active_actions_list->setCurrentRow(row - 1);
    }
}

void TToolbarEditor::onDownButtonClicked() {
    WZDEBUG("");

    int row = active_actions_list->currentRow();
    if (row >= 0 && row < active_actions_list->count() - 1) {
        QListWidgetItem* i = active_actions_list->takeItem(row);
        active_actions_list->insertItem(row + 1, i);
        active_actions_list->setCurrentRow(row + 1);
    }
}

void TToolbarEditor::onRightButtonClicked() {
    WZDEBUG("");

    int row = all_actions_list->currentRow();
    if (row >= 0) {
        int dest_row = active_actions_list->currentRow();
        if (dest_row < 0) dest_row = 0;
        QListWidgetItem* item = all_actions_list->item(row)->clone();
        WZDEBUG(QString("'%1' %2").arg(item->text()).arg(dest_row));
        active_actions_list->insertItem(dest_row, item);
        active_actions_list->setCurrentRow(dest_row);
    }
}

void TToolbarEditor::onLeftButtonClicked() {
    WZDEBUG("");

    int row = active_actions_list->currentRow();
    if (row >= 0) {
        delete active_actions_list->takeItem(row);
        if (row >= active_actions_list->count()) row--;
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
            QString iText = TActionsEditor::cleanActionText(
                        iconText, actionName).trimmed();
            QString actionText = iconDisplayText(action);
            if (actionText != iText) {
                WZINFO(QString("Updating icon text for action '%1' from '%2'"
                               " to '%3'")
                        .arg(actionName)
                        .arg(action->iconText())
                        .arg(iconText));
                action->setIconText(iconText);
                action->setProperty("modified", true);
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
                    }
                }
            }
        }
    }

    return list;
}

void TToolbarEditor::onCurrentRowChanged(int currentRow) {

    left_button->setEnabled(currentRow >= 0);
    up_button->setEnabled(currentRow > 0);
    down_button->setEnabled(currentRow >= 0
                            && currentRow < active_actions_list->count() - 1);
    //right_button->setEnabled(true);
}

QAction* TToolbarEditor::findAction(const QString& actionName,
                                    const QList<QAction*>& actionList) {

    for (int i = 0; i < actionList.count(); i++) {
        QAction* action = actionList.at(i);
        if (action->objectName() == actionName)
            return action;
    }

    return 0;
}

} // namespace Action
} // namespace Gui

#include "moc_toolbareditor.cpp"

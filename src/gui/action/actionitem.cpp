#include "gui/action/actionitem.h"
#include "gui/action/actionseditor.h"
#include "gui/action/action.h"
#include "images.h"
#include "wzdebug.h"

#include <QList>
#include <QAction>
#include <QDataStream>
#include <QString>
#include <QApplication>


namespace Gui {
namespace Action {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Action::TActionItem)

// statics
QList<QAction*> TActionItem::allActions;

static QString SeparatorDescription() {
    return qApp->translate("Gui::Action::TToolbarEditor", "Separator");
}

QString TActionItem::actionNameFromListWidgetItem(QListWidgetItem* item) {

    if (!item) {
        return "separator";
    }
    QString name = item->text();
    if (name == SeparatorDescription()) {
        return "separator";
    }

    int i = name.indexOf("(");
    if (i >= 0) {
        name = name.mid(i + 1);
    }
    if (name.endsWith(")")) {
        name = name.left(name.length() - 1);
    }
    return name;
}

QString TActionItem::iconDisplayText(QAction* action) {

    QString s = action->iconText();
    if (s.isEmpty()) {
        s = action->text();
    }
    return TActionsEditor::cleanActionText(s, action->objectName());
}

void TActionItem::init() {

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

TActionItem::TActionItem(QAction* act) :
    QListWidgetItem(0, QListWidgetItem::UserType),
    action(act) {

    init();
}

TActionItem::TActionItem(const QString& actionName) :
    QListWidgetItem(0, QListWidgetItem::UserType) {

    action = findAction(actionName, allActions);
    if (!action) {
        action = new QAction(SeparatorDescription());
        action->setObjectName("separator");
        action->setSeparator(true);
    }

    WZDEBUG(QString("Created item for action '%1' from string")
            .arg(action->objectName()));
    init();
}

TActionItem::TActionItem(const TActionItem& other) :
    QListWidgetItem(other),
    action(other.action) {
    // TODO: see help, type not copied...
    WZDEBUG(QString("Created copy of '%1'").arg(action->objectName()));
}

TActionItem* TActionItem::clone() const {
    WZDEBUG(action->objectName());
    return new TActionItem(*this);
}

QAction* TActionItem::getAction() const {
    return action;
}

QString TActionItem::getActionName() const {
    return action->objectName();
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

void TActionItem::read(QDataStream& in) {

    QListWidgetItem::read(in);
    QString actionName;
    in >> actionName;
    WZDEBUG(actionName);
    action = findAction(actionName, allActions);
    if (!action) {
        WZERROR(QString("Action '%1' not found").arg(actionName));
        // Need an action to not crash on null
        action = allActions[0];
    }
}

void TActionItem::write(QDataStream& out) const {
    WZDEBUG(action->objectName());

    QListWidgetItem::write(out);
    out << action->objectName();
}

} // namespace Action
} // namespace Gui

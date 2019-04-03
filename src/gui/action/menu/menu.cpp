#include "gui/action/menu/menu.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"
#include "images.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenu::TMenu(QWidget* parent,
             const QString& name,
             const QString& text,
             const QString& icon) :
    QMenu(parent) {

    menuAction()->setObjectName(name);
    menuAction()->setText(text);

    QString iconName = icon.isEmpty() ? name : icon;
    if (iconName != "noicon") {
        menuAction()->setIcon(Images::icon(iconName));
    }
}

void TMenu::execSlot() {

    // If visible hide, so exec will show the menu again at the new cursor pos
    if (isVisible()) {
        hide();
    }
    exec(QCursor::pos());
}


} // namespace Menu
} // namespace Action
} // namespace Gui


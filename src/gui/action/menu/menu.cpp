#include "gui/action/menu/menu.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"
#include "images.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenu::TMenu(QWidget* parent,
             TMainWindow* mw,
             const QString& name,
             const QString& text,
             const QString& icon) :
    TMenuExec(parent),
    main_window(mw) {

    menuAction()->setObjectName(name);
    menuAction()->setText(text);

    QString iconName = icon.isEmpty() ? name : icon;
    if (iconName != "noicon") {
        menuAction()->setIcon(Images::icon(iconName));
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui


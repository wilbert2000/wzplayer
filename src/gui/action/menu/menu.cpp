#include "gui/action/menu/menu.h"
#include "gui/mainwindow.h"
#include "settings/mediasettings.h"
#include "images.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenu::TMenu(QWidget* parent,
             TMainWindow* w,
             const QString& name,
             const QString& text,
             const QString& icon) :
    TMenuExec(parent),
    main_window(w) {

    menuAction()->setObjectName(name);
    menuAction()->setText(text);

    QString iconName = icon.isEmpty() ? name : icon;
    if (iconName != "noicon") {
        menuAction()->setIcon(Images::icon(iconName));
    }

    connect(main_window, &TMainWindow::enableActions,
            this, &TMenu::enableActions);
    connect(main_window, &TMainWindow::mediaSettingsChanged,
            this, &TMenu::onMediaSettingsChanged);
}

TMenu::~TMenu() {
}

void TMenu::enableActions() {
}

void TMenu::onMediaSettingsChanged(Settings::TMediaSettings*) {
}

void TMenu::onAboutToShow() {
}

void TMenu::setVisible(bool visible) {

    if (visible)
        onAboutToShow();
    TMenuExec::setVisible(visible);
}

} // namespace Menu
} // namespace Action
} // namespace Gui


#include "gui/action/menu/menu.h"
#include <QEvent>
#include <QApplication>
#include <QDesktopWidget>
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
    QMenu(parent),
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
    QMenu::setVisible(visible);
}

void TMenu::addActionsTo(QWidget* w) {

    w->addAction(menuAction());

    QList<QAction*> acts = actions();
    for(int i = 0; i < acts.count(); i++) {
        QAction* a = acts[i];
        if (!a->isSeparator()) {
            w->addAction(a);
        }
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui


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

void execPopup(QWidget* w, QMenu* popup, QPoint p) {

    // Keep inside desktop
    QRect desktop = QApplication::desktop()->screenGeometry(w);
    QSize s = popup->sizeHint();
    if (p.x() <= desktop.x()) {
        p.rx() = desktop.x();
    } else if (p.x() + s.width() > desktop.x() + desktop.width()) {
        p.rx() = desktop.x() + desktop.width() - s.width();
    }
    if (p.y() <= desktop.y()) {
        p.ry() = desktop.y();
    } else if (p.y() + s.height() > desktop.y() + desktop.height()) {
        p.ry() = desktop.y() + desktop.height() - s.height();
    }

    // Evade mouse
    if (QCursor::pos().x() > p.x() && QCursor::pos().x() < p.x() + s.width()) {
        if (QCursor::pos().x() >= desktop.x() + desktop.width() - s.width()) {
            // Place menu to the left of mouse
            p.rx() = QCursor::pos().x() - s.width();
        } else {
            // Place menu to the right of mouse
            p.rx() = QCursor::pos().x();
        }
    }
    if (QCursor::pos().y() > p.y() && QCursor::pos().y() < p.y() + s.height()) {
        if (QCursor::pos().y() >= desktop.y() + desktop.height() - s.height()) {
            // Place menu above mouse
            p.ry() = QCursor::pos().y() - s.height();
        } else {
            // Place menu below mouse
            p.ry() = QCursor::pos().y();
        }
    }

    // Popup exec keeps menu inside screen too
    popup->exec(p);
}


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


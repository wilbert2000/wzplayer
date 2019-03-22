#include "gui/autohidetimer.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include "desktop.h"
#include "settings/preferences.h"


const int MOUSE_MOVED_TRESHOLD = 4;

using namespace Settings;

namespace Gui {


TAutoHideTimer::TAutoHideTimer(QObject *parent, QWidget* playerwin)
    : QTimer(parent),
    debug(logger()),
    disabled(0),
    autoHide(false),
    settingVisible(false),
    autoHideMouse(false),
    mouseHidden(false),
    draggingPlayerWindow(false),
    playerWindow(playerwin) {

    setSingleShot(true);
    setInterval(pref->floating_hide_delay);
    connect(this, &TAutoHideTimer::timeout, this, &TAutoHideTimer::onTimeOut);

    playerWindow->installEventFilter(this);
}

void TAutoHideTimer::start() {

    autoHide = true;
    QTimer::start();
}

void TAutoHideTimer::stop() {

    autoHide = false;
    QTimer::stop();

    // Show the widgets to save their visible state
    setVisible(true);
}

void TAutoHideTimer::enable() {

    disabled--;
    autoHideMouseLastPosition = QCursor::pos();
    if (disabled == 0 && (autoHide || autoHideMouse))
        QTimer::start();
}

void TAutoHideTimer::disable() {

    disabled++;
    if (disabled == 1 && autoHide)
        setVisible(true);
    showHiddenMouse();
    QTimer::stop();
}

void TAutoHideTimer::setAutoHideMouse(bool on) {

    autoHideMouse = on;
    autoHideMouseLastPosition = QCursor::pos();
    if (autoHideMouse)
        QTimer::start();
    else
        showHiddenMouse();
}

void TAutoHideTimer::add(QAction* action, QWidget* w) {

    TAutoHideItem item(action, w);
    items[action->objectName()] = item;
    if (action->isChecked()) {
        actions.append(action);
        widgets.append(w);
    }
    connect(action, &QAction::toggled, this, &TAutoHideTimer::onActionToggled);
}

void TAutoHideTimer::setVisible(bool visible) {

    settingVisible = true;

    int screen = QApplication::desktop()->screenNumber(playerWindow);
    for(int i = 0; i < actions.size(); i++) {
        QAction* action = actions.at(i);
        if (action->isChecked() != visible) {
            if (visible) {
                action->trigger();
            } else if (QApplication::desktop()->screenNumber(widgets.at(i))
                       == screen) {
                action->trigger();
            }
        }
    }

    settingVisible = false;
}

bool TAutoHideTimer::hiddenWidget() const {

    for(int i = 0; i < widgets.size(); i++) {
        if (widgets.at(i)->isHidden())
            return true;
    }
    return false;
}

bool TAutoHideTimer::mouseInsideShowArea() const {

    const int margin = 100;

    // Check bottom of screen
    if (pref->fullscreen) {
        QRect desktop = QApplication::desktop()->screenGeometry(playerWindow);
        int b = desktop.y() + desktop.height();
        QPoint p = QCursor::pos();
        if (p.x() >= desktop.x() && p.x() < desktop.x() + desktop.width()
            && p.y() > b - margin && p.y() <= b) {
            return true;
        }
    }

    // Check around widgets
    QPoint o(-margin, -margin);
    QSize s(2 * margin, 2 * margin);
    for (int i = 0; i < widgets.size(); i++) {
        QWidget* w = widgets.at(i);
        QRect showArea(w->mapToGlobal(o), w->size() + s);
        if (showArea.contains(QCursor::pos())) {
            return true;
        }
    }

    return false;
}

void TAutoHideTimer::onActionToggled(bool visible) {

    if (settingVisible)
        return;

    QString actioName = QObject::sender()->objectName();
    TItemMap::const_iterator i = items.find(actioName);
    if (i == items.end()) {
        WZWARN("action '" + actioName + "' not found");
        return;
    }

    TAutoHideItem item = i.value();
    if (visible) {
        actions.append(item.action);
        widgets.append(item.widget);
        if (autoHide && disabled == 0) {
            QTimer::start();
        }
    } else {
        actions.removeOne(item.action);
        widgets.removeOne(item.widget);
    }
}

void TAutoHideTimer::showHiddenMouse() {

    if (mouseHidden) {
        mouseHidden = false;
        playerWindow->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void TAutoHideTimer::hideMouse() {

    if (!mouseHidden) {
        mouseHidden = true;
        playerWindow->setCursor(QCursor(Qt::BlankCursor));
    }
}

void TAutoHideTimer::setDraggingPlayerWindow(bool dragging) {

    draggingPlayerWindow = dragging;
    if (autoHide && disabled == 0 && draggingPlayerWindow) {
        setVisible(false);
    }
}

bool TAutoHideTimer::haveWidgetToHide() const {

    int scrn = QApplication::desktop()->screenNumber(playerWindow);
    for(int i = 0; i < widgets.size(); i++) {
        QWidget* w = widgets.at(i);
        if (w->isVisible() && QApplication::desktop()->screenNumber(w) == scrn)
            return true;
    }
    return false;
}

void TAutoHideTimer::onTimeOut() {

    // Handle mouse
    if (autoHideMouse) {
        if ((QCursor::pos() - autoHideMouseLastPosition).manhattanLength()
            > MOUSE_MOVED_TRESHOLD) {
            showHiddenMouse();
            QTimer::start();
        } else if (disabled == 0) {
            hideMouse();
        }
    } else {
        showHiddenMouse();
    }
    autoHideMouseLastPosition = QCursor::pos();

    // Hide widgets when no mouse buttons down and mouse outside show area
    if (autoHide && disabled == 0 && haveWidgetToHide()) {
        if (QApplication::mouseButtons() || mouseInsideShowArea()) {
            QTimer::start();
        } else {
            setVisible(false);
        }
    }
}

bool TAutoHideTimer::eventFilter(QObject* obj, QEvent* event) {

    bool button = event->type() == QEvent::MouseButtonPress
                  || event->type() == QEvent::MouseButtonRelease;
    bool mouse = button;

    if (event->type() == QEvent::MouseMove
        && (QCursor::pos() - autoHideMouseLastPosition).manhattanLength()
            > MOUSE_MOVED_TRESHOLD) {
        mouse = true;
    }

    // Handle mouse
    if (mouse) {
        showHiddenMouse();
        if (autoHideMouse && disabled == 0) {
            QTimer::start();
        }
        autoHideMouseLastPosition = QCursor::pos();
    }

    // Handle widgets
    if (autoHide && disabled == 0 && mouse) {
        // Don't show when left button still down, like when dragging
        if ((QApplication::mouseButtons() & Qt::LeftButton) == 0
            && hiddenWidget()) {
            if (button || mouseInsideShowArea()) {
                setVisible(true);
                QTimer::start();
            }
        }
    }

    return QTimer::eventFilter(obj, event);
}

} //namespace Gui


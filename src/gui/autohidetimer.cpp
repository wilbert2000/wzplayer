#include "gui/autohidetimer.h"
#include <QDebug>
#include <QApplication>
#include <QMouseEvent>
#include "gui/desktop.h"
#include "settings/preferences.h"

const int MOUSE_MOVED_TRESHOLD = 4;

using namespace Settings;

namespace Gui {

TAutoHideItem::~TAutoHideItem() {
}


TAutoHideTimer::TAutoHideTimer(QObject *parent, QWidget* playerwin)
    : QTimer(parent),
    debug(logger()),
    autoHide(false),
    enabled(true),
    settingVisible(false),
    autoHideMouse(false),
    mouseHidden(false),
    draggingPlayerWindow(false),
    playerWindow(playerwin) {

    setSingleShot(true);
    setInterval(pref->floating_hide_delay);
    connect(this, SIGNAL(timeout()), this, SLOT(onTimeOut()));

    playerWindow->installEventFilter(this);
}

TAutoHideTimer::~TAutoHideTimer() {
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

    enabled = true;
    autoHideMouseLastPosition = QCursor::pos();
    if (autoHide || autoHideMouse)
        QTimer::start();
}

void TAutoHideTimer::disable() {

    enabled = false;
    if (autoHide)
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
    connect(action, SIGNAL(toggled(bool)), this, SLOT(onActionToggled(bool)));
}

void TAutoHideTimer::setVisible(bool visible) {

    settingVisible = true;

    for(int i = 0; i < actions.size(); i++) {
        QAction* action = actions[i];
        if (action->isChecked() != visible) {
            action->trigger();
        }
    }

    settingVisible = false;
}

bool TAutoHideTimer::visibleWidget() const {

    for(int i = 0; i < widgets.size(); i++) {
        if (widgets[i]->isVisible())
            return true;
    }
    return false;
}

bool TAutoHideTimer::hiddenWidget() const {

    for(int i = 0; i < widgets.size(); i++) {
        if (widgets[i]->isHidden())
            return true;
    }
    return false;
}

bool TAutoHideTimer::mouseInsideShowArea() const {

    const int margin = 100;

    // Check bottom of screen
    if (pref->fullscreen
        && QCursor::pos().y() > TDesktop::size(playerWindow).height() - margin) {
        return true;
    }

    // Check around widgets
    for (int i = 0; i < widgets.size(); i++) {
        QWidget* w = widgets[i];
        QRect showArea(w->mapToGlobal(QPoint(0, 0)) - QPoint(margin, margin),
                       w->size() + QSize(margin, margin));
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
        logger()->warn("onActionToggled: action '" + actioName + "' not found");
        return;
    }

    TAutoHideItem item = i.value();
    if (visible) {
        actions.append(item.action);
        widgets.append(item.widget);
        if (autoHide && enabled) {
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
    if (autoHide && enabled && draggingPlayerWindow && visibleWidget()) {
        setVisible(false);
    }
}

void TAutoHideTimer::onTimeOut() {

    // Handle mouse
    if (autoHideMouse) {
        if ((QCursor::pos() - autoHideMouseLastPosition).manhattanLength()
            > MOUSE_MOVED_TRESHOLD) {
            showHiddenMouse();
            QTimer::start();
        } else if (enabled) {
            hideMouse();
        }
    } else {
        showHiddenMouse();
    }
    autoHideMouseLastPosition = QCursor::pos();

    // Hide widgets when no mouse buttons down and mouse outside show area
    if (autoHide && enabled && visibleWidget()) {
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
        if (autoHideMouse && enabled) {
            QTimer::start();
        }
        autoHideMouseLastPosition = QCursor::pos();
    }

    // Handle widgets
    if (autoHide && enabled && mouse) {
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


#include "gui/dockwidget.h"
#include "gui/mainwindow.h"
#include "desktop.h"
#include "wztimer.h"

#include <QApplication>


using namespace Settings;

namespace Gui {

TDockWidget::TDockWidget(QWidget* parent,
                         QWidget* aPanel,
                         const QString& objectName,
                         const QString& title) :
    QDockWidget(title, parent),
    panel(aPanel),
    lastArea(Qt::NoDockWidgetArea) {

    setObjectName(objectName);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);

    // Fix focus and invisible docks
    connect(qApp, &QApplication::focusChanged,
            this, &TDockWidget::onFocusChanged);
    connect(this, &TDockWidget::visibilityChanged,
            this, &TDockWidget::onDockVisibilityChanged);
    connect(this, &TDockWidget::dockLocationChanged,
            this, &TDockWidget::onDockLocationChanged);
    connect(toggleViewAction(), &QAction::triggered,
            this, &TDockWidget::onToggleViewTriggered);

    hide();

    // QTBUG-48296 keeps panels inside desktop
    // Use alt+left mouse button to override
}

void TDockWidget::onDockLocationChanged(Qt::DockWidgetArea area) {
    lastArea = area;
}

Qt::DockWidgetArea TDockWidget::getArea() const {

    if (isFloating() || !isVisible() || !panel->isVisible()) {
        return Qt::NoDockWidgetArea;
    }
    return lastArea;
}

void TDockWidget::resizeMainWindow(bool visible) {

    QRect r = mainWindow->geometry();
    if (lastArea == Qt::LeftDockWidgetArea
            || lastArea == Qt::RightDockWidgetArea) {
        if (visible) {
            r.setWidth(r.width() + frameSize().width());
            if (lastArea == Qt::LeftDockWidgetArea) {
                r.moveLeft(r.x() - frameSize().width());
            }
        } else {
            r.setWidth(r.width() - frameSize().width());
            if (lastArea == Qt::LeftDockWidgetArea) {
                r.moveLeft(r.x() + frameSize().width());
            }
        }
    } else if (visible) {
        r.setHeight(r.height() + frameSize().height());
        if (lastArea == Qt::TopDockWidgetArea) {
            r.moveTop(r.y() - frameSize().width());
        }
    } else {
        r.setHeight(r.height() - frameSize().height());
        if (lastArea == Qt::TopDockWidgetArea) {
            r.moveTop(r.y() + frameSize().width());
        }
    }

    mainWindow->setGeometry(r);

    if (visible) {
        if (TDesktop::keepInsideDesktop(mainWindow)) {
            mainWindow->optimizeSizeTimer->logStart();
        }
    }
}

void TDockWidget::triggerResize(bool visible) {

    if (Gui::mainWindow->dockNeedsResize(this, lastArea)) {
        WZTRACEOBJ("Resizing");
        resizeMainWindow(visible);
    }
}

void TDockWidget::closeEvent(QCloseEvent* e) {

    QDockWidget::closeEvent(e);
    triggerResize(false);
}

// Note: called when toggleViewAction() triggered, not when close button used
void TDockWidget::onToggleViewTriggered(bool visible) {

    if (visible) {
        // If !visible resize already triggered by closeEvent()
        triggerResize(visible);

        // The toggleViewAction won't raise the dock when making it visible
        if (visibleRegion().isEmpty()) {
            WZTRACEOBJ("Raising");
            raise();
        }
    }
}

// When tabbing around, Qt tabs into non visible docks.
// Fixed by raising the dock if it is not visible.
void TDockWidget::onFocusChanged(QWidget*, QWidget *now) {

    //WZTRACEOBJ(QString("From %1 to %2")
    //           .arg(old ? old->objectName() : "none")
    //           .arg(now ? now->objectName() : "none"));

    if (now && now == focusWidget() && visibleRegion().isEmpty()) {
        WZTRACEOBJ("Got focus while not visible, raising");
        raise();
    }
}

// When a tabbed dock has focus and we switch to another dock, switch focus
// with it, to prevent the focus staying behind on a no longer visible dock.
void TDockWidget::onDockVisibilityChanged(bool visible) {
    //WZTOBJ << "Visible arg" << visible
    //       << "isVisible()" << isVisible()
    //       << "Focus widget"
    //       << (qApp->focusWidget()
    //           ? qApp->focusWidget()->objectName()
    //           : "none");

    if (visible && focusProxy()) {
        QString proxyName = focusProxy()->objectName();
        QWidget* fw = qApp->focusWidget();
        while (fw) {
            QString name = fw->objectName();
            if (name == proxyName // Just a speedup
                    || name == objectName()) {
                break;
            } else if (name.endsWith("_widget") || name.endsWith("_dock")) {
                WZTOBJ << "Current focus" << name
                       << "- Setting focus to" << proxyName;
                focusProxy()->setFocus();
                break;
            } else {
                fw = fw->parentWidget();
            }
        }
    }
}

} // namespace Gui

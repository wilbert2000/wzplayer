#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "desktop.h"
#include "wzdebug.h"

#include <QApplication>


using namespace Settings;

namespace Gui {

TDockWidget::TDockWidget(QWidget* parent,
                         const QString& objectName,
                         const QString& title) :
    QDockWidget(title, parent) {

    setObjectName(objectName);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);

    connect(qApp, &QApplication::focusChanged,
            this, &TDockWidget::onFocusChanged);
    connect(this, &TDockWidget::visibilityChanged,
            this, &TDockWidget::onDockVisibilityChanged);

    // QTBUG-48296 keeps panels inside desktop
    // Use alt+left mouse button to override
}

// When tabbing around, Qt tabs into non visible docks.
// Fixed by raising the dock if it is not visible.
void TDockWidget::onFocusChanged(QWidget*, QWidget *now) {

    //WZTRACEOBJ(QString("From %1 to %2")
    //           .arg(old ? old->objectName() : "none")
    //           .arg(now ? now->objectName() : "none"));

    if (now && now == focusWidget()) {
        if (visibleRegion().isEmpty()) {
            WZTRACEOBJ(QString("Got focus while not visible, raising %1")
                       .arg(objectName()));
            raise();
        }
    }
}

// When a tabbed dock has focus and we switch to another dock, switch focus
// with it, to prevent the focus staying behind on a no longer visible dock.
void TDockWidget::onDockVisibilityChanged(bool visible) {
    //WZTRACEOBJ(QString("Visible %1 Focus widget %2")
    //           .arg(visible)
    //           .arg(qApp->focusWidget()
    //                ? qApp->focusWidget()->objectName()
    //                : "none"));

    if (visible && focusProxy()) {
        QString proxyName = focusProxy()->objectName();
        QWidget* fw = qApp->focusWidget();
        while (fw) {
            QString name = fw->objectName();
            if (name == "playerwindow" // Just a speedup
                    || name == proxyName // Just a speedup
                    || name == objectName()) {
                break;
            } else if (name.endsWith("widget") || name.endsWith("dock")) {
                WZTRACEOBJ(QString("Currently active %1. Setting focus to %2")
                           .arg(name).arg(proxyName));
                focusProxy()->setFocus();
                break;
            } else {
                fw = fw->parentWidget();
            }
        }
    }
}

} // namespace Gui

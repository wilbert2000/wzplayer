#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "desktop.h"


using namespace Settings;

namespace Gui {

TDockWidget::TDockWidget(const QString& title,
                         QWidget* parent,
                         const QString& objectName) :
    QDockWidget(title, parent) {

    setObjectName(objectName);
    setAcceptDrops(true);
    // QTBUG-48296 keeps panels inside desktop
    // Use alt+left mouse button to override
}

TDockWidget::~TDockWidget() {
}

} // namespace Gui

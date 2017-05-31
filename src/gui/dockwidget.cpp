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
    //setAllowedAreas(Qt::AllDockWidgetAreas); // default
}

TDockWidget::~TDockWidget() {
}

} // namespace Gui

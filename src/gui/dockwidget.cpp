#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "desktop.h"


using namespace Settings;

namespace Gui {

TDockWidget::TDockWidget(QWidget* parent) :
    QDockWidget(parent),
    restore(false) {

    setAcceptDrops(true);
}

TDockWidget::~TDockWidget() {
}

void TDockWidget::loadConfig() {
    restore = isVisible() && isFloating();
}

void TDockWidget::onShowMainWindow() {

    if (restore) {
        WZDEBUG("showing dock " + objectName());
        show();
    } else {
        WZDEBUG("not restoring dock " + objectName());
    }
}

void TDockWidget::onHideMainWindow() {
    WZDEBUG("");

    restore = isVisible() && isFloating();
    if (restore) {
        hide();
    }
}

} // namespace Gui

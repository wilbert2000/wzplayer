#include "gui/action/menu/menuhelp.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenuHelp::TMenuHelp(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "help_menu", tr("Help"), "noicon") {

    addAction(mw->requireAction("help_cl_options"));
    addAction(mw->requireAction("help_check_updates"));
    addAction(mw->requireAction("help_about"));
}

} // namespace Menu
} // namespace Action
} // namespace Gui


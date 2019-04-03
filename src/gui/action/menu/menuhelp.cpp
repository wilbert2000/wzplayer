#include "gui/action/menu/menuhelp.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenuHelp::TMenuHelp(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "help_menu", tr("Help"), "noicon") {

    addAction(mw->findAction("help_cl_options"));
    addAction(mw->findAction("help_check_updates"));
    addAction(mw->findAction("help_about"));
}

} // namespace Menu
} // namespace Action
} // namespace Gui


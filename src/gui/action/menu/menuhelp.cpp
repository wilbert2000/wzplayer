#include "gui/action/menu/menuhelp.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {
namespace Menu {

TMenuHelp::TMenuHelp(TMainWindow* mw) :
    TMenu(mw, mw, "help_menu", tr("&Help"), "noicon") {

    // Menu Help
    TAction* a = new TAction(this, "cl_options", tr("&Command line options"),
                             "cl_help");
    connect(a, &TAction::triggered, main_window, &TMainWindow::helpCLOptions);

    a = new TAction(this, "check_updates", tr("Check for &updates"),
                    "pref_updates");
    connect(a, &TAction::triggered,
            main_window, &TMainWindow::helpCheckUpdates);

    a = new TAction(this, "about", tr("About &WZPlayer"), "logo");
    connect(a, &TAction::triggered,
            main_window, &TMainWindow::helpAbout);
}

} // namespace Menu
} // namespace Action
} // namespace Gui


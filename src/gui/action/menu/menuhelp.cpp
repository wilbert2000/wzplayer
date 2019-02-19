#include "gui/action/menu/menuhelp.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {
namespace Menu {

TMenuHelp::TMenuHelp(TMainWindow* mw) :
    TMenu(mw, mw, "help_menu", tr("Help"), "noicon") {

    // Menu Help
    TAction* a = new TAction(mw, "cl_options", tr("Command line options"),
                             "cl_help");
    connect(a, &TAction::triggered, mw, &TMainWindow::helpCLOptions);
    addAction(a);

    a = new TAction(mw, "check_updates", tr("Check for updates"),
                    "pref_updates");
    connect(a, &TAction::triggered, mw, &TMainWindow::helpCheckUpdates);
    addAction(a);

    a = new TAction(mw, "about", tr("About WZPlayer"), "logo");
    connect(a, &TAction::triggered, mw, &TMainWindow::helpAbout);
    addAction(a);
}

} // namespace Menu
} // namespace Action
} // namespace Gui


#include "gui/action/menuhelp.h"
#include "gui/base.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {

TMenuHelp::TMenuHelp(TBase* mw) :
    TMenu(mw, mw, "help_menu", tr("&Help"), "noicon") {

	// Menu Help
	TAction* a = new TAction(this, "cl_options", tr("&Command line options"), "cl_help");
    connect(a, SIGNAL(triggered()), main_window, SLOT(helpCLOptions()));

    a = new TAction(this, "check_updates", tr("Check for &updates"), "pref_updates");
    connect(a, SIGNAL(triggered()), main_window, SLOT(helpCheckUpdates()));

	a = new TAction(this, "about", tr("About &WZPlayer"), "logo");
    connect(a, SIGNAL(triggered()), main_window, SLOT(helpAbout()));

    addActionsTo(main_window);
}

} // namespace Action
} // namespace Gui


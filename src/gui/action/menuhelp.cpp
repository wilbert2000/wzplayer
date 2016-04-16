#include "gui/action/menuhelp.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {

TMenuHelp::TMenuHelp(QWidget* parent)
    : TMenu(parent, "help_menu", tr("&Help"), "noicon") {

	// Menu Help
	TAction* a = new TAction(this, "cl_options", tr("&Command line options"), "cl_help");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCLOptions()));

	a = new TAction(this, "check_updates", tr("Check for &updates"));
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCheckUpdates()));

	a = new TAction(this, "about", tr("About &WZPlayer"), "logo");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpAbout()));

	addActionsTo(parent);
}

} // namespace Action
} // namespace Gui


#include "gui/action/menuhelp.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {

TMenuHelp::TMenuHelp(QWidget* parent)
	: TMenu(parent, this, "help_menu", QT_TR_NOOP("&Help"), "noicon") {

	// Menu Help
	TAction* a = new TAction(this, "cl_options", QT_TR_NOOP("&Command line options"), "cl_help");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCLOptions()));

	a = new TAction(this, "check_updates", QT_TR_NOOP("Check for &updates"));
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCheckUpdates()));

	a = new TAction(this, "about", QT_TR_NOOP("About &WZPlayer"), "logo");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpAbout()));

	addActionsTo(parent);
}

} // namespace Action
} // namespace Gui


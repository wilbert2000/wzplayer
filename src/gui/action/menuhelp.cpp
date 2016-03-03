#include "gui/action/menuhelp.h"
#include "gui/action/action.h"

namespace Gui {
namespace Action {

TMenuHelp::TMenuHelp(QWidget* parent)
	: TMenu(parent, this, "help_menu", QT_TR_NOOP("&Help"), "noicon") {

	// Menu Help
	TAction* a = new TAction(this, "first_steps", QT_TR_NOOP("First Steps &Guide"), "guide");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpFirstSteps()));

	a = new TAction(this, "faq", QT_TR_NOOP("&FAQ"));
	connect(a, SIGNAL(triggered()), parent, SLOT(helpFAQ()));

	a = new TAction(this, "cl_options", QT_TR_NOOP("&Command line options"), "cl_help");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCLOptions()));

	a = new TAction(this, "check_updates", QT_TR_NOOP("Check for &updates"));
	connect(a, SIGNAL(triggered()), parent, SLOT(helpCheckUpdates()));

	a = new TAction(this, "about_smplayer", QT_TR_NOOP("About &SMPlayer"), "logo");
	connect(a, SIGNAL(triggered()), parent, SLOT(helpAbout()));

	addActionsTo(parent);
}

} // namespace Action
} // namespace Gui


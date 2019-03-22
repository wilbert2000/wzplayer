#include "gui/action/menu/menuexec.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenuExec::TMenuExec(QWidget* parent) :
    QMenu(parent) {
}

void TMenuExec::execSlot() {

    // If visible hide, so exec will show the menu again at the new cursor pos
    if (isVisible()) {
        hide();
    }
    exec(QCursor::pos());
}

} // namespace Menu
} // namespace Action
} // namespace Gui


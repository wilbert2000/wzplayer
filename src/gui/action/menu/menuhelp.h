#ifndef GUI_ACTION_MENU_MENUHELP_H
#define GUI_ACTION_MENU_MENUHELP_H

#include "gui/action/menu/menu.h"

namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TMenuHelp : public TMenu {
public:
    explicit TMenuHelp(QWidget* parent, TMainWindow* mw);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUHELP_H

#ifndef GUI_ACTION_HELPMENU_H
#define GUI_ACTION_HELPMENU_H

#include "gui/action/menu.h"

namespace Gui {

class TMainWindow;
namespace Action {

class TAction;


class TMenuHelp : public TMenu {
public:
    explicit TMenuHelp(TMainWindow* mw);
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_HELPMENU_H

#ifndef GUI_HELPMENU_H
#define GUI_HELPMENU_H

#include "gui/action/menu.h"

namespace Gui {
namespace Action {

class TAction;


class TMenuHelp : public TMenu {
public:
	explicit TMenuHelp(QWidget* parent);
};

} // namespace Action
} // namespace Gui

#endif // GUI_HELPMENU_H

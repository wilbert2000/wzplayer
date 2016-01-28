#ifndef GUI_OPTIONSMENU_H
#define GUI_OPTIONSMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {
namespace Action {

class TAction;

class TMenuOptions : public TMenu {
public:
	TMenuOptions(QWidget* parent,
				 TCore* core,
				 QMenu* toolBarMenu,
				 QWidget* playlist,
				 QWidget* logWindow);
};

} // namespace Action
} // namespace Gui

#endif // GUI_OPTIONSMENU_H

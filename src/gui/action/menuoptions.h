#ifndef GUI_OPTIONSMENU_H
#define GUI_OPTIONSMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;

class TMenuOptions : public TMenu {
public:
	TMenuOptions(QWidget* parent,
				 TCore* core,
				 QMenu* toolBarMenu,
				 QWidget* playlist,
				 QWidget* logWindow);
};

} // namespace Gui

#endif // GUI_OPTIONSMENU_H

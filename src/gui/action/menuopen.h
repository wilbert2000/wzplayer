#ifndef GUI_OPENMENU_H
#define GUI_OPENMENU_H

#include "gui/action/menu.h"

class TCore;

namespace Gui {

class TAction;
class TBase;

class TOpenMenu : public TMenu {
	Q_OBJECT
public:
	explicit TOpenMenu(TBase* parent, TCore* core, QWidget* playlist);
	void updateRecents();
private:
	TBase* main_window;
	TMenu* recentfiles_menu;
	TAction* clearRecentsAct;
private slots:
	void clearRecentsList();
};

} // namespace Gui

#endif // GUI_OPENMENU_H

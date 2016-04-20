#ifndef GUI_OPENMENU_H
#define GUI_OPENMENU_H

#include "gui/action/menu.h"


namespace Gui {

class TBase;

namespace Action {

class TAction;


class TMenuOpen : public TMenu {
	Q_OBJECT
public:
    explicit TMenuOpen(TBase* mw, QWidget* playlist);
	void updateRecents();
private:
	TMenu* recentfiles_menu;
	TAction* clearRecentsAct;
private slots:
	void clearRecentsList();
}; // class TmenuOpen

} // namespace Action
} // namespace Gui

#endif // GUI_OPENMENU_H

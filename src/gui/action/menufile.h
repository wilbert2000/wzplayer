#ifndef GUI_ACTION_MENUFILE_H
#define GUI_ACTION_MENUFILE_H

#include "gui/action/menu.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;


class TMenuFile : public TMenu {
    Q_OBJECT
public:
    explicit TMenuFile(TMainWindow* mw);
    void updateRecents();

private:
    TMenu* recentfiles_menu;
    TAction* clearRecentsAct;

private slots:
    void clearRecentsList();
}; // class TmenuOpen

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUFILE_H

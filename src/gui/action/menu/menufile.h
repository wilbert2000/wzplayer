#ifndef GUI_ACTION_MENU_MENUFILE_H
#define GUI_ACTION_MENU_MENUFILE_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;

namespace Menu {

class TMenuFile : public TMenu {
    Q_OBJECT
public:
    explicit TMenuFile(QWidget* parent, TMainWindow* mw, TMenu* favMenu);

private:
    TMenu* recentFilesMenu;
    void updateRecents();

private slots:
    void onRecentsChanged();
    void onSettingsChanged();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUFILE_H

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
    explicit TMenuFile(TMainWindow* mw);
    virtual ~TMenuFile();
    void updateRecents();

protected slots:
    virtual void enableActions();

private:
    TMenu* recentfiles_menu;
    TAction* clearRecentsAct;
    TAction* saveThumbnailAct;

private slots:
    void clearRecentsList();
    void onSettingsChanged();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUFILE_H

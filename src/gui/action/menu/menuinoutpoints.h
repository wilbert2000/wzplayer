#ifndef GUI_ACTION_MENU_MENUINOUTPOINTS_H
#define GUI_ACTION_MENU_MENUINOUTPOINTS_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;

namespace Menu {

class TMenuInOut : public TMenu {
    Q_OBJECT
public:
    explicit TMenuInOut(TMainWindow* mw);

protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();

private:
    QActionGroup* group;
    TAction* repeatInOutAct;

private slots:
    void upd();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUINOUTPOINTS_H

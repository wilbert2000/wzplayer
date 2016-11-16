#ifndef GUI_ACTION_MENU_MENUASPECT_H
#define GUI_ACTION_MENU_MENUASPECT_H

#include "gui/action/menu/menu.h"


namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuAspect : public TMenu {
    Q_OBJECT
public:
    explicit TMenuAspect(TMainWindow* mw);

protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();

private:
    TActionGroup* group;
    TAction* aspectAutoAct;
    TAction* aspectDisabledAct;
    TAction* nextAspectAct;

    void upd();

private slots:
    void onAspectRatioChanged(int);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUASPECT_H

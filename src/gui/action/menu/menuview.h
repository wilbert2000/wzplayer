#ifndef GUI_ACTION_MENU_MENUVIEW_H
#define GUI_ACTION_MENU_MENUVIEW_H

#include "gui/action/menu/menu.h"
#include "gui/action/actiongroup.h"


namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TOSDGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TOSDGroup(TMainWindow* mw);
};


class TToggleStayOnTopAction : public TAction {
    Q_OBJECT
public:
    explicit TToggleStayOnTopAction(TMainWindow* mw);
private slots:
    void onStayOnTopChanged(int stayOnTop);
};

class TStayOnTopGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TStayOnTopGroup(TMainWindow* mw);
};

class TMenuStayOnTop : public TMenu {
    Q_OBJECT
public:
    explicit TMenuStayOnTop(QWidget* parent, TMainWindow* mw);
};


class TMenuView : public TMenu {
    Q_OBJECT
public:
    explicit TMenuView(QWidget* parent, TMainWindow* mw, QMenu* toolBarMenu);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIEW_H

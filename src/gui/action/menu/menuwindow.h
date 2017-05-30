#ifndef GUI_ACTION_MENU_MENUWINDOW_H
#define GUI_ACTION_MENU_MENUWINDOW_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;
class TDockWidget;
class TAutoHideTimer;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuStayOnTop : public TMenu {
    Q_OBJECT
public:
    explicit TMenuStayOnTop(TMainWindow* mw);
    // Group to enable/disable together
    TActionGroup* group;

protected:
    virtual void onAboutToShow();

private:
    TAction* toggleStayOnTopAct;

private slots:
    void onTriggered(QAction* action);
};

class TMenuWindow : public TMenu {
public:
    TMenuWindow(TMainWindow* mw,
                QMenu* toolBarMenu,
                TDockWidget* playlistDock,
                TDockWidget* logDock,
                TAutoHideTimer* autoHideTimer);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUWINDOW_H

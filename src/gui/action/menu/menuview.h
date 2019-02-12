#ifndef GUI_ACTION_MENU_MENUVIEW_H
#define GUI_ACTION_MENU_MENUVIEW_H

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

class TMenuView : public TMenu {
    Q_OBJECT
public:
    explicit TMenuView(TMainWindow* mw,
                       QMenu* toolBarMenu,
                       TDockWidget* playlistDock,
                       TDockWidget* logDock,
                       TAutoHideTimer* autoHideTimer);

protected slots:
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*) override;

private:
    TAction* propertiesAct;
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIEW_H

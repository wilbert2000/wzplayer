#ifndef GUI_ACTION_MENUWINDOW_H
#define GUI_ACTION_MENUWINDOW_H

#include "gui/action/menu.h"


namespace Gui {

class TBase;

namespace Action {

class TAction;
class TActionGroup;

class TMenuStayOnTop : public TMenu {
    Q_OBJECT
public:
    explicit TMenuStayOnTop(TBase* mw);
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
    TMenuWindow(TBase* mw,
                QMenu* toolBarMenu,
                QWidget* playlist,
                QWidget* logWindow);
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUWINDOW_H

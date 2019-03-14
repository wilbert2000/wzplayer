#ifndef GUI_ACTION_MENU_MENU_H
#define GUI_ACTION_MENU_MENU_H

#include "gui/action/menu/menuexec.h"


namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TMenu : public TMenuExec {
    Q_OBJECT
public:
    explicit TMenu(QWidget* parent,
                   TMainWindow* mw,
                   const QString& name,
                   const QString& text,
                   const QString& icon = QString());

protected:
    TMainWindow* main_window;
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENU_H

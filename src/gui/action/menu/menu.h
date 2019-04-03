#ifndef GUI_ACTION_MENU_MENU_H
#define GUI_ACTION_MENU_MENU_H

#include "gui/action/menu/menuexec.h"


namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TMenu : public TMenuExec {
public:
    explicit TMenu(QWidget* parent,
                   const QString& name,
                   const QString& text,
                   const QString& icon = QString());
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENU_H

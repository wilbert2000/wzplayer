#ifndef GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H
#define GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H

#include "gui/action/actiongroup.h"
#include "gui/action/menu/menu.h"

namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TColorSpaceGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TColorSpaceGroup(TMainWindow* mw);
};

class TMenuVideoColorSpace : public TMenu {
    Q_OBJECT
public:
    TMenuVideoColorSpace(QWidget* parent, TMainWindow* mw);
}; // class TMenuVideoColorSpace

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H

#ifndef GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H
#define GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H

#include "log4qt/logger.h"
#include "gui/action/actiongroup.h"
#include "gui/action/menu/menu.h"

namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {

class TColorSpaceGroup : public TActionGroup {
    Q_OBJECT

public:
    explicit TColorSpaceGroup(QWidget* parent);
};

class TMenuVideoColorSpace : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuVideoColorSpace(TMainWindow* mw);

protected:
    virtual void enableActions() override;
    virtual void onMediaSettingsChanged(Settings::TMediaSettings* m) override;

private:
    TColorSpaceGroup* group;
}; // class TMenuVideoColorSpace

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIDEOCOLORSPACE_H

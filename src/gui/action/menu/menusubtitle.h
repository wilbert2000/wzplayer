#ifndef GUI_ACTION_MENU_MENUSUBTITLE_H
#define GUI_ACTION_MENU_MENUSUBTITLE_H

#include "gui/action/menu/menu.h"
#include "gui/action/actiongroup.h"
#include "log4qt/logger.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;

namespace Menu {

class TClosedCaptionsGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TClosedCaptionsGroup(TMainWindow* mw);
};

class TSubFPSGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TSubFPSGroup(TMainWindow* mw);
};

class TMenuSubtitle : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuSubtitle(QWidget* parent, TMainWindow* mw);

private:
    TMenu* subtitleTrackMenu;
    TMenu* secondarySubtitleTrackMenu;

private slots:
    void subtitleTrackGroupsChanged(Action::TAction* next,
                                    Action::TActionGroup* subGroup,
                                    Action::TActionGroup* secSubGroup);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUSUBTITLE_H

#ifndef GUI_ACTION_MENU_MENUVIDEOTRACKS_H
#define GUI_ACTION_MENU_MENUVIDEOTRACKS_H

#include "gui/action/menu/menu.h"
#include "log4qt/logger.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuVideoTracks : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit TMenuVideoTracks(TMainWindow* mw);
protected:
    virtual void enableActions();
private:
    TAction* nextVideoTrackAct;
    TActionGroup* videoTrackGroup;
private slots:
    void updateVideoTracks();
}; // class TMenuVideoTracks

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIDEOTRACKS_H

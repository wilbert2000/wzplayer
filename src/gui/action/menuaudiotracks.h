#ifndef GUI_ACTION_MENUAUDIOTRACKS_H
#define GUI_ACTION_MENUAUDIOTRACKS_H

#include "gui/action/menu.h"
#include "log4qt/logger.h"


namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

class TMenuAudioTracks : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit TMenuAudioTracks(TMainWindow* mw);
protected:
    virtual void enableActions();
private:
    TAction* nextAudioTrackAct;
    TActionGroup* audioTrackGroup;
private slots:
    void updateAudioTracks();
}; // class TMenuAudioTracks

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUAUDIOTRACKS_H

#ifndef GUI_ACTION_MENU_MENUSUBTITLE_H
#define GUI_ACTION_MENU_MENUSUBTITLE_H

#include "gui/action/menu/menu.h"
#include "log4qt/logger.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuSubFPS : public TMenu {
public:
    explicit TMenuSubFPS(TMainWindow* mw);
    TActionGroup* group;
protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
    virtual void onAboutToShow();
private:
    friend class TMenuSubtitle;
};

class TMenuSubtitle : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuSubtitle(TMainWindow* mw);

protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
    TAction* decSubPosAct;
    TAction* incSubPosAct;
    TAction* decSubScaleAct;
    TAction* incSubScaleAct;

    TAction* decSubDelayAct;
    TAction* incSubDelayAct;
    TAction* subDelayAct;

    TAction* incSubStepAct;
    TAction* decSubStepAct;

    TAction* seekNextSubAct;
    TAction* seekPrevSubAct;

    TAction* nextSubtitleAct;
    TMenu* subtitleTrackMenu;
    TActionGroup* subtitleTrackGroup;
    TMenu* secondarySubtitleTrackMenu;
    TActionGroup* secondarySubtitleTrackGroup;
    TAction* useForcedSubsOnlyAct;

    TAction* loadSubsAct;
    TAction* unloadSubsAct;
    TMenuSubFPS* subFPSMenu;

    TAction* useCustomSubStyleAct;

private slots:
    void updateSubtitles();
    void onPreferencesChanged();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUSUBTITLE_H

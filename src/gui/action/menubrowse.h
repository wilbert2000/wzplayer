#ifndef GUI_BROWSEMENU_H
#define GUI_BROWSEMENU_H

#include "gui/action/menu.h"
#include "log4qt/logger.h"

class TCore;

namespace Gui {
namespace Action {

class TAction;
class TActionGroup;


class TMenuBrowse : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuBrowse(TBase* mw, TCore* c);

protected:
    virtual void enableActions();

private:
    TCore* core;

    TMenu* titlesMenu;
    TActionGroup* titleGroup;

    TAction* prevChapterAct;
    TAction* nextChapterAct;
    TMenu* chaptersMenu;
    TActionGroup* chapterGroup;

    TAction* nextAngleAct;
    TMenu* anglesMenu;
    TActionGroup* angleGroup;

#if PROGRAM_SWITCH
    TMenu* programMenu;
    TActionGroup* programGroup;
#endif

    TAction* dvdnavUpAct;
    TAction* dvdnavDownAct;
    TAction* dvdnavLeftAct;
    TAction* dvdnavRightAct;

    TAction* dvdnavMenuAct;
    TAction* dvdnavPrevAct;
    TAction* dvdnavSelectAct;
    TAction* dvdnavMouseAct;

private slots:
    void updateTitles();
    void updateChapters();
    void updateAngles();
}; // class TMenuBrowse

} // namespace Action
} // namespace Gui

#endif // GUI_BROWSEMENU_H

#ifndef GUI_ACTION_MENU_MENUBROWSE_H
#define GUI_ACTION_MENU_MENUBROWSE_H

#include "gui/action/menu/menu.h"
#include "log4qt/logger.h"


namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuBrowse : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuBrowse(TMainWindow* mw);

protected:
    virtual void enableActions();

private:
    TMenu* titlesMenu;
    TActionGroup* titleGroup;

    TAction* prevChapterAct;
    TAction* nextChapterAct;
    TMenu* chaptersMenu;
    TActionGroup* chapterGroup;

    TAction* nextAngleAct;
    TMenu* anglesMenu;
    TActionGroup* angleGroup;

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

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUBROWSE_H

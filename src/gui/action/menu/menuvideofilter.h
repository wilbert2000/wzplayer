#ifndef GUI_ACTION_MENU_MENUVIDEOFILTER_H
#define GUI_ACTION_MENU_MENUVIDEOFILTER_H

#include "gui/action/menu/menu.h"
#include <QActionGroup>


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TFilterGroup : public QActionGroup {
    Q_OBJECT
public:
    explicit TFilterGroup(TMainWindow* mw);

    TAction* addLetterboxAct;
    void updateFilters();
private:
    TAction* postProcessingAct;
    TAction* deblockAct;
    TAction* deringAct;
    TAction* gradfunAct;
    TAction* addNoiseAct;
    TAction* softwareScalingAct;
    TAction* phaseAct;
};


class TMenuVideoFilter : public TMenu {
public:
    explicit TMenuVideoFilter(QWidget* parent, TMainWindow* mw);
};

} // namespace Menu
} // namespace Action
} // namespace GUI

#endif // GUI_ACTION_MENU_MENUVIDEOFILTER_H

#ifndef GUI_ACTION_MENUINOUTPOINTS_H
#define GUI_ACTION_MENUINOUTPOINTS_H

#include "gui/action/menu.h"


namespace Gui {

class TBase;

namespace Action {

class TAction;


class TMenuInOut : public TMenu {
    Q_OBJECT
public:
    explicit TMenuInOut(TBase* mw);

protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();

private:
    QActionGroup* group;
    TAction* repeatInOutAct;

private slots:
    void upd();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUINOUTPOINTS_H

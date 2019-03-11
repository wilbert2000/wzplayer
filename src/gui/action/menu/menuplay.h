#ifndef GUI_ACTION_MENU_MENUPLAY_H
#define GUI_ACTION_MENU_MENUPLAY_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;

namespace Menu {

class TMenuSeek: public TMenu {
    Q_OBJECT
public:
    explicit TMenuSeek(TMainWindow* mnw,
                       const QString& name,
                       const QString& text,
                       int seekIntOffset);

public slots:
    void updateDefaultAction(QAction* action);
signals:
    void defaultActionChanged(QAction* action);
};


class TMenuPlay : public TMenu {
    Q_OBJECT
public:
    explicit TMenuPlay(TMainWindow* mw);

protected:
    virtual void enableActions();

private:
    TAction* seekToAct;
}; // class TMenuPlay

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUPLAY_H

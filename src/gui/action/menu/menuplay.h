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
    explicit TMenuSeek(QWidget* parent,
                       TMainWindow* mnw,
                       const QString& name,
                       const QString& text,
                       int seekIntOffset);
public slots:
    void updateDefaultAction(QAction* action);
signals:
    void defaultActionChanged(QAction* action);
};


class TMenuPlay : public TMenu {
public:
    explicit TMenuPlay(QWidget* parent, TMainWindow* mw);
}; // class TMenuPlay

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUPLAY_H

#ifndef GUI_ACTION_MENU_MENUPLAY_H
#define GUI_ACTION_MENU_MENUPLAY_H

#include "gui/action/menu/menu.h"
#include "player/state.h"
#include <QActionGroup>


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


class TPlaySpeedGroup : public QActionGroup {
    Q_OBJECT
public:
    explicit TPlaySpeedGroup(TMainWindow* mw);
private slots:
    void onPlayerStateChanged(Player::TState state);
};


class TInOutGroup : public QActionGroup {
    Q_OBJECT
public:
    explicit TInOutGroup(TMainWindow* mw);
private:
    TAction* repeatInOutAct;
private slots:
    void onPlayerStateChanged(Player::TState state);
    void onRepeatInOutChanged();
};


class TMenuPlay : public TMenu {
public:
    explicit TMenuPlay(QWidget* parent, TMainWindow* mw);
}; // class TMenuPlay

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUPLAY_H

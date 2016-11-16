#ifndef GUI_ACTION_MENU_MENUPLAY_H
#define GUI_ACTION_MENU_MENUPLAY_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;

namespace Playlist {
class TPlaylist;
}

namespace Action {

class TAction;

namespace Menu {

class TMenuSeek: public TMenu {
    Q_OBJECT
public:
    explicit TMenuSeek(QWidget* parent,
                       TMainWindow* mainwindow,
                       const QString& name,
                       const QString& text,
                       const QString& sign);

public slots:
    void updateDefaultAction();

protected:
    TAction* frameAct;
    TAction* seek1Act;
    TAction* seek2Act;
    TAction* seek3Act;
    TAction* plAct;

    virtual void enableActions();

    int actionToInt(QAction* action) const;
    TAction* intToAction(int i) const;

protected slots:
    void setJumpTexts();

private:
    QString seek_sign;

    QString timeForJumps(int secs) const;

private slots:
    void onTriggered(QAction* action);
};


class TMenuPlay : public TMenu {
	Q_OBJECT
public:
    explicit TMenuPlay(TMainWindow* mw, Gui::Playlist::TPlaylist* playlist);

protected:
    virtual void enableActions();

private:
    TAction* seekToAct;
}; // class TMenuPlay

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUPLAY_H

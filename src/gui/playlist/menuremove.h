#ifndef GUI_PLAYLIST_MENUREMOVE_H
#define GUI_PLAYLIST_MENUREMOVE_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;

namespace Action {
class TAction;
}

namespace Playlist {

class TPlaylist;

class TMenuRemove : public Gui::Action::Menu::TMenu {
    Q_OBJECT
public:
    explicit TMenuRemove(TPlaylist* pl, TMainWindow* mw);

protected:
    virtual void enableActions();
    virtual void onAboutToShow();

private:
    TPlaylist* playlist;
    Gui::Action::TAction* removeSelectedAct;
    Gui::Action::TAction* removeSelectedFromDiskAct;
    Gui::Action::TAction* removeAllAct;

private slots:
    void enableRemoveFromDiskAction();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_MENUREMOVE_H

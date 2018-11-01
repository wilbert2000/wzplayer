#ifndef GUI_PLAYLIST_MENUCONTEXT_H
#define GUI_PLAYLIST_MENUCONTEXT_H


#include "gui/action/menu/menu.h"


namespace Gui {

namespace Action {
class TAction;
}

namespace Playlist {

class TPlaylist;


class TMenuContext : public Gui::Action::Menu::TMenu {
    Q_OBJECT
public:
    explicit TMenuContext(TPlaylist* pl, TMainWindow* mw);

protected:
    virtual void enableActions();
    virtual void onAboutToShow();

private:
    TPlaylist* playlist;
    Gui::Action::TAction* editNameAct;
    Gui::Action::TAction* newFolderAct;
    Gui::Action::TAction* findPlayingAct;

    Gui::Action::TAction* cutAct;
    Gui::Action::TAction* copyAct;
    Gui::Action::TAction* pasteAct;

private slots:
    void enablePaste();
};


} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_MENUCONTEXT_H

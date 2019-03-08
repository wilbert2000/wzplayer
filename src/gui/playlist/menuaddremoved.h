#ifndef GUI_PLAYLIST_MENUADDREMOVED_H
#define GUI_PLAYLIST_MENUADDREMOVED_H

#include "gui/action/menu/menu.h"


class QTreeWidgetItem;

namespace Gui {

class TMainWindow;

namespace Playlist {

class TPlaylist;
class TPlaylistWidget;
class TPlaylistItem;


class TMenuAddRemoved : public Gui::Action::Menu::TMenu {
    Q_OBJECT
public:
    explicit TMenuAddRemoved(TPlaylist* playlist, TMainWindow* w,
                             TPlaylistWidget* plw);
    virtual ~TMenuAddRemoved();

signals:
    void addRemovedItem(QString s);

protected:
    virtual void onAboutToShow() override;

private:
    TPlaylistWidget* playlistWidget;
    TPlaylistItem* item;

private slots:
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*);
    void onTriggered(QAction* action);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_MENUADDREMOVED_H

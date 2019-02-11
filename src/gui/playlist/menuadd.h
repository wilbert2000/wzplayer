#ifndef GUI_PLAYLIST_MENUADD_H
#define GUI_PLAYLIST_MENUADD_H

#include "gui/action/menu/menu.h"
#include "wzdebug.h"


class QTreeWidgetItem;

namespace Gui {

class TMainWindow;

namespace Action {
class TAction;
}

namespace Playlist {

class TPlaylist;
class TPlaylistWidget;
class TPlaylistWidgetItem;


class TAddRemovedMenu : public Gui::Action::Menu::TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TAddRemovedMenu(QWidget* parent, TMainWindow* w,
                             TPlaylist* playlist);
    virtual ~TAddRemovedMenu();

signals:
    void addRemovedItem(QString s);

protected:
    virtual void onAboutToShow() override;

private:
    TPlaylistWidget* playlistWidget;
    TPlaylistWidgetItem* item;

private slots:
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*);
    void onTriggered(QAction* action);
};


class TMenuAdd : public Gui::Action::Menu::TMenu {
public:
    explicit TMenuAdd(QWidget* parent, TMainWindow* w, TPlaylist* playlist);
    virtual ~TMenuAdd();
};


} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_MENUADD_H

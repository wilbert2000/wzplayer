#ifndef GUI_PLAYLIST_FAVLIST_H
#define GUI_PLAYLIST_FAVLIST_H

#include "gui/playlist/plist.h"
#include "wzdebug.h"
#include <QTime>


class QTreeWidgetItem;
class QTimer;

namespace Gui {
namespace Action {
namespace Menu {
class TMenu;
}
}
namespace Playlist {

class TPlaylist;

class TFavList : public TPList {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TFavList(TDockWidget* parent, TMainWindow* mw, TPlaylist* playlst);

    virtual void startPlay() override;
    Action::Menu::TMenu* getFavMenu() const { return favMenu; }
    void loadSettings();
    void saveSettings();

public slots:
    virtual void enableActions() override;
    virtual void findPlayingItem() override;

protected:
    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) override;

protected slots:
    virtual bool saveAs() override;
    virtual void refresh() override;

private:
    TPlaylist* playlist;
    Action::Menu::TMenu* favMenu;
    QAction* currentFavAction;
    QIcon currentFavIcon;
    QTimer* requestUpdateTimer;
    QTime requestAge;

    QAction* fAction(QMenu* menu, const QString& filename) const;
    QAction* findAction(const QString& filename) const;
    void markCurrentFavAction(QAction* action);
    void updFavMenu(QMenu* menu, TPlaylistItem* folder);
    void requestUpdate();

private slots:
    void onRequestUpdateTimeout();
    void onAddedItems();
    void onModifiedChanged();
    void onPlaylistPlayingItemChanged(TPlaylistItem* item);
    void onFavMenuTriggered(QAction* action);
    void updateFavMenu();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_FAVLIST_H


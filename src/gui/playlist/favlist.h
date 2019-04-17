#ifndef GUI_PLAYLIST_FAVLIST_H
#define GUI_PLAYLIST_FAVLIST_H

#include "gui/playlist/plist.h"
#include "wzdebug.h"
#include <QIcon>


class QTreeWidgetItem;
class QTimer;
class TWZTimer;

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
    explicit TFavList(TDockWidget* parent, TPlaylist* playlst);

    Action::Menu::TMenu* getFavMenu() const { return favMenu; }

public slots:
    virtual void enableActions() override;
    virtual bool findPlayingItem() override;

protected:
    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) override;

protected slots:
    virtual void openPlaylistDialog() override;
    virtual bool saveAs() override;
    virtual void refresh() override;
    virtual void removeAll() override;

private:
    bool loaded;
    TPlaylist* playlist;
    QAction* loadFavoritesAction;
    QAction* currentFavAction;
    Action::Menu::TMenu* favMenu;
    QIcon currentFavIcon;
    TWZTimer* updateTimer;
    TWZTimer* saveTimer;
    TWZTimer* updatePlayingItemTimer;

    void createToolbar();
    QAction* fAction(QMenu* menu, const QString& filename) const;
    QAction* findAction(const QString& filename) const;
    void markCurrentFavAction(QAction* action);
    void updFavMenu(QMenu* menu, TPlaylistItem* folder);
    void requestUpdate();

private slots:
    void loadFavorites();
    void onDockToggled(bool visible);
    void onSaveTimerTimeout();
    void onUpdateTimerTimeout();
    void onAddedItems();
    void onModifiedChanged();
    void updatePlayingItem();
    void onFavMenuTriggered(QAction* action);
    void updateFavMenu();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_FAVLIST_H


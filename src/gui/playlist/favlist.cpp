#include "gui/playlist/favlist.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/menu/menu.h"
#include "gui/action/action.h"
#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "player/player.h"
#include "images.h"

#include <QDir>


namespace Gui {
namespace Playlist {

TFavList::TFavList(TDockWidget *parent, TMainWindow* mw, TPlaylist* playlst) :
    TPList(parent, mw, "favlist", "fav", tr("Favorites")),
    playlist(playlst),
    currentFavAction(0) {

    mainWindow->addAction(addPlayingFileAct);

    favMenu = new Action::Menu::TMenu(this, mw, "favorites_menu",
                                      tr("Favorites"));
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());

    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::playingItemChanged,
            this, &TFavList::onPlaylistPlayingItemChanged);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TFavList::onModifiedChanged,
            Qt::QueuedConnection);

    if (!QDir().mkpath(Settings::TPaths::favoritesPath())) {
        WZERROR(QString("Failed to create favorites directory '%1'. %2")
                .arg(Settings::TPaths::favoritesPath())
                .arg(strerror(errno)));
    }
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
}

void TFavList::enableActions() {

    if (disableEnableActions) {
        return;
    }
    disableEnableActions++;

    WZTRACE("State " + player->stateToString());
    TPList::enableActions();
    findPlayingAct->setEnabled(!player->mdat.filename.isEmpty());

    disableEnableActions--;
}

// Currently not used
void TFavList::startPlay() {
    WZDEBUG("");
    playlist->startPlay();
}

void TFavList::playItem(TPlaylistItem* item, bool keepPaused) {

    if (item) {
        // Could ignore keepPaused, only used by playlist playPrev/Next
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
           player->setStartPausedOnce();
        }
        playlist->open(item->filename(), item->baseName());
    }
}

bool TFavList::saveAs() {

    bool result = TPList::saveAs();
    // Restore favorites filename
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
    return result;
}

void TFavList::refresh() {
    WZDEBUG("");

    if (maybeSave()) {
        clear(false);
        if (QFileInfo(Settings::TPaths::favoritesPath()).exists()) {
            add(QStringList() << playlistFilename);
        }
    }
}

void TFavList::findPlayingItem() {

    TPlaylistItem* i = playlistWidget->findFilename(player->mdat.filename);
    if (i) {
        makeActive();
        if (i == playlistWidget->currentItem()) {
            playlistWidget->scrollToItem(i);
        } else {
            playlistWidget->setCurrentItem(i);
        }
    } else {
        playlist->findPlayingItem();
    }
}

QAction* TFavList::fAction(QMenu* menu, const QString& filename) const {

    QList<QAction*> actions = menu->actions();
    for(int i = 0; i < actions.count(); i++) {
        QAction* action = actions.at(i);
        QVariant v = action->data();
        if (v.canConvert<TPlaylistItem*>()) {
            TPlaylistItem* item = v.value<TPlaylistItem*>();
            if (filename == item->filename()) {
                WZDEBUG("Found '" + filename + "'");
                return action;
            }
        } else {

        }
    }

    return 0;
}

QAction* TFavList::findAction(const QString& filename) const {
    return fAction(favMenu, filename);
}

void TFavList::onPlaylistPlayingItemChanged(TPlaylistItem* item) {
    WZTRACE("");

    if (currentFavAction) {
        markCurrentFavAction(0);
    }

    if (item) {
        QAction* action = findAction(item->filename());
        if (action) {
            markCurrentFavAction(action);
        }
    }

    // See if new playing item can be found in fav list
    playlistWidget->setPlayingItem(
                playlistWidget->findFilename(item->filename()),
                PSTATE_PLAYING);
}

void TFavList::onModifiedChanged() {

    if (playlistWidget->isModified()) {
        WZDEBUG("Modified set");
        updateFavMenu();
    } else {
        WZDEBUG("Modified cleared");
    }
}

void TFavList::markCurrentFavAction(QAction* action) {

    if (currentFavAction) {
        QFont f = currentFavAction->font();
        f.setBold(false);
        currentFavAction->setFont(f);
    }
    currentFavAction = action;
    if (currentFavAction) {
        QFont f = currentFavAction->font();
        f.setBold(true);
        currentFavAction->setFont(f);
    }
}

void TFavList::onFavMenuTriggered(QAction* action) {
    WZDEBUG("");

    if (action->data().isValid()) {
        TPlaylistItem* item = action->data().value<TPlaylistItem*>();
        QString filename = item->filename();
        if (!filename.isEmpty()) {
            playlist->open(filename, item->baseName());
            markCurrentFavAction(action);
        }
    }
}

void TFavList::updFavMenu(QMenu* menu, TPlaylistItem* folder) {

    for(int i = 0; i < folder->childCount(); i++) {
        TPlaylistItem* item = folder->plChild(i);
        if (item->isFolder()) {
            QMenu* m = new QMenu(item->baseName(), menu);
            updFavMenu(m, item);
            connect(m, &QMenu::triggered, this, &TFavList::onFavMenuTriggered);
            menu->addMenu(m);
        } else {
            QAction* a = new QAction(item->baseName(), menu);
            a->setData(QVariant::fromValue(item));
            // TODO: a->setIcon
            a->setStatusTip(item->filename());
            if (item == playlistWidget->playingItem) {
                markCurrentFavAction(a);
            }
            menu->addAction(a);
        }
    }
}

void TFavList::updateFavMenu() {
    WZDEBUG("");

    currentFavAction = 0;
    favMenu->clear();
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());

    TPlaylistItem* root = playlistWidget->root();
    if (root && root->childCount()) {
        favMenu->addSeparator();
        updFavMenu(favMenu, root);
    }

    save();
}

void TFavList::saveSettings() {

    Settings::pref->beginGroup(objectName());
    playlistWidget->saveSettings(Settings::pref);
    Settings::pref->endGroup();
}

void TFavList::loadSettings() {

    Settings::pref->beginGroup(objectName());
    playlistWidget->loadSettings(Settings::pref);
    Settings::pref->endGroup();
}

} // namespace Playlist
} // namespace Gui

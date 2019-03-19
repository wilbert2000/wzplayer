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
#include "iconprovider.h"

#include <QDir>
#include <QTimer>


namespace Gui {
namespace Playlist {

TFavList::TFavList(TDockWidget *parent, TMainWindow* mw, TPlaylist* playlst) :
    TPList(parent, mw, "favlist", "fav", tr("Favorites")),
    playlist(playlst),
    currentFavAction(0) {

    playlistWidget->setSort(TPlaylistItem::COL_ORDER, Qt::AscendingOrder);

    mainWindow->addAction(addPlayingFileAct);

    favMenu = new Action::Menu::TMenu(this, mw, "favorites_menu",
                                      tr("Favorites"));
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());

    // Timer to merge the TPlist::addedItems and/or multiple
    // TPlaylistWidget::modified signals into one call to updateFavMenu
    requestUpdateTimer = new QTimer(this);
    requestUpdateTimer->setSingleShot(true);
    requestUpdateTimer->setInterval(250);
    connect(requestUpdateTimer, &QTimer::timeout,
            this, &TFavList::onRequestUpdateTimeout);

    connect(this, &TFavList::addedItems,
            this, &TFavList::onAddedItems);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TFavList::onModifiedChanged);
    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::playingItemChanged,
            this, &TFavList::onPlaylistPlayingItemChanged);

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
                WZTRACE("Found '" + filename + "'");
                return action;
            }
        } else {
            WZERROR("Action without item");
        }
    }

    return 0;
}

QAction* TFavList::findAction(const QString& filename) const {
    return fAction(favMenu, filename);
}

void TFavList::onPlaylistPlayingItemChanged(TPlaylistItem* item) {
    WZTRACE(item ? item->text(TPlaylistItem::COL_NAME) : "null");

    if (currentFavAction) {
        markCurrentFavAction(0);
    }

    TPlaylistItem* playingItem = 0;
    if (item) {
        QAction* action = findAction(item->filename());
        if (action) {
            markCurrentFavAction(action);
            playingItem = action->data().value<TPlaylistItem*>();
        }
    }

    playlistWidget->setPlayingItem(playingItem, PSTATE_PLAYING);
}

void TFavList::onRequestUpdateTimeout() {
    WZDEBUG("");

    if (playlistWidget->isBusy() && requestAge.elapsed() < 1000) {
        requestUpdateTimer->start();
    } else {
        updateFavMenu();
    }
}

void TFavList::requestUpdate() {
    WZDEBUG("");

    currentFavAction = 0;
    if (!requestUpdateTimer->isActive()) {
        requestAge.start();
    }
    requestUpdateTimer->start();
 }

void TFavList::onAddedItems() {
    WZDEBUG("");
    requestUpdate();
}

void TFavList::onModifiedChanged() {

    if (playlistWidget->isModified()) {
        WZDEBUG("Modified set");
        requestUpdate();
    } else {
        WZDEBUG("Modified cleared");
    }
}

void TFavList::markCurrentFavAction(QAction* action) {

    if (action != currentFavAction) {
        if (currentFavAction) {
            currentFavAction->setIcon(currentFavIcon);
        }
        currentFavAction = action;
        if (currentFavAction) {
            currentFavIcon = action->icon();
            currentFavAction->setIcon(iconProvider.iconPlaying);
        }
    }
}

void TFavList::onFavMenuTriggered(QAction* action) {
    WZDEBUG("");

    if (action->data().isValid()) {
        TPlaylistItem* item = action->data().value<TPlaylistItem*>();
        QString filename = item->filename();
        if (!filename.isEmpty()) {
            markCurrentFavAction(action);
            playlist->open(filename, item->baseName());
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
            a->setIcon(item->getItemIcon());
            a->setStatusTip(item->filename());
            if (item->filename() == player->mdat.filename) {
                markCurrentFavAction(a);
                playlistWidget->setPlayingItem(item, PSTATE_PLAYING);
            }
            menu->addAction(a);
        }
    }
}

void TFavList::updateFavMenu() {
    WZDEBUG("");

    currentFavAction = 0;
    playlistWidget->setPlayingItem(0);
    favMenu->clear();
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());

    TPlaylistItem* root = playlistWidget->root();
    if (root && root->childCount()) {
        favMenu->addSeparator();
        updFavMenu(favMenu, root);
    }

    if (playlistWidget->isModified()) {
        save();
    }
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

#include "gui/playlist/favlist.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/menu/menu.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "player/player.h"
#include "images.h"
#include "iconprovider.h"
#include "wztimer.h"
#include "wzdebug.h"

#include <QDir>
#include <QTimer>
#include <QMessageBox>


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TFavList)

namespace Gui {
namespace Playlist {

TFavList::TFavList(TDockWidget *parent, TPlaylist* playlst) :
    TPList(parent, "favlist", "fav", tr("Favorites")),
    loaded(false),
    playlist(playlst),
    currentFavAction(0) {

    playlistWidget->setSort(TPlaylistItem::COL_ORDER, Qt::AscendingOrder);

    // Repeat and shuffle
    repeatAct = mainWindow->requireAction("pl_repeat");
    shuffleAct = mainWindow->requireAction("pl_shuffle");

    // Load favorites action
    loadFavoritesAction = new Action::TAction(this, "fav_load",
                                              tr("Load favorites"), "noicon");
    loadFavoritesAction->setIcon(iconProvider.openIcon);
    connect(loadFavoritesAction, &Action::TAction::triggered,
            this, &TFavList::loadFavorites);
    // Load favorites when loading of media done
    mainWindow->runActionsLater("fav_load", false, true);
    // Load favorites when dock becomes visible
    connect(dock->toggleViewAction(), &QAction::toggled,
            this, &TFavList::onDockToggled);

    createToolbar();

    favMenu = new Action::Menu::TMenu(this, "favorites_menu", tr("Favorites"));
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());
    connect(favMenu, &Action::Menu::TMenu::triggered,
            this, &TFavList::onFavMenuTriggered);
    // Load favorites when menu shown
    connect(favMenu, &Action::Menu::TMenu::aboutToShow,
            this, &TFavList::loadFavorites);

    // Timers to merge TPlist::addedItems and/or multiple
    // TPlaylistWidget::modified signals
    updateTimer = new TWZTimer(this, "fav_update_timer");
    updateTimer->setSingleShot(true);
    updateTimer->setInterval(0);
    connect(updateTimer, &TWZTimer::timeout,
            this, &TFavList::onUpdateTimerTimeout);

    // Timer to save favorites
    saveTimer = new TWZTimer(this, "fav_save_timer");
    saveTimer->setSingleShot(true);
    saveTimer->setInterval(10000);
    connect(saveTimer, &TWZTimer::timeout,
            this, &TFavList::onSaveTimerTimeout);

    connect(playlistWidget, &TPlaylistWidget::addedItems,
            this, &TFavList::onAddedItems);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TFavList::onModifiedChanged);

    // Timer to merge multiple updates of playing item from playlist
    updatePlayingItemTimer = new TWZTimer(this, "fav_update_playing_item_timer");
    updatePlayingItemTimer->setSingleShot(true);
    updatePlayingItemTimer->setInterval(250);
    connect(updatePlayingItemTimer, &QTimer::timeout,
            this, &TFavList::updatePlayingItem);
    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::playingItemUpdated,
            updatePlayingItemTimer, &TWZTimer::logStart);

    if (!QDir().mkpath(Settings::TPaths::favoritesPath())) {
        WZERROR(QString("Failed to create favorites directory '%1'. %2")
                .arg(Settings::TPaths::favoritesPath())
                .arg(strerror(errno)));
    }
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
}

void TFavList::createToolbar() {

    QStringList actions;
    actions << "fav_open"
            << "fav_saveas"
            << "separator"
            << "fav_add_menu"
            << "fav_remove_menu"
            << "separator"
            << "fav_refresh"
            << "fav_browse_dir";
    toolbar->setDefaultActions(actions);
}

void TFavList::enableActions() {

    if (disableEnableActions) {
        WZTRACE("Disabled");
        return;
    }
    disableEnableActions++;

    WZTRACE("State " + player->stateToString());
    TPList::enableActions();
    findPlayingAct->setEnabled(!player->mdat.filename.isEmpty());

    disableEnableActions--;
}

void TFavList::playItem(TPlaylistItem* item, bool keepPaused) {

    if (item) {
        reachedEndOfPlaylist = false;
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
           player->setStartPausedOnce();
        }
        playlist->open(item->filename(), item->baseName());
    } else {
        reachedEndOfPlaylist = true;
    }
}

void TFavList::openPlaylistDialog() {

    if (playlistWidget->hasPlayableItems()) {
        int res = QMessageBox::question(this, tr("Import favorites"),
            tr("This will import a new favorites playlist, overwriting"
               " your current favorites.\n"
               "Are you sure you want to proceed?"),
            QMessageBox::Yes,
            QMessageBox::No | QMessageBox::Default | QMessageBox::Escape);
        if (res != QMessageBox::Yes) {
            return;
        }
    }
    TPList::openPlaylistDialog();
}

bool TFavList::saveAs() {

    bool result = TPList::saveAs();
    // Restore favorites filename
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
    return result;
}

void TFavList::refresh() {
    WZTRACE("");

    if (maybeSave()) {
        loaded = true;
        clear(false);
        if (QFileInfo(Settings::TPaths::favoritesPath()).exists()) {
            playlistWidget->addFiles(QStringList() << playlistFilename);
        }
    }
}

void TFavList::loadFavorites() {

    if (loaded) {
        WZTRACE("Already loaded");
    } else {
        WZTRACE("Loading favorites");
        refresh();
    }
}

void TFavList::onDockToggled(bool visible) {
    WZDEBUG(visible);

    if (visible) {
        loadFavorites();
    }
}

bool TFavList::findPlayingItem() {

    if (TPList::findPlayingItem()) {
        return true;
    } else {
        return playlist->findPlayingItem();
    }
}

void TFavList::removeAll() {

    if (QMessageBox::question(
                this, tr("Confirm clear favorites"),
                tr("Favorites are automatically saved to disk. Clearing them"
                   " will remove your favorites from '%1'.\n"
                   "Are you sure you want to proceed?")
                .arg(playlistFilename),
                QMessageBox::Yes,
                QMessageBox::No | QMessageBox::Default | QMessageBox::Escape)
            == QMessageBox::Yes) {
        TPList::removeAll();
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
        }
    }

    return 0;
}

QAction* TFavList::findAction(const QString& filename) const {
    return fAction(favMenu, filename);
}

void TFavList::updatePlayingItem() {

    if (currentFavAction) {
        markCurrentFavAction(0);
    }

    TPlaylistItem* playingItem = 0;
    TPlaylistItemState state = PSTATE_STOPPED;
    TPlaylistItem* item = playlist->getPlaylistWidget()->playingItem;
    if (item) {
        QAction* action = findAction(item->filename());
        if (action) {
            markCurrentFavAction(action);
            playingItem = action->data().value<TPlaylistItem*>();
            state = item->state();
            WZT << "Found" << item->editName() << "in favorites";
        }
    }

    playlistWidget->setPlayingItem(playingItem, state);
}

void TFavList::onSaveTimerTimeout() {
    WZT;

    if (playlistWidget->isModified()) {
        if (isBusy() || playlistWidget->isEditing()) {
            saveTimer->logStart();
        } else {
            setPlaylistFilename(Settings::TPaths::favoritesFilename());
            save(false);
        }
    }
}

void TFavList::onUpdateTimerTimeout() {
    WZT;

    if (isBusy()) {
        updateTimer->logStart();
    } else {
        updateFavMenu();
    }
}

void TFavList::requestUpdate() {
    WZT;
    currentFavAction = 0;
    updateTimer->logStart();
 }

void TFavList::onAddedItems() {
    WZT;
    requestUpdate();
}

void TFavList::onModifiedChanged() {

    if (playlistWidget->isModified()) {
        WZTRACE("Modified set");
        requestUpdate();
    } else {
        WZTRACE("Modified cleared");
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
    WZTRACE("");

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

    TPlaylistItem* playingItem = playlist->getPlaylistWidget()->playingItem;
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
            if (playingItem && item->filename() == playingItem->filename()) {
                markCurrentFavAction(a);
                playlistWidget->setPlayingItem(item, playingItem->state());
            }
            menu->addAction(a);
        }
    }
}

void TFavList::updateFavMenu() {
    WZT << "Menu active" << favMenu->isActiveWindow();

    currentFavAction = 0;
    playlistWidget->setPlayingItem(0);

    // Using clear() on an active menu is not appreciated
    if (favMenu->isActiveWindow()) {
        QList<QAction*> actions = favMenu->actions();
        for(int i = actions.count() - 1; i >= 2 ; i--) {
            // Kind of hard to test whether this works...
            QAction* action = actions.at(i);
            favMenu->removeAction(action);
            QMenu* menu = action->menu();
            if (menu) {
                // Assuming the action is owned by the menu
                menu->deleteLater();
            } else {
                action->deleteLater();
            }
        }
    } else {
        favMenu->clear();
        favMenu->addAction(addPlayingFileAct);
        favMenu->addAction(dock->toggleViewAction());
    }

    TPlaylistItem* root = playlistWidget->root();
    if (root && root->childCount()) {
        favMenu->addSeparator();
        updFavMenu(favMenu, root);
    }

    if (playlistWidget->isModified()) {
        saveTimer->logStart();
    }
}

} // namespace Playlist
} // namespace Gui

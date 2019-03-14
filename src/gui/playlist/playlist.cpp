/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/playlistitem.h"
#include "gui/playlist/addfilesthread.h"
#include "gui/playlist/menuaddremoved.h"
#include "gui/mainwindow.h"
#include "gui/multilineinputdialog.h"
#include "gui/action/action.h"
#include "gui/msg.h"
#include "gui/filedialog.h"
#include "player/player.h"
#include "wzfiles.h"
#include "images.h"
#include "extensions.h"
#include "version.h"

#include <QDesktopServices>
#include <QToolBar>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTextCodec>
#include <QMimeData>
#include <QClipboard>
#include <QTimer>


namespace Gui {

using namespace Action;

namespace Playlist {


TPlaylist::TPlaylist(QWidget* parent, TMainWindow* mw) :
    QWidget(parent),
    debug(logger()),
    mainWindow(mw),
    thread(0),
    restartThread(false),
    disableEnableActions(0),
    reachedEndOfPlaylist(false) {

    setObjectName("playlist");
    setAcceptDrops(true);

    createTree();
    createActions();

    connect(player, &Player::TPlayer::newMediaStartedPlaying,
            this, &TPlaylist::onNewMediaStartedPlaying);
    connect(player, &Player::TPlayer::playerError,
            this, &TPlaylist::onPlayerError);
    connect(player, &Player::TPlayer::titleTrackChanged,
            this, &TPlaylist::onTitleTrackChanged);
    connect(player, &Player::TPlayer::mediaEOF,
            this, &TPlaylist::onMediaEOF, Qt::QueuedConnection);
    connect(player, &Player::TPlayer::noFileToPlay,
            this, &TPlaylist::resumePlay, Qt::QueuedConnection);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(toolbar);
    layout->addWidget(playlistWidget);
    setLayout(layout);

    // Random seed
    QTime t;
    t.start();
    qsrand(t.hour() * 3600 + t.minute() * 60 + t.second());
}

TPlaylist::~TPlaylist() {

    // Prevent onThreadFinished handling results
    thread = 0;
}

void TPlaylist::createTree() {

    playlistWidget = new TPlaylistWidget(this);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TPlaylist::onModifiedChanged,
            Qt::QueuedConnection);
    connect(playlistWidget, &TPlaylistWidget::itemActivated,
             this, &TPlaylist::onItemActivated);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TPlaylist::enableActions);
}

void TPlaylist::createActions() {

    // Open playlist
    openPlaylistAct = new TAction(mainWindow, "pl_open",
                                  tr("Open playlist..."), "noicon",
                                  QKeySequence("Ctrl+P"));
    openPlaylistAct->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(openPlaylistAct, &TAction::triggered,
            this, &TPlaylist::openPlaylistDialog);

    // Save playlist
    saveAct = new TAction(mainWindow, "pl_save", tr("Save playlist"), "noicon",
                          QKeySequence("Ctrl+S"));
    saveAct->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(saveAct, &TAction::triggered, this, &TPlaylist::save);

    // SaveAs
    saveAsAct = new TAction(mainWindow, "pl_saveas",
                            tr("Save playlist as..."), "noicon");
    saveAsAct->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));
    connect(saveAsAct, &TAction::triggered, this, &TPlaylist::saveAs);

    // Refresh
    refreshAct = new TAction(mainWindow, "pl_refresh", tr("Refresh playlist"),
                             "noicon", Qt::Key_F5);
    refreshAct->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    connect(refreshAct, &TAction::triggered, this, &TPlaylist::refresh);
    connect(playlistWidget, &TPlaylistWidget::refresh,
            this, &TPlaylist::refresh, Qt::QueuedConnection);

    // Browse directory
    browseDirAct = new TAction(mainWindow, "pl_browse_dir",
                               tr("Browse directory"), "noicon");
    browseDirAct->setIcon(QIcon(style()->standardIcon(QStyle::SP_DirOpenIcon)));
    connect(browseDirAct, &TAction::triggered, this, &TPlaylist::browseDir);

    // Stop
    stopAct = new TAction(mainWindow, "stop", tr("Stop"), "",
                          Qt::Key_MediaStop);
    connect(stopAct, &TAction::triggered, this, &TPlaylist::stop);

    // Play
    playAct = new TAction(mainWindow, "play", tr("Play"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, &TAction::triggered, this, &Playlist::TPlaylist::play);

    // Play/pause
    playOrPauseAct = new TAction(mainWindow, "play_or_pause", tr("Play"),
                                 "play", Qt::Key_Space);
    // Add MCE remote key
    playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playOrPauseAct, &TAction::triggered, this, &TPlaylist::playOrPause);

    // Play in new window
    openInNewWindowAct = new TAction(mainWindow, "play_in_new_window",
                             tr("Play in new window"), "play",
                             Qt::CTRL | Qt::Key_Space);
    connect(openInNewWindowAct, &TAction::triggered,
            this, &TPlaylist::openInNewWindow);

    // Pause
    pauseAct = new TAction(mainWindow, "pause", tr("Pause"), "",
                           QKeySequence("Media Pause")); // MCE remote key
    connect(pauseAct, &TAction::triggered, player, &Player::TPlayer::pause);

    // Play next
    playNextAct = new TAction(mainWindow, "play_next", tr("Play next"), "next",
                              QKeySequence(">"));
    playNextAct->addShortcut(QKeySequence("."));
    playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
    playNextAct->setData(4);
    connect(playNextAct, &TAction::triggered, this, &TPlaylist::playNext);

    // Play prev
    playPrevAct = new TAction(mainWindow, "play_prev", tr("Play previous"),
                              "previous", QKeySequence("<"));
    playPrevAct->addShortcut(QKeySequence(","));
    playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
    playPrevAct->setData(4);
    connect(playPrevAct, &TAction::triggered, this, &TPlaylist::playPrev);

    // Repeat
    repeatAct = new TAction(mainWindow, "pl_repeat", tr("Repeat playlist"),
                            "", Qt::CTRL | Qt::Key_Backslash);
    repeatAct->setCheckable(true);
    connect(repeatAct, &TAction::triggered,
            this, &TPlaylist::onRepeatToggled);

    // Shuffle
    shuffleAct = new TAction(mainWindow, "pl_shuffle", tr("Shuffle playlist"),
                             "shuffle", Qt::ALT | Qt::Key_Backslash);
    shuffleAct->setCheckable(true);
    connect(shuffleAct, &TAction::triggered,
            this, &TPlaylist::onShuffleToggled);


    // Context menu
    Action::Menu::TMenu* contextMenu = new Action::Menu::TMenu(
        this, mainWindow, "pl_context_menu", tr("Playlist context menu"));
    connect(contextMenu, &Action::Menu::TMenu::aboutToShow,
            this, &TPlaylist::enableActions);

    contextMenu->addAction(playAct);
    contextMenu->addAction(openInNewWindowAct);

    contextMenu->addSeparator();
    // Edit name
    editNameAct = new TAction(this, "pl_edit_name", tr("Edit name..."), "",
                              Qt::Key_F2);
    connect(editNameAct, &TAction::triggered,
            playlistWidget, &TPlaylistWidget::editName);
    contextMenu->addAction(editNameAct);
    addAction(editNameAct);

    // New folder
    newFolderAct = new TAction(this, "pl_new_folder", tr("New folder"), "noicon",
                               Qt::Key_F10);
    newFolderAct->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    connect(newFolderAct, &TAction::triggered, this, &TPlaylist::newFolder);
    contextMenu->addAction(newFolderAct);
    addAction(newFolderAct);

    // Find playing
    findPlayingAct = new TAction(this, "pl_find_playing",
                                 tr("Find playing item"), "", Qt::Key_F3);
    connect(findPlayingAct, &TAction::triggered,
            this, &TPlaylist::findPlayingItem);
    contextMenu->addAction(findPlayingAct);
    mainWindow->addAction(findPlayingAct);


    contextMenu->addSeparator();
    // Cut
    cutAct = new TAction(this, "pl_cut", tr("Cut file name(s)"), "",
                         QKeySequence("Ctrl+X"));
    connect(cutAct, &TAction::triggered, this, &TPlaylist::cut);
    contextMenu->addAction(cutAct);
    addAction(cutAct);

    // Copy
    copyAct = new TAction(this, "pl_copy", tr("Copy file name(s)"), "",
                          QKeySequence("Ctrl+C"));
    connect(copyAct, &TAction::triggered, this, &TPlaylist::copySelected);
    contextMenu->addAction(copyAct);
    mainWindow->addAction(copyAct);

    // Paste
    pasteAct = new TAction(this, "pl_paste", tr("Paste file name(s)"), "",
                           QKeySequence("Ctrl+V"));
    connect(pasteAct, &TAction::triggered, this, &TPlaylist::paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &TPlaylist::enablePaste);
    contextMenu->addAction(pasteAct);
    mainWindow->addAction(pasteAct);

    contextMenu->addSeparator();

    // Add menu
    playlistAddMenu = new Menu::TMenu(this, mainWindow, "pl_add_menu",
                                      tr("Add to playlist"), "noicon");
    playlistAddMenu->menuAction()->setIcon(style()->standardIcon(
                                               QStyle::SP_DialogOkButton));

    // Add playing
    addPlayingFileAct = new TAction(this, "pl_add_playing",
                                    tr("Add playing file"));
    playlistAddMenu->addAction(addPlayingFileAct);
    connect(addPlayingFileAct, &TAction::triggered,
            this, &TPlaylist::addPlayingFile);

    // Add files
    TAction* a = new TAction(this, "pl_add_files", tr("Add file(s)..."),
                             "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPlaylist::addFilesDialog);

    // Add dir
    a = new TAction(this, "pl_add_directory", tr("Add directory..."), "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPlaylist::addDirectory);

    // Add URLs
    a = new TAction(this, "pl_add_urls", tr("Add URL(s)..."));
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPlaylist::addUrls);

    // Add removed sub menu
    playlistAddMenu->addMenu(new TMenuAddRemoved(this, mainWindow,
                                                 playlistWidget));

    contextMenu->addMenu(playlistAddMenu);

    // Remove menu
    playlistRemoveMenu = new Menu::TMenu(this, mainWindow, "pl_remove_menu",
                                         tr("Remove from playlist"), "noicon");
    playlistRemoveMenu->menuAction()->setIcon(style()->standardIcon(
                                                QStyle::SP_DialogCancelButton));
    connect(playlistRemoveMenu, &Menu::TMenu::aboutToShow,
            this, &TPlaylist::enableRemoveMenu);

    // Delete from playlist
    removeSelectedAct = new TAction(this, "pl_delete",
        tr("Delete from playlist"), "noicon", Qt::Key_Delete);
    removeSelectedAct->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    playlistRemoveMenu->addAction(removeSelectedAct);
    connect(removeSelectedAct, &TAction::triggered,
            this, &TPlaylist::removeSelected);

    // Delete from disk
    removeSelectedFromDiskAct = new TAction(this, "pl_delete_from_disk",
        tr("Delete from disk..."), "noicon", Qt::SHIFT | Qt::Key_Delete);
    removeSelectedFromDiskAct->setIcon(style()->standardIcon(
                                           QStyle::SP_DialogDiscardButton));
    playlistRemoveMenu->addAction(removeSelectedFromDiskAct);
    connect(removeSelectedFromDiskAct, &TAction::triggered,
            this, &TPlaylist::removeSelectedFromDisk);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TPlaylist::enableRemoveFromDiskAction);

    // Clear playlist
    removeAllAct = new TAction(this, "pl_clear", tr("Clear playlist"),
                               "noicon", Qt::CTRL | Qt::Key_Delete);
    removeAllAct->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    playlistRemoveMenu->addAction(removeAllAct);
    connect(removeAllAct, &TAction::triggered, this, &TPlaylist::removeAll);

    contextMenu->addMenu(playlistRemoveMenu);

    playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playlistWidget, &TPlaylistWidget::customContextMenuRequested,
            contextMenu, &Action::Menu::TMenu::execSlot);

    contextMenu->addSeparator();
    contextMenu->addAction(refreshAct);
    contextMenu->addAction(browseDirAct);

    // Toolbar
    toolbar = new QToolBar(this);
    toolbar->setObjectName("playlisttoolbar");

    toolbar->addAction(openPlaylistAct);
    toolbar->addAction(saveAct);;
    toolbar->addAction(saveAsAct);

    toolbar->addSeparator();
    add_button = new QToolButton(this);
    add_button->setMenu(playlistAddMenu);
    add_button->setPopupMode(QToolButton::InstantPopup);
    add_button->setDefaultAction(playlistAddMenu->menuAction());
    toolbar->addWidget(add_button);

    remove_button = new QToolButton(this);
    remove_button->setMenu(playlistRemoveMenu);
    remove_button->setPopupMode(QToolButton::InstantPopup);
    remove_button->setDefaultAction(playlistRemoveMenu->menuAction());
    toolbar->addWidget(remove_button);

    toolbar->addSeparator();
    toolbar->addAction(refreshAct);
    toolbar->addAction(browseDirAct);
    toolbar->addAction(shuffleAct);
    toolbar->addAction(repeatAct);
}

void TPlaylist::getFilesToPlay(QStringList& files) const {

    TPlaylistItem* root = playlistWidget->root();
    if (root == 0) {
        return;
    }

    QString fn = root->filename();
    if (!fn.isEmpty()) {
        files.append(fn);
        return;
    }

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        files.append(i->filename());
        it++;
    }
}

void TPlaylist::clear() {

    abortThread();
    playlistWidget->clr();
    playlistFilename = "";
    setPlaylistTitle();
}

void TPlaylist::onThreadFinished() {
    WZDEBUG("");

    if (thread == 0) {
        // Only during destruction, so no need to enable actions
        WZDEBUG("Thread is gone");
        return;
    }

    // Get data from thread
    TPlaylistItem* root = thread->root;
    thread->root = 0;
    if (!thread->latestDir.isEmpty()) {
        Settings::pref->last_dir = thread->latestDir;
    }

    // Clean up
    delete thread;
    thread = 0;

    if (root == 0) {
        // Thread aborted
        if (restartThread) {
            WZDEBUG("Thread aborted, starting new request");
            addStartThread();
        } else {
            WZDEBUG("Thread aborted");
            addFiles.clear();
            enableActions();
        }
        return;
    }

    QString msg = addFiles.count() == 1 ? addFiles.at(0) : "";
    addFiles.clear();

    if (root->childCount() == 0) {
        delete root;
        if (msg.isEmpty()) {
            msg = tr("Found nothing to play.");
        } else {
            msg = tr("Found no files to play in '%1'.").arg(msg);
        }
        WZINFO(msg);
        QMessageBox::information(this, tr("Nothing to play"), msg);
        enableActions();
        return;
    }

    // Returns a newly created root when all items are replaced
    root = playlistWidget->add(root, addTarget);
    if (root) {
        playlistFilename = root->filename();
        WZINFO("Playlist filename set to '" + playlistFilename + "'");
        setPlaylistTitle();
        Settings::pref->addRecent(playlistFilename, root->fname());
    }

    if (addStartPlay) {
        if (!addFileToPlay.isEmpty()) {
            TPlaylistItem* item = playlistWidget->findFilename(addFileToPlay);
            if (item) {
                playItem(item);
                return;
            }
        }
        startPlay();
        return;
    }

    enableActions();
}

void TPlaylist::abortThread() {

    if (thread) {
        WZDEBUG("Aborting add files thread");
        addStartPlay = false;
        restartThread = false;
        thread->abort();
    }
}

void TPlaylist::addStartThread() {

    if (thread) {
        // Thread still running, abort it and restart it in onThreadFinished()
        WZDEBUG("Add files thread still running. Aborting it...");
        restartThread = true;
        thread->abort();
    } else {
        WZDEBUG("Starting add files thread");
        restartThread = false;

        // Allow single image
        bool addImages = Settings::pref->addImages
                         || ((addFiles.count() == 1)
                             && extensions.isImage(addFiles.at(0)));

        thread = new TAddFilesThread(this,
                                     addFiles,
                                     Settings::pref->nameBlacklist,
                                     Settings::pref->addDirectories,
                                     Settings::pref->addVideo,
                                     Settings::pref->addAudio,
                                     Settings::pref->addPlaylists,
                                     addImages);

        connect(thread, &TAddFilesThread::finished,
                this, &TPlaylist::onThreadFinished);
        connect(thread, &TAddFilesThread::displayMessage,
                msgSlot, &TMsgSlot::msg);

        thread->start();
        enableActions();
    }
}

void TPlaylist::add(const QStringList& files,
                    bool startPlay,
                    TPlaylistItem* target,
                    const QString& fileToPlay) {
    debug << "add files" << files << "startPlay" << startPlay << debug;

    addFiles = files;
    addStartPlay = startPlay;
    addTarget = target;
    addFileToPlay = fileToPlay;

    addStartThread();
}

void TPlaylist::addFilesDialog() {

    QStringList files = TFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        add(files, false, playlistWidget->plCurrentItem());
    }
}

void TPlaylist::addPlayingFile() {
   WZDEBUG("");

   QString fn = player->mdat.filename;
   if (!fn.isEmpty()) {
       add(QStringList() << fn, false, playlistWidget->plCurrentItem());
   }
}

void TPlaylist::addDirectory() {

    QString s = TFileDialog::getExistingDirectory(
                this, tr("Choose a directory"), Settings::pref->last_dir);
    if (!s.isEmpty()) {
        add(QStringList() << s, false, playlistWidget->plCurrentItem());
    }
}

void TPlaylist::addUrls() {

    TMultilineInputDialog d(this);
    if (d.exec() == QDialog::Accepted && d.lines().count() > 0) {
        add(d.lines(), false, playlistWidget->plCurrentItem());
    }
}

void TPlaylist::addRemovedItem(const QString& item) {
    WZDEBUG("'" + item + "'");

    // TODO: just add the item instead of refresh
    refresh();
}

void TPlaylist::onRepeatToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Repeat playlist set"));
    else
        msgOSD(tr("Repeat playlist cleared"));
}

void TPlaylist::onShuffleToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Shuffle playlist set"));
    else
        msgOSD(tr("Shuffle playlist cleared"));
}

void TPlaylist::play() {
    WZDEBUG("");

    TPlaylistItem* item = playlistWidget->plCurrentItem();
    if (item) {
        playItem(item);
    } else {
        player->play();
    }
}

void TPlaylist::playOrPause() {
    WZDEBUG("state " + player->stateToString());

    switch (player->state()) {
        case Player::STATE_PLAYING: player->pause(); break;
        case Player::STATE_PAUSED: player->play(); break;

        case Player::STATE_STOPPING:
        case Player::STATE_RESTARTING:
        case Player::STATE_LOADING: break;

        case Player::STATE_STOPPED:
        default:
            if (reachedEndOfPlaylist) {
                playNext(true);
            } else if (playlistWidget->playingItem) {
                playItem(playlistWidget->playingItem);
            } else if (playlistWidget->plCurrentItem()) {
                playItem(playlistWidget->plCurrentItem());
            } else {
                player->play();
            }
            break;
    }
}

void TPlaylist::stop() {
    WZDEBUG("state " + player->stateToString());

    abortThread();
    player->stop();
    TPlaylistItem* item = playlistWidget->playingItem;
    if (item) {
        if (item->state() != PSTATE_STOPPED && item->state() != PSTATE_FAILED) {
            item->setState(PSTATE_STOPPED);
        }
        playlistWidget->setCurrentItem(item);
    }

    // TODO: maybe add playlistWidget->stop() for copyThread?
}

TPlaylistItem* TPlaylist::getRandomItem() const {

    bool foundFreeItem = false;
    int selected = (int) ((double) playlistWidget->countChildren() * qrand()
                          / (RAND_MAX + 1.0));
    bool foundSelected = false;

    do {
        int idx = 0;
        QTreeWidgetItemIterator it(playlistWidget);
        while (*it) {
            TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
            if (!i->isFolder()) {
                if (idx == selected) {
                    foundSelected = true;
                }

                if (!i->played() && i->state() != PSTATE_FAILED) {
                    if (foundSelected) {
                        WZTRACE("Selecting '" + i->filename() + "'");
                        return i;
                    } else {
                        foundFreeItem = true;
                    }
                }

                idx++;
            }
            it++;
        }
    } while (foundFreeItem);

    WZDEBUG("End of playlist");
    return 0;
}

bool TPlaylist::haveUnplayedItems() const {

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (!i->isFolder() && !i->played() && i->state() != PSTATE_FAILED) {
            return true;
        }
        ++it;
    }

    return false;
}

void TPlaylist::startPlay() {
    WZDEBUG("");

    TPlaylistItem* item = playlistWidget->firstPlaylistItem();
    if (item) {
        if (shuffleAct->isChecked()) {
            playItem(getRandomItem());
        } else {
            playItem(item);
        }
    } else {
        WZINFO("Nothing to play");
    }
}

void TPlaylist::playItem(TPlaylistItem* item, bool keepPaused) {

    reachedEndOfPlaylist = false;
    while (item && item->isFolder()) {
        item = playlistWidget->getNextPlaylistItem(item);
    }
    if (item) {
        WZDEBUG("'" + item->filename() + "'");
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
           player->setStartPausedOnce();
        }
        playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        player->open(item->filename(), playlistWidget->hasSingleItem());
    } else {
        WZDEBUG("end of playlist");
        stop();
        reachedEndOfPlaylist = true;
        msg(tr("End of playlist"), 0);
        emit playlistFinished();
    }
}

void TPlaylist::playNext(bool loop_playlist) {
    WZDEBUG("");

    TPlaylistItem* item;
    if (shuffleAct->isChecked()) {
        item = getRandomItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            // Restart the playlist
            playlistWidget->clearPlayed();
            item = getRandomItem();
        }
    } else {
        item = playlistWidget->getNextPlaylistItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            // Select first item in playlist
            item = playlistWidget->firstPlaylistItem();
        }
    }
    playItem(item, player->mdat.image);
}

void TPlaylist::playPrev() {
    WZDEBUG("");

    TPlaylistItem* item = playlistWidget->playingItem;
    if (item && shuffleAct->isChecked()) {
        item = playlistWidget->findPreviousPlayedTime(item);
    } else {
        item = playlistWidget->getPreviousPlaylistWidgetItem();
    }
    if (item == 0) {
        item = playlistWidget->lastPlaylistItem();
    }
    if (item) {
        playItem(item, player->mdat.image);
    }
}

void TPlaylist::resumePlay() {

    TPlaylistItem* item = playlistWidget->firstPlaylistItem();
    if (item) {
        playItem(item);
    }
}

QString TPlaylist::playingFile() const {
    return playlistWidget->playingFile();
}

void TPlaylist::removeSelected(bool deleteFromDisk) {
    WZDEBUG("");

    if (!isActiveWindow()) {
        WZWARN("ignoring remove actiom while playlist not active window");
        return;
    }
    if (!isVisible()) {
        WZWARN("ignoring remove action while playlist not visible");
        return;
    }

    disableEnableActions++;
    playlistWidget->removeSelected(deleteFromDisk);
    if (playlistFilename.isEmpty() && !playlistWidget->hasItems()) {
        playlistWidget->clearModified();
    }
    disableEnableActions--;
    enableActions();
}

void TPlaylist::removeSelectedFromDisk() {
    removeSelected(true);
}

void TPlaylist::removeAll() {
    clear();
}

void TPlaylist::refresh() {

    if (!playlistFilename.isEmpty() && maybeSave()) {
        QString playing;
        if (player->statePOP()) {
            playing = playingFile();
            if (!playing.isEmpty()) {
                player->saveRestartState();
            }
        }
        clear();
        add(QStringList() << playlistFilename, !playing.isEmpty(), 0, playing);
    }
}

bool TPlaylist::browseDirEnabled() {
    return !player->mdat.filename.isEmpty()
            || (isVisible() && playlistWidget->plCurrentItem());
}

void TPlaylist::browseDir() {

    QString fn = player->mdat.filename;
    if (isVisible()) {
        TPlaylistItem* item = playlistWidget->plCurrentItem();
        if (item) {
            fn = item->filename();
        }
    }
    if (fn.isEmpty()) {
        return;
    }

    QUrl url;
    QFileInfo fi(fn);
    if (fi.exists()) {
        if (fi.isDir()) {
            url = QUrl::fromLocalFile(fi.absoluteFilePath());
        } else {
            url = QUrl::fromLocalFile(fi.absolutePath());
        }
    } else {
        url.setUrl(fn);
    }

    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, tr("Open URL failed"),
                             tr("Failed to open URL '%1'")
                             .arg(url.toString(QUrl::None)));
    }
}

void TPlaylist::openInNewWindow() {
    WZDEBUG("");

    QStringList files;
    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        files << i->filename();
        ++it;
    }

    if (files.count() == 0) {
        if (playlistWidget->plCurrentItem()) {
            files << playlistWidget->plCurrentItem()->filename();
        } else if (player->mdat.filename.isEmpty()) {
            return;
        } else {
            files << player->mdat.filename;
        }
    }

    // Save settings and modified playlist
    mainWindow->save();

    QProcess p;
    if (p.startDetached(qApp->applicationFilePath(), files)) {
        WZINFO("started new instance");
    } else {
        QString msg = strerror(errno);
        WZERROR(QString("Failed to start '%1'. %2")
                .arg(qApp->applicationFilePath()).arg(msg));
        QMessageBox::warning(this, tr("Start failed"),
                             tr("Failed to start '%1'. %2")
                             .arg(qApp->applicationFilePath()).arg(msg),
                             QMessageBox::Ok);
    }
}

void TPlaylist::onItemActivated(QTreeWidgetItem* i, int) {
    WZDEBUG("");

    TPlaylistItem* item = static_cast<TPlaylistItem*>(i);
    if (item && !item->isFolder()) {
        playItem(item);
    }
}

void TPlaylist::enablePaste() {
    pasteAct->setEnabled(thread == 0
                         && QApplication::clipboard()->mimeData()->hasText());
}

void TPlaylist::enableRemoveFromDiskAction() {

    TPlaylistItem* current = playlistWidget->plCurrentItem();
    removeSelectedFromDiskAct->setEnabled(
                thread == 0
                && current
                && (!current->isWZPlaylist()
                    || current->isSymLink()
                    || current->childCount() == 0));
    // Leaving test for non media files in directory to deleteFileFromDisk()
}

void TPlaylist::enableRemoveMenu() {

    bool e = thread == 0 && playlistWidget->hasItems();
    removeSelectedAct->setEnabled(e);
    removeAllAct->setEnabled(e);
    enableRemoveFromDiskAction();
}


void TPlaylist::enableActions() {

    if (disableEnableActions) {
        return;
    }
    WZTRACE("State " + player->stateToString());

    Player::TState s = player->state();
    bool enable = (s == Player::STATE_STOPPED
                   || s == Player::STATE_PLAYING
                   || s == Player::STATE_PAUSED)
                  && thread == 0;

    if (!enable) {
        QString text;
        if (thread) {
            text = tr("Loading");
        } else {
            text = player->stateToString();
        }
        playOrPauseAct->setTextAndTip(tr("%1...").arg(text));
        if (s == Player::STATE_STOPPING) {
            playOrPauseAct->setIcon(Images::icon("stopping"));
            playOrPauseAct->setEnabled(false);
        } else {
            playOrPauseAct->setIcon(Images::icon("loading"));
            playOrPauseAct->setEnabled(s == Player::STATE_PLAYING);
        }
    } else if (s == Player::STATE_PLAYING) {
        TPlaylistItem* playingItem = playlistWidget->playingItem;
        if (playingItem == 0) {
            playingItem = playlistWidget->findFilename(player->mdat.filename);
        }
        if (playingItem && playingItem->state() != PSTATE_PLAYING) {
            disableEnableActions++;
            playlistWidget->setPlayingItem(playingItem, PSTATE_PLAYING);
            disableEnableActions--;
        }
        playOrPauseAct->setTextAndTip(tr("&Pause"));
        playOrPauseAct->setIcon(Images::icon("pause"));
        playOrPauseAct->setEnabled(true);
    } else {
        // STATE_PAUSED
        playOrPauseAct->setTextAndTip(tr("&Play"));
        playOrPauseAct->setIcon(Images::icon("play"));
        playOrPauseAct->setEnabled(true);
    }

    openPlaylistAct->setEnabled(s != Player::STATE_STOPPING);
    //saveAct->setEnabled(true);
    //saveAsAct->setEnabled(true);
    refreshAct->setEnabled(!playlistFilename.isEmpty());
    browseDirAct->setEnabled(browseDirEnabled());

    stopAct->setEnabled(thread
                        || s == Player::STATE_PLAYING
                        || s == Player::STATE_PAUSED
                        || s == Player::STATE_RESTARTING
                        || s == Player::STATE_LOADING);

    bool haveItems = playlistWidget->hasItems();
    openInNewWindowAct->setEnabled(haveItems
                                   || !player->mdat.filename.isEmpty());
    pauseAct->setEnabled(s == Player::STATE_PLAYING);

    bool e = enable && haveItems;
    playNextAct->setEnabled(e);
    playPrevAct->setEnabled(e);

    // Context menu
    bool current = playlistWidget->plCurrentItem();
    editNameAct->setEnabled(enable && current);
    newFolderAct->setEnabled(enable);
    findPlayingAct->setEnabled(playlistWidget->playingItem);

    cutAct->setEnabled(enable && current);
    copyAct->setEnabled(current || !player->mdat.filename.isEmpty());
    enablePaste();

    // Add menu
    addPlayingFileAct->setEnabled(!player->mdat.filename.isEmpty());
    // Remove menu
    enableRemoveMenu();

    // Repeat and shuffle are always enabled
}

void TPlaylist::onPlayerError() {

    if (player->state() != Player::STATE_STOPPING) {
        TPlaylistItem* item = playlistWidget->playingItem;
        if (item && item->filename() == player->mdat.filename) {
            item->setState(PSTATE_FAILED);
            playlistWidget->setCurrentItem(item);
            playlistWidget->scrollToItem(item);
        }
    }
}

void TPlaylist::onNewMediaStartedPlaying() {

    const TMediaData* md = &player->mdat;
    QString filename = md->filename;
    QString current_filename = playlistWidget->playingFile();

    if (md->disc.valid) {
        // Handle disk
        if (md->titles.count() == playlistWidget->countItems()) {
            TDiscName cur_disc(current_filename);
            if (cur_disc.valid
                && cur_disc.protocol == md->disc.protocol
                && cur_disc.device == md->disc.device) {
                WZDEBUG("Item is from current disc");
                return;
            }
        }
    } else if (filename == current_filename) {
        // Handle current item started playing
        TPlaylistItem* item = playlistWidget->playingItem;
        if (item == 0) {
            return;
        }
        bool modified = false;

        // Update item name
        if (!item->edited()) {
            QString name = md->name();
            if (name != item->baseName()) {
                WZDEBUG(QString("Updating name from '%1' to '%2'")
                        .arg(item->baseName()).arg(name));
                item->setName(name, item->extension(), false);
                modified = true;
            }
        }

        // Update item duration
        if (!md->image && md->duration > 0) {
            if (!this->playlistFilename.isEmpty()
                && qAbs(md->duration - item->duration()) > 1) {
                modified = true;
            }
            WZDEBUG(QString("Updating duration from %1 to %2")
                    .arg(item->duration()).arg(md->duration));
            item->setDuration(md->duration);
        }

        if (modified) {
            item->setModified();
        } else {
            WZDEBUG("Item considered uptodate");
        }

        // Pause a single image
        if (player->mdat.image && playlistWidget->hasSingleItem()) {
            mainWindow->runActionsLater("pause", true);
        }

        return;
    }

    // Create new playlist
    WZDEBUG("Creating new playlist for '" + filename + "'");
    clear();

    if (md->disc.valid) {
        // Add disc titles without sorting
        playlistWidget->setSort(-1, Qt::AscendingOrder);
        TPlaylistItem* root = playlistWidget->root();
        root->setFilename(filename, md->name());
        // setFilename() won't recognize iso's as folder.
        // No need to set icon as it is not visible
        root->setFolder(true);

        TDiscName disc = md->disc;
        foreach(const Maps::TTitleData& title, md->titles) {
            disc.title = title.getID();
            TPlaylistItem* i = new TPlaylistItem(
                        root,
                        disc.toString(),
                        title.getDisplayName(false),
                        title.getDuration(),
                        false /* protext name */);
            if (title.getID() == md->titles.getSelectedID()) {
                playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
            }
        }
    } else {
        // Add current file
        TPlaylistItem* current = new TPlaylistItem(
                    playlistWidget->root(),
                    filename,
                    md->name(),
                    md->duration,
                    false);
        playlistWidget->setPlayingItem(current, PSTATE_PLAYING);
    }

    setPlaylistTitle();
    WZDEBUG("Created new playlist for '" + filename + "'");
}

void TPlaylist::onMediaEOF() {
    WZDEBUG("");
    playNext(false);
}

void TPlaylist::onTitleTrackChanged(int id) {
    WZDEBUG(QString::number(id));

    if (id < 0) {
        playlistWidget->setPlayingItem(0);
        return;
    }

    // Search for id
    TDiscName disc = player->mdat.disc;
    disc.title = id;
    QString filename = disc.toString();

    TPlaylistItem* i = playlistWidget->findFilename(filename);
    if (i) {
        playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
    } else {
        WZWARN("title id " + QString::number(id) + " filename '" + filename
               + "' not found in playlist");
    }
}

void TPlaylist::copySelection(const QString& actionName) {

    QString text;
    int copied = 0;

    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        text += i->filename() + "\n";
        copied++;
        it++;
    }

    if (copied == 0 && player->mdat.filename.count()) {
        text = player->mdat.filename + "\n";
        copied = 1;
    }

    if (copied > 0) {
        if (copied == 1) {
            // Remove trailing new line
            text = text.left(text.length() - 1);
            msgOSD(actionName + " " + text);
        } else {
            msgOSD(tr("%1 %2 file names",
                   "Action 'Copied'' or 'Cut'', number of file names")
                .arg(actionName).arg(copied));
        }
        QApplication::clipboard()->setText(text);
    }
}

void TPlaylist::copySelected() {
    copySelection(tr("Copied"));
}

void TPlaylist::paste() {

    QStringList files = QApplication::clipboard()->text()
                        .split("\n",  QString::SkipEmptyParts);
    if (files.count()) {
        TPlaylistItem* parent = playlistWidget->plCurrentItem();
        if (parent && !parent->isFolder()) {
            parent = parent->plParent();
        }
        add(files, false, parent);
    }
}

void TPlaylist::cut() {

    copySelection(tr("Cut"));
    removeSelected();
}

void TPlaylist:: setPlaylistTitle() {

    QString title;
    TPlaylistItem* root = playlistWidget->root();
    if (root) {
        title = root->baseName();
        if (title.isEmpty()) {
            title = root->fname();
        }
    }

    title = tr("Playlist%1%2%3",
               "1 optional white space,"
               " 2 optional playlist name,"
               " 3 optional playlist modified star")
        .arg(title.isEmpty() ? "" : " ")
        .arg(title)
        .arg(playlistWidget->modified() ? "*" : "");

    emit playlistTitleChanged(title);
}

void TPlaylist::onModifiedChanged() {
    setPlaylistTitle();
}

void TPlaylist::newFolder() {
    WZDEBUG("");

    TPlaylistItem* parent = playlistWidget->plCurrentItem();
    if (parent == 0) {
        parent = playlistWidget->root();
    } else if (!parent->isFolder()) {
        parent = parent->plParent();
    }

    QString path = parent->filename();
    if (path.isEmpty()) {
        if (QMessageBox::question(this, tr("Save playlist?"),
                tr("To create folders the playlist needs to be saved first."
                   " Do you want to save it now?"),
                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            if (saveAs()) {
                QTimer::singleShot(0, this, &TPlaylist::newFolder);
            }
        }
        return;
    }

    QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::information(this, tr("Information"),
            tr("Failed to create a new folder. Could not find '%1'").arg(path));
        return;
    }

    if (!fi.isDir()) {
        // TODO: confirm for non wzplaylist?
        path = fi.absolutePath();
    }

    QString baseName = tr("New folder");
    QDir dir(path);
    int i = 2;
    QString name = baseName;
    while (dir.exists(name)) {
        name = baseName + " " + QString::number(i++);
    }

    if (!dir.mkdir(name)) {
        QString error = strerror(errno);
        WZERROR(QString("Failed to create directory '%1' in '%2'. %3")
                .arg(name).arg(path).arg(error));
        QMessageBox::warning (this, tr("Error"),
                              tr("Failed to create folder '%1' in '%2'. %3")
                              .arg(name).arg(path).arg(error));
        return;
    }

    TPlaylistItem* item = new TPlaylistItem(parent, path + "/" + baseName,
                                            baseName, 0, false);
    item->setModified();
    playlistWidget->setCurrentItem(item);
    playlistWidget->editName();
}

void TPlaylist::findPlayingItem() {

    TPlaylistItem* i = playlistWidget->playingItem;
    if (i) {
        if (!isVisible()) {
            // parent is playlistdock
            QWidget* p = qobject_cast<QWidget*>(parent());
            p->setVisible(true);
        }
        if (i == playlistWidget->currentItem()) {
            playlistWidget->scrollToItem(i);
        } else {
            playlistWidget->setCurrentItem(i);
        }
    }
}

// Drag&drop
void TPlaylist::dragEnterEvent(QDragEnterEvent *e) {
    debug << "dragEnterEvent" << e->mimeData()->formats();
    debug << debug;

    if (e->mimeData()->hasUrls()) {
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
            return;
        }
        if (e->possibleActions() & Qt::CopyAction) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
    }
    QWidget::dragEnterEvent(e);
}

void TPlaylist::dropEvent(QDropEvent *e) {
    debug << "dropEvent" << e->mimeData()->formats();
    debug << debug;

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl& url, e->mimeData()->urls()) {
            files.append(url.toString());
        }

        if (files.count()) {
            // TODO: see dropIndicator for above/below
            QTreeWidgetItem* target = playlistWidget->itemAt(
                        e->pos()
                        - playlistWidget->pos()
                        - playlistWidget->viewport()->pos());
            add(files, false, static_cast<TPlaylistItem*>(target));
        }

        e->accept();
        return;
    }
    QWidget::dropEvent(e);
}

void TPlaylist::open(const QString &fileName, const QString& name) {
    WZDEBUG("'" + fileName + "'");

    if (fileName.isEmpty()) {
        WZDEBUG("Nothing to play");
        return;
    }
    if (!maybeSave()) {
        return;
    }

    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            openDirectory(fi.absoluteFilePath());
            return;
        }
        if (extensions.isPlaylist(fi)) {
            openPlaylist(fi.absoluteFilePath());
            return;
        }
        Settings::pref->last_dir = fi.absolutePath();
    }

    WZDEBUG("Starting new playlist");
    clear();
    playItem(new TPlaylistItem(playlistWidget->root(),
                               fileName,
                               name,
                               0,
                               !name.isEmpty()));
    WZDEBUG("done");
}

void TPlaylist::openFileDialog() {
    WZDEBUG("");

    QString s = TFileDialog::getOpenFileName(
        this,
        tr("Choose a file"),
        Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;"
        + tr("Video") + extensions.video().forFilter() + ";;"
        + tr("Audio") + extensions.audio().forFilter() + ";;"
        + tr("Playlists") + extensions.playlists().forFilter() + ";;"
        + tr("Images") + extensions.images().forFilter() + ";;"
        + tr("All files") +" (*.*)");

    if (!s.isEmpty()) {
        open(s);
    }
}


void TPlaylist::openFiles(const QStringList& files, const QString& fileToPlay) {
    WZDEBUG("");

    if (files.empty()) {
        WZDEBUG("No files in list to open");
        return;
    }

    if (maybeSave()) {
        clear();
        add(files, true, 0, fileToPlay);
    }
}

void TPlaylist::openDirectory(const QString& dir) {
    WZDEBUG("'" + dir + "'");

    Settings::pref->last_dir = dir;
    if (TWZFiles::directoryContainsDVD(dir)) {
        // onNewMediaStartedPlaying() will clear and pickup the playlist
        player->open(dir);
    } else {
        clear();
        add(QStringList() << dir, true);
    }
}

void TPlaylist::openDirectoryDialog() {
    WZDEBUG("");

    if (maybeSave()) {
        QString s = TFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"), Settings::pref->last_dir);
        if (!s.isEmpty()) {
            openDirectory(s);
        }
    }
}

void TPlaylist::openPlaylist(const QString& filename) {

    Settings::pref->last_dir = QFileInfo(filename).absolutePath();
    clear();
    add(QStringList() << filename, true);
}

void TPlaylist::openPlaylistDialog() {

    if (maybeSave()) {
        QString s = TFileDialog::getOpenFileName(this, tr("Choose a file"),
            Settings::pref->last_dir,
            tr("Playlists") + extensions.playlists().forFilter() + ";;"
            + tr("All files") +" (*.*)");

        if (!s.isEmpty()) {
            openPlaylist(s);
        }
    }
}

bool TPlaylist::saveM3uFolder(TPlaylistItem* folder,
                              const QString& path,
                              QTextStream& stream,
                              bool linkFolders,
                              bool& savedMetaData) {
    WZDEBUG("Saving '" + folder->filename() + "'");

    bool result = true;
    for(int idx = 0; idx < folder->childCount(); idx++) {
        TPlaylistItem* i = folder->plChild(idx);
        QString filename = i->filename();

        if (i->isPlaylist()) {
            if (i->modified()) {
                if (!saveM3u(i, filename, i->isWZPlaylist())) {
                    result = false;
                }
            } else {
                WZDEBUG("Playlist '" + filename + "' not modified");
            }
        } else if (i->isFolder()) {
            if (linkFolders) {
                if (i->modified()) {
                    QFileInfo fi(filename, TConfig::WZPLAYLIST);
                    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
                    if (!saveM3u(i, filename, linkFolders)) {
                        result = false;
                    }
                } else {
                    WZDEBUG("Folder '" + filename + "' not modified");
                }
            } else {
                // Note: savedMetaData destroyed as dummy here.
                // It is only used for WZPlaylists.
                if (saveM3uFolder(i, path, stream, linkFolders, savedMetaData)) {
                    WZINFO("Succesfully saved '" + filename + "'");
                } else {
                    result = false;
                }
                continue;
            }
        } else {
            int d = (int) i->duration();
            stream << "#EXTINF:" << d << "," << i->baseName() << "\n";
            if (!savedMetaData) {
                if (d || i->edited()) {
                    savedMetaData = true;
                }
            }
        }

        if (filename.startsWith(path)) {
            filename = filename.mid(path.length());
        }
        stream << filename << "\n";
    }

    return result;
}

bool TPlaylist::saveM3u(TPlaylistItem* folder,
                        const QString& filename,
                        bool wzplaylist) {
    WZDEBUG("Saving '" + filename + "'");

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        // Ok to fail on wzplaylist
        if (wzplaylist) {
            WZINFO(QString("Ignoring failed save of '%1'. %2")
                   .arg(filename).arg(f.errorString()));
            return true;
        }

        WZERROR("Failed to save '" + filename + "'. " + f.errorString());
        // TODO: skip remaining  msgs...
        QMessageBox::warning(this, tr("Save failed"),
                             tr("Failed to open '%1' for writing. %2")
                             .arg(filename).arg(f.errorString()),
                             QMessageBox::Ok);
        return false;
    }

    QTextStream stream(&f);
    if (QFileInfo(filename).suffix().toLower() == "m3u") {
        stream.setCodec(QTextCodec::codecForLocale());
    } else {
        stream.setCodec("UTF-8");
    }

    stream << "#EXTM3U" << "\n"
           << "# Playlist created by WZPlayer " << TVersion::version << "\n";

    // Keep track of whether we saved anything usefull
    bool savedMetaData = false;

    if (wzplaylist && folder->getBlacklistCount() > 0) {
        savedMetaData = true;
        foreach(const QString& fn, folder->getBlacklist()) {
            WZDEBUG("Blacklisting '" + fn + "'");
            stream << "#WZP-blacklist:" << fn << "\n";
        }
    }

    bool result = saveM3uFolder(folder, path, stream, wzplaylist, savedMetaData);

    // TODO: test result
    stream.flush();
    f.close();

    // Remove wzplaylist if nothing interesting to remember
    if (wzplaylist && !savedMetaData) {
        if (f.remove()) {
            WZINFO("Removed '" + filename + "'");
        } else {
            WZWARN(QString("Failed to remove '%1'. %2")
                    .arg(filename).arg(f.errorString()));
        }
    } else {
        WZINFO("Saved '" + filename + "'");
    }

    return result;
}

bool TPlaylist::saveM3u(const QString& filename, bool linkFolders) {

    TPlaylistItem* root = playlistWidget->root();
    return saveM3u(root, filename, linkFolders);
}

bool TPlaylist::save() {
    WZINFO("'" + playlistFilename + "'");

    if (playlistFilename.isEmpty()) {
        return saveAs();
    }

    bool wzplaylist = false;
    QFileInfo fi(playlistFilename);
    if (fi.isDir()) {
        fi.setFile(fi.absoluteFilePath(), TConfig::WZPLAYLIST);
        wzplaylist = true;
    } else if (fi.fileName() == TConfig::WZPLAYLIST) {
        wzplaylist = true;
    } else if (player->mdat.disc.valid) {
        // saveAs() force adds the playlist extension
        if (!extensions.isPlaylist(fi)) {
            return saveAs();
        }
    }
    msgOSD(tr("Saving %1").arg(fi.fileName()), 0);

    playlistFilename = QDir::toNativeSeparators(fi.absoluteFilePath());
    playlistWidget->root()->setFilename(playlistFilename);
    setPlaylistTitle();
    Settings::pref->last_dir = fi.absolutePath();

    bool result = saveM3u(playlistFilename, wzplaylist);
    if (result) {
        playlistWidget->clearModified();
        msgOSD(tr("Saved '%1'").arg(fi.fileName()));
    } else {
        // Error box and log already done, but need to remove 0 secs save msg
        msgOSD(tr("Failed to save '%1'").arg(fi.fileName()));
    }

    return result;
}

bool TPlaylist::saveAs() {

    QString s = TFileDialog::getSaveFileName(this, tr("Choose a filename"),
        Settings::pref->last_dir,
        tr("Playlists") + extensions.playlists().forFilter());

    if (s.isEmpty()) {
        return false;
    }

    // If filename has no extension, force add it. save() depends om it
    // when saving a playlist for discs.
    QFileInfo fi(s);
    if (fi.suffix().toLower() != "m3u8") {
        fi.setFile(s + ".m3u8");
    }

    if (fi.exists()) {
        int res = QMessageBox::question(this, tr("Confirm overwrite?"),
                                        tr("The file %1 already exists.\n"
                                           "Do you want to overwrite it?")
                                        .arg(fi.absoluteFilePath()),
                                        QMessageBox::Yes, QMessageBox::No,
                                        QMessageBox::NoButton);
        if (res == QMessageBox::No) {
            return false;
        }
    }

    playlistFilename = fi.absoluteFilePath();
    playlistWidget->root()->setFilename(playlistFilename);
    return save();
}

bool TPlaylist::maybeSave() {

    if (!playlistWidget->modified()) {
        WZTRACE("Playlist not modified");
        return true;
    }

    if (playlistFilename.isEmpty()) {
        WZDEBUG("Discarding unnamed playlist");
        return true;
    }

    QFileInfo fi(playlistFilename);
    if (fi.fileName().compare(TConfig::WZPLAYLIST, caseSensitiveFileNames)
            == 0) {
        return save();
    }

    if (fi.isDir()) {
        if (Settings::pref->useDirectoriePlaylists) {
            return save();
        }
        WZDEBUG("Discarding changes. Saving directorie playlists is disabled.");
        return true;

    }

    int res = QMessageBox::question(this, tr("Playlist modified"),
        tr("The playlist has been modified, do you want to save the changes to"
           " \"%1\"?").arg(playlistFilename),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    switch (res) {
        case QMessageBox::No:
            playlistWidget->clearModified();
            WZINFO("Selected no save");
            return true;
        case QMessageBox::Cancel:
            WZINFO("Selected cancel save");
            return false;
        default:
            WZINFO("Selected save");
            return save();
    }
}

void TPlaylist::saveSettings() {

    Settings::pref->beginGroup("playlist");
    Settings::pref->setValue("repeat", repeatAct->isChecked());
    Settings::pref->setValue("shuffle", shuffleAct->isChecked());
    playlistWidget->saveSettings(Settings::pref);
    Settings::pref->endGroup();
}

void TPlaylist::loadSettings() {

    using namespace Settings;

    pref->beginGroup("playlist");
    repeatAct->setChecked(pref->value("repeat", repeatAct->isChecked())
                          .toBool());
    shuffleAct->setChecked(pref->value("shuffle", shuffleAct->isChecked())
                           .toBool());
    playlistWidget->loadSettings(pref);
    pref->endGroup();
}

} // namespace Playlist
} // namespace Gui

#include "moc_playlist.cpp"

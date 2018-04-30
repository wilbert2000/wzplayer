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

#include <QDesktopServices>
#include <QToolBar>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QMenu>
#include <QSettings>
#include <QInputDialog>
#include <QToolButton>
#include <QVBoxLayout>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTextCodec>
#include <QMimeData>
#include <QClipboard>
#include <QtAlgorithms>
#include <QProcess>
#include <QApplication>

#include "gui/mainwindow.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/playlistwidgetitem.h"
#include "gui/playlist/addfilesthread.h"
#include "player/player.h"
#include "gui/multilineinputdialog.h"
#include "gui/action/menu/menuinoutpoints.h"
#include "gui/action/action.h"
#include "gui/msg.h"
#include "images.h"
#include "wzfiles.h"
#include "gui/filedialog.h"
#include "extensions.h"
#include "version.h"


using namespace Settings;

namespace Gui {

using namespace Action;

namespace Playlist {


TAddRemovedMenu::TAddRemovedMenu(QWidget* parent,
                                 TMainWindow* w,
                                 TPlaylistWidget* plWidget) :
    TMenu(parent, w, "pl_add_removed_menu", tr("Add &removed item")),
    playlistWidget(plWidget) {

    connect(this, &TAddRemovedMenu::triggered,
            this, &TAddRemovedMenu::onTriggered);
}

TAddRemovedMenu::~TAddRemovedMenu() {
}

void TAddRemovedMenu::onAboutToShow() {

    clear();
    int c = 0;
    item = playlistWidget->currentPlaylistWidgetItem();
    if (item) {
        if (!item->isFolder()) {
            item = item->plParent();
        }
        if (item) {
            WZDEBUG("'" + item->filename() + "'");

            foreach(const QString& s, item->getBlacklist()) {
                QAction* action = new QAction(s, this);
                action->setData(s);
                addAction(action);
                c++;
            }
        }
    }

    if (c == 0) {
        QAction* action = new QAction(tr("No removed items"), this);
        action->setEnabled(false);
        addAction(action);
    }
}

void TAddRemovedMenu::onTriggered(QAction* action) {

    QString s = action->data().toString();
    if (!s.isEmpty()) {
        // Check item still valid
        item = playlistWidget->validateItem(item);
        if (item) {
            if (item->whitelist(s)) {
                WZDEBUG("whitelisted '" + s + "'");
                playlistWidget->setModified(item);
                emit addRemovedItem(s);
            } else {
                WZWARN("'" + s + "' not blacklisted");
            }
        } else {
            WZWARN("item no longer existing");
        }
    }
}


TPlaylist::TPlaylist(QWidget* parent, TMainWindow* mw) :
    QWidget(parent),
    debug(logger()),
    main_window(mw),
    thread(0),
    restartThread(false),
    disable_enableActions(false) {

    setAcceptDrops(true);

    createTree();
    createActions();
    createToolbar();
    createPopupMenu();

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

    connect(main_window, &TMainWindow::enableActions,
            this, &TPlaylist::enableActions);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(playlistWidget);
    layout->addWidget(toolbar);
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
    playlistWidget->setObjectName("playlist_tree");

    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TPlaylist::onModifiedChanged);
    connect(playlistWidget, &TPlaylistWidget::itemActivated,
             this, &TPlaylist::onItemActivated);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TPlaylist::enableActions);
}

void TPlaylist::createActions() {

    // Open
    openAct = new TAction(this, "pl_open", tr("Open &playlist..."), "",
                          QKeySequence("Ctrl+P"));
    connect(openAct, &TAction::triggered, this, &TPlaylist::open);

    // Save
    saveAct = new TAction(this, "pl_save", tr("&Save playlist"), "save",
                          QKeySequence("Ctrl+S"));
    connect(saveAct, &TAction::triggered, this, &TPlaylist::save);

    // SaveAs
    saveAsAct = new TAction(this, "pl_saveas", tr("S&ave playlist as..."),
                            "saveas");
    connect(saveAsAct, &TAction::triggered, this, &TPlaylist::saveAs);

    // Properties
    propertiesAct = new TAction(this, "view_properties",
        tr("&View properties..."), "info", Qt::SHIFT | Qt::Key_P);
    propertiesAct->setCheckable(true);
    connect(propertiesAct, &TAction::triggered,
            main_window, &TMainWindow::showFilePropertiesDialog);

    // Open directory
    openDirectoryAct = new TAction(this, "pl_open_directory",
                                   tr("&Open directory"));
    openDirectoryAct->setIcon(style()->standardPixmap(QStyle::SP_DirOpenIcon));
    connect(openDirectoryAct, &TAction::triggered,
            this, &TPlaylist::openFolder);

    // Refresh
    refreshAct = new TAction(this, "pl_refresh", tr("R&efresh playlist"), "",
                             Qt::Key_F5);
    connect(refreshAct, &TAction::triggered,
            this, &TPlaylist::refresh);

    // Stop
    stopAct = new TAction(this, "stop", tr("&Stop"), "", Qt::Key_MediaStop);
    connect(stopAct, &TAction::triggered, this, &TPlaylist::stop);

    // Play
    playAct = new TAction(this, "play", tr("P&lay selected"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, &TAction::triggered, this, &TPlaylist::play);

    // Play in new window
    playNewAct = new TAction(this, "play_new", tr("Pl&ay in new window"),
                             "play", Qt::CTRL | Qt::Key_Space);
    connect(playNewAct, &TAction::triggered,
            this, &TPlaylist::openInNewWindow);

    // Pause
    pauseAct = new TAction(this, "pause", tr("Pause"), "",
                           QKeySequence("Media Pause")); // MCE remote key
    connect(pauseAct, &TAction::triggered, player, &Player::TPlayer::pause);

    // Play/pause
    playOrPauseAct = new TAction(this, "play_or_pause", tr("&Play"), "play",
                                 Qt::Key_Space);
    // Add MCE remote key
    playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playOrPauseAct, &TAction::triggered,
            this, &TPlaylist::playOrPause);

    // Next
    nextAct = new TAction(this, "pl_next", tr("Play &next"), "next",
                          QKeySequence(">"));
    nextAct->addShortcut(QKeySequence("."));
    nextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
    connect(nextAct, &TAction::triggered, this, &TPlaylist::playNext);

    // Prev
    prevAct = new TAction(this, "pl_prev", tr("Play pre&vious"), "previous",
                          QKeySequence("<"));
    prevAct->addShortcut(QKeySequence(","));
    prevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
    connect(prevAct, &TAction::triggered, this, &TPlaylist::playPrev);

    // In-out menu
    inOutMenu = new Menu::TMenuInOut(main_window);
    addActions(inOutMenu->actions());

    // Repeat
    repeatAct = inOutMenu->findChild<TAction*>("pl_repeat");
    connect(repeatAct, &TAction::triggered,
            this, &TPlaylist::onRepeatToggled);

    // Shuffle
    shuffleAct = inOutMenu->findChild<TAction*>("pl_shuffle");
    connect(shuffleAct, &TAction::triggered,
            this, &TPlaylist::onShuffleToggled);

    // Add menu
    add_menu = new Menu::TMenu(this, main_window, "pl_add_menu",
                               tr("&Add to playlist"), "plus");

    addCurrentAct = new TAction(add_menu, "pl_add_current",
                                tr("Add &current file"));
    connect(addCurrentAct, &TAction::triggered,
            this, &TPlaylist::addCurrentFile);

    addFilesAct = new TAction(add_menu, "pl_add_files", tr("Add &file(s)..."));
    connect(addFilesAct, &TAction::triggered,
            this, &TPlaylist::addFilesDialog);

    addDirectoryAct = new TAction(add_menu, "pl_add_directory",
                                  tr("Add &directory..."));
    connect(addDirectoryAct, &TAction::triggered,
            this, &TPlaylist::addDirectory);

    addUrlsAct = new TAction(add_menu, "pl_add_urls", tr("Add &URL(s)..."));
    connect(addUrlsAct, &TAction::triggered,
            this, &TPlaylist::addUrls);

    // Add removed sub menu
    add_removed_menu = new TAddRemovedMenu(add_menu, main_window,
                                               playlistWidget);
    add_menu->addMenu(add_removed_menu);
    connect(add_removed_menu, &TAddRemovedMenu::addRemovedItem,
            this, &TPlaylist::addRemovedItem);

    addActions(add_menu->actions());

    // Remove menu
    remove_menu = new Menu::TMenu(this, main_window, "pl_remove_menu",
                                  tr("&Remove from playlist"), "minus");

    removeSelectedAct = new TAction(remove_menu, "pl_remove_selected",
                                    tr("&Remove from list"), "",
                                    Qt::Key_Delete);
    connect(removeSelectedAct, &TAction::triggered,
            this, &TPlaylist::removeSelected);

    removeSelectedFromDiskAct = new TAction(remove_menu, "pl_delete_from_disk",
                                            tr("&Delete from disk..."));
    connect(removeSelectedFromDiskAct, &TAction::triggered,
            this, &TPlaylist::removeSelectedFromDisk);

    removeAllAct = new TAction(remove_menu, "pl_remove_all",
                               tr("&Clear playlist"), "",
                               Qt::CTRL | Qt::Key_Delete);
    connect(removeAllAct, &TAction::triggered,
            this, &TPlaylist::removeAll);

    addActions(remove_menu->actions());

    // Edit
    editNameAct = new TAction(this, "pl_edit_name", tr("&Edit name..."));
    // TODO:              "", Qt::Key_F2);
    connect(editNameAct, &TAction::triggered,
            this, &TPlaylist::editName);

    // New folder
    newFolderAct = new TAction(this, "pl_new_folder", tr("&New folder..."));
    // TODO:              "", Qt::Key_F10);
    connect(newFolderAct, &TAction::triggered,
            this, &TPlaylist::newFolder);

    // Find playing
    findPlayingAct = new TAction(this, "pl_find_playing",
                                 tr("&Find playing item"));
    connect(findPlayingAct, &TAction::triggered,
            this, &TPlaylist::findPlayingItem);

    // Cut
    cutAct = new TAction(this, "pl_cut", tr("&Cut file name(s)"), "",
                          QKeySequence("Ctrl+X"));
    connect(cutAct, &TAction::triggered, this, &TPlaylist::cut);

    // Copy
    copyAct = new TAction(this, "pl_copy", tr("&Copy file name(s)"), "",
                          QKeySequence("Ctrl+C"));
    connect(copyAct, &TAction::triggered, this, &TPlaylist::copySelected);

    // Paste
    pasteAct = new TAction(this, "pl_paste", tr("&Paste file name(s)"), "",
                          QKeySequence("Ctrl+V"));
    connect(pasteAct, &TAction::triggered, this, &TPlaylist::paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &TPlaylist::enablePaste);


    // Add actions to main window
    main_window->addActions(actions());
}

void TPlaylist::createToolbar() {

    toolbar = new QToolBar(this);
    toolbar->setObjectName("playlisttoolbar");

    toolbar->addAction(openAct);
    toolbar->addAction(saveAct);;
    toolbar->addAction(saveAsAct);
    toolbar->addAction(openDirectoryAct);

    toolbar->addSeparator();

    add_button = new QToolButton(this);
    add_button->setMenu(add_menu);
    add_button->setPopupMode(QToolButton::InstantPopup);
    add_button->setDefaultAction(add_menu->menuAction());
    toolbar->addWidget(add_button);

    remove_button = new QToolButton(this);
    remove_button->setMenu(remove_menu);
    remove_button->setPopupMode(QToolButton::InstantPopup);
    remove_button->setDefaultAction(remove_menu->menuAction());
    toolbar->addWidget(remove_button);

    toolbar->addSeparator();
    toolbar->addAction(shuffleAct);
    toolbar->addAction(repeatAct);
    toolbar->addAction(inOutMenu->findChild<TAction*>("repeat_in_out"));

    toolbar->addSeparator();
    toolbar->addAction(playOrPauseAct);
    toolbar->addAction(prevAct);
    toolbar->addAction(nextAct);
}

void TPlaylist::createPopupMenu() {

    popup = new QMenu(this);
    popup->addAction(editNameAct);
    popup->addAction(newFolderAct);
    popup->addAction(findPlayingAct);
    popup->addSeparator();
    popup->addAction(cutAct);
    popup->addAction(copyAct);
    popup->addAction(pasteAct);
    popup->addSeparator();
    popup->addAction(playOrPauseAct);
    popup->addAction(stopAct);
    popup->addAction(playAct);
    popup->addAction(playNewAct);
    popup->addSeparator();
    popup->addMenu(inOutMenu);
    popup->addSeparator();
    popup->addMenu(add_menu);
    popup->addMenu(remove_menu);
    popup->addSeparator();
    popup->addAction(propertiesAct);
    popup->addAction(openDirectoryAct);
    popup->addAction(refreshAct);

    connect(playlistWidget, &TPlaylistWidget::customContextMenuRequested,
            this, &TPlaylist::showContextMenu);
}

void TPlaylist::retranslateStrings() {

    setPlaylistTitle();
}

void TPlaylist::getFilesToPlay(QStringList& files) const {

    TPlaylistWidgetItem* root = playlistWidget->root();
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
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        QString fn = i->filename();
        if (!fn.isEmpty()) {
            files.append(fn);
        }
        it++;
    }
}

void TPlaylist::clear() {

    abortThread();
    playlistWidget->clr();
    filename = "";
    setPlaylistTitle();
}

void TPlaylist::onThreadFinished() {
    WZDEBUG("");

    if (thread == 0) {
        // Only during destruction, so no need to enable actions
        WZDEBUG("thread is gone");
        return;
    }

    // Get data from thread
    TPlaylistWidgetItem* root = thread->root;
    thread->root = 0;
    if (!thread->latestDir.isEmpty()) {
        pref->last_dir = thread->latestDir;
    }

    // Clean up
    delete thread;
    thread = 0;

    if (root == 0) {
        // Thread aborted
        if (restartThread) {
            WZDEBUG("thread aborted, starting new request");
            addFilesStartThread();
        } else {
            WZDEBUG("thread aborted");
            addFilesFiles.clear();
            enableActions();
        }
        return;
    }

    QString msg = addFilesFiles.count() == 1 ? addFilesFiles[0] : "";
    addFilesFiles.clear();

    if (root->childCount() == 0) {
        if (msg.isEmpty()) {
            msg = tr("Found nothing to play.");
        } else {
            msg = tr("Found no files to play in '%1'.").arg(msg);
        }

        delete root;
        enableActions();

        QMessageBox::information(this, tr("Nothing to play"), msg);
        return;
    }

    // Returns a newly created root
    root = playlistWidget->add(root, addFilesTarget);
    if (root) {
        filename = root->filename();
        WZDEBUG("filename set to '" + filename + "'");
        setPlaylistTitle();

        // Let the player have a go at a failed file name
        if (root->childCount() == 1
            && root->plChild(0)->state() == PSTATE_FAILED) {
            root->plChild(0)->setState(PSTATE_LOADING);
        }
    }

    if (addFilesStartPlay) {
        if (!addFilesFileToPlay.isEmpty()) {
            TPlaylistWidgetItem* w = findFilename(addFilesFileToPlay);
            if (w) {
                playItem(w);
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
        WZDEBUG("aborting add files thread");
        addFilesStartPlay = false;
        restartThread = false;
        thread->abort();
    }
}

void TPlaylist::addFilesStartThread() {

    if (thread) {
        // Thread still running, abort it and restart it in onThreadFinished()
        WZDEBUG("add files thread still running. Aborting it...");
        restartThread = true;
        thread->abort();
    } else {
        WZDEBUG("starting add files thread");
        restartThread = false;

        // Allow single image
        bool addImages = pref->addImages
                         || ((addFilesFiles.count() == 1)
                             && extensions.isImage(addFilesFiles.at(0)));

        thread = new TAddFilesThread(this,
                                     addFilesFiles,
                                     pref->nameBlacklist,
                                     pref->addDirectories,
                                     pref->addVideo,
                                     pref->addAudio,
                                     pref->addPlaylists,
                                     addImages);

        connect(thread, &TAddFilesThread::finished,
                this, &TPlaylist::onThreadFinished);
        connect(thread, &TAddFilesThread::displayMessage,
                msgSlot, &TMsgSlot::msg);

        thread->start();
        enableActions();
    }
}

void TPlaylist::addFiles(const QStringList& files,
                         bool startPlay,
                         QTreeWidgetItem* target,
                         const QString& fileToPlay,
                         bool searchForItems) {
    debug << "addFiles: files" << files << "startPlay" << startPlay << debug;

    addFilesFiles = files;
    addFilesStartPlay = startPlay;
    addFilesTarget = dynamic_cast<TPlaylistWidgetItem*>(target);
    addFilesFileToPlay = fileToPlay;
    addFilesSearchItems = searchForItems;

    addFilesStartThread();
}

void TPlaylist::addFilesDialog() {

    QStringList files = TFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        playlistWidget->setModified(playlistWidget->currentItem());
        addFiles(files, false, playlistWidget->currentItem());
    }
}

void TPlaylist::addCurrentFile() {
   WZDEBUG("");

    if (!player->mdat.filename.isEmpty()) {
        TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
            playlistWidget->root(),
            player->mdat.filename,
            player->mdat.name(),
            player->mdat.duration,
            false);
        i->setPlayed(true);
        i->setModified();
    }
}

void TPlaylist::addDirectory() {

    QString s = TFileDialog::getExistingDirectory(this,
                    tr("Choose a directory"), pref->last_dir);

    if (!s.isEmpty()) {
        addFiles(QStringList() << s, false, playlistWidget->currentItem());
    }
}

void TPlaylist::addUrls() {

    TMultilineInputDialog d(this);
    if (d.exec() == QDialog::Accepted && d.lines().count() > 0) {
        playlistWidget->setModified(playlistWidget->currentItem());
        addFiles(d.lines(), false, playlistWidget->currentItem());
    }
}

void TPlaylist::addRemovedItem(QString item) {
    WZDEBUG("'" + item + "'");

    if (!filename.isEmpty() || saveAs()) {
        refresh();
    }
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

    TPlaylistWidgetItem* item = playlistWidget->currentPlaylistWidgetItem();
    if (item) {
        playItem(item);
    } else {
        player->play();
    }
}

void TPlaylist::playOrPause() {
    WZDEBUG("state " + player->stateToString());

    if (player->state() == Player::STATE_PLAYING) {
        player->pause();
    } else if (player->state() == Player::STATE_PAUSED) {
        player->play();
    } else if (playlistWidget->playing_item) {
        playItem(playlistWidget->playing_item);
    } else if (playlistWidget->currentItem()) {
        playItem(playlistWidget->currentPlaylistWidgetItem());
    } else {
        player->play();
    }
}

void TPlaylist::stop() {
    WZDEBUG("state " + player->stateToString());

    abortThread();
    player->stop();
}

TPlaylistWidgetItem* TPlaylist::getRandomItem() const {

    bool foundFreeItem = false;
    int selected = (int) ((double) playlistWidget->countChildren() * qrand()
                          / (RAND_MAX + 1.0));
    bool foundSelected = false;

    do {
        int idx = 0;
        QTreeWidgetItemIterator it(playlistWidget);
        while (*it) {
            TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
            if (!i->isFolder()) {
                if (idx == selected) {
                    foundSelected = true;
                }

                if (!i->played() && i->state() != PSTATE_FAILED) {
                    if (foundSelected) {
                        WZDEBUG("selecting '" + i->filename() + "'");
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

    WZDEBUG("end of playlist");
    return 0;
}

bool TPlaylist::haveUnplayedItems() const {

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder() && !i->played() && i->state() != PSTATE_FAILED) {
            return true;
        }
        ++it;
    }

    return false;
}

void TPlaylist::startPlay() {
    WZDEBUG("");

    TPlaylistWidgetItem* item = playlistWidget->firstPlaylistWidgetItem();
    if (item) {
        if (shuffleAct->isChecked()) {
            playItem(getRandomItem());
        } else {
            playItem(item);
        }
    } else {
        WZINFO("nothing to play");
    }
}

void TPlaylist::playItem(TPlaylistWidgetItem* item) {

    while (item && item->isFolder()) {
        item = playlistWidget->getNextPlaylistWidgetItem(item);
    }
    if (item) {
        WZDEBUG("'" + item->filename() + "'");
        playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        player->open(item->filename(), playlistWidget->hasSingleItem());
    } else {
        WZDEBUG("end of playlist");
        msg(tr("End of playlist"), 0);
        emit playlistFinished();
    }
}

void TPlaylist::playNext(bool loop_playlist) {
    WZDEBUG("");

    TPlaylistWidgetItem* item = 0;
    if (shuffleAct->isChecked()) {
        item = getRandomItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            playlistWidget->clearPlayed();
            item = getRandomItem();
        }
    } else {
        item = playlistWidget->getNextPlaylistWidgetItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            item = playlistWidget->firstPlaylistWidgetItem();
        }
    }
    playItem(item);
}

void TPlaylist::playPrev() {
    WZDEBUG("");

    TPlaylistWidgetItem* i = playlistWidget->playing_item;
    if (i && shuffleAct->isChecked()) {
        i = playlistWidget->findPreviousPlayedTime(i);
    } else {
        i = playlistWidget->getPreviousPlaylistWidgetItem();
    }
    if (i == 0) {
        i = playlistWidget->lastPlaylistWidgetItem();
    }
    if (i) {
        playItem(i);
    }
}

void TPlaylist::playDirectory(const QString &dir) {
    WZDEBUG("'" + dir + "'");

    if (TWZFiles::directoryContainsDVD(dir)) {
        // onNewMediaStartedPlaying() will pickup the playlist
        player->open(dir);
    } else {
        clear();
        addFiles(QStringList() << dir, true);
    }
}

void TPlaylist::resumePlay() {

    TPlaylistWidgetItem* item = playlistWidget->firstPlaylistWidgetItem();
    if (item) {
        playItem(item);
    }
}

bool TPlaylist::deleteFileFromDisk(const QString& filename,
                                   const QString& playingFile) {

    QFileInfo fi(filename);
    if (!fi.exists()) {
        return true;
    }

    if (!fi.isFile()) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot delete '%1', it does not seem to be a file.")
            .arg(filename));
        return false;
    }

    // Ask the user for confirmation
    int res = QMessageBox::question(this, tr("Confirm file deletion"),
        tr("You're about to delete '%1' from your drive.").arg(filename)
        + "<br>"+
        tr("This action cannot be undone. Are you sure you want to proceed?"),
        QMessageBox::Yes, QMessageBox::No);

    if (res == QMessageBox::Yes) {
        // Cannot delete file on Windows when in use...
        if (filename == playingFile && player->state() != Player::STATE_STOPPED) {
            player->stop();
        }
        if (QFile::remove(filename)) {
            return true;
        }
        QMessageBox::warning(this, tr("Deletion failed"),
                             tr("Failed to delete '%1'").arg(filename));
    }

    return false;
}

void TPlaylist::removeSelected(bool deleteFromDisk) {
    WZDEBUG("");

    if (!isActiveWindow()) {
        WZINFO("ignoring remove actiom while not active window");
        return;
    }
    if (!isVisible()) {
        WZINFO("ignoring remove action while not visible");
        return;
    }

    disable_enableActions = true;
    TPlaylistWidgetItem* root = playlistWidget->root();

    // Save currently playing item, which might be deleted
    QString playing = playingFile();

    // Move current out of selection
    TPlaylistWidgetItem* newCurrent =
            playlistWidget->currentPlaylistWidgetItem();
    do {
        newCurrent = playlistWidget->getNextItem(newCurrent, false);
    } while (newCurrent && newCurrent->isSelected());

    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (i != root
            && (!deleteFromDisk
                || deleteFileFromDisk(i->filename(), playing))) {
            WZDEBUG("removing '" + i->filename() + "'");

            TPlaylistWidgetItem* parent = i->plParent();
            if (parent) {
                parent->blacklist(i->fname());
            }
            delete i;

            // Clean up empty folders
            while (parent && parent->childCount() == 0 && parent != root) {
                TPlaylistWidgetItem* gp = parent->plParent();
                if (parent == newCurrent) {
                    newCurrent = gp;
                }
                if (gp) {
                    gp->blacklist(parent->fname());
                }

                if (parent->isWZPlaylist()) {
                    if (QFile::remove(parent->filename())) {
                        WZINFO("removed '" + parent->filename() + "' from disk");
                    } else {
                        WZERROR("failed to remove '" + parent->filename()
                                + "' from disk");
                    }
                }

                delete parent;
                parent = gp;
            }
            if (parent) {
                playlistWidget->setModified(parent);
            }
        }
        it++;
    }

    playlistWidget->playing_item = findFilename(playing);
    if (newCurrent && newCurrent != root) {
        playlistWidget->setCurrentItem(newCurrent);
    } else {
        playlistWidget->setCurrentItem(
            playlistWidget->firstPlaylistWidgetItem());
    }

    if (filename.isEmpty() && !playlistWidget->hasItems()) {
        playlistWidget->clearModified();
    }

    disable_enableActions = false;
    enableActions();
}

void TPlaylist::removeSelectedFromDisk() {
    removeSelected(true);
}

void TPlaylist::removeAll() {
    clear();
}

void TPlaylist::refresh() {

    if (!filename.isEmpty() && maybeSave()) {
        QString fn = filename;
        QString current;
        if (player->state() == Player::STATE_PLAYING) {
            current = playingFile();
            if (!current.isEmpty()) {
                player->saveRestartTime();
                player->stop();
            }
        }
        clear();
        addFiles(QStringList() << fn, !current.isEmpty(), 0, current);
    }
}

void TPlaylist::openFolder() {

    TPlaylistWidgetItem* i = playlistWidget->currentPlaylistWidgetItem();
    if (i) {
        QUrl url;
        QFileInfo fi(i->filename());
        if (fi.exists()) {
            if (i->isPlaylist()
                || !i->isFolder()
                || fi.fileName() == TConfig::WZPLAYLIST) {
                url = QUrl::fromLocalFile(fi.absolutePath());
            } else {
                url = QUrl::fromLocalFile(i->filename());
            }
        } else {
            url.setUrl(i->filename());
        }

        debug << "openFolder: opening" << url;
        debug << debug;
        QDesktopServices::openUrl(url);
    }
}

void TPlaylist::openInNewWindow() {
    WZDEBUG("'" + qApp->applicationFilePath() + "'");

    QStringList files;
    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        files << i->filename();
        ++it;
    }

    QProcess p;
    if (p.startDetached(qApp->applicationFilePath(), files)) {
        WZINFO("started new instance");
    } else {
        WZERROR("failed to start '" + qApp->applicationFilePath() + "'");
        QMessageBox::warning(this, tr("Start failed"),
                             tr("Failed to start '%1'")
                             .arg(qApp->applicationFilePath()),
                             QMessageBox::Ok);
    }
}

void TPlaylist::showContextMenu(const QPoint & pos) {

    if (!popup->isVisible()) {
        Menu::execPopup(this, popup,
                        playlistWidget->viewport()->mapToGlobal(pos));
    }
}

void TPlaylist::onItemActivated(QTreeWidgetItem* item, int) {
    WZDEBUG("");

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(item);
    if (i && !i->isFolder()) {
        playItem(i);
    }
}

void TPlaylist::enableActions() {

    if (disable_enableActions) {
        WZDEBUG("disabled");
        return;
    }
    WZDEBUG("state " + player->stateToString());

    // Note: there is always something selected if there are items
    bool haveItems = playlistWidget->hasItems();

    TPlaylistWidgetItem* playing_item = playlistWidget->playing_item;
    TPlaylistWidgetItem* current_item =
            playlistWidget->currentPlaylistWidgetItem();

    Player::TState s = player->state();
    bool enable = (s == Player::STATE_STOPPED || s == Player::STATE_PLAYING
                   || s == Player::STATE_PAUSED)
                  && thread == 0;
    bool e = enable && haveItems;
    bool playerHasFilename = player->mdat.filename.count();

    openAct->setEnabled(thread == 0);
    saveAct->setEnabled(e);
    saveAsAct->setEnabled(e);

    playAct->setEnabled(enable && (playerHasFilename || haveItems));
    pauseAct->setEnabled(s == Player::STATE_PLAYING);

    playOrPauseAct->setEnabled(enable && (playerHasFilename || haveItems));

    if (!enable) {
        QString text;
        if (thread) {
            text = tr("loading");
        } else {
            text = player->stateToString();
        }
        playOrPauseAct->setTextAndTip(text + "...");
        playOrPauseAct->setIcon(Images::icon("loading"));
    } else if (s == Player::STATE_PLAYING) {
        disable_enableActions = true;
        if (playing_item == 0) {
            playing_item = findFilename(player->mdat.filename);
        }
        if (playing_item && playing_item->state() != PSTATE_PLAYING) {
            playlistWidget->setPlayingItem(playing_item, PSTATE_PLAYING);
        }
        playOrPauseAct->setTextAndTip(tr("&Pause"));
        playOrPauseAct->setIcon(Images::icon("pause"));
        disable_enableActions = false;
    } else {
        playOrPauseAct->setTextAndTip(tr("&Play"));
        playOrPauseAct->setIcon(Images::icon("play"));
    }

    // Stop action
    stopAct->setEnabled(thread
                        || s == Player::STATE_PLAYING
                        || s == Player::STATE_PAUSED
                        || s == Player::STATE_RESTARTING
                        || s == Player::STATE_LOADING);

    // Prev/Next
    bool changed = false;
    if (e != nextAct->isEnabled()) {
        nextAct->setEnabled(e);
        changed = true;
    }
    if (e != prevAct->isEnabled()) {
        prevAct->setEnabled(e);
        changed = true;
    }
    if (changed) {
        // Update forward/rewind menus
        emit enablePrevNextChanged();
    }

    addCurrentAct->setEnabled(playerHasFilename && thread == 0);

    e = haveItems && thread == 0;
    removeSelectedAct->setEnabled(e);
    removeSelectedFromDiskAct->setEnabled(e && current_item
                                          && !current_item->isFolder());
    removeAllAct->setEnabled(e);

    editNameAct->setEnabled(e && current_item);
    newFolderAct->setEnabled(thread == 0);
    findPlayingAct->setEnabled(playing_item);
    cutAct->setEnabled(e);
    copyAct->setEnabled(haveItems || playerHasFilename);

    openDirectoryAct->setEnabled(current_item);
    refreshAct->setEnabled(enable && !filename.isEmpty());
}

void TPlaylist::onPlayerError() {

    if (player->state() != Player::STATE_STOPPING) {
        TPlaylistWidgetItem* item = playlistWidget->playing_item;
        if (item) {
            if (item->filename() == player->mdat.filename) {
                item->setState(PSTATE_FAILED);
                playlistWidget->scrollToItem(item);
            }
            playlistWidget->setPlayingItem(0);
        }
    }
}

void TPlaylist::onNewMediaStartedPlaying() {

    const TMediaData* md = &player->mdat;
    QString filename = md->filename;
    QString current_filename = playlistWidget->playingFile();

    if (md->disc.valid) {
        if (md->titles.count() == playlistWidget->countItems()) {
            TDiscName cur_disc(current_filename);
            if (cur_disc.valid
                && cur_disc.protocol == md->disc.protocol
                && cur_disc.device == md->disc.device) {
                WZDEBUG("item is from current disc");
                return;
            }
        }
    } else if (filename == current_filename) {
        TPlaylistWidgetItem* item = playlistWidget->playing_item;
        if (item == 0) {
            return;
        }
        bool modified = false;

        // Update item name
        if (!item->edited()) {
            QString name = md->name();
            if (name != item->baseName()) {
                WZDEBUG("updating name from '" + item->baseName() + "' to '"
                        + name + "'");
                item->setName(name, item->extension(), false);
                modified = true;
            }
        }

        // Update item duration
        if (!md->image && md->duration > 0) {
            if (!this->filename.isEmpty()
                && qAbs(md->duration - item->duration()) > 1) {
                modified = true;
            }
            WZDEBUG("updating duration from "
                    + QString::number(item->duration()) + " to "
                    + QString::number(md->duration));
            item->setDuration(md->duration);
        }

        if (modified) {
            playlistWidget->setModified(item);
        } else {
            WZDEBUG("item considered uptodate");
        }
        return;
    }

    // Create new playlist
    WZDEBUG("creating new playlist for '" + filename + "'");
    clear();

    if (md->disc.valid) {
        // Add disc titles without sorting
        playlistWidget->setSort(-1, Qt::AscendingOrder);
        TDiscName disc = md->disc;
        foreach(const Maps::TTitleData title, md->titles) {
            disc.title = title.getID();
            TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
                playlistWidget->root(),
                disc.toString(),
                title.getDisplayName(false),
                title.getDuration(),
                false);
            if (title.getID() == md->titles.getSelectedID()) {
                playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
            }
        }
        TPlaylistWidgetItem* root = playlistWidget->root();
        root->setFilename(filename, md->name());
    } else {
        // Add current file
        TPlaylistWidgetItem* current = new TPlaylistWidgetItem(
            playlistWidget->root(),
            filename,
            md->name(),
            md->duration,
            false);
        playlistWidget->setPlayingItem(current, PSTATE_PLAYING);

        // Add associated files to playlist
        // TODO: remove from main thread
        if (md->selected_type == TMediaData::TYPE_FILE
            && pref->mediaToAddToPlaylist != TPreferences::NoFiles) {
            WZDEBUG("searching for files to add to playlist for '" + filename
                    + "'");
            QStringList files_to_add = TWZFiles::filesForPlaylist(filename,
                pref->mediaToAddToPlaylist);
            if (files_to_add.isEmpty()) {
                WZDEBUG("none found");
            } else {
                addFiles(files_to_add);
            }
        }
    }

    setPlaylistTitle();
    WZDEBUG("created new playlist for '" + filename + "'");
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

    TPlaylistWidgetItem* i = playlistWidget->findFilename(filename);
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
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
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
        QTreeWidgetItem* parent = playlistWidget->currentItem();
        if (parent && parent->childCount() == 0) {
            parent = parent->parent();
        }
        playlistWidget->setModified(parent);
        addFiles(files, false, playlistWidget->currentItem(), "", true);
    }
}

void TPlaylist::enablePaste() {
    pasteAct->setEnabled(QApplication::clipboard()->mimeData()->hasText());
}

void TPlaylist::cut() {

    copySelection(tr("Cut"));
    removeSelected();
}

void TPlaylist:: setPlaylistTitle() {

    QString title;
    TPlaylistWidgetItem* root = playlistWidget->root();
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

void TPlaylist::editName() {
    WZDEBUG("");

    TPlaylistWidgetItem* current = playlistWidget->currentPlaylistWidgetItem();
    if (current) {
        current->setFlags(current->flags() | Qt::ItemIsEditable);
        playlistWidget->editItem(current, TPlaylistWidgetItem::COL_NAME);
        current->setFlags(current->flags() & ~Qt::ItemIsEditable);
    }
}

void TPlaylist::newFolder() {
    WZDEBUG("");

    TPlaylistWidgetItem* current = playlistWidget->currentPlaylistWidgetItem();
    if (current) {
        if (!current->isFolder()) {
            current = current->plParent();
        }
        QString base = tr("New folder");
        QString path = current->pathPlusSep();
        QDir dir(path);

        int i = 2;
        QString name = base;
        while (dir.exists(name)) {
            name = base + " " + QString::number(i++);
        }

        QString fn = path + name;

        if (dir.mkdir(name)) {
            TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
                current,
                fn,
                name,
                0,
                false);
            i->setModified();
            playlistWidget->setCurrentItem(i);
            editName();
        } else {
            WZERROR("Failed to create directory '" + fn + "'");
            QMessageBox::warning (this,
                tr("Error"),
                tr("Failed to create folder '%1'").arg(fn),
                QMessageBox::Ok,
                QMessageBox::NoButton,
                QMessageBox::NoButton);

        }
    }
}

void TPlaylist::findPlayingItem() {

    TPlaylistWidgetItem* i = playlistWidget->playing_item;
    if (i) {
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
        foreach(const QUrl url, e->mimeData()->urls()) {
            files.append(url.toString());
        }

        if (files.count()) {
            // TODO: see dropIndicator for above/below
            QTreeWidgetItem* target = playlistWidget->itemAt(e->pos()
                - playlistWidget->pos() - playlistWidget->viewport()->pos());

            if (target) {
                QTreeWidgetItem* parent;
                if (target->childCount()) {
                    parent = target;
                } else {
                    parent = target->parent();
                }
                playlistWidget->setModified(parent);
            }

            addFiles(files, false, target);
        }

        e->accept();
        return;
    }
    QWidget::dropEvent(e);
}

void TPlaylist::openPlaylist(const QString& filename) {

    pref->last_dir = QFileInfo(filename).absolutePath();
    addFiles(QStringList() << filename, true);
}

void TPlaylist::open() {

    if (maybeSave()) {
        QString s = TFileDialog::getOpenFileName(this, tr("Choose a file"),
            pref->last_dir,
            tr("Playlists") + extensions.playlists().forFilter() + ";;"
            + tr("All files") +" (*.*)");

        if (!s.isEmpty()) {
            openPlaylist(s);
        }
    }
}

bool TPlaylist::saveM3uFolder(TPlaylistWidgetItem* folder,
                              const QString& path,
                              QTextStream& stream,
                              bool linkFolders) {
    WZDEBUG("saving '" + folder->filename() + "'");

    bool result = true;
    for(int idx = 0; idx < folder->childCount(); idx++) {
        TPlaylistWidgetItem* i = folder->plChild(idx);
        QString filename = i->filename();

        if (i->isPlaylist()) {
            bool modified = i->modified();

            // Switch pls to m3u8
            QFileInfo fi(i->filename());
            if (fi.suffix().toLower() == "pls") {
                // Replace extension pls with m3u8
                WZWARN("saving '" + i->filename() + "' as m3u8");
                filename = filename.left(filename.length() - 4) + ".m3u8";
                i->setFilename(filename, i->baseName());
                modified = true;
            }

            if (modified) {
                if (!saveM3u(i, filename, fi.fileName() == TConfig::WZPLAYLIST)) {
                    result = false;
                }
            } else {
                WZINFO("playlist '" + i->filename() + "' not modified");
            }
        } else if (i->isFolder()) {
            if (linkFolders) {
                if (i->modified()) {
                    QFileInfo fi(i->filename(), TConfig::WZPLAYLIST);
                    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
                    if (!saveM3u(i, filename, linkFolders)) {
                        result = false;
                    }
                } else {
                    WZINFO("folder not modified '" + i->filename() + "'");
                }
            } else {
                if (saveM3uFolder(i, path, stream, linkFolders)) {
                    WZINFO("succesfully saved '" + i->filename() + "'");
                } else {
                    result = false;
                }
                continue;
            }
        } else {
            stream << "#EXTINF:" << (int) i->duration()
                   << "," << i->baseName() << "\n";
        }

        if (filename.startsWith(path)) {
            filename = filename.mid(path.length());
        }
        stream << filename << "\n";
    }

    return result;
}

bool TPlaylist::saveM3u(TPlaylistWidgetItem* folder,
                        const QString& filename,
                        bool wzplaylist) {
    WZDEBUG("saving '" + filename + "'");

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        // Ok to fail on wzplaylist
        if (wzplaylist) {
            WZINFO("ignoring failed save '" + filename + "'");
            return true;
        }

        WZERROR("failed to save '" + filename + "'");

        // TODO: skip remaining  msgs...
        QMessageBox::warning(this, tr("Save failed"),
                             tr("Failed to open \"%1\" for writing.")
                             .arg(filename), QMessageBox::Ok);

        return false;
    }

    QTextStream stream(&f);
    if (QFileInfo(filename).suffix().toLower() != "m3u") {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    stream << "#EXTM3U" << "\n"
           << "# Playlist created by WZPlayer " << TVersion::version << "\n";

    if (wzplaylist) {
        foreach(const QString& fn, folder->getBlacklist()) {
            WZDEBUG("blacklisting '" + fn + "'");
            stream << "#WZP-blacklist:" << fn << "\n";
        }
    }

    bool result = saveM3uFolder(folder, path, stream, wzplaylist);

    stream.flush();
    f.close();

    WZDEBUG("saved '" + filename + "'");
    return result;
}

bool TPlaylist::saveM3u(const QString& filename, bool linkFolders) {
    WZDEBUG("");

    TPlaylistWidgetItem* root = playlistWidget->root();
    return saveM3u(root, filename, linkFolders);
}

bool TPlaylist::savePls(const QString& filename) {
    WZDEBUG("'" + filename + "'");

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    QSettings set(filename, QSettings::IniFormat);
    set.beginGroup("playlist");

    int n = 0;
    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            QString filename = i->filename();
            if (filename.startsWith(path)) {
                filename = filename.mid(path.length());
            }
            QString ns = QString::number(n + 1);
            set.setValue("File" + ns, filename);
            set.setValue("Title" + ns, i->baseName());
            set.setValue("Length" + ns, (int) i->duration());
            n++;
        }
        ++it;
    }

    set.setValue("NumberOfEntries", n);
    set.setValue("Version", 2);

    set.endGroup();
    set.sync();

    if (set.status() == QSettings::NoError) {
        return true;
    }

    WZERROR("failed to save '" + filename + "'");
    QMessageBox::warning(this, tr("Save failed"),
                         tr("Failed to save '%1'").arg(filename),
                         QMessageBox::Ok);
    return false;
}

bool TPlaylist::save() {
    WZINFO("'" + filename + "'");

    if (filename.isEmpty()) {
        return saveAs();
    }

    bool wzplaylist = false;
    QFileInfo fi(filename);
    if (fi.isDir()) {
        fi.setFile(fi.absoluteFilePath(), TConfig::WZPLAYLIST);
        wzplaylist = true;
    } else if (fi.fileName() == TConfig::WZPLAYLIST) {
        wzplaylist = true;
    }
    msgOSD(tr("Saving %1").arg(fi.fileName()), 0);

    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
    TPlaylistWidgetItem* root = playlistWidget->root();
    root->setFilename(filename, fi.completeBaseName());
    setPlaylistTitle();
    pref->last_dir = fi.absolutePath();

    bool result;
    if (fi.suffix().toLower() == "pls") {
        result = savePls(filename);
    } else {
        result = saveM3u(filename, wzplaylist);
    }

    if (result) {
        playlistWidget->clearModified();
        WZINFO("succesfully saved '" + fi.absoluteFilePath() + "'");
        msgOSD(tr("Saved '%1'").arg(fi.fileName()));
    } else {
        // Error box and log already done, but need to remove 0 secs save msg
        msgOSD(tr("Failed to save '%1'").arg(fi.fileName()));
    }

    return result;
}

bool TPlaylist::saveAs() {

    QString s = TFileDialog::getSaveFileName(this, tr("Choose a filename"),
        pref->last_dir, tr("Playlists") + extensions.playlists().forFilter());

    if (s.isEmpty()) {
        return false;
    }

    // If filename has no extension, add it
    QFileInfo fi(s);
    if (fi.suffix().isEmpty()) {
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

    filename = fi.absoluteFilePath();
    return save();
}

bool TPlaylist::maybeSave() {

    if (!playlistWidget->modified()) {
        WZDEBUG("playlist not modified");
        return true;
    }

    if (filename.isEmpty()) {
        WZDEBUG("discarding changes to unnamed playlist");
        return true;
    }

    if (QFileInfo(filename).fileName() == TConfig::WZPLAYLIST) {
        WZDEBUG("auto saving wzplaylist.m3u8");
        return save();
    }

    if (!playlistWidget->root()->isPlaylist()) {
        // Directorie
        if (pref->useDirectoriePlaylists) {
            WZDEBUG("auto saving directorie");
            return save();
        }
        WZDEBUG("discarding changes to directorie playlist");
        return true;
    }

    int res = QMessageBox::question(this, tr("Playlist modified"),
                                    tr("The playlist has been modified, do you"
                                       " want to save the changes to \"%1\"?")
                                    .arg(filename),
                                    QMessageBox::Yes, QMessageBox::No,
                                    QMessageBox::Cancel);

    switch (res) {
        case QMessageBox::No:
            playlistWidget->clearModified();
            WZINFO("selected no save");
            return true;
        case QMessageBox::Cancel:
            WZINFO("selected canceling save");
            return false;
        default: return save();
    }
}

void TPlaylist::saveSettings() {
    WZDEBUG("");

    pref->beginGroup("playlist");
    pref->setValue("repeat", repeatAct->isChecked());
    pref->setValue("shuffle", shuffleAct->isChecked());
    pref->endGroup();
}

void TPlaylist::loadSettings() {
    WZDEBUG("");

    pref->beginGroup("playlist");
    repeatAct->setChecked(pref->value("repeat",
                                      repeatAct->isChecked()).toBool());
    shuffleAct->setChecked(pref->value("shuffle",
                                       shuffleAct->isChecked()).toBool());
    pref->endGroup();
}

} // namespace Playlist
} // namespace Gui

#include "moc_playlist.cpp"

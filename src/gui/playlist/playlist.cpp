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
#include "gui/action/menuinoutpoints.h"
#include "gui/action/action.h"
#include "gui/msg.h"
#include "images.h"
#include "helper.h"
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
    debug(logger()),
    playlistWidget(plWidget) {

    connect(this, SIGNAL(triggered(QAction*)),
            this, SLOT(onTriggered(QAction*)));

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
            logger()->debug("onAboutToShow: '%1'", item->filename());

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
                logger()->debug("onTriggered: whitelisted '%1'", s);
                playlistWidget->setModified(item);
                emit addRemovedItem(s);
            } else {
                logger()->warn("onTriggered: '%1' not blacklisted", s);
            }
        } else {
            logger()->warn("onTriggered: item no longer existing");
        }
    }
}


TPlaylist::TPlaylist(TMainWindow* mw) :
    QWidget(mw),
    debug(logger()),
    main_window(mw),
    thread(0),
    restartThread(false),
    disable_enableActions(false) {

    setAcceptDrops(true);

    createTree();
    createActions();
    createToolbar();

    connect(player, SIGNAL(newMediaStartedPlaying()),
            this, SLOT(onNewMediaStartedPlaying()));
    connect(player, SIGNAL(playerError(int)),
            this, SLOT(onPlayerError()));
    connect(player, SIGNAL(titleTrackChanged(int)),
            this, SLOT(onTitleTrackChanged(int)));
    connect(player, SIGNAL(mediaEOF()),
            this, SLOT(onMediaEOF()), Qt::QueuedConnection);
    connect(player, SIGNAL(noFileToPlay()),
            this, SLOT(resumePlay()), Qt::QueuedConnection);

    connect(main_window, SIGNAL(enableActions()),
            this, SLOT(enableActions()));

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

    connect(playlistWidget, SIGNAL(modifiedChanged()),
            this, SLOT(onModifiedChanged()));
    connect(playlistWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
             this, SLOT(onItemActivated(QTreeWidgetItem*, int)));
    connect(playlistWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,
                                                      QTreeWidgetItem*)),
            this, SLOT(enableActions()));
}

void TPlaylist::createActions() {

    // Open
    openAct = new TAction(this, "pl_open", tr("Open &playlist..."), "",
                          QKeySequence("Ctrl+P"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    // Save
    saveAct = new TAction(this, "pl_save", tr("&Save playlist"), "save",
                          QKeySequence("Ctrl+S"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    // SaveAs
    saveAsAct = new TAction(this, "pl_saveas", tr("S&ave playlist as..."),
                            "saveas");
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    // Properties
    propertiesAct = new TAction(this, "view_properties",
        tr("&View properties..."), "info", Qt::SHIFT | Qt::Key_P);
    propertiesAct->setCheckable(true);
    connect(propertiesAct, SIGNAL(triggered(bool)),
            main_window, SLOT(showFilePropertiesDialog(bool)));

    // Open directory
    openDirectoryAct = new TAction(this, "pl_open_directory",
                                   tr("&Open directory"));
    openDirectoryAct->setIcon(style()->standardPixmap(QStyle::SP_DirOpenIcon));
    connect(openDirectoryAct, SIGNAL(triggered()), this, SLOT(openFolder()));

    // Refresh
    refreshAct = new TAction(this, "pl_refresh", tr("R&efresh playlist"), "",
                             Qt::Key_F5);
    connect(refreshAct, SIGNAL(triggered()), this, SLOT(refresh()));

    // Stop
    stopAct = new TAction(this, "stop", tr("&Stop"), "", Qt::Key_MediaStop);
    connect(stopAct, SIGNAL(triggered()), this, SLOT(stop()));

    // Play
    playAct = new TAction(this, "play", tr("P&lay selected"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, SIGNAL(triggered()), this, SLOT(play()));

    // Play in new window
    playNewAct = new TAction(this, "play_new", tr("Pl&ay in new window"),
                             "play", Qt::CTRL | Qt::Key_Space);
    connect(playNewAct, SIGNAL(triggered()), this, SLOT(openInNewWindow()));

    // Pause
    pauseAct = new TAction(this, "pause", tr("Pause"), "",
                           QKeySequence("Media Pause")); // MCE remote key
    connect(pauseAct, SIGNAL(triggered()), player, SLOT(pause()));

    // Play/pause
    playOrPauseAct = new TAction(this, "play_or_pause", tr("&Play"), "play",
                                 Qt::Key_Space);
    // Add MCE remote key
    playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playOrPauseAct, SIGNAL(triggered()), this, SLOT(playOrPause()));

    // Next
    nextAct = new TAction(this, "pl_next", tr("Play &next"), "next",
                          QKeySequence(">"));
    nextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
    connect(nextAct, SIGNAL(triggered()), this, SLOT(playNext()));

    // Prev
    prevAct = new TAction(this, "pl_prev", tr("Play pre&vious"), "previous",
                          QKeySequence("<"));
    prevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
    connect(prevAct, SIGNAL(triggered()), this, SLOT(playPrev()));

    // In-out menu
    inOutMenu = new TMenuInOut(main_window);
    addActions(inOutMenu->actions());

    // Repeat
    repeatAct = inOutMenu->findChild<TAction*>("pl_repeat");
    connect(repeatAct, SIGNAL(triggered(bool)),
            this, SLOT(onRepeatToggled(bool)));

    // Shuffle
    shuffleAct = inOutMenu->findChild<TAction*>("pl_shuffle");
    connect(shuffleAct, SIGNAL(triggered(bool)),
            this, SLOT(onShuffleToggled(bool)));

    // Add menu
    add_menu = new TMenu(this, main_window, "pl_add_menu",
                         tr("&Add to playlist"), "plus");

    addCurrentAct = new TAction(add_menu, "pl_add_current",
                                tr("Add &current file"));
    connect(addCurrentAct, SIGNAL(triggered()), this, SLOT(addCurrentFile()));

    addFilesAct = new TAction(add_menu, "pl_add_files", tr("Add &file(s)..."));
    connect(addFilesAct, SIGNAL(triggered()), this, SLOT(addFiles()));

    addDirectoryAct = new TAction(add_menu, "pl_add_directory",
                                  tr("Add &directory..."));
    connect(addDirectoryAct, SIGNAL(triggered()), this, SLOT(addDirectory()));

    addUrlsAct = new TAction(add_menu, "pl_add_urls", tr("Add &URL(s)..."));
    connect(addUrlsAct, SIGNAL(triggered()), this, SLOT(addUrls()));

    // Add removed sub menu
    add_removed_menu = new TAddRemovedMenu(add_menu, main_window,
                                               playlistWidget);
    add_menu->addMenu(add_removed_menu);
    connect(add_removed_menu, SIGNAL(addRemovedItem(QString)),
            this, SLOT(addRemovedItem(QString)));

    addActions(add_menu->actions());

    // Remove menu
    remove_menu = new TMenu(this, main_window, "pl_remove_menu",
                            tr("&Remove from playlist"), "minus");

    removeSelectedAct = new TAction(remove_menu, "pl_remove_selected",
                                    tr("&Remove from list"), "",
                                    Qt::Key_Delete);
    connect(removeSelectedAct, SIGNAL(triggered()),
            this, SLOT(removeSelected()));

    removeSelectedFromDiskAct = new TAction(remove_menu, "pl_delete_from_disk",
                                            tr("&Delete from disk..."));
    connect(removeSelectedFromDiskAct, SIGNAL(triggered()),
            this, SLOT(removeSelectedFromDisk()));

    removeAllAct = new TAction(remove_menu, "pl_remove_all",
                               tr("&Clear playlist"), "",
                               Qt::CTRL | Qt::Key_Delete);
    connect(removeAllAct, SIGNAL(triggered()), this, SLOT(removeAll()));

    addActions(remove_menu->actions());

    // Edit
    editAct = new TAction(this, "pl_edit", tr("&Edit name..."), "",
                          Qt::Key_Return);
    connect(editAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));

    // Find playing
    findPlayingAct = new TAction(this, "pl_find_playing",
                                 tr("&Find playing item"));
    connect(findPlayingAct, SIGNAL(triggered()), this, SLOT(findPlayingItem()));

    // Cut
    cutAct = new TAction(this, "pl_cut", tr("&Cut file name(s)"), "",
                          QKeySequence("Ctrl+X"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    // Copy
    copyAct = new TAction(this, "pl_copy", tr("&Copy file name(s)"), "",
                          QKeySequence("Ctrl+C"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copySelected()));

    // Paste
    pasteAct = new TAction(this, "pl_paste", tr("&Paste file name(s)"), "",
                          QKeySequence("Ctrl+V"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
    connect(QApplication::clipboard(), SIGNAL(dataChanged()),
            this, SLOT(enablePaste()));


    // Add actions to main window
    main_window->addActions(actions());
}

void TPlaylist::createToolbar() {

    toolbar = new QToolBar(this);

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

    // Popup menu
    popup = new QMenu(this);
    popup->addAction(editAct);
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

    connect(playlistWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(showContextMenu(const QPoint &)));
}

void TPlaylist::retranslateStrings() {

    // Icon
    setWindowIcon(Images::icon("logo", 64));
    setWinTitle();
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
        files.append(i->filename());
        it++;
    }
}

void TPlaylist::clear() {

    abortThread();
    playlistWidget->clr();
    filename = "";
    setWinTitle();
}

void TPlaylist::onThreadFinished() {
    logger()->debug("onThreadFinished");

    if (thread == 0) {
        // Only during destruction, so no need to enable actions
        logger()->debug("onThreadFinished: thread is gone");
        return;
    }

    // Get data from thread
    TPlaylistWidgetItem* root = thread->root;
    thread->root = 0;
    if (!thread->latestDir.isEmpty()) {
        pref->latest_dir = thread->latestDir;
    }

    // Clean up
    delete thread;
    thread = 0;

    if (root == 0) {
        // Thread aborted
        if (restartThread) {
            logger()->debug("onThreadFinished: thread aborted,"
                            " starting new request");
            addFilesStartThread();
        } else {
            logger()->debug("onThreadFinished: thread aborted");
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
        logger()->debug("onThreadFinished: filename set to '%1'", filename);
        setWinTitle();

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
        logger()->debug("abortThread: aborting add files thread");
        addFilesStartPlay = false;
        restartThread = false;
        thread->abort();
    }
}

void TPlaylist::addFilesStartThread() {

    if (thread) {
        // Thread still running, abort it and restart it in onThreadFinished()
        logger()->debug("addFilesStartThread: add files thread still running."
                        " Aborting it...");
        restartThread = true;
        thread->abort();
    } else {
        logger()->debug("addFilesStartThread: starting add files thread");
        restartThread = false;
        thread = new TAddFilesThread(this,
                                     addFilesFiles,
                                     pref->nameBlacklist,
                                     pref->addDirectories,
                                     pref->addVideo,
                                     pref->addAudio,
                                     pref->addPlaylists,
                                     pref->addImages);

        connect(thread, SIGNAL(finished()), this, SLOT(onThreadFinished()));
        connect(thread, SIGNAL(displayMessage(QString, int)),
                msgSlot, SLOT(msg(QString, int)));

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

void TPlaylist::addFiles() {

    QStringList files = TFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), pref->latest_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        playlistWidget->setModified(playlistWidget->currentItem());
        addFiles(files, false, playlistWidget->currentItem());
    }
}

void TPlaylist::addCurrentFile() {
   logger()->debug("addCurrentFile");

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
                    tr("Choose a directory"), pref->latest_dir);

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
    logger()->debug("addRemovedItem: '%1'", item);

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
    logger()->debug("play");

    TPlaylistWidgetItem* item = playlistWidget->currentPlaylistWidgetItem();
    if (item) {
        playItem(item);
    } else {
        player->play();
    }
}

void TPlaylist::playOrPause() {
    logger()->debug("playOrPause: state " + player->stateToString());

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
    logger()->debug("stop: state " + player->stateToString());

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
                        logger()->debug("getRandomItem: selecting '%1'",
                                        i->filename());
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

    logger()->debug("getRandomItem: end of playlist");
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
    logger()->debug("startPlay");

    TPlaylistWidgetItem* item = playlistWidget->firstPlaylistWidgetItem();
    if (item) {
        if (shuffleAct->isChecked()) {
            playItem(getRandomItem());
        } else {
            playItem(item);
        }
    } else {
        logger()->info("startPlay: nothing to play");
    }
}

void TPlaylist::playItem(TPlaylistWidgetItem* item) {

    while (item && item->isFolder()) {
        item = playlistWidget->getNextPlaylistWidgetItem(item);
    }
    if (item) {
        logger()->debug("playItem: '%1'", item->filename());
        playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        player->open(item->filename(), playlistWidget->hasSingleItem());
    } else {
        logger()->debug("playItem: end of playlist");
        msg(tr("End of playlist"), 7000);
        emit playlistEnded();
    }
}

void TPlaylist::playNext(bool loop_playlist) {
    logger()->debug("playNext");

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
    logger()->debug("playPrev");

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
    logger()->debug("playDirectory");

    if (Helper::directoryContainsDVD(dir)) {
        // onNewMediaStartedPlaying() will pickup the playlist
        playlistWidget->enableSort(false);
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
    logger()->debug("removeSelected");

    if (!isActiveWindow()) {
        logger()->info("removeSelected: ignoring remove actiom while not"
                       " active window");
        return;
    }
    if (!isVisible()) {
        logger()->info("removeSelected: ignoring remove action while not"
                       " visible");
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
            logger()->debug("removeSelected: removing '%1'", i->filename());

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
                        logger()->info("removeSelected: removed '%1' from disk",
                                       parent->filename());
                    } else {
                        logger()->error("removeSelected: failed to remove '%1'"
                                        " from disk",
                                        parent->filename());
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
        QString folder;
        QFileInfo fi(i->filename());
        if (i->isPlaylist()
            || !i->isFolder()
            || fi.fileName() == TConfig::WZPLAYLIST) {
            folder = fi.absolutePath();
        } else {
            folder = i->filename();
        }

        if (!folder.isEmpty()) {
            QUrl url(folder);
            if (url.scheme().isEmpty()) {
                url = QUrl::fromLocalFile(folder);
            }

            debug << "openFolder: opening" << url;
            debug << debug;
            QDesktopServices::openUrl(url);
        }
    }
}

void TPlaylist::openInNewWindow() {
    logger()->debug("openInNewWindow: '%1'", qApp->applicationFilePath());

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
        logger()->info("openInNewWindow: started new instance");
    } else {
        logger()->error("openInNewWindow: failed to start '%1'",
                        qApp->applicationFilePath());
        QMessageBox::warning(this, tr("Start failed"),
                             tr("Failed to start '%1'")
                             .arg(qApp->applicationFilePath()),
                             QMessageBox::Ok);
    }
}

void TPlaylist::showContextMenu(const QPoint & pos) {

    if (!popup->isVisible()) {
        Action::execPopup(this, popup,
                          playlistWidget->viewport()->mapToGlobal(pos));
    }
}

void TPlaylist::onItemActivated(QTreeWidgetItem* item, int) {
    logger()->debug("onItemActivated");

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(item);
    if (i && !i->isFolder()) {
        playItem(i);
    }
}

void TPlaylist::enableActions() {

    if (disable_enableActions) {
        logger()->debug("enableActions: disabled");
        return;
    }
    logger()->debug("enableActions: state " + player->stateToString());

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

    editAct->setEnabled(e && current_item);
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
    logger()->debug("onNewMediaStartedPlaying: md->filename '%1'"
                    ", current_filename '%2'", filename, current_filename);

    if (md->disc.valid) {
        if (md->titles.count() == playlistWidget->countItems()) {
            TDiscName cur_disc(current_filename);
            if (cur_disc.valid
                && cur_disc.protocol == md->disc.protocol
                && cur_disc.device == md->disc.device) {
                logger()->debug("onNewMediaStartedPlaying: item is from current"
                                " disc");
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
                logger()->debug("onNewMediaStartedPlaying: updating name from"
                                " '%1' to '%2'", item->baseName(), name);
                item->setName(name, item->extension());
                modified = true;
            }
        }

        // Update item duration
        if (!md->image && md->duration > 0) {
            if (!this->filename.isEmpty()
                && qAbs(md->duration - item->duration()) > 1) {
                modified = true;
            }
            logger()->debug("onNewMediaStartedPlaying: updating duration from"
                            " %1 to %2",
                            QString::number(item->duration()),
                            QString::number(md->duration));
            item->setDuration(md->duration);
        }

        if (modified) {
            playlistWidget->setModified(item);
        } else {
            logger()->debug("onNewMediaStartedPlaying: item considered"
                            " uptodate");
        }
        return;
    }

    // Create new playlist
    logger()->debug("onNewMediaStartedPlaying: creating new playlist for '%1'",
                    filename);
    clear();

    if (md->disc.valid) {
        // Add disc titles
        playlistWidget->enableSort(false);
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

        // TODO: remove from main thread
        // Add associated files to playlist
        if (md->selected_type == TMediaData::TYPE_FILE
            && pref->mediaToAddToPlaylist != TPreferences::NoFiles) {
            logger()->debug("onNewMediaStartedPlaying: searching for files to"
                            " add to playlist for '%1'", filename);
            QStringList files_to_add = Helper::filesForPlaylist(filename,
                pref->mediaToAddToPlaylist);
            if (files_to_add.isEmpty()) {
                logger()->debug("onNewMediaStartedPlaying: none found");
            } else {
                addFiles(files_to_add);
            }
        }
    }

    setWinTitle();
    logger()->debug("onNewMediaStartedPlaying: created new playlist for '%1'",
                    filename);
}

void TPlaylist::onMediaEOF() {
    logger()->debug("onMediaEOF");
    playNext(false);
}

void TPlaylist::onTitleTrackChanged(int id) {
    logger()->debug("onTitleTrackChanged: %1", id);

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
        logger()->warn("onTitleTrackChanged: title id %1 filename '%2' not"
                       " found in playlist",
                       QString::number(id), filename);
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

void TPlaylist:: setWinTitle() {

    QString title;

    TPlaylistWidgetItem* root = playlistWidget->root();
    if (root) {
        title = root->baseName();
        if (title.isEmpty()) {
            title = root->fname();
        }
    }

    title = tr("WZPlaylist%1%2%3",
               "1 optional white space,"
               " 2 optional playlist name,"
               " 3 optional playlist modified star")
        .arg(title.isEmpty() ? "" : " ")
        .arg(title)
        .arg(playlistWidget->modified() ? "*" : "");
    setWindowTitle(title);

    // Inform the playlist dock
    emit windowTitleChanged();
}

void TPlaylist::onModifiedChanged() {
    setWinTitle();
}

void TPlaylist::editCurrentItem() {

    TPlaylistWidgetItem* current = playlistWidget->currentPlaylistWidgetItem();
    if (current) {
        editItem(current);
    }
}

bool TPlaylist::rename(TPlaylistWidgetItem* item, const QString& newName) {

    QString nn = QDir::toNativeSeparators(
                     QFileInfo(item->filename()).absolutePath());
    if (!nn.endsWith(QDir::separator())) {
        nn += QDir::separator();
    }
    nn += QDir::toNativeSeparators(newName);

    if (QFile::rename(item->filename(), nn)) {
        logger()->info("rename: renamed file '%1' to '%2'",
                       item->filename(), nn);
        item->setFilename(nn, QFileInfo(newName).completeBaseName());
    } else {
        logger()->error("rename: failed to rename '%1' to '%2'",
                        item->filename(), nn);
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to rename '%1' to '%2'")
                             .arg(item->filename()).arg(nn));
        return false;
    }

    return true;
}

void TPlaylist::editItem(TPlaylistWidgetItem* item) {

    bool renameFile = (item->plParent()->isWZPlaylist()
                       || !item->plParent()->isPlaylist())
                      && QFileInfo(item->filename()).exists();

    QString name = item->baseName();
    QString ext = item->extension();
    if (renameFile && !ext.isEmpty()) {
        name += "." + ext;
    }

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Edit name"), tr("Name:"),
                                            QLineEdit::Normal, name, &ok);
    if (!ok || newName == name) {
        return;
    }

    if (renameFile && !newName.isEmpty()) {
        if (!rename(item, newName)) {
            return;
        }
    } else {
        logger()->info("rename: renaming '%1' to '%2'", name, newName);
        item->setName(newName, ext, true);
    }

    playlistWidget->setModified(item);
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

void TPlaylist::hideEvent(QHideEvent*) {
    emit visibilityChanged(false);
}

void TPlaylist::showEvent(QShowEvent*) {
    emit visibilityChanged(true);
}

void TPlaylist::openPlaylist(const QString& filename) {

    pref->latest_dir = QFileInfo(filename).absolutePath();
    addFiles(QStringList() << filename, true);
}

void TPlaylist::open() {

    if (maybeSave()) {
        QString s = TFileDialog::getOpenFileName(this, tr("Choose a file"),
            pref->latest_dir,
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
    logger()->debug("savem3uFolder: saving '%1'", folder->filename());

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
                logger()->warn("saveM3uFolder: saving '%1' as m3u8",
                               i->filename());
                filename = filename.left(filename.length() - 4) + ".m3u8";
                i->setFilename(filename, i->baseName());
                modified = true;
            }

            if (modified) {
                if (!saveM3u(i, filename, fi.fileName() == TConfig::WZPLAYLIST)) {
                    result = false;
                }
            } else {
                logger()->info("saveM3uFolder: playlist '%1' not modified",
                               i->filename());
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
                    logger()->info("saveM3uFolder: folder not modified '%1'",
                                   i->filename());
                }
            } else {
                if (saveM3uFolder(i, path, stream, linkFolders)) {
                    logger()->info("savem3uFolder: succesfully saved '%1'",
                                   i->filename());
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
    logger()->debug("saveM3u: saving '%1'", filename);

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        // Ok to fail on wzplaylist
        if (wzplaylist) {
            logger()->info("saveM3u: ignoring failed save '%1'", filename);
            return true;
        }

        logger()->error("saveM3u: failed to save '%1'", filename);

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
            logger()->debug("saveM3u: blacklisting '%1'", fn);
            stream << "#WZP-blacklist:" << fn << "\n";
        }
    }

    bool result = saveM3uFolder(folder, path, stream, wzplaylist);

    stream.flush();
    f.close();

    logger()->debug("saveM3u: saved '%1'", filename);
    return result;
}

bool TPlaylist::saveM3u(const QString& filename, bool linkFolders) {
    logger()->debug("saveM3u: link folders %1", linkFolders);

    TPlaylistWidgetItem* root = playlistWidget->root();
    return saveM3u(root, filename, linkFolders);
}

bool TPlaylist::savePls(const QString& filename) {
    logger()->debug("savePls: '%1'", filename);

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

    logger()->error("savePls: failed to save '%1'", filename);
    QMessageBox::warning(this, tr("Save failed"),
                         tr("Failed to save '%1'").arg(filename),
                         QMessageBox::Ok);
    return false;
}

bool TPlaylist::save() {
    logger()->info("save: '%1'", filename);

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
    setWinTitle();
    pref->latest_dir = fi.absolutePath();

    bool result;
    if (fi.suffix().toLower() == "pls") {
        result = savePls(filename);
    } else {
        result = saveM3u(filename, wzplaylist);
    }

    if (result) {
        playlistWidget->clearModified();
        logger()->info("save: succesfully saved '%1'", fi.absoluteFilePath());
        msgOSD(tr("Saved '%1'").arg(fi.fileName()));
    } else {
        // Error box and log already done, but need to remove 0 secs save msg
        msgOSD(tr("Failed to save '%1'").arg(fi.fileName()));
    }

    return result;
}

bool TPlaylist::saveAs() {

    QString s = TFileDialog::getSaveFileName(this, tr("Choose a filename"),
        pref->latest_dir, tr("Playlists") + extensions.playlists().forFilter());

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
        logger()->debug("maybeSave: not modified");
        return true;
    }

    if (filename.isEmpty()) {
        logger()->debug("maybeSave: discarding changes to unnamed playlist");
        return true;
    }

    if (QFileInfo(filename).fileName() == TConfig::WZPLAYLIST) {
        logger()->debug("maybeSave: auto saving wzplaylist.m3u8");
        return save();
    }

    if (!playlistWidget->root()->isPlaylist()) {
        // Directorie
        if (pref->useDirectoriePlaylists) {
            logger()->debug("maybeSave: auto saving directorie");
            return save();
        }
        logger()->debug("maybeSave: discarding changes to directorie playlist");
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
            logger()->info("maybeSave: no saving");
            return true;
        case QMessageBox::Cancel:
            logger()->info("maybeSave: canceling save");
            return false;
        default: return save();
    }
}

void TPlaylist::saveSettings() {
    logger()->debug("saveSettings");

    pref->beginGroup("playlist");
    pref->setValue("repeat", repeatAct->isChecked());
    pref->setValue("shuffle", shuffleAct->isChecked());
    pref->endGroup();
}

void TPlaylist::loadSettings() {
    logger()->debug("loadSettings");

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

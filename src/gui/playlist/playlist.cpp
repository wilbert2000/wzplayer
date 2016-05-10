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

#include <QDebug>
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

#include "gui/base.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/playlistwidgetitem.h"
#include "gui/playlist/addfilesthread.h"
#include "core.h"
#include "gui/multilineinputdialog.h"
#include "gui/action/menuinoutpoints.h"
#include "gui/action/action.h"
#include "images.h"
#include "iconprovider.h"
#include "helper.h"
#include "filedialog.h"
#include "extensions.h"
#include "version.h"


using namespace Settings;

namespace Gui {

using namespace Action;

namespace Playlist {


TPlaylist::TPlaylist(TBase* mw, TCore* c) :
    QWidget(mw),
    main_window(mw),
    core(c),
    recursive_add_directories(true),
    disable_enableActions(false),
    modified(false),
    thread(0) {

    createTree();
    createActions();
	createToolbar();

    connect(core, SIGNAL(newMediaStartedPlaying()),
            this, SLOT(onNewMediaStartedPlaying()));
    connect(core, SIGNAL(playerError(int)),
            this, SLOT(onPlayerError()));
    connect(core, SIGNAL(titleTrackChanged(int)),
			this, SLOT(onTitleTrackChanged(int)));
	connect(core, SIGNAL(mediaEOF()),
			this, SLOT(onMediaEOF()), Qt::QueuedConnection);
	connect(core, SIGNAL(noFileToPlay()),
            this, SLOT(resumePlay()), Qt::QueuedConnection);

    connect(main_window, SIGNAL(enableActions()),
            this, SLOT(enableActions()));

	QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(playlistWidget);
	layout->addWidget(toolbar);
	setLayout(layout);

	setAcceptDrops(true);
	setAttribute(Qt::WA_NoMousePropagation);

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

    connect(playlistWidget, SIGNAL(modified()),
            this, SLOT(setModified()));
    connect(playlistWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
             this, SLOT(onItemActivated(QTreeWidgetItem*, int)));
    connect(playlistWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
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

    // Stop
    stopAct = new TAction(this, "stop", tr("&Stop"), "", Qt::Key_MediaStop);
    connect(stopAct, SIGNAL(triggered()), this, SLOT(stop()));

    // Play
    playAct = new TAction(this, "play", tr("P&lay"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, SIGNAL(triggered()), this, SLOT(play()));

    // Pause
    pauseAct = new TAction(this, "pause", tr("Pause"), "",
                           QKeySequence("Media Pause")); // MCE remote key
    connect(pauseAct, SIGNAL(triggered()), core, SLOT(pause()));

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
    inOutMenu = new TMenuInOut(main_window, core);
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

    addDirectoryAct = new TAction(add_menu, "pl_add_directory", tr("Add &directory..."));
	connect(addDirectoryAct, SIGNAL(triggered()), this, SLOT(addDirectory()));

    addUrlsAct = new TAction(add_menu, "pl_add_urls", tr("Add &URL(s)..."));
	connect(addUrlsAct, SIGNAL(triggered()), this, SLOT(addUrls()));

    addActions(add_menu->actions());

    // Remove menu
    remove_menu = new TMenu(this, main_window, "pl_remove_menu",
                            tr("&Remove from playlist"), "minus");

    removeSelectedAct = new TAction(remove_menu, "pl_remove_selected",
                                    tr("&Remove from list"), "", Qt::Key_Delete);
	connect(removeSelectedAct, SIGNAL(triggered()), this, SLOT(removeSelected()));

    removeSelectedFromDiskAct = new TAction(remove_menu, "pl_delete_from_disk",
                                            tr("&Delete from disk..."));
    connect(removeSelectedFromDiskAct, SIGNAL(triggered()),
            this, SLOT(removeSelectedFromDisk()));

    removeAllAct = new TAction(remove_menu, "pl_remove_all",
                               tr("&Clear playlist"), "", Qt::CTRL | Qt::Key_Delete);
	connect(removeAllAct, SIGNAL(triggered()), this, SLOT(removeAll()));

    addActions(remove_menu->actions());

    // Cut
    cutAct = new TAction(this, "pl_cut", tr("&Cut"), "",
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

    // Edit
    editAct = new TAction(this, "pl_edit", tr("&Edit name..."), "", Qt::Key_Return);
    connect(editAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));

    // Add actions to main window
    main_window->addActions(actions());
}

void TPlaylist::createToolbar() {

	toolbar = new QToolBar(this);

	toolbar->addAction(openAct);
	toolbar->addAction(saveAct);;

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
    toolbar->addAction(playAct);
    toolbar->addAction(playOrPauseAct);
    toolbar->addAction(prevAct);
	toolbar->addAction(nextAct);

	// Popup menu
    popup = new QMenu(this);
    popup->addAction(editAct);
    popup->addSeparator();
    popup->addAction(cutAct);
    popup->addAction(copyAct);
    popup->addAction(pasteAct);
    popup->addSeparator();
    popup->addAction(stopAct);
    popup->addAction(playAct);
    popup->addAction(playOrPauseAct);
    popup->addSeparator();
    popup->addMenu(inOutMenu);
    popup->addSeparator();
    popup->addMenu(add_menu);
    popup->addMenu(remove_menu);

    connect(playlistWidget, SIGNAL(customContextMenuRequested(const QPoint &)),
			this, SLOT(showContextMenu(const QPoint &)));
}

void TPlaylist::retranslateStrings() {
	//qDebug("Gui::TPlaylist::retranslateStrings");

	// Icon
	setWindowIcon(Images::icon("logo", 64));
    setWinTitle();
}

void TPlaylist::msg(const QString& s) {
    emit displayMessageOnOSD(s, TConfig::MESSAGE_DURATION);
}

void TPlaylist::getFilesAppend(QStringList& files) const {

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        files.append(i->filename());
        it++;
    }
}

void TPlaylist::clear() {

    if (thread) {
        qDebug() << "Gui::TPlaylist::clear: add files thread still running,"
                    " aborting it";
        addFilesFiles.clear();
        thread->abort();
    }
    playlistWidget->clr();
    filename = "";
    title = "";
    modified = false;
    setWinTitle();
}

void TPlaylist::onThreadFinished() {
    qDebug() << "Gui::Playlist::TPlaylist::onThreadFinished";

    if (thread == 0) {
        // Only during destruction, so no need to enable actions
        qDebug() << "Gui::Playlist::TPlaylist::onThreadFinished: thread gone";
        return;
    }

    // Get data from thread
    TPlaylistWidgetItem* root = thread->root;
    thread->root = 0;
    QTreeWidgetItem* current = thread->currentItem;
    if (thread->latestDir.count()) {
        pref->latest_dir = thread->latestDir;
    }

    // Clean up
    delete thread;
    thread = 0;

    if (root == 0) {
        // Thread aborted
        if (addFilesFiles.count()) {
            qDebug() << "Gui::Playlist::TPlaylist::onThreadFinished: thread"
                        " aborted, restarting it";

            addFilesStartThread();
        } else {
            qDebug() << "Gui::Playlist::TPlaylist::onThreadFinished: thread"
                        " aborted";
            enableActions();
        }
        return;
    }

    QString msg = addFilesFiles.count() == 1 ? addFilesFiles[0] : "";
    addFilesFiles.clear();

    if (root->childCount() == 0) {
        if (msg.count()) {
            msg = tr("Found no files to play in %1.").arg(msg);
        } else {
            msg = tr("Found nothing to play in the requested locations.");
        }

        delete root;
        enableActions();

        QMessageBox::information(this, "Found no files", msg);
        return;
    }


    // TODO: make sure addFilesTarget still valid
    QString fn = playlistWidget->add(root, addFilesTarget, current);
    if (fn.count()) {
        filename = fn;
        setWinTitle(QFileInfo(fn).fileName());
    }

    if (addFilesStartPlay) {
        if (addFilesFileToPlay.count()) {
            TPlaylistWidgetItem* w = findFilename(addFilesFileToPlay);
            if (w) {
                // Ok to not sort, only used for restarting app
                playItem(w);
                return;
            }
        }
        startPlay(true);
    } else {
        enableActions();
    }
}

void TPlaylist::addFilesStartThread() {
    qDebug() << "Gui::Playlist::TPlaylist::addFilesStartThread";

    if (thread) {
        // Thread still running, abort it and restart it in onThreadFinished()
        qDebug("Gui::Playlist::TPlaylist::addFilesStartThread: add files"
               " thread still running. Aborting it...");
        thread->abort();
    } else {
        thread = new TAddFilesThread(this,
                                     addFilesFiles,
                                     recursive_add_directories,
                                     addFilesSearchItems);

        connect(thread, SIGNAL(finished()), this, SLOT(onThreadFinished()));
        connect(thread, SIGNAL(displayMessage(QString, int)),
                this, SIGNAL(displayMessage(QString, int)));

        thread->start();
        enableActions();
    }
}

void TPlaylist::addFiles(const QStringList& files,
                         bool startPlay,
                         QTreeWidgetItem* target,
                         const QString& fileToPlay,
                         bool searchForItems) {
    qDebug() << "Gui::Playlist::TPlaylist::addFiles: files" << files
             << "startPlay" << startPlay;

    addFilesFiles = files;
    addFilesStartPlay = startPlay;
    addFilesTarget = target;
    addFilesFileToPlay = fileToPlay;
    addFilesSearchItems = searchForItems;

    addFilesStartThread();
}

void TPlaylist::addFiles() {

    TExtensions e;
    QStringList files = MyFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), pref->latest_dir,
        tr("Multimedia") + e.multimedia().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        addFiles(files, false, playlistWidget->currentItem());
        setModified();
    }
}

void TPlaylist::addCurrentFile() {
    qDebug("Gui::TPlaylist::addCurrentFile");

    if (core->mdat.filename.count()) {
        TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
            playlistWidget->currentPlaylistWidgetFolder(),
            playlistWidget->currentItem(),
            core->mdat.filename,
            core->mdat.displayName(),
            core->mdat.duration,
            false,
            iconProvider.iconForFile(core->mdat.filename));
        i->setPlayed(true);
    }
}

void TPlaylist::addDirectory() {

    QString s = MyFileDialog::getExistingDirectory(this,
                    tr("Choose a directory"), pref->latest_dir);

    if (!s.isEmpty()) {
        addFiles(QStringList() << s, false, playlistWidget->currentItem());
    }
}

void TPlaylist::addUrls() {

    TMultilineInputDialog d(this);
    if (d.exec() == QDialog::Accepted && d.lines().count() > 0) {
        addFiles(d.lines(), false, playlistWidget->currentItem());
        setModified();
    }
}

void TPlaylist::onRepeatToggled(bool toggled) {

    if (toggled)
        msg(tr("Repeat playlist set"));
    else
        msg(tr("Repeat playlist cleared"));
}

void TPlaylist::onShuffleToggled(bool toggled) {

    if (toggled)
        msg(tr("Shuffle playlist set"));
    else
        msg(tr("Shuffle playlist cleared"));
}

void TPlaylist::play() {
    qDebug() << "Gui::TPlaylist:play";

    if (playlistWidget->currentItem()) {
        playItem(playlistWidget->currentPlaylistWidgetItem());
    } else {
        core->play();
    }
}

void TPlaylist::playOrPause() {
    qDebug() << "Gui::TPlaylist::playOrPause: state" << core->stateToString();

    if (core->state() == STATE_PLAYING) {
        core->pause();
    } else 	if (core->state() == STATE_PAUSED) {
        core->play();
    } else if (playlistWidget->playing_item) {
        playItem(playlistWidget->playing_item);
    } else if (playlistWidget->currentItem()) {
        playItem(playlistWidget->currentPlaylistWidgetItem());
    } else {
        core->play();
    }
}

void TPlaylist::stop() {
    qDebug() << "Gui::TPlaylist::stop: state" << core->stateToString();

    core->stop();
    if (thread) {
        qDebug() << "Gui::TPlaylist::stop: stopping add files thread";
        addFilesStartPlay = false;
        thread->stop();
    }
}

TPlaylistWidgetItem* TPlaylist::getRandomItem() const {

    if (playlistWidget->topLevelItemCount() <= 0) {
        return 0;
    }

    bool foundFreeItem = false;
    int selected = (int) ((double) playlistWidget->countChildren() * qrand() / (RAND_MAX + 1.0));
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
                        qDebug() << "Gui::TPlaylist::getRandomItem: selecting"
                                 << i->filename();
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

    qDebug() << "Gui::TPlaylist::getRandomItem: end of playlist";
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

void TPlaylist::startPlay(bool sort) {
    qDebug("Gui::TPlaylist::startPlay");

    if (playlistWidget->topLevelItemCount() > 0) {
        playlistWidget->enableSort(sort);
        if (shuffleAct->isChecked()) {
            playItem(getRandomItem());
        } else {
            playItem(playlistWidget->firstPlaylistWidgetItem());
        }
    } else {
        emit displayMessage(tr("Found no files to play"), 6000);
    }
}

void TPlaylist::playItem(TPlaylistWidgetItem* item) {

    while (item && (item->isFolder() || item->filename().isEmpty())) {
        item = playlistWidget->getNextPlaylistWidgetItem(item);
    }
    if (item) {
        qDebug() << "Gui::TPlaylist::playItem:" << item->filename();
        playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        core->open(item->filename());
    } else {
        qDebug("Gui::TPlaylist::playItem: end of playlist");
		emit playlistEnded();
	}
}

void TPlaylist::playNext(bool allow_reshuffle) {
	qDebug("Gui::TPlaylist::playNext");

    TPlaylistWidgetItem* item = 0;
	if (shuffleAct->isChecked()) {
        item = getRandomItem();
        if (item == 0 && (repeatAct->isChecked() || allow_reshuffle)) {
            playlistWidget->clearPlayed();
            item = getRandomItem();
		}
    } else {
        item = playlistWidget->getNextPlaylistWidgetItem();
        if (item == 0) {
            item = playlistWidget->firstPlaylistWidgetItem();
        }
	}
	playItem(item);
}

void TPlaylist::playPrev() {
	qDebug("Gui::TPlaylist::playPrev");

    TPlaylistWidgetItem* i = playlistWidget->playing_item;
    if (shuffleAct->isChecked() && i) {
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
	qDebug("Gui::TPlaylist::playDirectory");

    if (Helper::directoryContainsDVD(dir)) {
        // onNewMediaStartedPlaying() will pickup the playlist
        playlistWidget->enableSort(false);
        core->open(dir);
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
			tr("Cannot delete '%1', it does not seem to be a file.").arg(filename));
		return false;
	}

	// Ask the user for confirmation
	int res = QMessageBox::question(this, tr("Confirm file deletion"),
		tr("You're about to delete the file '%1' from your drive.").arg(filename) + "<br>"+
		tr("This action cannot be undone. Are you sure you want to proceed?"),
		QMessageBox::Yes, QMessageBox::No);

	if (res == QMessageBox::Yes) {
		// Cannot delete file on Windows when it is in use
        if (filename == playingFile && core->state() != STATE_STOPPED) {
            core->stop();
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
	qDebug("Gui::TPlaylist::removeSelected");

    disable_enableActions = true;
    QString playing = playingFile();
    QTreeWidgetItem* newCurrent = playlistWidget->currentItem();
    do {
        newCurrent = newCurrent->parent();
    } while (newCurrent && newCurrent->isSelected());

    QTreeWidgetItemIterator it(playlistWidget, QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!deleteFromDisk || deleteFileFromDisk(i->filename(), playing)) {
            qDebug() << "Gui::TPlaylist::removeSelected: removing"
                     << i->filename();
            QTreeWidgetItem* parent = i->parent();
            delete i;

            // Clean up empty folders
            while (parent && parent->childCount() == 0) {
                QTreeWidgetItem* gp = parent->parent();
                if (parent == newCurrent) {
                    newCurrent = gp;
                }
                delete parent;
                parent = gp;
            }
            setModified();
        }
        it++;
    }

    if (playlistWidget->topLevelItemCount() == 0) {
        playlistWidget->setPlayingItem(0);
        playlistWidget->setCurrentItem(0);
        setModified(false);
    } else {
        playlistWidget->playing_item = findFilename(playing);
        if (newCurrent) {
            playlistWidget->setCurrentItem(newCurrent);
        } else {
            playlistWidget->setCurrentItem(playlistWidget->firstPlaylistWidgetItem());
        }
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

void TPlaylist::showContextMenu(const QPoint & pos) {

	if (!popup->isVisible()) {
        Action::execPopup(this, popup, playlistWidget->viewport()->mapToGlobal(pos));
	}
}

void TPlaylist::onItemActivated(QTreeWidgetItem* item, int) {
    qDebug() << "Gui::TPlaylist::onItemActivated";

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(item);
    if (i && !i->isFolder()) {
        playItem(i);
    }
}

void TPlaylist::enableActions() {

    if (disable_enableActions) {
        qDebug() << "Gui::TPlaylist::enableActions: disabled";
        return;
    }
    qDebug() << "Gui::TPlaylist::enableActions: state" << core->stateToString();

    // Note: there is always something selected if c > 0
    int c = playlistWidget->topLevelItemCount();

    TPlaylistWidgetItem* playing_item = playlistWidget->playing_item;
    TPlaylistWidgetItem* current_item = playlistWidget->currentPlaylistWidgetItem();

    TCoreState s = core->state();
    bool enable = (s == STATE_STOPPED || s == STATE_PLAYING || s == STATE_PAUSED)
                  && thread == 0;
    bool coreHasFilename = core->mdat.filename.count();

    openAct->setEnabled(thread == 0);
    saveAct->setEnabled(enable && c > 0);

    playAct->setEnabled(enable && (coreHasFilename || c > 0));
    pauseAct->setEnabled(s == STATE_PLAYING);

    playOrPauseAct->setEnabled(enable && (coreHasFilename || c > 0));

    if (!enable) {
        QString text;
        if (thread) {
            text = tr("loading");
        } else {
            text = core->stateToString();
        }
        playOrPauseAct->setTextAndTip(text + "...");
        playOrPauseAct->setIcon(Images::icon("loading"));
    } else if (s == STATE_PLAYING) {
        if (playing_item && playing_item->state() != PSTATE_PLAYING) {
            disable_enableActions = true;
            playlistWidget->setPlayingItem(playing_item, PSTATE_PLAYING);
            disable_enableActions = false;
        }
        playOrPauseAct->setTextAndTip(tr("&Pause"));
        playOrPauseAct->setIcon(Images::icon("pause"));
    } else {
        playOrPauseAct->setTextAndTip(tr("&Play"));
        playOrPauseAct->setIcon(Images::icon("play"));
    }

    // Stop action
    stopAct->setEnabled(s == STATE_PLAYING || s == STATE_PAUSED || thread);

    // Prev/Next
    bool changed = false;
    bool e = enable && c > 0;
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

    addCurrentAct->setEnabled(coreHasFilename && thread == 0);

    e = c > 0 && thread == 0;
    removeSelectedAct->setEnabled(e);
    removeSelectedFromDiskAct->setEnabled(e && current_item
                                          && !current_item->isFolder());
    removeAllAct->setEnabled(e);

    cutAct->setEnabled(e);
    copyAct->setEnabled(c > 0 || coreHasFilename);
    editAct->setEnabled(e && current_item);
}

void TPlaylist::onPlayerError() {

    TPlaylistWidgetItem* item = playlistWidget->playing_item;
    if (item) {
        item->setState(PSTATE_FAILED);
        playlistWidget->scrollToItem(item);
        playlistWidget->setPlayingItem(0);
    }
}

void TPlaylist::onNewMediaStartedPlaying() {

    const TMediaData* md = &core->mdat;
    QString filename = md->filename;
    QString current_filename = playlistWidget->playingFile();

    if (filename == current_filename) {
        qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: new file is current item");
        TPlaylistWidgetItem* item = playlistWidget->playing_item;
        if (item && !md->disc.valid) {
            if (!item->edited()) {
                item->setName(md->displayName());
            }
            if (md->duration > 0) {
                item->setDuration(md->duration);
            }
        }
        return;
    }

    if (md->disc.valid && md->titles.count() == playlistWidget->countItems()) {
        TDiscName cur_disc(current_filename);
        if (cur_disc.valid
            && cur_disc.protocol == md->disc.protocol
            && cur_disc.device == md->disc.device) {
            qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: new file is from current disc");
            return;
        }
    }

    // Create new playlist
    clear();

    if (md->disc.valid) {
        // Add disc titles
        setWinTitle(core->mdat.title);
        TDiscName disc = md->disc;
        QIcon icon = iconProvider.iconForFile(disc.toString());
        foreach(const Maps::TTitleData title, md->titles) {
            disc.title = title.getID();
            TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
                playlistWidget->root(), 0, disc.toString(),
                title.getDisplayName(false), title.getDuration(), false, icon);
            if (title.getID() == md->titles.getSelectedID()) {
                playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
            }
        }
    } else {
        // Add current file
        TPlaylistWidgetItem* current = new TPlaylistWidgetItem(
            playlistWidget->root(), 0, filename, title, core->mdat.duration,
            false, iconProvider.iconForFile(filename));
        playlistWidget->setPlayingItem(current, PSTATE_PLAYING);

        // Add associated files to playlist
        if (md->selected_type == TMediaData::TYPE_FILE
            && pref->media_to_add_to_playlist != TPreferences::NoFiles) {
            qDebug() << "Gui::TPlaylist::onNewMediaStartedPlaying: searching for"
                        " files to add to playlist for" << filename;
            QStringList files_to_add = Helper::filesForPlaylist(filename,
                pref->media_to_add_to_playlist);
            if (files_to_add.isEmpty()) {
                qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: none found");
            } else {
                addFiles(files_to_add);
            }
        }
    }

    qDebug() << "Gui::TPlaylist::onNewMediaStartedPlaying: created new playlist"
                "for" << filename;
}

void TPlaylist::onMediaEOF() {
	qDebug("Gui::Tplaylist::onMediaEOF");
    playNext(false);
}

void TPlaylist::onTitleTrackChanged(int id) {
	qDebug("Gui::TPlaylist::onTitleTrackChanged: %d", id);

	if (id < 0) {
        playlistWidget->setPlayingItem(0);
		return;
	}

    // Search for id
    TDiscName disc = core->mdat.disc;
	disc.title = id;
	QString filename = disc.toString();

    TPlaylistWidgetItem* i = playlistWidget->findFilename(filename);
    if (i) {
        playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
    } else {
        qWarning() << "Gui::TPlaylist::onTitleTrackChanged: title id" << id
                   << "filename" << filename << "not found in playlist";
    }
}

void TPlaylist::copySelected(const QString& actionName) {

	QString text;
	int copied = 0;

    QTreeWidgetItemIterator it(playlistWidget, QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        text += i->filename() + "\n";
        copied++;
        it++;
    }

    if (copied == 0 && core->mdat.filename.count()) {
        text = core->mdat.filename + "\n";
		copied = 1;
	}

	if (copied > 0) {
		if (copied == 1) {
			// Remove trailing new line
			text = text.left(text.length() - 1);
            msg(actionName + " " + text);
		} else {
            msg(tr("%1 %2 file names",
                   "1 action Copied or Cut, 2 number of file names")
                .arg(copied));
		}
		QApplication::clipboard()->setText(text);
	}
}

void TPlaylist::paste() {

    QStringList files = QApplication::clipboard()->text()
                        .split("\n",  QString::SkipEmptyParts);
    if (files.count()) {
        addFiles(files, false, playlistWidget->currentItem(), "", true);
        setModified();
    }
}

void TPlaylist::enablePaste() {
    pasteAct->setEnabled(QApplication::clipboard()->mimeData()->hasText());
}

void TPlaylist::cut() {

    copySelected("Cut");
    removeSelected();
}

void TPlaylist:: setWinTitle(QString s) {

    if (s.count()) {
        title = s;
    }

    QString winTitle = tr("WZPlaylist%1%2%3",
        "optional white space, optional playlist name, optional modified star")
        .arg(title.isEmpty() ? "" : " ")
        .arg(title)
        .arg(modified ? "*" : "");
    setWindowTitle(winTitle);

    // Inform the playlist dock
    emit windowTitleChanged();
}

void TPlaylist::setModified(bool mod) {

    if (mod) {
        if (!modified) {
            modified = true;
            setWinTitle();
        }
    } else if (modified) {
        modified = false;
        setWinTitle();
    }
}

void TPlaylist::editCurrentItem() {

    TPlaylistWidgetItem* current = playlistWidget->currentPlaylistWidgetItem();
    if (current) {
        editItem(current);
    }
}

void TPlaylist::editItem(TPlaylistWidgetItem* item) {

    QString current_name = item->name();
    if (current_name.isEmpty()) {
        current_name = item->filename();
    }
    QString saved_name = current_name;

	bool ok;
    QString text = QInputDialog::getText(this, tr("Edit name"),
        tr("Name to display in playlist:"),	QLineEdit::Normal,
		current_name, &ok);
    if (ok && text != saved_name) {
        item->setName(text);
        item->setEdited(true);
        setModified();
	}
}

// Drag&drop
void TPlaylist::dragEnterEvent(QDragEnterEvent *e) {
    qDebug() << "Gui::TPlaylist::dragEnterEvent" << e->mimeData()->formats();

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
    qDebug() << "Gui::TPlaylist::dropEvent" << e->mimeData()->formats();

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl url, e->mimeData()->urls()) {
            files.append(url.toString());
        }

        if (files.count()) {
            // TODO: see dropIndicator for above/below
            QTreeWidgetItem* item = playlistWidget->itemAt(e->pos()
                - playlistWidget->pos() - playlistWidget->viewport()->pos());
            addFiles(files, false, item);
            setModified();
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

void TPlaylist::closeEvent(QCloseEvent* e)  {

    if (maybeSave()) {
        saveSettings();
        e->accept();
    } else {
        e->ignore();
    }
}

bool TPlaylist::saveM3u(QString file) {
    qDebug() << "Gui::TPlaylist::saveM3u:" << file;

    QString dir_path = QDir::toNativeSeparators(QFileInfo(file).path());
    if (!dir_path.endsWith(QDir::separator())) {
        dir_path += QDir::separator();
    }

    bool utf8 = QFileInfo(file).suffix().toLower() == "m3u8";

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream stream(&f);
    if (utf8) {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    stream << "#EXTM3U" << "\n"
           << "# Playlist created by WZPlayer " << TVersion::version
           << "\n";

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            stream << "#EXTINF:" << i->duration() << "," << i->name() << "\n";

            QString filename = i->filename();

            // Try to save the filename as relative instead of absolute
            // Normalize separator to match dir_path
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
            if (QFileInfo(filename).exists()) {
                filename = QDir::toNativeSeparators(filename);
            }
#endif
            if (filename.startsWith(dir_path)) {
                filename = filename.mid(dir_path.length());
            }
            stream << filename << "\n";
        }
        ++it;
    }

    f.close();
    setModified(false);

    return true;
}

bool TPlaylist::savePls(QString file) {
    qDebug() << "Gui::TPlaylist::savePls:" << file;

    QString dir_path = QDir::toNativeSeparators(QFileInfo(file).path());
    if (!dir_path.endsWith(QDir::separator())) {
        dir_path += QDir::separator();
    }

    QSettings set(file, QSettings::IniFormat);
    set.beginGroup("playlist");

    int n = 0;
    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            QString filename = i->filename();

            // Normalize path separator to match dir_path
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
            if (QFileInfo(filename).exists()) {
                filename = QDir::toNativeSeparators(filename);
            }
#endif

            // Try to save the filename as relative instead of absolute
            if (filename.startsWith(dir_path)) {
                filename = filename.mid(dir_path.length());
            }

            set.setValue("File" + QString::number(n + 1), filename);
            set.setValue("Title" + QString::number(n + 1), i->name());
            set.setValue("Length" + QString::number(n + 1), (int) i->duration());

            n++;
        }
        ++it;
    }

    set.setValue("NumberOfEntries", n);
    set.setValue("Version", 2);

    set.endGroup();

    set.sync();

    bool ok = set.status() == QSettings::NoError;
    if (ok) {
        modified = false;
    }

    return ok;
}

void TPlaylist::openPlaylist(const QString& filename) {

    pref->latest_dir = QFileInfo(filename).absolutePath();
    addFiles(QStringList() << filename, true);
}

void TPlaylist::open() {

    if (maybeSave()) {
        TExtensions e;
        QString s = MyFileDialog::getOpenFileName(
                        this,
                        tr("Choose a file"),
                        pref->latest_dir,
            tr("Playlists") + e.playlist().forFilter() + ";;"
            + tr("All files") +" (*.*)");

		if (!s.isEmpty()) {
            openPlaylist(s);
		}
	}
}

bool TPlaylist::save() {

    QFileInfo fi;
    if (filename.isEmpty()) {
        TExtensions e;
        QString s = MyFileDialog::getSaveFileName(
                        this, tr("Choose a filename"), pref->latest_dir,
                        tr("Playlists") + e.playlist().forFilter());

        if (s.isEmpty()) {
            return false;
        }

        // If filename has no extension, add it
        fi.setFile(s);
        if (fi.suffix().isEmpty()) {
            fi.setFile(s + ".m3u8");
        }

        if (fi.exists()) {
            int res = QMessageBox::question(this,
                tr("Confirm overwrite?"), tr("The file %1 already exists.\n"
                "Do you want to overwrite it?").arg(fi.absoluteFilePath()),
                QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
            if (res == QMessageBox::No) {
                return false;
            }
        }
    } else {
        // TODO: add save as
        fi.setFile(filename);
        if (fi.isDir()) {
            fi.setFile(fi.absoluteFilePath()
                       + "/" + TConfig::PROGRAM_ID + ".m3u8");
        }
    }

    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
    pref->latest_dir = fi.absolutePath();
    setWinTitle(fi.fileName());

    bool result;
    if (fi.suffix().toLower() == "pls") {
        result = savePls(filename);
    } else {
        result = saveM3u(filename);
    }

    if (result) {
        msg(tr("Saved %1").arg(fi.fileName()));
    } else {
        QMessageBox::warning(this, tr("Save failed"),
                             tr("Failed to save %1").arg(filename),
                             QMessageBox::Ok);
    }
    return result;
}

bool TPlaylist::maybeSave() {

    if (!modified)
        return true;

    int res = QMessageBox::question(this,
        tr("Playlist modified"),
        tr("The playlist has been modified, do you want to save the changes?"),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    switch (res) {
        case QMessageBox::No: setModified(false); return true;
        case QMessageBox::Cancel: return false; // Cancel operation
        default: return save();
    }
}

void TPlaylist::saveSettings() {
	qDebug("Gui::TPlaylist::saveSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("playlist");
    set->setValue("recursive_add_directories", recursive_add_directories);
	set->setValue("repeat", repeatAct->isChecked());
	set->setValue("shuffle", shuffleAct->isChecked());
	set->endGroup();
}

void TPlaylist::loadSettings() {
	qDebug("Gui::TPlaylist::loadSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("playlist");
    recursive_add_directories = set->value("recursive_add_directories",
                                         recursive_add_directories).toBool();
    repeatAct->setChecked(set->value("repeat",
                                     repeatAct->isChecked()).toBool());
    shuffleAct->setChecked(set->value("shuffle",
                                      shuffleAct->isChecked()).toBool());
	set->endGroup();
}

} // namespace Playlist
} // namespace Gui

#include "moc_playlist.cpp"

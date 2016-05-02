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

#include "gui/playlist.h"

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
#include "gui/playlistwidget.h"
#include "core.h"
#include "gui/multilineinputdialog.h"
#include "gui/action/menuinoutpoints.h"
#include "gui/action/action.h"
#include "images.h"
#include "helper.h"
#include "filedialog.h"
#include "extensions.h"
#include "version.h"


using namespace Settings;

namespace Gui {

using namespace Action;


TPlaylist::TPlaylist(TBase* mw, TCore* c) :
    QWidget(mw),
    main_window(mw),
    core(c) ,
    recursive_add_directory(true),
    notify_sel_changed(true),
    modified(false) {

    createTree();
    createActions();
	createToolbar();

    connect(core, SIGNAL(startPlayingNewMedia()),
            this, SLOT(onStartPlayingNewMedia()));
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
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	setAcceptDrops(true);
	setAttribute(Qt::WA_NoMousePropagation);

	// Random seed
	QTime t;
	t.start();
	qsrand(t.hour() * 3600 + t.minute() * 60 + t.second());
}

TPlaylist::~TPlaylist() {
}

void TPlaylist::createTree() {

    playlistWidget = new TPlaylistWidget(this);
    playlistWidget->setObjectName("playlist_tree");
    connect(playlistWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
             this, SLOT(onItemActivated(QTreeWidgetItem*, int)));
}

void TPlaylist::createActions() {

    // Open
    openAct = new TAction(this, "pl_open", tr("Open &playlist..."), "",
                          QKeySequence("Ctrl+P"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(load()));

    // Save
    saveAct = new TAction(this, "pl_save", tr("&Save playlist"), "save", QKeySequence("Ctrl+W"));
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    // Play/pause
    playOrPauseAct = new TAction(this, "pl_play_or_pause", tr("&Play"), "play",
                                 Qt::Key_Space);
    // Add MCE remote key
    playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playOrPauseAct, SIGNAL(triggered()), this, SLOT(playOrPause()));
    connect(this, SIGNAL(visibilityChanged(bool)),
            this, SLOT(onVisibilityChanged(bool)));

    // Stop action, only in play menu, but owned by playlist
    stopAct = new TAction(this, "stop", tr("&Stop"), "", Qt::Key_MediaStop);
    connect(stopAct, SIGNAL(triggered()), core, SLOT(stop()));

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
    popup->addAction(playOrPauseAct);
    popup->addAction(stopAct);
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

    playlistWidget->clr();
    setModified(false);
}

void TPlaylist::cleanAndAddItem(QString filename, QString name,
                                double duration) {

    bool protect_name = !name.isEmpty();
    QString alt_name = filename;

    QFileInfo fi(filename);
    if (fi.exists()) {
        filename = QDir::toNativeSeparators(fi.absoluteFilePath());
        alt_name = fi.fileName();
    } else if (!playlist_path.isEmpty()) {
        fi.setFile(playlist_path, filename);
        if (fi.exists()) {
            filename = QDir::toNativeSeparators(fi.absoluteFilePath());
            alt_name = fi.fileName();
        }
    }
    if (name.isEmpty()) {
        name = alt_name;
    }
    TPlaylistWidgetItem* i = new TPlaylistWidgetItem(playlistWidget->root(),
                                                     filename,
                                                     name,
                                                     duration,
                                                     false);

    // Protect name
    if (protect_name) {
        i->setEdited(true);
    }
}

void TPlaylist::addCurrentFile() {
    qDebug("Gui::TPlaylist::addCurrentFile");

    if (core->mdat.filename.count()) {
        TPlaylistWidgetItem* i = new TPlaylistWidgetItem(
            playlistWidget->currentPlaylistWidgetFolder(),
            core->mdat.filename,
            core->mdat.displayName(),
            core->mdat.duration,
            false);
        i->setPlayed(true);
    }
}

void TPlaylist::addFile(QTreeWidgetItem* parent, const QString &filename) {
	qDebug() << "Gui::TPlaylist::addFile:" << filename;
	// Note: currently addFile loads playlists and addDirectory skips them,
	// giving a nice balance. Load if the individual file is requested,
	// skip when adding a directory.

	QFileInfo fi(filename);
	if (fi.exists()) {
		QString ext = fi.suffix().toLower();
		if (ext == "m3u" || ext == "m3u8") {
			loadM3u(filename, false, false);
		} else if (ext == "pls") {
			loadIni(filename, false, false);
		} else {
            new TPlaylistWidgetItem(parent, QDir::toNativeSeparators(filename), fi.fileName(), 0, false);
		}

		pref->latest_dir = fi.absolutePath();
	} else {
		QString name;
		TDiscName disc(filename);
		if (disc.valid) {
			// See also TTitleData::getDisplayName() from titletracks.cpp
			if (disc.protocol == "cdda" || disc.protocol == "vcd") {
				name = tr("Track %1").arg(QString::number(disc.title));
			} else {
				name = tr("Title %1").arg(QString::number(disc.title));
			}
		} else {
			name = filename;
		}
        new TPlaylistWidgetItem(parent, filename, name, 0, false);
	}
}

void TPlaylist::addDirectory(QTreeWidgetItem* parent, const QString &dir) {
    qDebug() << "Gui::TPlaylist::addDirectory:" << dir;

    static TExtensions ext;
    static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

    emit displayMessage(dir, 0);

    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(0,
                                                     dir,
                                                     QDir(dir).dirName(),
                                                     0,
                                                     true);

    QFileInfo fi;
    foreach(const QString filename, QDir(dir).entryList()) {
        if (filename != "." && filename != "..") {
            fi.setFile(dir, filename);
            if (fi.isDir()) {
                if (recursive_add_directory) {
                    addDirectory(w, fi.absoluteFilePath());
                }
            } else if (rx_ext.indexIn(fi.suffix()) >= 0) {
                addFile(w, fi.absoluteFilePath());
            }
        }
    }

    if (w->childCount()) {
        parent->addChild(w);
        pref->latest_dir = dir;
    } else {
        delete w;
    }
}

void TPlaylist::addDirectory() {

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					pref->latest_dir);

	if (!s.isEmpty()) {
        addDirectory(playlistWidget->root(), s);
	}
}

void TPlaylist::addFileOrDir(QTreeWidgetItem* parent, const QString& filename) {

	if (QFileInfo(filename).isDir()) {
        addDirectory(parent, filename);
	} else {
        addFile(parent, filename);
	}
}

void TPlaylist::addFiles(const QStringList &files, bool insert) {
    qDebug() << "Gui::TPlaylist::addFiles";

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // When inserting, save the current item and anything behind it into tail
    /*
    TPlaylistItemList tail;
    TPlaylistWidgetItem* i = currentPlaylistWidgetItem();
    if (insert && i && pl.count()) {
        int idx = playlistWidget->indexOfTopLevelItem(i);
        tail = pl.mid(idx);
        for(int r = 0; r < tail.count(); r++) {
            pl.removeLast();
        }
    } else {
        insert = false; // append
    }
    */

    // Add the files
    foreach(QString file, files) {

        // Check here instead of in addFile() for performance
        if (file.startsWith("file:")) {
            file = QUrl(file).toLocalFile();
        }

#ifdef Q_OS_WIN
        {
            // Check for Windows shortcuts
            QFileInfo fi(file);
            if (fi.isSymLink()) {
                file = fi.symLinkTarget();
            }
        }
#endif

        addFileOrDir(playlistWidget->root(), file);
    }

    // Restore tail
    //pl.append(tail);

    //updateView();
    //playlistWidget->setCurrentItem(i);

    QApplication::restoreOverrideCursor();

    qDebug() << "Gui::TPlaylist::addFiles: done";
}

void TPlaylist::addFiles() {

	TExtensions e;
	QStringList files = MyFileDialog::getOpenFileNames(this,
		tr("Select one or more files to open"), pref->latest_dir,
		tr("Multimedia") + e.multimedia().forFilter() + ";;" +
			tr("All files") +" (*.*)");

	if (files.count() > 0)
		addFiles(files);
}

void TPlaylist::addUrls() {

	TMultilineInputDialog d(this);
	if (d.exec() == QDialog::Accepted) {
		playlist_path = pref->latest_dir;
        foreach(const QString url, d.lines()) {
            if (url.count()) {
                // TODO:
                cleanAndAddItem(url, "", 0);
            }
		}
	}
}

void TPlaylist::onRepeatToggled(bool toggled) {

    // Enable depends on repeat
    enableActions();

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

void TPlaylist::playOrPause() {
    qDebug() << "Gui::TPlaylist:playOrPause: state" << core->stateToString()
             << "visible" << isVisible();

    TPlaylistWidgetItem* item = playlistWidget->currentPlaylistWidgetItem();

    if (item && isVisible() && item != playlistWidget->playing_item) {
        playItem(item);
    } else if (core->state() == STATE_PLAYING) {
        core->pause();
    } else 	if (core->state() == STATE_PAUSED) {
        core->play();
    } else if (playlistWidget->playing_item) {
        playItem(playlistWidget->playing_item);
    } else {
        core->open();
    }
}

TPlaylistWidgetItem* TPlaylist::chooseRandomItem() const {

    if (playlistWidget->topLevelItemCount() <= 0) {
        return 0;
    }

    bool foundFreeItem = false;
    int selected = (int) ((double) playlistWidget->count() * qrand() / (RAND_MAX + 1.0));
    bool foundSelected = false;
    int idx = 0;

    do {
        QTreeWidgetItemIterator it(playlistWidget);
        while (*it) {
            if (idx == selected) {
                foundSelected = true;
            }

            TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
            if (!i->isFolder() && !i->played() && i->state() != PSTATE_FAILED) {
                if (foundSelected) {
                    qDebug() << "Gui::TPlaylist::chooseRandomItem: selecting"
                             << i->filename();
                    return i;
                } else {
                    foundFreeItem = true;
                }
            }

            idx++;
            it++;
        }

        idx = 0;
    } while (foundFreeItem);

    qDebug() << "Gui::TPlaylist::chooseRandomItem: end of playlist";
    return 0;
}

void TPlaylist::startPlay() {
	qDebug("Gui::TPlaylist::startPlay");

    if (playlistWidget->topLevelItemCount() > 0) {
		if (shuffleAct->isChecked())
			playItem(chooseRandomItem());
		else
            playItem(playlistWidget->firstPlaylistWidgetItem());
	} else {
        emit displayMessage(tr("Found no files to play"), 6000);
	}
}

void TPlaylist::playItem(TPlaylistWidgetItem* item) {

    while (item && (item->isFolder()
                    || item->filename().isEmpty())) {
        item = playlistWidget->getNextPlaylistWidgetItem(item);
    }
    if (item) {
        qDebug() << "Gui::TPlaylist::playItem:" << item->filename();
        item->setState(PSTATE_LOADING);
        playlistWidget->setPlayingItem(item);
        core->open(item->filename());
    } else {
        qDebug("Gui::TPlaylist::playItem: end of playlist");
		emit playlistEnded();
	}
}

void TPlaylist::playNext() {
	qDebug("Gui::TPlaylist::playNext");

    TPlaylistWidgetItem* item = 0;
	if (shuffleAct->isChecked()) {
		item = chooseRandomItem();
        if (item == 0 && repeatAct->isChecked()) {
            playlistWidget->clearPlayed();
            item = chooseRandomItem();
		}
    } else {
        item = playlistWidget->getNextPlaylistWidgetItem();
        if (item == 0 && repeatAct->isChecked()) {
            item = playlistWidget->firstPlaylistWidgetItem();
        }
	}
	playItem(item);
}

void TPlaylist::playPrev() {
	qDebug("Gui::TPlaylist::playPrev");

    TPlaylistWidgetItem* i = playlistWidget->getPreviousPlaylistWidgetItem();
    if (i) {
        playItem(i);
	}
}

void TPlaylist::playDirectory(const QString &dir) {
	qDebug("Gui::TPlaylist::playDirectory");

    setWinTitle(QDir(dir).dirName());

    QString playlist = dir + "/" + TConfig::PROGRAM_ID + ".m3u8";
    if (QFileInfo(playlist).exists()) {
        loadM3u(playlist);
    } else if (Helper::directoryContainsDVD(dir)) {
        // onStartPlayingNewMedia() will pickup the playlist
		core->open(dir);
	} else {
        clear();
        core->setState(STATE_LOADING);
        addDirectory(playlistWidget->root(), dir);
        startPlay();
	}
}

void TPlaylist::resumePlay() {

    TPlaylistWidgetItem* item = playlistWidget->firstPlaylistWidgetItem();
    if (item) {
        playItem(item);
	}
}

bool TPlaylist::deleteFileFromDisk(const QString& filename) {

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
        if (filename == playingFile()
            && core->state() != STATE_STOPPED) {
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

    // TODO: reverse order
    QTreeWidgetItemIterator it(playlistWidget, QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!deleteFromDisk || deleteFileFromDisk(i->filename())) {
            delete i;
            setModified();
        }
        it++;
    }
}

void TPlaylist::removeSelectedFromDisk() {
	removeSelected(true);
}

void TPlaylist::removeAll() {
	clear();
}

void TPlaylist::showContextMenu(const QPoint & pos) {
	qDebug("Gui::TPlaylist::showContextMenu: x: %d y: %d", pos.x(), pos.y());

	if (!popup->isVisible()) {
        Action::execPopup(this, popup, playlistWidget->viewport()->mapToGlobal(pos));
	}
}

void TPlaylist::onItemActivated(QTreeWidgetItem* item, int) {
    qDebug() << "Gui::TPlaylist::onItemActivated";

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(item);
    if (i) {
        if (i->isFolder()) {
            i->setExpanded(!i->isExpanded());
        } else {
            playItem(i);
        }
    }
}

void TPlaylist::onVisibilityChanged(bool) {

    // Play depends on visible
    if (core->state() == STATE_PLAYING)
        enableActions();
}

void TPlaylist::enableActions() {

    // Note: there is always something selected when c > 0
    int c = playlistWidget->topLevelItemCount();
    TCoreState s = core->state();
    QString state_str = core->stateToString();
    qDebug() << "Gui::TPlaylist::enableActions: state" << state_str
             << "top level count" << c
             << "visible" << isVisible();

    saveAct->setEnabled(c > 0);

    bool enable = s == STATE_STOPPED || s == STATE_PLAYING || s == STATE_PAUSED;
    TPlaylistWidgetItem* playing_item = playlistWidget->playing_item;
    TPlaylistWidgetItem* current_item = playlistWidget->currentPlaylistWidgetItem();

    playOrPauseAct->setEnabled(enable
        && (core->mdat.filename.count() || playing_item || current_item));

    if (!enable) {
        playOrPauseAct->setTextAndTip(state_str);
        playOrPauseAct->setIcon(Images::icon("loading"));
    } else if (s == STATE_PLAYING) {
        playing_item->setState(PSTATE_PLAYING);
        if (current_item && isVisible() && current_item != playing_item) {
            playOrPauseAct->setTextAndTip(tr("&Play selected"));
            playOrPauseAct->setIcon(Images::icon("play"));
        } else {
            playOrPauseAct->setTextAndTip(tr("&Pause"));
            playOrPauseAct->setIcon(Images::icon("pause"));
        }
    } else if (playing_item && playing_item->state() == PSTATE_LOADING) {
        playOrPauseAct->setTextAndTip(tr("Loading"));
        playOrPauseAct->setIcon(Images::icon("loading"));
    } else {
        playOrPauseAct->setTextAndTip(tr("&Play"));
        playOrPauseAct->setIcon(Images::icon("play"));
    }

    // Stop action
    stopAct->setEnabled(s == STATE_PLAYING || s == STATE_PAUSED);

    // Prev/Next
    bool changed = false;
    bool e = enable && ((c > 0 && repeatAct->isChecked())
                        || playlistWidget->getNextPlaylistWidgetItem());
    if (e != nextAct->isEnabled()) {
        nextAct->setEnabled(e);
        changed = true;
    }
    e = enable && ((c > 0 && repeatAct->isChecked())
                   || playlistWidget->getPreviousPlaylistWidgetItem());
    if (e != prevAct->isEnabled()) {
        prevAct->setEnabled(e);
        changed = true;
    }
    if (changed) {
        // Update forward/rewind menus
        emit enablePrevNextChanged();
    }

    addCurrentAct->setEnabled(core->mdat.filename.count());

    removeSelectedAct->setEnabled(c > 0);
    removeSelectedFromDiskAct->setEnabled(c > 0);
    removeAllAct->setEnabled(c > 0);

    cutAct->setEnabled(c > 0);
    copyAct->setEnabled(c > 0);
    editAct->setEnabled(current_item || playing_item);
}

void TPlaylist::onPlayerError() {

    TPlaylistWidgetItem* item = playlistWidget->playing_item;
    if (item) {
        item->setState(PSTATE_FAILED);
        playlistWidget->scrollToItem(item);
    }
}

void TPlaylist::onStartPlayingNewMedia() {

    const TMediaData* md = &core->mdat;
	QString filename = md->filename;
    QString current_filename = playlistWidget->playingFile();

	if (filename == current_filename) {
        qDebug("Gui::TPlaylist::onStartPlayingNewMedia: new file is current item");
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

    if (md->disc.valid && md->titles.count() == playlistWidget->count()) {
		TDiscName cur_disc(current_filename);
		if (cur_disc.valid
			&& cur_disc.protocol == md->disc.protocol
			&& cur_disc.device == md->disc.device) {
            qDebug("Gui::TPlaylist::onStartPlayingNewMedia: new file is from current disc");
			return;
		}
	}

	// Create new playlist
	clear();

    TPlaylistWidgetItem* current = 0;
	if (md->disc.valid) {
		// Add disc titles
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
                current = i;
			}
		}
	} else {
		// Add current file
        current = new TPlaylistWidgetItem(playlistWidget->root(),
                                          filename,
                                          core->mdat.displayName(),
                                          core->mdat.duration,
                                          false);

		// Add associated files to playlist
		if (md->selected_type == TMediaData::TYPE_FILE
			&& pref->media_to_add_to_playlist != TPreferences::NoFiles) {
            qDebug() << "Gui::TPlaylist::onStartPlayingNewMedia: searching for files to add to playlist for"
					 << filename;
			QStringList files_to_add = Helper::filesForPlaylist(
				filename, pref->media_to_add_to_playlist);
			if (files_to_add.isEmpty()) {
                qDebug("Gui::TPlaylist::onStartPlayingNewMedia: none found");
			} else {
				addFiles(files_to_add);
			}
		}
	}

	// Mark current item as played
    if (current) {
        current->setPlayed(true);
	}

    qDebug() << "Gui::TPlaylist::onStartPlayingNewMedia: created new playlist for"
             << filename;
}

void TPlaylist::onMediaEOF() {
	qDebug("Gui::Tplaylist::onMediaEOF");
	playNext();
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
        i->setState(PSTATE_PLAYING);
        playlistWidget->setPlayingItem(i);
    } else {
        qWarning() << "Gui::TPlaylist::onTitleTrackChanged: title id" << id
                   << "filename" << filename << "not found in playlist";
    }
}

void TPlaylist::copySelected() {

	QString text;
	int copied = 0;

    QTreeWidgetItemIterator it(playlistWidget, QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        text += i->filename() + "\n";
        copied++;
        it++;
    }

    if (copied == 0 && playlistWidget->playing_item) {
        text = playlistWidget->playing_item->filename() + "\n";
		copied = 1;
	}

	if (copied > 0) {
		if (copied == 1) {
			// Remove trailing new line
			text = text.left(text.length() - 1);
            msg(tr("Copied %1").arg(text));
		} else {
            msg(tr("Copied %1 file names").arg(copied));
		}
		QApplication::clipboard()->setText(text);
	}
}

void TPlaylist::paste() {

    QStringList files = QApplication::clipboard()->text()
                        .split("\n",  QString::SkipEmptyParts);
    if (files.count()) {
        setModified();
        addFiles(files, true);
    }
}

void TPlaylist::enablePaste() {
    pasteAct->setEnabled(QApplication::clipboard()->mimeData()->hasText());
}

void TPlaylist::cut() {

    copySelected();
    removeSelected();
}

void TPlaylist:: setWinTitle(QString s) {

    if (s.count()) {
        title = s;
    }
    QString winTitle = tr("WZPlaylist%1%2%3",
        "optional white space, optional playlist name, optional modified *")
        .arg(title.isEmpty() ? "" : " ").arg(title).arg(modified ? "*" : "");
    setWindowTitle(winTitle);
    emit windowTitleChanged();
}

void TPlaylist::setPlaylistFilename(const QString& name) {

    QFileInfo fi(name);
    playlist_filename = fi.absoluteFilePath();
    pref->latest_dir = fi.absolutePath();
    setWinTitle(fi.fileName());
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
    } else if (playlistWidget->playing_item) {
        editItem(playlistWidget->playing_item);
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

void TPlaylist::onDropRow(int, int) {
    /*
    qDebug() << "Gui::TPlaylist:onDropRow:" << srow << drow
             << playlistWidget->dropRows;

    int delta = drow - srow;
    if (delta == 0) {
        return;
    }

    QList<int>* rows = &playlistWidget->dropRows;
    if (delta > 0) {
        qSort(rows->begin(), rows->end(), qGreater<int>());
    } else {
        qSort(*rows);
    }

    notify_sel_changed = false;
    playlistWidget->clearSelection();

    foreach(int s, *rows) {
        int d = s + delta;
        qDebug() << s << d;
        // Delete and insert
        pl.insert(d, pl.takeAt(s));
        // Set destination selected
        playlistWidget->selRow(d, true);
        // Keep playing item uptodate
        if (delta > 0 && s < playing_item && d >= playing_item) {
            playing_item--;
        } else if (delta < 0 && s > playing_item && d <= playing_item) {
            playing_item++;
        } else if (s == playing_item) {
            playing_item = d;
        }
    }
    rows->clear();

    updateView();
    notify_sel_changed = true;
    enableActions();
    setModified();
    */
}

// Drag&drop
void TPlaylist::dragEnterEvent(QDragEnterEvent *e) {
    qDebug() << "Gui::TPlaylist::dragEnterEvent" << e->mimeData()->formats();

	if (e->mimeData()->hasUrls()) {
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
        } else if (e->possibleActions() & Qt::CopyAction) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
        }
    }
}

void TPlaylist::dropEvent(QDropEvent *e) {
    qDebug() << "Gui::TPlaylist::dropEvent" << e->mimeData()->formats();

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl url, e->mimeData()->urls()) {
            files.append(url.toString());
        }

        if (files.count()) {
            QTreeWidgetItem* item = playlistWidget->itemAt(e->pos() - playlistWidget->pos());
            if (item) {
                playlistWidget->setCurrentItem(item);
                addFiles(files, true);
            } else {
                addFiles(files);
            }
            setModified();
        }

        e->accept();
    }
}


void TPlaylist::hideEvent(QHideEvent*) {
	emit visibilityChanged(false);
}

void TPlaylist::showEvent(QShowEvent*) {
	emit visibilityChanged(true);
}

void TPlaylist::closeEvent(QCloseEvent* e)  {
	saveSettings();
	e->accept();
}

void TPlaylist::loadM3u(const QString&file, bool clear, bool play) {
	qDebug("Gui::TPlaylist::loadM3u");

    QRegExp info("^#EXTINF:(.*),(.*)");
    bool utf8 = QFileInfo(file).suffix().toLower() == "m3u8";

	QFile f(file);
	if (f.open(QIODevice::ReadOnly)) {
        setPlaylistFilename(file);
        playlist_path = QFileInfo(file).path();
        if (clear) {
            this->clear();
        }

		QTextStream stream(&f);
		if (utf8) {
			stream.setCodec("UTF-8");
		} else {
			stream.setCodec(QTextCodec::codecForLocale());
		}

        QString name;
		double duration = 0;

		while (!stream.atEnd()) {
			QString line = stream.readLine().trimmed();
			// Ignore empty lines
			if (line.isEmpty()) {
				continue;
			}

			qDebug() << "Gui::TPlaylist::loadM3u: line:" << line;
			if (info.indexIn(line) >= 0) {
				duration = info.cap(1).toDouble();
				name = info.cap(2);
				qDebug() << "Gui::TPlaylist::loadM3u: name:" << name << "duration:" << duration;
			} else if (line.startsWith("#")) {
				// Ignore comments
			} else {
                cleanAndAddItem(line, name, duration);
				name = "";
				duration = 0;
			}
		}

		f.close();
		if (play) {
			startPlay();
		}
	}
}

void TPlaylist::loadIni(const QString &file, bool clear, bool play) {
	qDebug("Gui::TPlaylist::loadIni");

	if (!QFile::exists(file)) {
		qDebug("Gui::TPlaylist::loadIni: '%s' doesn't exist, doing nothing", file.toUtf8().constData());
		return;
	}

    setPlaylistFilename(file);
	playlist_path = QFileInfo(file).path();

	QSettings set(file, QSettings::IniFormat);
	set.beginGroup("playlist");

	if (set.status() == QSettings::NoError) {
		if (clear)
			this->clear();
		QString filename;
		QString name;
		double duration;

		int num_items = set.value("NumberOfEntries", 0).toInt();

		for (int n = 0; n < num_items; n++) {
			filename = set.value("File" + QString::number(n + 1), "").toString();
			name = set.value("Title" + QString::number(n + 1), "").toString();
			duration = (double) set.value("Length" + QString::number(n + 1), 0).toInt();
            cleanAndAddItem(filename, name, duration);
		}
	}

	set.endGroup();

	if (play && (set.status() == QSettings::NoError)) {
		startPlay();
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
        ++it;
	}

	f.close();
    setModified(false);

	return true;
}

void TPlaylist::load() {

	if (maybeSave()) {
		TExtensions e;
		QString s = MyFileDialog::getOpenFileName(
			this, tr("Choose a file"),
			pref->latest_dir,
			tr("Playlists") + e.playlist().forFilter());

		if (!s.isEmpty()) {
			QFileInfo fi(s);
			pref->latest_dir = fi.absolutePath();
			if (fi.suffix().toLower() == "pls")
				loadIni(s);
			else
				loadM3u(s);
		}
	}
}

bool TPlaylist::save() {

	TExtensions e;
	QString s = MyFileDialog::getSaveFileName(
        this, tr("Choose a filename"), pref->latest_dir,
		tr("Playlists") + e.playlist().forFilter());

    if (s.isEmpty()) {
        return false;
    }

    // If filename has no extension, add it
    if (QFileInfo(s).suffix().isEmpty()) {
        s = s + ".m3u8";
    }
    if (QFileInfo(s).exists()) {
        int res = QMessageBox::question(this,
            tr("Confirm overwrite?"),
            tr("The file %1 already exists.\n"
            "Do you want to overwrite it?").arg(s),
            QMessageBox::Yes,
            QMessageBox::No,
            QMessageBox::NoButton);
        if (res == QMessageBox::No) {
            return false;
        }
    }

    setPlaylistFilename(s);
    return saveM3u(s);
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
	set->setValue("recursive_add_directory", recursive_add_directory);
	set->setValue("repeat", repeatAct->isChecked());
	set->setValue("shuffle", shuffleAct->isChecked());
	set->endGroup();
}

void TPlaylist::loadSettings() {
	qDebug("Gui::TPlaylist::loadSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("playlist");
	recursive_add_directory = set->value("recursive_add_directory", recursive_add_directory).toBool();
	repeatAct->setChecked(set->value("repeat", repeatAct->isChecked()).toBool());
	shuffleAct->setChecked(set->value("shuffle", shuffleAct->isChecked()).toBool());
	set->endGroup();
}

} // namespace Gui

#include "moc_playlist.cpp"

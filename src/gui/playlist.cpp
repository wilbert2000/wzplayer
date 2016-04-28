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
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QTextCodec>
#include <QMimeData>
#include <QClipboard>

#include "gui/base.h"
#include "gui/tablewidget.h"
#include "gui/multilineinputdialog.h"
#include "gui/action/menuinoutpoints.h"
#include "gui/action/action.h"
#include "core.h"
#include "images.h"
#include "helper.h"
#include "filedialog.h"
#include "extensions.h"
#include "version.h"


using namespace Settings;

namespace Gui {

using namespace Action;


TPlaylistItem::TPlaylistItem() :
    _duration(0),
    _played(false),
    _deleted(false),
    _edited(false),
    _failed(false) {
}

TPlaylistItem::TPlaylistItem(const QString &filename, const QString &name,
                             double duration) :
    _filename(filename),
    _name(name),
    _duration(duration),
    _played(false),
    _deleted(false),
    _edited(false),
    _failed(false) {

	_directory = QFileInfo(filename).absolutePath();
}

TPlaylist::TPlaylist(TBase* mw, TCore* c) :
    QWidget(mw),
    current_item(-1),
    loading(false),
    main_window(mw),
    core(c) ,
    recursive_add_directory(true),
    save_playlist_in_config(false),
    modified(false),
    notify_sel_changed(true) {

    createTable();
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
	layout->addWidget(listView);
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

void TPlaylist::createTable() {

	listView = new TTableWidget(0, COL_COUNT, this);
	listView->setObjectName("playlist_table");
	listView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	listView->setSelectionBehavior(QAbstractItemView::SelectRows);
	listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	listView->setContextMenuPolicy(Qt::CustomContextMenu);
    //listView->setShowGrid(false);
	listView->setSortingEnabled(false);
    const int ICON_SIZE = 22;
    listView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

#if QT_VERSION >= 0x050000
    listView->horizontalHeader()->setSectionResizeMode(COL_PLAY, QHeaderView::Fixed);
    listView->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
    listView->horizontalHeader()->setSectionResizeMode(COL_TIME, QHeaderView::Fixed);
#else
    listView->horizontalHeader()->setResizeMode(COL_PLAY, QHeaderView::Fixed);
    listView->horizontalHeader()->setResizeMode(COL_NAME, QHeaderView::Stretch);
    listView->horizontalHeader()->setResizeMode(COL_TIME, QHeaderView::Fixed);
#endif

    // TODO: with ResizeToContents the first time the play column is too small...
    // TODO: with ResizeToContents the first time the time column is too small...
    const int MARGINS = 7;
    listView->setColumnWidth(COL_PLAY, ICON_SIZE + MARGINS);
    listView->setColumnWidth(COL_TIME, listView->fontMetrics().width("00:00:00") + MARGINS);

    listView->setDragEnabled(true);
    listView->setAcceptDrops(true);
    listView->setDragDropMode(QAbstractItemView::DragDrop);
    listView->setDragDropOverwriteMode(false);
    listView->setDropIndicatorShown(true);
    listView->setDefaultDropAction(Qt::MoveAction);
    connect(listView, SIGNAL(dropRow(int,int)),
            this, SLOT(onDropRow(int, int)), Qt::QueuedConnection);

	connect(listView, SIGNAL(cellActivated(int,int)),
			 this, SLOT(onCellActivated(int, int)));

	// EDIT BY NEO -->
	connect(listView->horizontalHeader(), SIGNAL(sectionClicked(int)),
			this, SLOT(sortBy(int)));
	// <--
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

    // Up
    moveUpAct = new TAction(this, "pl_move_up", tr("Move &up"));
    connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveItemsUp()));

    // Down
    moveDownAct = new TAction(this, "pl_move_down", tr("Move &down"));
    connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveItemsDown()));

    connect(listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)));

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

	// Copy
    copyAct = new TAction(this, "pl_copy", tr("&Copy filename(s)"), "",
                          QKeySequence("Ctrl+C"));
	connect(copyAct, SIGNAL(triggered()), this, SLOT(copySelected()));

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
    toolbar->addAction(inOutMenu->findChild<TAction*>("repeat_in_out"));
    toolbar->addAction(repeatAct);
    toolbar->addAction(shuffleAct);
    toolbar->addSeparator();
    toolbar->addAction(prevAct);
	toolbar->addAction(nextAct);
    toolbar->addSeparator();
    toolbar->addAction(moveUpAct);
    toolbar->addAction(moveDownAct);

	// Popup menu
    popup = new QMenu(this);
    popup->addAction(editAct);
    popup->addAction(copyAct);
    popup->addSeparator();
    popup->addAction(playOrPauseAct);
    popup->addAction(stopAct);
    popup->addSeparator();
    popup->addMenu(inOutMenu);
    popup->addSeparator();
    popup->addMenu(add_menu);
    popup->addMenu(remove_menu);

	connect(listView, SIGNAL(customContextMenuRequested(const QPoint &)),
			this, SLOT(showContextMenu(const QPoint &)));
}

void TPlaylist::retranslateStrings() {
	//qDebug("Gui::TPlaylist::retranslateStrings");

	// Icon
	setWindowIcon(Images::icon("logo", 64));
	setWindowTitle(tr("%1 - Playlist").arg(TConfig::PROGRAM_NAME));

	listView->setHorizontalHeaderLabels(QStringList()
										<< "   "
										<< tr("Name")
										<< tr("Length"));
}

void TPlaylist::msg(const QString& s) {
    emit displayMessageOnOSD(s, TConfig::MESSAGE_DURATION);
}

void TPlaylist::getFilesAppend(QStringList& files) const {

	TPlaylistItemList::const_iterator i;
	for (i = pl.constBegin(); i != pl.constEnd(); i++) {
		files.append((*i).filename());
	}
}

void TPlaylist::updateView(int current) {
    qDebug("Gui::TPlaylist::updateView");

    if (current >= -1) {
        notify_sel_changed = false;
        listView->clearSelection();
        current_item = current;
    }

    listView->setRowCount(pl.count());

    for (int i = 0; i < pl.count(); i++) {
        const TPlaylistItem& item = pl.value(i);

        // Icon
        if (item.failed()) {
            listView->setIcon(i, COL_PLAY, Images::icon("failed"));
        } else if (i == current_item) {
            if (loading) {
                listView->setIcon(i, COL_PLAY, Images::icon("loading"));
            } else {
                listView->setIcon(i, COL_PLAY, Images::icon("play"));
            }
        } else if (item.played()) {
            listView->setIcon(i, COL_PLAY, Images::icon("ok"));
        } else {
            listView->setIcon(i, COL_PLAY, QPixmap());
        }

        // Name
        QString text = item.name();
        if (text.isEmpty())
            text = item.filename();
        listView->setText(i, COL_NAME, text);

        // Set tooltip to filename
        listView->item(i, COL_NAME)->setToolTip(item.filename());

        // Duration
        if (item.duration() > 0) {
            text = Helper::formatTime(qRound(item.duration()));
        } else {
            text = "";
        }
        listView->setText(i, COL_TIME, text);
    }

    if (current >= -1) {
        if (current >= 0 && current < pl.count()) {
            listView->setCurrentCell(current, 0);
        }
        notify_sel_changed = true;
    }

    // Takes a long time with large playlists...
    if (pl.count() < 1000) {
        listView->resizeRowsToContents();
    }

    enableActions();
}

void TPlaylist::setCurrentItem(int current) {
    qDebug("Gui::TPlaylist::setCurrentItem: from %d to %d", current_item, current);

    // Give old current_item an icon
    if (current_item >= 0
        && current_item != current
        && current_item < pl.count()
        && current_item < listView->rowCount()) {
        if (pl[current_item].played()) {
            listView->setIcon(current_item, COL_PLAY, Images::icon("ok"));
        } else if (pl[current_item].failed()){
            listView->setIcon(current_item, COL_PLAY, Images::icon("failed"));
        } else {
            listView->setIcon(current_item, COL_PLAY, QPixmap());
        }
    }

    notify_sel_changed = false;
    listView->clearSelection();
    current_item = current;

    if (current_item >= 0 && current_item < listView->rowCount()) {
        if (loading) {
            listView->setIcon(current_item, COL_PLAY, Images::icon("loading"));
        } else {
            listView->setIcon(current_item, COL_PLAY, Images::icon("play"));
        }
        listView->setCurrentCell(current_item, 0);
    }

    enableActions();
    notify_sel_changed = true;
}

void TPlaylist::clear() {

	pl.clear();
	listView->clearContents();
	listView->setRowCount(0);
    setCurrentItem(-1);
	modified = false;
}

void TPlaylist::addItem(const QString& filename, QString name, double duration) {
	pl.append(TPlaylistItem(filename, name, duration));
}

void TPlaylist::cleanAndAddItem(QString filename, QString name, double duration) {

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
	addItem(filename, name, duration);

	// Protect name
	if (protect_name) {
		pl[pl.count() - 1].setEdited(true);
	}
}

void TPlaylist::addCurrentFile() {
	qDebug("Gui::TPlaylist::addCurrentFile");

	if (!core->mdat.filename.isEmpty()) {
		addItem(core->mdat.filename, core->mdat.displayName(), core->mdat.duration);
		pl[pl.count() - 1].setPlayed(true);
        updateView(pl.count() - 1);
	}
}

void TPlaylist::addFile(const QString &filename) {
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
			addItem(QDir::toNativeSeparators(filename), fi.fileName(), 0);
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
		addItem(filename, name, 0);
	}
}

void TPlaylist::addDirectory(const QString &dir) {
	qDebug() << "Gui::TPlaylist::addDirectory:" << dir;

	static TExtensions ext;
	static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

	emit displayMessage(dir, 0);

	bool found_something = false;
	QFileInfo fi;
	QStringList dir_list = QDir(dir).entryList();
	QStringList::Iterator it = dir_list.begin();
	while(it != dir_list.end()) {
		QString filename = *it;
		if (filename != "." && filename != "..") {
			fi.setFile(dir, filename);
			if (fi.isDir()) {
				if (recursive_add_directory) {
					addDirectory(fi.absoluteFilePath());
				}
			} else if (rx_ext.indexIn(fi.suffix()) >= 0) {
				addFile(fi.absoluteFilePath());
			}
			found_something = true;
		}
		++it;
	}

	if (found_something) {
		pref->latest_dir = dir;
	}
}

void TPlaylist::addDirectory() {

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					pref->latest_dir);

	if (!s.isEmpty()) {
		addDirectory(s);
	}
}

void TPlaylist::addFileOrDir(const QString &filename) {

	if (QFileInfo(filename).isDir()) {
		addDirectory(filename);
	} else {
		addFile(filename);
	}
}

void TPlaylist::addFiles(const QStringList &files) {
	qDebug("Gui::TPlaylist::addFiles");

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QStringList::ConstIterator it = files.constBegin();
	while(it != files.constEnd()) {
		addFileOrDir(*it);
		++it;
	}

	updateView();
	QApplication::restoreOverrideCursor();

	qDebug("Gui::TPlaylist::addFiles: done");
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
                cleanAndAddItem(url, "", 0);
            }
		}
		updateView();
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
             << "current row" << listView->currentRow()
             << "current item" << current_item
             << "visible" << isVisible();

    int r = listView->currentRow();
    if (isVisible() && r >= 0 && r < count() && r != current_item) {
        playItem(listView->currentRow());
    } else if (core->state() == STATE_PLAYING) {
        core->pause();
    } else 	if (core->state() == STATE_PAUSED) {
        core->play();
    } else if (current_item >= 0) {
        playItem(current_item);
    } else {
        core->open();
    }
}

int TPlaylist::chooseRandomItem() {

	// Create list of items not yet played
	QList <int> fi;
	for (int n = 0; n < pl.count(); n++) {
        if (!pl[n].played() && !pl[n].failed())
			fi.append(n);
	}
	if (fi.count() == 0)
		return -1; // none free

	int selected = (int) ((double) fi.count() * qrand() / (RAND_MAX + 1.0));
	selected = fi[selected];

	qDebug("Gui::TPlaylist::chooseRandomItem: selected item: %d", selected);
	return selected;
}

void TPlaylist::startPlay() {
	qDebug("Gui::TPlaylist::startPlay");

	if (pl.count() > 0) {
		if (shuffleAct->isChecked())
			playItem(chooseRandomItem());
		else
			playItem(0);
	} else {
		emit displayMessage(tr("Found no files to play"), 0);
	}
}

void TPlaylist::playItem(int n) {
	qDebug("Gui::TPlaylist::playItem: %d (count: %d)", n, pl.count());

	if (n < 0 || n >= pl.count()) {
		qDebug("Gui::TPlaylist::playItem: out of range");
		emit playlistEnded();
		return;
	}

	TPlaylistItem& item = pl[n];
	if (!item.filename().isEmpty()) {
        loading = true;
        setCurrentItem(n);
        listView->setIcon(n, COL_PLAY, Images::icon("forward_menu"));
		core->open(item.filename());
	}
}

void TPlaylist::clearPlayedTag() {

	for (int n = 0; n < pl.count(); n++) {
		pl[n].setPlayed(false);
	}
	updateView();
}

void TPlaylist::playNext() {
	qDebug("Gui::TPlaylist::playNext");

	int item;
	if (shuffleAct->isChecked()) {
		// Shuffle
		item = chooseRandomItem();
		if (item < 0) {
			clearPlayedTag();
			if (repeatAct->isChecked()) {
				item = chooseRandomItem();
			}
		}
	} else if (current_item < pl.count() - 1) {
		item = current_item + 1;
	} else {
		clearPlayedTag();
		if (repeatAct->isChecked()) {
			item = 0;
		} else {
			item = -1;
		}
	}
	playItem(item);
}

void TPlaylist::playPrev() {
	qDebug("Gui::TPlaylist::playPrev");

	if (count() > 0) {
		if (current_item > 0) {
			playItem(current_item - 1);
		} else {
			playItem(count() - 1);
		}
	}
}

void TPlaylist::playDirectory(const QString &dir) {
	qDebug("Gui::TPlaylist::playDirectory");

	if (Helper::directoryContainsDVD(dir)) {
		core->open(dir);
	} else {
        core->setState(STATE_LOADING);
		clear();
		addDirectory(dir);
		sort();
		// sort() can set modified
		modified = false;
		startPlay();
	}
}

void TPlaylist::resumePlay() {

	if (pl.count() > 0) {
		if (current_item < 0 || current_item >= pl.count()) {
			current_item = 0;
		}
		playItem(current_item);
	}
}

bool TPlaylist::deleteFileFromDisk(int i) {

	QString filename = pl[i].filename();
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
		if (i == current_item && core->state() != STATE_STOPPED) {
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

	for (int i = listView->rowCount() - 1; i >= 0; i--) {
		if (listView->isSelected(i, 0)
			&& (!deleteFromDisk || deleteFileFromDisk(i))) {
			pl[i].setMarkForDeletion(true);
			if (i < current_item) {
				current_item--;
			} else if (i == current_item) {
				current_item = -1;
			}
		}
	}

	TPlaylistItemList::iterator it;
	for (it = pl.begin(); it != pl.end(); ++it) {
		if ((*it).markedForDeletion()) {
			qDebug() << "Removed" << (*it).filename();
			it = pl.erase(it);
			it--;
			modified = true;
		}
	}

	if (pl.isEmpty())
		modified = false;

    updateView(current_item);
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
		Action::execPopup(this, popup, listView->viewport()->mapToGlobal(pos));
	}
}

void TPlaylist::onCellActivated(int row, int) {
	//qDebug("Gui::TPlaylist::onCellActivated: row: %d", row);
	playItem(row);
}

void TPlaylist::updateCurrentItem() {
	qDebug() << "Gui::TPlaylist::updateCurrentItem:" << current_item;

	if (current_item >= 0 && current_item < pl.count()) {
		TPlaylistItem& item = pl[current_item];
		item.setPlayed(true);
        item.setFailed(false);
        TMediaData* md = &core->mdat;
		if (!md->disc.valid) {
			if (!item.edited()) {
				item.setName(md->displayName());
			}
			if (md->duration > 0) {
				item.setDuration(md->duration);
			}
		}
		updateView();
	}
}

void TPlaylist::onSelectionChanged(const QItemSelection&, const QItemSelection&) {

    // TODO: this is too expensive...
    if (notify_sel_changed)
        enableActions();
}

void TPlaylist::onVisibilityChanged(bool) {

    // Play depends on visible
    if (core->state() == STATE_PLAYING)
        enableActions();
}

void TPlaylist::enableUpDown(const QItemSelection& selected) {

    int c = count();
    bool sel2 = selected.count() > 1;
    bool e = listView->currentRow() > 0;
    moveUpAct->setEnabled(c > 1 && (e || sel2));
    e = listView->currentRow() >= 0
        && listView->currentRow() < listView->rowCount() - 1;
    moveDownAct->setEnabled(c > 1 && (e  || sel2));

}

void TPlaylist::scrollToCurrentItem() {
    listView->scrollToItem(listView->item(current_item, 0));
}

void TPlaylist::enableActions() {

    // Note: there is always something selected when c > 0
    int c = count();
    TCoreState s = core->state();
    QString state_str = core->stateToString();
    qDebug() << "Gui::TPlaylist::enableActions: state" << state_str
             << "current item" << current_item
             << "current row" << listView->currentRow()
             << "count" << c
             << "visible" << isVisible();

    saveAct->setEnabled(c > 0);

    bool enable = s == STATE_STOPPED || s == STATE_PLAYING || s == STATE_PAUSED;

    playOrPauseAct->setEnabled(enable
        && (current_item >= 0
            || listView->currentRow() >= 0
            || core->mdat.filename.count()));

    // Update icon
    if (!enable) {
        playOrPauseAct->setTextAndTip(state_str);
        playOrPauseAct->setIcon(Images::icon("loading"));
    } else if (s == STATE_PLAYING) {
        if (loading) {
            loading = false;
            pl[current_item].setFailed(false);
        }
        int r = listView->currentRow();
        if (isVisible() && r != current_item && r >= 0 && r < count()) {
            playOrPauseAct->setTextAndTip(tr("&Play selected"));
            playOrPauseAct->setIcon(Images::icon("play"));
        } else {
            playOrPauseAct->setTextAndTip(tr("&Pause"));
            playOrPauseAct->setIcon(Images::icon("pause"));
        }
    } else if (loading) {
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
                        || (c > 1 && current_item < c - 1));
    if (e != nextAct->isEnabled()) {
        nextAct->setEnabled(e);
        changed = true;
    }
    e = enable && ((c > 0 && repeatAct->isChecked()) || (c > 1 && current_item > 0));
    if (e != prevAct->isEnabled()) {
        prevAct->setEnabled(e);
        changed = true;
    }
    if (changed) {
        // Update forward/rewind menus
        emit enablePrevNextChanged();
    }

    // Move up/down
    enableUpDown(listView->selectionModel()->selection());
    addCurrentAct->setEnabled(core->mdat.filename.count());

    removeSelectedAct->setEnabled(c > 0);
    removeSelectedFromDiskAct->setEnabled(c > 0);
    removeAllAct->setEnabled(c > 0);

    copyAct->setEnabled(c > 0);
    editAct->setEnabled(listView->currentRow() >= 0
                        || current_item >= 0);

    if (s == STATE_PLAYING && current_item == listView->currentRow()) {
        scrollToCurrentItem();
    }
}

void TPlaylist::onPlayerError() {

    loading = false;
    if (current_item >= 0 && current_item < count()) {
        pl[current_item].setFailed(true);
        updateView();
        scrollToCurrentItem();
    }
}

void TPlaylist::onStartPlayingNewMedia() {

    loading = false;
    const TMediaData* md = &core->mdat;
	QString filename = md->filename;
	QString current_filename;
	if (current_item >= 0 && current_item < pl.count()) {
		current_filename = pl.at(current_item).filename();
	}

	if (filename == current_filename) {
        qDebug("Gui::TPlaylist::onStartPlayingNewMedia: new file is current item");
		updateCurrentItem();
		return;
	}

	if (md->disc.valid && md->titles.count() == count()) {
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

    int current = 0;
	if (md->disc.valid) {
		// Add disc titles
		TDiscName disc = md->disc;
        foreach(const Maps::TTitleData title, md->titles) {
            disc.title = title.getID();
			addItem(disc.toString(), title.getDisplayName(false),
					title.getDuration());
			if (title.getID() == md->titles.getSelectedID()) {
                current = title.getID() - md->titles.firstID();
			}
		}
	} else {
		// Add current file
		addItem(filename, core->mdat.displayName(), core->mdat.duration);

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
    if (current >= 0 && current < pl.count()) {
        pl[current].setPlayed(true);
	}
    updateView(current);

    qDebug() << "Gui::TPlaylist::onStartPlayingNewMedia: created new playlist with"
			 << count() << "items for" << filename;
}

void TPlaylist::onMediaEOF() {
	qDebug("Gui::Tplaylist::onMediaEOF");
	playNext();
}

void TPlaylist::onTitleTrackChanged(int id) {
	qDebug("Gui::TPlaylist::onTitleTrackChanged: %d", id);

	if (id < 0) {
		setCurrentItem(-1);
		return;
	}

	// Search for title on file name instead of using id as index,
	// because user can change order of the playlist.
	TDiscName disc = core->mdat.disc;
	disc.title = id;
	QString filename = disc.toString();

	for(int i = 0; i < pl.count(); i++) {
		if (pl[i].filename() == filename) {
			pl[i].setPlayed(true);
			setCurrentItem(i);
			return;
		}
	}

	qWarning() << "Gui::TPlaylist::onTitleTrackChanged: title id" << id
			   << "filename" << filename << "not found in playlist";
}

void TPlaylist::swapItems(int item1, int item2) {

	// Save current row
	int fix_current_row = -1;
	if (listView->currentRow() == item1) {
		fix_current_row = item2;
	} else if (listView->currentRow() == item2) {
		fix_current_row = item1;
	}

	// Swap playlist data
	pl.swap(item1, item2);

	// Swap current_item
	if (current_item == item1) {
		current_item = item2;
	} else if (current_item == item2) {
		current_item = item1;
	}

	// Swap selected
	bool selected1 = listView->isSelected(item1, 0);
	bool selected2 = listView->isSelected(item2, 0);
	if (selected1 != selected2) {
		listView->selRow(item1, selected2);
		listView->selRow(item2, selected1);
	}

	// Fix currentRow
	if (fix_current_row >= 0) {
		QItemSelection sel = listView->selectionModel()->selection();
		listView->setCurrentCell(fix_current_row, 0);
		listView->selectionModel()->select(sel, QItemSelectionModel::Current | QItemSelectionModel::Select);
	}

	modified = true;
}

void TPlaylist::moveItemsUp() {

	for(int row = 1; row < listView->rowCount(); row++) {
		if (listView->isSelected(row, 0)) {
			swapItems(row, row - 1);
		}
	}
	updateView();
}

void TPlaylist::moveItemsDown() {

	for(int row = listView->rowCount() - 2; row >= 0; row--) {
		if (listView->isSelected(row, 0)) {
			swapItems(row, row + 1);
		}
	}
	updateView();
}

void TPlaylist::copySelected() {

	QString text;
	int copied = 0;
	for (int i = 0; i < listView->rowCount(); i++) {
		if (listView->isSelected(i, 0) && i < pl.count()) {
			QString fn = pl[i].filename();
			if (!fn.isEmpty()) {
				text += fn + "\n";
				copied++;
			}
		}
	}

	if (copied == 0 && current_item >= 0 && current_item < pl.count()) {
		text = pl.at(current_item).filename() + "\n";
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

void TPlaylist::editCurrentItem() {

    int current = listView->currentRow();
    if (current < 0) {
        current = current_item;
    }
    if (current >= 0) {
        editItem(current);
    }
}

void TPlaylist::editItem(int item) {

	QString current_name = pl[item].name();
    if (current_name.isEmpty()) {
		current_name = pl[item].filename();
    }
    QString saved_name = current_name;

	bool ok;
    QString text = QInputDialog::getText(this, tr("Edit name"),
        tr("Name to display in playlist:"),	QLineEdit::Normal,
		current_name, &ok);
    if (ok && text != saved_name) {
		// user entered something and pressed OK
        pl[item].setName(text);
        pl[item].setEdited(true);
		modified = true;
		updateView();
	}
}

void TPlaylist::sort() {
    notify_sel_changed = false;
	sortBy(COL_NAME, false, false, 0);
    notify_sel_changed = true;
    enableActions();
}

void TPlaylist::sortBy(int section) {
    notify_sel_changed = false;
    sortBy(section, true, false, 0);
    notify_sel_changed = true;
    enableActions();
}

void TPlaylist::sortBy(int section, bool allow_revert, bool revert, int count) {

	// Bubble sort
	bool swapped = false;

	for (int n = 1; n < pl.count() - count; n++) {

		int last = n - 1;
		int current = n;

		// Revert the sort
		if (revert) {
			last = n;
			current = n - 1;
		}

		int compare = 0;

		if (section == 0) {
			// Sort by played
			if (!pl[last].played() && pl[current].played()) {
				compare = 1;
			}
		} else if (section == 1) {
			// Sort alphabetically on dir then filename
			TPlaylistItem& lastItem = pl[last];
			TPlaylistItem& currentItem = pl[current];
			compare = lastItem.directory().compare(currentItem.directory());
			if (compare == 0) {
				compare = lastItem.filename().compare(currentItem.filename());
			}
		} else if (pl[last].duration() > pl[current].duration()) {
			compare = 1;
		}

		// Swap items
		if (compare > 0) {
			swapItems(n, n - 1);
			swapped = true;
		}
	}


	if (swapped) {
		// Sort until there is nothing to sort
		sortBy(section, allow_revert, revert, ++count);
	} else if (allow_revert && !revert && count == 0) {
		// Revert sort
		sortBy(section, allow_revert, true, 0);
	} else {
		updateView();
	}
}

void TPlaylist::onDropRow(int srow, int drow) {
    qDebug() << "Gui::TPlaylist:onDropRow:" << srow << drow
             << listView->dropRows;

    int d = drow - srow;
    if (d > 0) {
        for(int i = listView->dropRows.count() - 1; i >= 0; i--) {
            int s = listView->dropRows[i];
            pl.insert(s + d, pl.takeAt(s));
            if (s == current_item) {
                current_item = s + d;
            }
        }
    } else {
        for(int i = 0; i < listView->dropRows.count(); i++) {
            int s = listView->dropRows[i];
            pl.insert(s + d, pl.takeAt(s));
            if (s == current_item) {
                current_item = s + d;
            }
        }
    }
    listView->dropRows.clear();
    modified = true;

    listView->clearSelection();
    updateView();
    listView->setCurrentCell(drow, 0);
}

// Drag&drop
void TPlaylist::dragEnterEvent(QDragEnterEvent *e) {
	qDebug("Gui::TPlaylist::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void TPlaylist::dropEvent(QDropEvent *e) {
    qDebug("Gui::TPlaylist::dropEvent");

    QStringList files;

    if (e->mimeData()->hasUrls()) {
        foreach(QUrl url, e->mimeData()->urls()) {
            if (url.scheme() == "file") {
                files.append(url.toLocalFile());
            } else {
                files.append(url.toString());
            }
        }
    }

#ifdef Q_OS_WIN
    files = Helper::resolveSymlinks(files); // Check for Windows shortcuts
#endif

    addFiles(files);
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

	bool utf8 = QFileInfo(file).suffix().toLower() == "m3u8";

	QRegExp info("^#EXTINF:(.*),(.*)");

	QFile f(file);
	if (f.open(QIODevice::ReadOnly)) {
		playlist_path = QFileInfo(file).path();
		if (clear)
			this->clear();
		QTextStream stream(&f);
		if (utf8) {
			stream.setCodec("UTF-8");
		} else {
			stream.setCodec(QTextCodec::codecForLocale());
		}

		QString name = "";
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
		updateView();
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

	updateView();

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
		   << " \n";

	TPlaylistItemList::iterator it;
	for (it = pl.begin(); it != pl.end(); it++) {
		stream << "#EXTINF:";
		stream << (*it).duration() << ",";
		stream << (*it).name() << "\n";

		QString filename = (*it).filename();

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

	f.close();
	modified = false;

	return true;
}

bool TPlaylist::saveIni(QString file) {
	qDebug() << "Gui::TPlaylist::saveIni:" << file;

	QString dir_path = QDir::toNativeSeparators(QFileInfo(file).path());
	if (!dir_path.endsWith(QDir::separator())) {
		dir_path += QDir::separator();
	}

	QSettings set(file, QSettings::IniFormat);
	set.beginGroup("playlist");

	TPlaylistItemList::iterator it;
	for (int n = 0; n < pl.count(); n++) {
		QString filename = pl[n].filename();

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
		set.setValue("Title" + QString::number(n + 1), pl[n].name());
		set.setValue("Length" + QString::number(n + 1), (int) pl[n].duration());
	}

	set.setValue("NumberOfEntries", pl.count());
	set.setValue("Version", 2);

	set.endGroup();

	set.sync();

	bool ok = set.status() == QSettings::NoError;
	if (ok) {
		modified = false;
	}

	return ok;
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
		this, tr("Choose a filename"),
		pref->latest_dir,
		tr("Playlists") + e.playlist().forFilter());

    if (s.isEmpty()) {
        return false;
    }

    // If filename has no extension, add it
    if (QFileInfo(s).suffix().isEmpty()) {
        s = s + ".m3u";
    }
    if (QFileInfo(s).exists()) {
        int res = QMessageBox::question(this,
            tr("Confirm overwrite?"),
            tr("The file %1 already exists.\n"
            "Do you want to overwrite?").arg(s),
            QMessageBox::Yes,
            QMessageBox::No,
            QMessageBox::NoButton);
        if (res == QMessageBox::No) {
            return false;
        }
    }
    pref->latest_dir = QFileInfo(s).absolutePath();
    if (QFileInfo(s).suffix().toLower() == "pls") {
        return saveIni(s);
    }
    return saveM3u(s);
}

bool TPlaylist::maybeSave() {

	if (!modified)
		return true;

	int res = QMessageBox::question(this,
        tr("Playlist modified"),
        tr("The playlist is modified, do you want to save it?"),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    switch (res) {
        case QMessageBox::No: modified = false; return true;
        case QMessageBox::Cancel: return false; // Cancel operation
        default: return save();
    }
}

void TPlaylist::saveSettings() {
	qDebug("Gui::TPlaylist::saveSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("playlist");

	set->setValue("recursive_add_directory", recursive_add_directory);
	set->setValue("save_playlist_in_config", save_playlist_in_config);
	set->setValue("repeat", repeatAct->isChecked());
	set->setValue("shuffle", shuffleAct->isChecked());

	set->endGroup();

	if (save_playlist_in_config) {
		//Save current list
		set->beginGroup("playlist_contents");

		set->setValue("count", (int) pl.count());
		for (int n=0; n < pl.count(); n++) {
			set->setValue(QString("item_%1_filename").arg(n), pl[n].filename());
			set->setValue(QString("item_%1_duration").arg(n), pl[n].duration());
			set->setValue(QString("item_%1_name").arg(n), pl[n].name());
		}
		set->setValue("current_item", current_item);
		set->setValue("modified", modified);

		set->endGroup();
	}
}

void TPlaylist::loadSettings() {
	qDebug("Gui::TPlaylist::loadSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("playlist");

	recursive_add_directory = set->value("recursive_add_directory", recursive_add_directory).toBool();
	save_playlist_in_config = set->value("save_playlist_in_config", save_playlist_in_config).toBool();
	repeatAct->setChecked(set->value("repeat", repeatAct->isChecked()).toBool());
	shuffleAct->setChecked(set->value("shuffle", shuffleAct->isChecked()).toBool());

	set->endGroup();

	if (save_playlist_in_config) {
		// Load latest list
		playlist_path = "";
		set->beginGroup("playlist_contents");

		int count = set->value("count", 0).toInt();
		QString filename, name;
		double duration;
		for (int n = 0; n < count; n++) {
			filename = set->value(QString("item_%1_filename").arg(n), "").toString();
			duration = set->value(QString("item_%1_duration").arg(n), -1).toDouble();
			name = set->value(QString("item_%1_name").arg(n), "").toString();
			cleanAndAddItem(filename, name, duration);
		}
		modified = set->value("modified", false).toBool();
        updateView(set->value("current_item", 0).toInt());

		set->endGroup();
	}
}

} // namespace Gui

#include "moc_playlist.cpp"

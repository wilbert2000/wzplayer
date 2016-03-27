/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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
#include <QDateTime>
#include <QSettings>
#include <QInputDialog>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QTextCodec>
#include <QApplication>
#include <QMimeData>
#include <QClipboard>

#include "version.h"
#include "helper.h"
#include "images.h"
#include "core.h"
#include "extensions.h"
#include "filedialog.h"
#include "settings/preferences.h"
#include "gui/action/action.h"
#include "gui/action/menu.h"
#include "gui/tablewidget.h"
#include "gui/multilineinputdialog.h"


using namespace Settings;

namespace Gui {

using namespace Action;


TPlaylistItem::TPlaylistItem()
	: _duration(0)
	, _played(false)
	, _deleted(false)
	, _edited(false) {
}

TPlaylistItem::TPlaylistItem(const QString &filename, const QString &name,
							 double duration)
	: _filename(filename)
	, _name(name)
	, _duration(duration)
	, _played(false)
	, _deleted(false)
	, _edited(false) {

	_directory = QFileInfo(filename).absolutePath();
}

TPlaylist::TPlaylist(QWidget* parent, TCore* c)
	: QWidget(parent, 0)
	, current_item(-1)
	, core(c)
	, recursive_add_directory(true)
	, save_playlist_in_config(true)
	, play_files_from_start(true)
	, modified(false) {

	createTable();
	createActions(parent);
	createToolbar();

	connect(core, SIGNAL(newMediaStartedPlaying()),
			this, SLOT(onNewMediaStartedPlaying()));
	connect(core, SIGNAL(mediaLoaded()),
			this, SLOT(onMediaLoaded()));
	connect(core, SIGNAL(titleTrackChanged(int)),
			this, SLOT(onTitleTrackChanged(int)));
	connect(core, SIGNAL(mediaEOF()),
			this, SLOT(onMediaEOF()), Qt::QueuedConnection);
	connect(core, SIGNAL(noFileToPlay()),
			this, SLOT(resumePlay()));

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
	listView->setShowGrid(false);
	listView->setSortingEnabled(false);
	//listView->setAlternatingRowColors(true);

#if QT_VERSION >= 0x050000
	listView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	listView->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
#else
	listView->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
	listView->horizontalHeader()->setResizeMode(COL_NAME, QHeaderView::Stretch);
#endif
	/*
	listView->horizontalHeader()->setResizeMode(COL_TIME, QHeaderView::ResizeToContents);
	listView->horizontalHeader()->setResizeMode(COL_PLAY, QHeaderView::ResizeToContents);
	*/
	listView->setIconSize(Images::icon("ok").size());

	// TODO: enable
	if (0) {
		listView->setSelectionMode(QAbstractItemView::SingleSelection);
		listView->setDragEnabled(true);
		listView->setAcceptDrops(true);
		listView->setDropIndicatorShown(true);
		listView->setDragDropMode(QAbstractItemView::InternalMove);
	}

	connect(listView, SIGNAL(cellActivated(int,int)),
			 this, SLOT(onCellActivated(int, int)));

	// EDIT BY NEO -->
	connect(listView->horizontalHeader(), SIGNAL(sectionClicked(int)),
			this, SLOT(sortBy(int)));
	// <--
}

void TPlaylist::createActions(QWidget* parent) {

	openAct = new TAction(this, "pl_open", QT_TR_NOOP("&Load"), "open");
	connect(openAct, SIGNAL(triggered()), this, SLOT(load()));

	saveAct = new TAction(this, "pl_save", QT_TR_NOOP("&Save"), "save");
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

	playAct = new TAction(this, "pl_play", QT_TR_NOOP("&Play"), "play");
	connect(playAct, SIGNAL(triggered()), this, SLOT(playCurrent()));

	nextAct = new TAction(this, "pl_next", QT_TR_NOOP("&Next"), "next", Qt::Key_N);
	connect(nextAct, SIGNAL(triggered()), this, SLOT(playNext()));

	prevAct = new TAction(this, "pl_prev", QT_TR_NOOP("Pre&vious"), "previous", Qt::Key_P);
	connect(prevAct, SIGNAL(triggered()), this, SLOT(playPrev()));

	moveUpAct = new TAction(this, "pl_move_up", QT_TR_NOOP("Move &up"), "up");
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(moveItemUp()));

	moveDownAct = new TAction(this, "pl_move_down", QT_TR_NOOP("Move &down"), "down");
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(moveItemDown()));

	repeatAct = new TAction(this, "pl_repeat", QT_TR_NOOP("&Repeat"), "repeat");
	repeatAct->setCheckable(true);

	shuffleAct = new TAction(this, "pl_shuffle", QT_TR_NOOP("S&huffle"), "shuffle");
	shuffleAct->setCheckable(true);

	// Add actions
	addCurrentAct = new TAction(this, "pl_add_current", QT_TR_NOOP("Add &current file"), "noicon");
	connect(addCurrentAct, SIGNAL(triggered()), this, SLOT(addCurrentFile()));

	addFilesAct = new TAction(this, "pl_add_files", QT_TR_NOOP("Add &file(s)"), "noicon");
	connect(addFilesAct, SIGNAL(triggered()), this, SLOT(addFiles()));

	addDirectoryAct = new TAction(this, "pl_add_directory", QT_TR_NOOP("Add &directory"), "noicon");
	connect(addDirectoryAct, SIGNAL(triggered()), this, SLOT(addDirectory()));

	addUrlsAct = new TAction(this, "pl_add_urls", QT_TR_NOOP("Add &URL(s)"), "noicon");
	connect(addUrlsAct, SIGNAL(triggered()), this, SLOT(addUrls()));

	// Remove actions
	removeSelectedAct = new TAction(this, "pl_remove_selected", QT_TR_NOOP("&Remove from list"), "noicon", Qt::Key_Delete);
	connect(removeSelectedAct, SIGNAL(triggered()), this, SLOT(removeSelected()));

	removeSelectedFromDiskAct = new TAction(this, "pl_delete_from_disk", QT_TR_NOOP("&Delete from disk..."), "noicon");
	connect(removeSelectedFromDiskAct, SIGNAL(triggered()), this, SLOT(removeSelectedFromDisk()));

	removeAllAct = new TAction(this, "pl_remove_all", QT_TR_NOOP("&Clear playlist"), "noicon");
	connect(removeAllAct, SIGNAL(triggered()), this, SLOT(removeAll()));

	// Copy
	copyAct = new TAction(this, "pl_copy", QT_TR_NOOP("&Copy"), "noicon", QKeySequence("Ctrl+C"));
	connect(copyAct, SIGNAL(triggered()), this, SLOT(copySelected()));

	// Edit
	editAct = new TAction(this, "pl_edit", QT_TR_NOOP("&Edit"), "noicon");
	connect(editAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));

	// Add actions to parent
	parent->addActions(actions());
}

void TPlaylist::createToolbar() {

	toolbar = new QToolBar(this);

	toolbar->addAction(openAct);
	toolbar->addAction(saveAct);;

	toolbar->addSeparator();
	toolbar->addAction(moveUpAct);
	toolbar->addAction(moveDownAct);

	add_menu = new QMenu(this);
	add_menu->addAction(addCurrentAct);
	add_menu->addAction(addFilesAct);
	add_menu->addAction(addDirectoryAct);
	add_menu->addAction(addUrlsAct);

	add_button = new QToolButton(this);
	add_button->setMenu(add_menu);
	add_button->setPopupMode(QToolButton::InstantPopup);

	remove_menu = new QMenu(this);
	remove_menu->addAction(removeSelectedAct);
	remove_menu->addAction(removeSelectedFromDiskAct);
	remove_menu->addAction(removeAllAct);

	remove_button = new QToolButton(this);
	remove_button->setMenu(remove_menu);
	remove_button->setPopupMode(QToolButton::InstantPopup);

	toolbar->addWidget(add_button);
	toolbar->addWidget(remove_button);

	toolbar->addSeparator();
	toolbar->addAction(playAct);
	toolbar->addAction(prevAct);
	toolbar->addAction(nextAct);
	toolbar->addSeparator();
	toolbar->addAction(repeatAct);
	toolbar->addAction(shuffleAct);

	// Popup menu
	popup = new QMenu(this);
	popup->addAction(playAct);
	popup->addSeparator();
	popup->addAction(copyAct);
	popup->addAction(editAct);
	popup->addSeparator();
	popup->addAction(removeSelectedAct);
	popup->addAction(removeSelectedFromDiskAct);

	connect(listView, SIGNAL(customContextMenuRequested(const QPoint &)),
			this, SLOT(showContextMenu(const QPoint &)));
}

void TPlaylist::retranslateStrings() {
	//qDebug("Gui::TPlaylist::retranslateStrings");

	// Icon
	setWindowIcon(Images::icon("logo", 64));
	setWindowTitle(tr("SMPlayer - Playlist"));

	listView->setHorizontalHeaderLabels(QStringList()
										<< "   "
										<< tr("Name")
										<< tr("Length"));

	// Tool buttons
	add_button->setIcon(Images::icon("plus"));
	add_button->setToolTip(tr("Add..."));
	remove_button->setIcon(Images::icon("minus"));
	remove_button->setToolTip(tr("Remove..."));
}

void TPlaylist::getFilesAppend(QStringList& files) const {

	TPlaylistItemList::const_iterator i;
	for (i = pl.constBegin(); i != pl.constEnd(); i++) {
		files.append((*i).filename());
	}
}

void TPlaylist::updateView() {
	qDebug("Gui::TPlaylist::updateView");

	listView->setRowCount(pl.count());

	for (int i = 0; i < pl.count(); i++) {
		TPlaylistItem& playlist_item = pl[i];

		// Icon
		if (i == current_item) {
			listView->setIcon(i, COL_PLAY, Images::icon("play"));
		} else if (playlist_item.played()) {
			listView->setIcon(i, COL_PLAY, Images::icon("ok"));
		} else {
			listView->setIcon(i, COL_PLAY, QPixmap());
		}

		// Name
		QString name = playlist_item.name();
		if (name.isEmpty())
			name = playlist_item.filename();
		listView->setText(i, COL_NAME, name);

		// Set tooltip to filename
		QTableWidgetItem* table_item = listView->item(i, COL_NAME);
		if (table_item) {
			table_item->setToolTip(playlist_item.filename());
		}

		// Duration
		listView->setText(i, COL_TIME,
			Helper::formatTime(qRound(playlist_item.duration())));
	}

	listView->resizeColumnToContents(COL_PLAY);
	listView->resizeColumnToContents(COL_TIME);
}

void TPlaylist::setCurrentItem(int current) {
	qDebug("Gui::TPlaylist::setCurrentItem: from %d to %d", current_item, current);

	// Give old current_item an icon
	if (current_item >= 0
		&& current_item != current
		&& current_item < pl.count()
		&& current_item < listView->rowCount()) {
		if (pl[current_item].played()) {
			// Set ok icon
			qDebug() << "Gui::TPlaylist::setCurrentItem: setting ok icon for" << current_item;
			listView->setIcon(current_item, COL_PLAY, Images::icon("ok"));
		} else {
			qDebug() << "Gui::TPlaylist::setCurrentItem: clearing icon for" << current_item;
			listView->setIcon(current_item, COL_PLAY, QPixmap());
		}
	}

	listView->clearSelection();
	current_item = current;

	if (current_item >= 0 && current_item < listView->rowCount()) {
		qDebug() << "Gui::TPlaylist::setCurrentItem: setting play icon for" << current_item;
		listView->setIcon(current_item, COL_PLAY, Images::icon("play"));
		listView->setCurrentCell(current_item, 0);
	}
}

void TPlaylist::clear() {

	pl.clear();
	listView->clearContents();
	listView->setRowCount(0);
	current_item = -1;
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
		updateView();
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

		latest_dir = fi.absolutePath();
	} else {
		QString name;
		bool ok;
		TDiscData disc = TDiscName::split(filename, &ok);
		if (ok) {
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
		}
		++it;
	}

	latest_dir = dir;
}

void TPlaylist::addDirectory() {

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					lastDir());

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

	setCursor(Qt::WaitCursor);

	QStringList::ConstIterator it = files.constBegin();
	while(it != files.constEnd()) {
		addFileOrDir(*it);
		++it;
	}

	updateView();
	unsetCursor();

	qDebug("Gui::TPlaylist::addFiles: done");
}

void TPlaylist::addFiles() {

	TExtensions e;
	QStringList files = MyFileDialog::getOpenFileNames(this,
		tr("Select one or more files to open"), lastDir(),
		tr("Multimedia") + e.multimedia().forFilter() + ";;" +
			tr("All files") +" (*.*)");

	if (files.count() > 0)
		addFiles(files);
}

void TPlaylist::addUrls() {

	TMultilineInputDialog d(this);
	if (d.exec() == QDialog::Accepted) {
		playlist_path = lastDir();
		QStringList urls = d.lines();
		foreach(QString u, urls) {
			if (!u.isEmpty())
				cleanAndAddItem(u, "", 0);
		}
		updateView();
	}
}

void TPlaylist::playCurrent() {

	int current = listView->currentRow();
	if (current >= 0) {
		playItem(current);
	}
}

int TPlaylist::chooseRandomItem() {

	// Create list of items not yet played
	QList <int> fi;
	for (int n = 0; n < pl.count(); n++) {
		if (!pl[n].played())
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

	if (shuffleAct->isChecked())
		playItem(chooseRandomItem());
	else
		playItem(0);
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
		item.setPlayed(true);
		setCurrentItem(n);
		if (play_files_from_start)
			core->open(item.filename(), 0);
		else core->open(item.filename());
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

void TPlaylist::resumePlay() {

	if (pl.count() > 0) {
		if (current_item < 0 || current_item >= pl.count()) {
			current_item = 0;
		}
		playItem(current_item);
	}
}

void TPlaylist::playDirectory(const QString &dir) {
	qDebug("Gui::TPlaylist::playDirectory");

	clear();
	addDirectory(dir);
	sort();
	// sort() can set modified
	modified = false;
	startPlay();
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

	listView->clearSelection();
	updateView();
	if (current_item >= 0) {
		listView->setCurrentCell(current_item, 0);
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
		Action::execPopup(this, popup, listView->viewport()->mapToGlobal(pos));
	}
}

void TPlaylist::onCellActivated(int row, int) {
	qDebug("Gui::TPlaylist::onCellActivated: row: %d", row);
	playItem(row);
}

void TPlaylist::onNewMediaStartedPlaying() {

	if (!pref->auto_add_to_playlist) {
		qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: add to playlist disabled by user");
		return;
	}

	QString filename = core->mdat.filename;
	QString current_filename;
	if (current_item >= 0 && current_item < pl.count()) {
		current_filename = pl[current_item].filename();
	}
	if (filename == current_filename) {
		qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: new file is current item");
		return;
	}

	Maps::TTitleTracks* titles = &core->mdat.titles;
	bool is_disc;
	TDiscData disc = TDiscName::split(filename, &is_disc);
	if (is_disc && titles->count() == count()) {
		bool cur_is_disc;
		TDiscData cur_disc = TDiscName::split(current_filename, &cur_is_disc);
		if (cur_is_disc && cur_disc.protocol == disc.protocol
			&& cur_disc.device == disc.device) {
			qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: new file is from current disc");
			return;
		}
	}

	// Create new playlist
	clear();

	if (is_disc) {
		// Add disc titles
		Maps::TTitleTracks::TMapIterator i = titles->getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTitleData title = i.value();
			disc.title = title.getID();
			addItem(TDiscName::join(disc), title.getDisplayName(false),
					title.getDuration());
			if (title.getID() == titles->getSelectedID()) {
				setCurrentItem(title.getID() - titles->firstID());
			}
		}
	} else {
		// Add current file
		addItem(filename, core->mdat.displayName(), core->mdat.duration);
		setCurrentItem(0);

		// Add associated files to playlist
		if (core->mdat.selected_type == TMediaData::TYPE_FILE) {
			qDebug() << "Gui::TPlaylist::onNewMediaStartedPlaying: searching for files to add to playlist for"
					 << filename;
			QStringList files_to_add = Helper::filesForPlaylist(filename, pref->media_to_add_to_playlist);
			if (files_to_add.isEmpty()) {
				qDebug("Gui::TPlaylist::onNewMediaStartedPlaying: none found");
			} else {
				addFiles(files_to_add);
			}
		}
	}

	// Mark current item as played
	if (current_item >= 0 && current_item < pl.count()) {
		pl[current_item].setPlayed(true);
	}
	updateView();

	qDebug() << "Gui::TPlaylist::onNewMediaStartedPlaying: created new playlist with" << count()
			 << "items for" << filename;
}

void TPlaylist::getMediaInfo() {
	qDebug("Gui::TPlaylist::getMediaInfo");

	QString filename = core->mdat.filename;
	QString title = core->mdat.displayName();
	double duration = core->mdat.duration;

	for (int n = 0; n < pl.count(); n++) {
		TPlaylistItem& item = pl[n];
		if (item.filename() == filename) {
			item.setPlayed(true);
			if (!item.edited()) {
				item.setName(title);
			}
			if (duration > 0) {
				item.setDuration(duration);
			}
			// Playlist can contain double items, so continue
		}
	}

	updateView();
}

void TPlaylist::onMediaLoaded() {
	qDebug("Gui::TPlaylist::onMediaLoaded");
	getMediaInfo();
}

void TPlaylist::onMediaEOF() {
	qDebug("Gui::Tplaylist::onMediaEOF");
	playNext();
}

void TPlaylist::onTitleTrackChanged(int id) {
	qDebug("Gui::TPlaylist::onTitleTrackChanged: %d", id);

	// Search for title on file name instead of using id as index,
	// because user can change order of the playlist.
	TDiscData disc = TDiscName::split(core->mdat.filename);
	disc.title = id;
	QString filename = TDiscName::join(disc);

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

void TPlaylist::moveItemUp() {

	for(int row = 1; row < listView->rowCount(); row++) {
		if (listView->isSelected(row, 0)) {
			swapItems(row, row - 1);
		}
	}
	updateView();
}

void TPlaylist::moveItemDown() {

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

	if (copied > 0) {
		if (copied == 1) {
			// Remove trailing new line
			text = text.left(text.length() - 1);
			emit displayMessage(tr("Copied %1").arg(text), TConfig::MESSAGE_DURATION);
		} else {
			emit displayMessage(tr("Copied %1 file names").arg(copied), TConfig::MESSAGE_DURATION);
		}
		QApplication::clipboard()->setText(text);
	}
}

void TPlaylist::editCurrentItem() {

	int current = listView->currentRow();
	if (current >= 0)
		editItem(current);
}

void TPlaylist::editItem(int item) {

	QString current_name = pl[item].name();
	if (current_name.isEmpty())
		current_name = pl[item].filename();

	bool ok;
	QString text = QInputDialog::getText(this,
		tr("Edit name"),
		tr("Type the name that will be displayed in the playlist for this file:"),
		QLineEdit::Normal,
		current_name, &ok);
	if (ok && !text.isEmpty()) {
		// user entered something and pressed OK
		pl[item].setEdited(true);
		pl[item].setName(text);
		modified = true;
		updateView();
	}
}

void TPlaylist::sort() {
	sortBy(COL_NAME, false, false, 0);
}

void TPlaylist::sortBy(int section) {
	sortBy(section, true, false, 0);
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
		QList <QUrl> urls = e->mimeData()->urls();
		for (int n = 0; n < urls.count(); n++) {
			QUrl url = urls[n];
			if (url.isValid()) {
				QString filename;
				if (url.scheme() == "file")
					filename = url.toLocalFile();
				else filename = url.toString();
				qDebug() << "Gui::TPlaylist::dropEvent: adding" << filename;
				files.append(filename);
			} else {
				qWarning() << "Gui::TPlaylist::dropEvent:: ignoring" << url.toString();
			}
		}
	}

#ifdef Q_OS_WIN
	files = Helper::resolveSymlinks(files); // Check for Windows shortcuts
#endif
	files.sort();

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
		   << "# Playlist created by SMPlayer " << Version::printable()
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
			lastDir(),
			tr("Playlists") + e.playlist().forFilter());

		if (!s.isEmpty()) {
			latest_dir = QFileInfo(s).absolutePath();
			if (QFileInfo(s).suffix().toLower() == "pls")
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
		lastDir(),
		tr("Playlists") + e.playlist().forFilter());

	if (!s.isEmpty()) {
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
		latest_dir = QFileInfo(s).absolutePath();
		if (QFileInfo(s).suffix().toLower() == "pls")
			return saveIni(s);
		else
			return saveM3u(s);
	} else {
		return false;
	}
}

bool TPlaylist::maybeSave() {

	if (!modified)
		return true;

	int res = QMessageBox::question(this,
				tr("Playlist modified"),
				tr("There are unsaved changes, do you want to save the playlist?"),
				QMessageBox::Yes,
				QMessageBox::No,
				QMessageBox::Cancel);

	switch (res) {
		case QMessageBox::No : return true; // Discard changes
		case QMessageBox::Cancel : return false; // Cancel operation
		default : return save();
	}
}

void TPlaylist::saveSettings() {
	qDebug("Gui::TPlaylist::saveSettings");

	QSettings* set = Settings::pref;

	set->beginGroup("directories");
	bool save_dirs = set->value("save_dirs", false).toBool();
	set->endGroup();

	set->beginGroup("playlist");

	set->setValue("repeat", repeatAct->isChecked());
	set->setValue("shuffle", shuffleAct->isChecked());

	set->setValue("recursive_add_directory", recursive_add_directory);
	set->setValue("save_playlist_in_config", save_playlist_in_config);
	set->setValue("play_files_from_start", play_files_from_start);

	if (save_dirs) {
		set->setValue("latest_dir", latest_dir);
	} else {
		set->setValue("latest_dir", "");
	}

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

	repeatAct->setChecked(set->value("repeat", repeatAct->isChecked()).toBool());
	shuffleAct->setChecked(set->value("shuffle", shuffleAct->isChecked()).toBool());

	recursive_add_directory = set->value("recursive_add_directory", recursive_add_directory).toBool();
	save_playlist_in_config = set->value("save_playlist_in_config", save_playlist_in_config).toBool();
	play_files_from_start = set->value("play_files_from_start", play_files_from_start).toBool();

	latest_dir = set->value("latest_dir", latest_dir).toString();

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
		updateView();
		setCurrentItem(set->value("current_item", 0).toInt());

		set->endGroup();
	}
}

QString TPlaylist::lastDir() {

	QString last_dir = latest_dir;
	if (last_dir.isEmpty())
		last_dir = pref->latest_dir;

	return last_dir;
}

} // namespace Gui

#include "moc_playlist.cpp"

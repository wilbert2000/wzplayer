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
	, automatically_play_next(true)
	, row_spacing(-1) // Default height
	, modified(false) {

	createTable();
	createActions(parent);
	createToolbar();

	connect(core, SIGNAL(newMediaStartedPlaying()),
			this, SLOT(onNewMediaStartedPlaying()));
	connect(core, SIGNAL(mediaLoaded()),
			this, SLOT(onMediaLoaded()));
	connect(core, SIGNAL(titleTrackChanged(int)),
			this, SLOT(playerSwitchedTitle(int)));
	connect(core, SIGNAL(mediaEOF()),
			this, SLOT(mediaEOF()), Qt::QueuedConnection);
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
	srand(t.hour() * 3600 + t.minute() * 60 + t.second());
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
             this, SLOT(itemDoubleClicked(int)));

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
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(upItem()));

	moveDownAct = new TAction(this, "pl_move_down", QT_TR_NOOP("Move &down"), "down");
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(downItem()));

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
	removeSelectedAct = new TAction(this, "pl_remove_selected", QT_TR_NOOP("Remove &selected"), "noicon");
	connect(removeSelectedAct, SIGNAL(triggered()), this, SLOT(removeSelected()));

	removeAllAct = new TAction(this, "pl_remove_all", QT_TR_NOOP("Remove &all"), "noicon");
	connect(removeAllAct, SIGNAL(triggered()), this, SLOT(removeAll()));

	// Copy
	copyAct = new TAction(this, "pl_copy", QT_TR_NOOP("&Copy"), "noicon", QKeySequence("Ctrl+C"));
	connect(copyAct, SIGNAL(triggered()), this, SLOT(copyCurrentItem()));

	// Edit
	editAct = new TAction(this, "pl_edit", QT_TR_NOOP("&Edit"), "noicon");
	connect(editAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));

	deleteSelectedFileFromDiskAct = new TAction(this, "pl_delete_from_disk", QT_TR_NOOP("&Delete file from disk"), "noicon");
	connect(deleteSelectedFileFromDiskAct, SIGNAL(triggered()), this, SLOT(deleteSelectedFileFromDisk()));

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
	popup->addAction(deleteSelectedFileFromDiskAct);

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

void TPlaylist::appendFiles(QStringList& files) const {

	TPlaylistItemList::const_iterator i;
	for (i = pl.constBegin(); i != pl.constEnd(); i++) {
		files.append((*i).filename());
	}
}

void TPlaylist::updateView() {
	qDebug("Gui::TPlaylist::updateView");

	listView->clearSelection();
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

		// Row spacing
		if (row_spacing >= 0) {
			listView->setRowHeight(i, listView->font().pointSize() + row_spacing);
		}
	}

	listView->resizeColumnToContents(COL_PLAY);
	listView->resizeColumnToContents(COL_TIME);

	if (current_item >= 0) {
		listView->setCurrentCell(current_item, 0);
	}
}

void TPlaylist::setCurrentItem(int current) {
	qDebug("Gui::TPlaylist::setCurrentItem: from %d to %d", current_item, current);

	// TODO: reduce mem trashing

	// Give old current_item an icon
	if (current_item >= 0 && current_item < listView->rowCount()) {
		if (current_item < pl.count()) {
			if (pl[current_item].played()) {
				// Set ok icon unless current not changing
				if (current_item != current) {
					qDebug() << "Gui::TPlaylist::setCurrentItem: setting ok icon for" << current_item;
					listView->setIcon(current_item, COL_PLAY, Images::icon("ok"));
				}
			} else if (current_item != current) {
				qDebug() << "Gui::TPlaylist::setCurrentItem: clearing icon for" << current_item;
				listView->setIcon(current_item, COL_PLAY, QPixmap());
			}
		}
	}

	current_item = current;

	if (current_item >= 0) {
		if (current_item < listView->rowCount()) {
			qDebug() << "Gui::TPlaylist::setCurrentItem: setting play icon for" << current_item;
			listView->setIcon(current_item, COL_PLAY, Images::icon("play"));
		}

		listView->clearSelection();
		listView->setCurrentCell(current_item, 0);
	}
}

void TPlaylist::clear() {

	pl.clear();
	listView->clearContents();
	listView->setRowCount(0);
	setCurrentItem(-1);
	modified = false;
}

void TPlaylist::remove(int i) {

	if (i >= 0 && i < pl.count()) {
		pl.removeAt(i);
		if(current_item == i && i == (pl.count() - 1))
			setCurrentItem(i - 1);
		modified = true;
		updateView();
	}
}

int TPlaylist::count() const {
	return pl.count();
}

bool TPlaylist::isEmpty() const {
	return pl.isEmpty();
}

void TPlaylist::addItem(QString filename, QString name, double duration) {
	//qDebug() << "Gui::TPlaylist::addItem:" << filename;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	filename = Helper::changeSlashes(filename);
#endif

	if (name.isEmpty()) {
		QFileInfo fi(filename);
		// Let's see if it looks like a file (no dvd://1 or something)
		if (filename.indexOf(QRegExp("^.*://.*")) == -1) {
			// Local file
			name = fi.fileName(); //fi.baseName(true);
		} else {
			// Stream
			name = filename;
		}
	}
	pl.append(TPlaylistItem(filename, name, duration));
}

// EDIT BY NEO -->
void TPlaylist::sortBy(int section) {
	qDebug("Gui::TPlaylist::sortBy: section %d", section);

	sortBy(section, false, 0);
}

void TPlaylist::sortBy(int section, bool revert, int count) {
	// Bubble sort
	bool swaped = false;

	for (int n = 0; n < (pl.count() - count); n++) {

		int last = n - 1;
		int current = n;

		// Revert the sort
		if (revert) {
			last = n;
			current = n - 1;
		}

		if (n > 0) {
			int compare = 0;

			if (section == 0) {
				// Sort by played
				bool lastItem = pl[last].played();
				bool currentItem = pl[current].played();

				if (!lastItem && currentItem) {
					compare = 1;
				} else if (lastItem && currentItem) {
					if (last == current_item) {
						compare = 1;
					} else {
						compare = -1;
					}
				} else {
					compare = -1;
				}
			} else if (section == 1) {
				// Sort alphabetically on dir then filename
				TPlaylistItem& lastItem = pl[last];
				TPlaylistItem& currentItem = pl[current];
				compare = lastItem.directory().compare(currentItem.directory());
				if (compare == 0) {
					compare = lastItem.filename().compare(currentItem.filename());
				}
			} else if (section == 2) {
				// Sort by duration
				double lastItem = pl[last].duration();
				double currentItem = pl[current].duration();

				if (lastItem == currentItem) {
					compare = 0;
				} else if (lastItem > currentItem) {
					compare = 1;
				} else {
					compare = -1;
				}
			}

			// Swap items
			if (compare > 0) {
				swapItems(n, n - 1);

				if (current_item == (n - 1)) {
					current_item = n;
				} else if (current_item == n) {
					current_item = n - 1;
				}

				listView->clearSelection();
				listView->setCurrentCell(n - 1, 0);

				swaped = true;
			}
		}
	}

	if (count == 0 && !swaped && !revert) {
		// Revert sort
		sortBy(section, true, 0);
	} else if(swaped) {
		// Sort until there is nothing to sort
		sortBy(section, revert, ++count);
	} else {
		updateView();
	}
}
// <--

void TPlaylist::load_m3u(const QString &file, bool clear, bool play) {
	qDebug("Gui::TPlaylist::load_m3u");

	bool utf8 = (QFileInfo(file).suffix().toLower() == "m3u8");

	QRegExp info("^#EXTINF:(.*),(.*)");

	QFile f(file);
	if (f.open(QIODevice::ReadOnly)) {
		playlist_path = QFileInfo(file).path();

		if (clear)
			this->clear();
		QString filename="";
		QString name="";
		double duration=0;

		QTextStream stream(&f);

		if (utf8)
			stream.setCodec("UTF-8");
		else
			stream.setCodec(QTextCodec::codecForLocale());

		QString line;
		while (!stream.atEnd()) {
			line = stream.readLine().trimmed();
			if (line.isEmpty()) continue; // Ignore empty lines

			qDebug("Gui::TPlaylist::load_m3u: line: '%s'", line.toUtf8().data());
			if (info.indexIn(line)!=-1) {
				duration = info.cap(1).toDouble();
				name = info.cap(2);
				qDebug("Gui::TPlaylist::load_m3u: name: '%s', duration: %f", name.toUtf8().data(), duration);
			} else if (line.startsWith("#")) {
				// Comment
				// Ignore
			} else {
				filename = line;
				QFileInfo fi(filename);
				if (fi.exists()) {
					filename = fi.absoluteFilePath();
				} else if (QFileInfo(playlist_path + "/" + filename).exists()) {
					filename = playlist_path + "/" + filename;
				}
				addItem(filename, name, duration);
				name = "";
				duration = 0;
			}
		}

		f.close();
		updateView();

		if (play)
			startPlay();
	}
}

void TPlaylist::load_pls(const QString &file, bool clear, bool play) {
	qDebug("Gui::TPlaylist::load_pls");

	if (!QFile::exists(file)) {
		qDebug("Gui::TPlaylist::load_pls: '%s' doesn't exist, doing nothing", file.toUtf8().constData());
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

			QFileInfo fi(filename);
			if (fi.exists()) {
				filename = fi.absoluteFilePath();
			} else if (QFileInfo(playlist_path + "/" + filename).exists()) {
				filename = playlist_path + "/" + filename;
			}
			addItem(filename, name, duration);
		}
	}

	set.endGroup();

	updateView();

	if (play && (set.status() == QSettings::NoError))
		startPlay();
}

bool TPlaylist::save_m3u(QString file) {
	qDebug("Gui::TPlaylist::save_m3u: '%s'", file.toUtf8().data());

	QString dir_path = QFileInfo(file).path();
	if (!dir_path.endsWith("/")) dir_path += "/";

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	dir_path = Helper::changeSlashes(dir_path);
#endif

	qDebug(" * dirPath: '%s'", dir_path.toUtf8().data());

	bool utf8 = (QFileInfo(file).suffix().toLower() == "m3u8");

	QFile f(file);
	if (f.open(QIODevice::WriteOnly)) {
		QTextStream stream(&f);

		if (utf8) 
			stream.setCodec("UTF-8");
		else
			stream.setCodec(QTextCodec::codecForLocale());

		QString filename;

		stream << "#EXTM3U" << "\n";
		stream << "# Playlist created by SMPlayer " << Version::printable() << " \n";

		TPlaylistItemList::iterator it;
		for (it = pl.begin(); it != pl.end(); ++it) {
			filename = (*it).filename();
			#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
			filename = Helper::changeSlashes(filename);
			#endif
			stream << "#EXTINF:";
			stream << (*it).duration() << ",";
			stream << (*it).name() << "\n";
			// Try to save the filename as relative instead of absolute
			if (filename.startsWith(dir_path)) {
				filename = filename.mid(dir_path.length());
			}
			stream << filename << "\n";
		}
		f.close();

		modified = false;
		return true;
	} else {
		return false;
	}
}


bool TPlaylist::save_pls(QString file) {
	qDebug("Gui::TPlaylist::save_pls: '%s'", file.toUtf8().data());

	QString dir_path = QFileInfo(file).path();
	if (!dir_path.endsWith("/")) dir_path += "/";

	#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	dir_path = Helper::changeSlashes(dir_path);
	#endif

	qDebug(" * dirPath: '%s'", dir_path.toUtf8().data());

	QSettings set(file, QSettings::IniFormat);
	set.beginGroup("playlist");
	
	QString filename;

	TPlaylistItemList::iterator it;
	for (int n=0; n < pl.count(); n++) {
		filename = pl[n].filename();
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		filename = Helper::changeSlashes(filename);
#endif

		// Try to save the filename as relative instead of absolute
		if (filename.startsWith(dir_path)) {
			filename = filename.mid(dir_path.length());
		}

		set.setValue("File"+QString::number(n+1), filename);
		set.setValue("Title"+QString::number(n+1), pl[n].name());
		set.setValue("Length"+QString::number(n+1), (int) pl[n].duration());
	}

	set.setValue("NumberOfEntries", pl.count());
	set.setValue("Version", 2);

	set.endGroup();

	set.sync();

	bool ok = (set.status() == QSettings::NoError);
	if (ok)
		modified = false;

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
				load_pls(s);
			else
				load_m3u(s);
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
			return save_pls(s);
		else
			return save_m3u(s);
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

void TPlaylist::playCurrent() {

	int current = listView->currentRow();
	if (current >= 0) {
		playItem(current);
	}
}

void TPlaylist::itemDoubleClicked(int row) {
	qDebug("Gui::TPlaylist::itemDoubleClicked: row: %d", row);
	playItem(row);
}

void TPlaylist::showContextMenu(const QPoint & pos) {
	qDebug("Gui::TPlaylist::showContextMenu: x: %d y: %d", pos.x(), pos.y());

	if (!popup->isVisible()) {
		Action::execPopup(this, popup, listView->viewport()->mapToGlobal(pos));
	}
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
		setCurrentItem(n);
		item.setPlayed(true);
		if (play_files_from_start)
			core->open(item.filename(), 0);
		else core->open(item.filename());
	}
}

void TPlaylist::playNext() {
	qDebug("Gui::TPlaylist::playNext");

	if (shuffleAct->isChecked()) {
		// Shuffle
		int chosen_item = chooseRandomItem();
		qDebug("Gui::TPlaylist::playNext: chosen_item: %d", chosen_item);
		if (chosen_item == -1) {
			clearPlayedTag();
			if (repeatAct->isChecked()) {
				chosen_item = chooseRandomItem();
				if (chosen_item == -1) chosen_item = 0;
			}
		}
		playItem(chosen_item);
	} else {
		bool finished_list = (current_item + 1 >= pl.count());
		if (finished_list)
			clearPlayedTag();

		if (finished_list && repeatAct->isChecked()) {
			playItem(0);
		} else {
			playItem(current_item + 1);
		}
	}
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
		if (current_item < 0)
			current_item = 0;
		playItem(current_item);
	}
}

void TPlaylist::playDirectory(const QString &dir) {
	qDebug("Gui::TPlaylist::playDirectory");

	clear();
	addDirectory(dir);
	sortBy(1);
	// sortBy() can change current_item and modified
	setCurrentItem(0);
	modified = false;
	latest_dir = dir;
	startPlay();
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
		// Add current file. onMediaLoaded will fill in name and duration.
		addItem(filename, "", 0);
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

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	filename = Helper::changeSlashes(filename);
#endif

	QString title = core->mdat.displayName();
	QString artist = core->mdat.meta_data.value("ARTIST");
	if (!artist.isEmpty())
		title = artist + " - " + title;

	double duration = core->mdat.duration;
	bool need_update = false;

	for (int n = 0; n < pl.count(); n++) {
		TPlaylistItem& item = pl[n];
		if (item.filename() == filename) {
			// TODO: better write protection...
			// Protect titles loaded from an external playlist
			// by only updating items with duration 0
			if (item.duration() == 0) {
				if (!item.edited()) {
					item.setName(title);
				}
				item.setDuration(duration);
				need_update = true;
			}
		}
	}

	if (need_update)
		updateView();
}

void TPlaylist::onMediaLoaded() {
	qDebug("Gui::TPlaylist::onMediaLoaded");
	getMediaInfo();
}

void TPlaylist::mediaEOF() {
	qDebug("Gui::Tplaylist::mediaEOF");

	if (automatically_play_next) {
		playNext();
	}
}

void TPlaylist::playerSwitchedTitle(int id) {
	qDebug("Gui::TPlaylist::playerSwitchedTitle: %d", id);

	// Search for title on file name instead of using id as index,
	// because user can change order of the playlist.
	TDiscData disc = TDiscName::split(core->mdat.filename);
	disc.title = id;
	QString filename = TDiscName::join(disc);

	for(int i = 0; i < pl.count(); i++) {
		if (pl[i].filename() == filename) {
			setCurrentItem(i);
			pl[i].setPlayed(true);
			return;
		}
	}

	qWarning() << "Gui::TPlaylist::playerSwitchedTitle: title id" << id
			   << "filename" << filename << "not found in playlist";
}

// Add current file to playlist
void TPlaylist::addCurrentFile() {
	qDebug("Gui::TPlaylist::addCurrentFile");

	if (!core->mdat.filename.isEmpty()) {
		addItem(core->mdat.filename, "", 0);
		getMediaInfo();
	}
}

void TPlaylist::addFile(const QString &filename) {
	qDebug() << "Gui::TPlaylist::addFile:" << filename;
	// Note: currently addFile loads playlists and addDirectory skips them,
	// giving a nice balance. Load if the individual file is requested,
	// skip when adding a directory.

	if (QFile::exists(filename)) {
		QFileInfo fi(filename);
		QString ext = fi.suffix().toLower();
		if (ext == "m3u" || ext == "m3u8") {
			load_m3u(filename, false, false);
		} else if (ext == "pls") {
			load_pls(filename, false, false);
		} /* else if (get_info) {
			// TODO: currently get_info is always false
			// Move it to a seperate thread before enabling
			TMediaData media_data;
			TInfoProvider::getInfo(filename, media_data);
			addItem(filename, media_data.displayName(), media_data.duration);
		} */ else {
			addItem(filename, fi.fileName(), 0);
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
		}
		addItem(filename, name, 0);
	}
}

void TPlaylist::addDirectory(const QString &dir) {
	qDebug() << "Gui::TPlaylist::addDirectory:" << dir;

	static TExtensions ext;
	static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

	emit displayMessage(dir, 0);

	QStringList dir_list = QDir(dir).entryList();
	QStringList::Iterator it = dir_list.begin();
	while(it != dir_list.end()) {
		QString filename = *it;
		if (filename != "." && filename != "..") {
			if (dir.right(1) != "/")
				filename = dir + "/" + filename;
			else filename = dir + filename;
			QFileInfo fi(filename);
			if (fi.isDir()) {
				if (recursive_add_directory)
					addDirectory(filename);
			} else if (rx_ext.indexIn(fi.suffix()) >= 0) {
				addFile(filename);
			}
		}
		++it;
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

void TPlaylist::addDirectory() {

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					lastDir());

	if (!s.isEmpty()) {
		addDirectory(s);
		latest_dir = s;
	}
}

void TPlaylist::addUrls() {

	TMultilineInputDialog d(this);
	if (d.exec() == QDialog::Accepted) {
		QStringList urls = d.lines();
		foreach(QString u, urls) {
			if (!u.isEmpty()) addItem(u, "", 0);
		}
		updateView();
	}
}

// Remove selected items
void TPlaylist::removeSelected() {
	qDebug("Gui::TPlaylist::removeSelected");

	int first_selected = -1;
	int number_previous_item = 0;

	for (int n=0; n < listView->rowCount(); n++) {
		if (listView->isSelected(n, 0)) {
			qDebug(" row %d selected", n);
			pl[n].setMarkForDeletion(true);
			number_previous_item++;
			if (first_selected == -1) first_selected = n;
		}
	}

	TPlaylistItemList::iterator it;
	for (it = pl.begin(); it != pl.end(); ++it) {
		if ((*it).markedForDeletion()) {
			qDebug("Remove '%s'", (*it).filename().toUtf8().data());
			it = pl.erase(it);
			it--;
			modified = true;
		}
	}


	if (first_selected < current_item) {
		current_item -= number_previous_item;
	}

	if (isEmpty())
		modified = false;
	updateView();

	if (first_selected >= listView->rowCount()) 
		first_selected = listView->rowCount() - 1;

	if ((first_selected > -1) && (first_selected < listView->rowCount())) {
		listView->clearSelection();
		listView->setCurrentCell(first_selected, 0);
		//listView->selectRow(first_selected);
	}
}

void TPlaylist::removeAll() {
	clear();
}

void TPlaylist::clearPlayedTag() {

	for (int n = 0; n < pl.count(); n++) {
		pl[n].setPlayed(false);
	}
	updateView();
}

int TPlaylist::chooseRandomItem() {
	// qDebug("Gui::TPlaylist::chooseRandomItem");

	QList <int> fi; //List of not played items (free items)
	for (int n = 0; n < pl.count(); n++) {
		if (!pl[n].played())
			fi.append(n);
	}

	qDebug("Gui::TPlaylist::chooseRandomItem: free items: %d", fi.count());

	if (fi.count() == 0) return -1; // none free

	int selected = (int) ((double) fi.count() * rand()/(RAND_MAX+1.0));
	qDebug("Gui::TPlaylist::chooseRandomItem: selected item: %d",
		   fi[selected]);
	return fi[selected];
}

void TPlaylist::swapItems(int item1, int item2) {

	pl.swap(item1, item2);
	modified = true;
}


void TPlaylist::upItem() {
	qDebug("Gui::TPlaylist::upItem");

	int current = listView->currentRow();
	qDebug(" currentRow: %d", current);

	moveItemUp(current);

}

void TPlaylist::downItem() {
	qDebug("Gui::TPlaylist::downItem");

	int current = listView->currentRow();
	qDebug(" currentRow: %d", current);

	moveItemDown(current);
}

void TPlaylist::moveItemUp(int current){
	qDebug("Gui::TPlaylist::moveItemUp");

	if (current > 0) {
		swapItems(current, current - 1);
		if (current_item == current - 1)
			current_item = current;
		else if (current_item == current)
			current_item = current - 1;
		updateView();
		listView->clearSelection();
		listView->setCurrentCell(current - 1, 0);
	}
}

void TPlaylist::moveItemDown(int current	){
	qDebug("Gui::TPlaylist::moveItemDown");

	if ((current >= 0) && (current < pl.count() - 1)) {
		swapItems(current, current + 1);
		if (current_item == current + 1)
			current_item = current;
		else if (current_item == current)
			current_item = current + 1;
		updateView();
		listView->clearSelection();
		listView->setCurrentCell(current + 1, 0);
	}
}

void TPlaylist::copyCurrentItem() {

	int current = listView->currentRow();
	if (current >= 0) {
		QString filename = pl[current].filename();
		if (!filename.isEmpty()) {
			QApplication::clipboard()->setText(filename);
			emit displayMessage(tr("Copied %1").arg(filename), TConfig::MESSAGE_DURATION);
		}
	}
}

void TPlaylist::editCurrentItem() {
	int current = listView->currentRow();
	if (current > -1) editItem(current);
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

void TPlaylist::deleteSelectedFileFromDisk() {
	qDebug("Gui::TPlaylist::deleteSelectedFileFromDisk");

	int current = listView->currentRow();
	if (current > -1) {
		// If more that one row is selected, select only the current one
		listView->clearSelection();
		listView->setCurrentCell(current, 0);

		QString filename = pl[current].filename();
		qDebug() << "Gui::TPlaylist::deleteSelectedFileFromDisk: current file:" << filename;

		QFileInfo fi(filename);
		if (fi.exists() && fi.isFile() && fi.isWritable()) {
			// Ask the user for confirmation
			int res = QMessageBox::question(this, tr("Confirm deletion"),
						tr("You're about to DELETE the file '%1' from your drive.").arg(filename) + "<br>"+
						tr("This action cannot be undone. Are you sure you want to proceed?"),
						QMessageBox::Yes, QMessageBox::No);

			if (res == QMessageBox::Yes) {
				// Delete file
				bool success = QFile::remove(filename);
				//bool success = false;

				if (success) {
					// Remove item from the playlist
					removeSelected();
				} else {
					QMessageBox::warning(this, tr("Deletion failed"),
						tr("It wasn't possible to delete '%1'").arg(filename));
				}
			}
		} else {
			qDebug("Gui::TPlaylist::deleteSelectedFileFromDisk: file doesn't exists, it's not a file or it's not writable");
			QMessageBox::information(this, tr("Error deleting the file"),
				tr("It's not possible to delete '%1' from the filesystem.").arg(filename));
		}
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
	set->setValue("automatically_play_next", automatically_play_next);

	set->setValue("row_spacing", row_spacing);

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
	automatically_play_next = set->value("automatically_play_next", automatically_play_next).toBool();

	row_spacing = set->value("row_spacing", row_spacing).toInt();

	latest_dir = set->value("latest_dir", latest_dir).toString();

	set->endGroup();

	if (save_playlist_in_config) {
		// Load latest list
		set->beginGroup("playlist_contents");

		int count = set->value("count", 0).toInt();
		QString filename, name;
		double duration;
		for (int n=0; n < count; n++) {
			filename = set->value(QString("item_%1_filename").arg(n), "").toString();
			duration = set->value(QString("item_%1_duration").arg(n), -1).toDouble();
			name = set->value(QString("item_%1_name").arg(n), "").toString();
			addItem(filename, name, duration);
		}
		setCurrentItem(set->value("current_item", 0).toInt());
		modified = set->value("modified", false).toBool();
		updateView();

		set->endGroup();
	}
}

QString TPlaylist::lastDir() {
	QString last_dir = latest_dir;
	if (last_dir.isEmpty()) last_dir = pref->latest_dir;
	return last_dir;
}

} // namespace Gui

#include "moc_playlist.cpp"

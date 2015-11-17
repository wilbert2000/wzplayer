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
#include <QDebug>

#include "gui/action/action.h"
#include "gui/action/menus.h"
#include "gui/tablewidget.h"
#include "filedialog.h"
#include "helper.h"
#include "images.h"
#include "settings/preferences.h"
#include "gui/multilineinputdialog.h"
#include "version.h"
#include "core.h"
#include "extensions.h"
#include "gui/guiconfig.h"

#include <stdlib.h>

#if USE_INFOPROVIDER
#include "gui/infoprovider.h"
#endif

#define DRAG_ITEMS 0

#define COL_PLAY 0
#define COL_NAME 1
#define COL_TIME 2

using namespace Settings;

namespace Gui {

TPlaylistItem::TPlaylistItem() :
	_duration(0),
	_played(false),
	_deleted(false),
	_edited(false) {
}

TPlaylistItem::TPlaylistItem(const QString &filename, const QString &name,
							 double duration) :
	_filename(filename),
	_name(name),
	_duration(duration),
	_played(false),
	_deleted(false),
	_edited(false) {
	_directory = QFileInfo(filename).absolutePath();
}

TPlaylist::TPlaylist(QWidget* parent, TCore* c, Qt::WindowFlags f) :
	QWidget(parent, f),
	current_item(0),
	core(c),
	modified(false),
	recursive_add_directory(true),
	automatically_get_info(false),
	save_playlist_in_config(true),
	play_files_from_start(true),
	automatically_play_next(true),
	row_spacing(-1) // Default height
{

	createTable();
	createActions();
	createToolbar();

	connect(core, SIGNAL(newMediaStartedPlaying()),
			this, SLOT(newMediaLoaded()));
	connect(core, SIGNAL(mediaLoaded()),
			this, SLOT(getMediaInfo()));
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

	// Save config every 5 minutes.
	save_timer = new QTimer(this);
	connect(save_timer, SIGNAL(timeout()), this, SLOT(maybeSaveSettings()));
	save_timer->start(5 * 60000);
}

TPlaylist::~TPlaylist() {
}

void TPlaylist::setModified(bool mod) {
	//qDebug("Gui::TPlaylist::setModified: %d", mod);

	modified = mod;
	emit modifiedChanged(modified);
}

void TPlaylist::createTable() {
	listView = new TTableWidget(0, COL_TIME + 1, this);
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

#if DRAG_ITEMS
	listView->setSelectionMode(QAbstractItemView::SingleSelection);
	listView->setDragEnabled(true);
	listView->setAcceptDrops(true);
	listView->setDropIndicatorShown(true);
	listView->setDragDropMode(QAbstractItemView::InternalMove);
#endif

	connect(listView, SIGNAL(cellActivated(int,int)),
             this, SLOT(itemDoubleClicked(int)));

	// EDIT BY NEO -->
	connect(listView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortBy(int)));
	// <--
}

void TPlaylist::createActions() {

	openAct = new TAction(this, "pl_open", QT_TR_NOOP("&Load"), "open");
	connect(openAct, SIGNAL(triggered()), this, SLOT(load()));

	saveAct = new Gui::TAction(this, "pl_save", QT_TR_NOOP("&Save"), "save");
	connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

	playAct = new Gui::TAction(this, "pl_play", QT_TR_NOOP("&Play"), "play");
	connect(playAct, SIGNAL(triggered()), this, SLOT(playCurrent()));

	nextAct = new Gui::TAction(this, "pl_next", QT_TR_NOOP("&Next"), "next", Qt::Key_N);
	connect(nextAct, SIGNAL(triggered()), this, SLOT(playNext()));

	prevAct = new Gui::TAction(this, "pl_prev", QT_TR_NOOP("Pre&vious"), "previous", Qt::Key_P);
	connect(prevAct, SIGNAL(triggered()), this, SLOT(playPrev()));

	moveUpAct = new Gui::TAction(this, "pl_move_up", QT_TR_NOOP("Move &up"), "up");
	connect(moveUpAct, SIGNAL(triggered()), this, SLOT(upItem()));

	moveDownAct = new Gui::TAction(this, "pl_move_down", QT_TR_NOOP("Move &down"), "down");
	connect(moveDownAct, SIGNAL(triggered()), this, SLOT(downItem()));

	repeatAct = new Gui::TAction(this, "pl_repeat", QT_TR_NOOP("&Repeat"), "repeat");
	repeatAct->setCheckable(true);

	shuffleAct = new Gui::TAction(this, "pl_shuffle", QT_TR_NOOP("S&huffle"), "shuffle");
	shuffleAct->setCheckable(true);

	// Add actions
	addCurrentAct = new Gui::TAction(this, "pl_add_current", QT_TR_NOOP("Add &current file"), "noicon");
	connect(addCurrentAct, SIGNAL(triggered()), this, SLOT(addCurrentFile()));

	addFilesAct = new Gui::TAction(this, "pl_add_files", QT_TR_NOOP("Add &file(s)"), "noicon");
	connect(addFilesAct, SIGNAL(triggered()), this, SLOT(addFiles()));

	addDirectoryAct = new Gui::TAction(this, "pl_add_directory", QT_TR_NOOP("Add &directory"), "noicon");
	connect(addDirectoryAct, SIGNAL(triggered()), this, SLOT(addDirectory()));

	addUrlsAct = new Gui::TAction(this, "pl_add_urls", QT_TR_NOOP("Add &URL(s)"), "noicon");
	connect(addUrlsAct, SIGNAL(triggered()), this, SLOT(addUrls()));

	// Remove actions
	removeSelectedAct = new Gui::TAction(this, "pl_remove_selected", QT_TR_NOOP("Remove &selected"), "noicon");
	connect(removeSelectedAct, SIGNAL(triggered()), this, SLOT(removeSelected()));

	removeAllAct = new Gui::TAction(this, "pl_remove_all", QT_TR_NOOP("Remove &all"), "noicon");
	connect(removeAllAct, SIGNAL(triggered()), this, SLOT(removeAll()));

	// Edit
	editAct = new Gui::TAction(this, "pl_edit", QT_TR_NOOP("&Edit"), "noicon");
	connect(editAct, SIGNAL(triggered()), this, SLOT(editCurrentItem()));

	deleteSelectedFileFromDiskAct = new Gui::TAction(this, "pl_delete_from_disk", QT_TR_NOOP("&Delete file from disk"), "noicon");
	connect(deleteSelectedFileFromDiskAct, SIGNAL(triggered()), this, SLOT(deleteSelectedFileFromDisk()));
}

void TPlaylist::createToolbar() {
	toolbar = new QToolBar(this);
	toolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	toolbar->addAction(openAct);
	toolbar->addAction(saveAct);;
	toolbar->addSeparator();

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
	toolbar->addSeparator();
	toolbar->addAction(prevAct);
	toolbar->addAction(nextAct);
	toolbar->addSeparator();
	toolbar->addAction(repeatAct);
	toolbar->addAction(shuffleAct);
	toolbar->addSeparator();
	toolbar->addAction(moveUpAct);
	toolbar->addAction(moveDownAct);

	// Popup menu
	popup = new QMenu(this);
	popup->addAction(playAct);
	popup->addAction(removeSelectedAct);
	popup->addAction(editAct);
	popup->addAction(deleteSelectedFileFromDiskAct);

	connect(listView, SIGNAL(customContextMenuRequested(const QPoint &)),
			 this, SLOT(showContextMenu(const QPoint &)));
}

void TPlaylist::retranslateStrings() {
	//qDebug("Gui::TPlaylist::retranslateStrings");

	listView->setHorizontalHeaderLabels(QStringList() << "   " <<
        tr("Name") << tr("Length"));

	// Tool buttons
	add_button->setIcon(Images::icon("plus"));
	add_button->setToolTip(tr("Add..."));
	remove_button->setIcon(Images::icon("minus"));
	remove_button->setToolTip(tr("Remove..."));

	// Icon
	setWindowIcon(Images::icon("logo", 64));
	setWindowTitle(tr("SMPlayer - Playlist"));

	adding_msg = tr("Adding");
}

void TPlaylist::list() {
	qDebug("Gui::TPlaylist::list");

	TPlaylistItemList::iterator it;
	for (it = pl.begin(); it != pl.end(); ++it) {
		qDebug("filename: '%s', name: '%s' duration: %f",
               (*it).filename().toUtf8().data(), (*it).name().toUtf8().data(),
               (*it).duration());
	}
}


QString TPlaylist::print(QString seperator){
	qDebug("Gui::TPlaylist::print");
	QString output = "";

	TPlaylistItemList::iterator it;
	for (it = pl.begin(); it != pl.end(); ++it) {
		output += it->filename() + seperator + it->name() + seperator + QString::number(it->duration()) + "\r\n";
	}

	return output;
}

void TPlaylist::updateView() {
	qDebug("Gui::TPlaylist::updateView");

	listView->setRowCount(pl.count());

	QString name;
	QString time;

	for (int n = 0; n < pl.count(); n++) {
		TPlaylistItem& item = pl[n];
		name = item.name();
		if (name.isEmpty()) name = item.filename();
		time = Helper::formatTime(qRound(item.duration()));
		
		listView->setText(n, COL_NAME, name);
		listView->setText(n, COL_TIME, time);

		if (item.played()) {
			listView->setIcon(n, COL_PLAY, Images::icon("ok"));
		} else {
			listView->setIcon(n, COL_PLAY, QPixmap());
		}

		if (row_spacing >= 0)
			listView->setRowHeight(n, listView->font().pointSize() + row_spacing);
	}

	listView->resizeColumnToContents(COL_PLAY);
	listView->resizeColumnToContents(COL_TIME);

	setCurrentItem(current_item);
}

void TPlaylist::setCurrentItem(int current) {

	// Give old current_item an icon
	if ((current_item >= 0) && (current_item < listView->rowCount())) {
		if (current_item < pl.count() && pl[current_item].played()) {
			listView->setIcon(current_item, COL_PLAY, Images::icon("ok"));
		} else {
			listView->setIcon(current_item, COL_PLAY, QPixmap());
		}
	}

	current_item = current;

	if (current_item >= 0) {
		if (current_item < listView->rowCount()) {
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
	setCurrentItem(0);
	setModified(false);
}

void TPlaylist::remove(int i){
	if(i >= 0 && i < pl.count()){
		pl.removeAt(i);
		if(current_item == i && i == (pl.count() - 1))
			setCurrentItem(i - 1);
		setModified(true);
		updateView();
	} //end if
}

int TPlaylist::count() {
	return pl.count();
}

bool TPlaylist::isEmpty() {
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
	//setModified(true); // Better set the modified on a higher level
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
          if(compare > 0) {
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

    if ((count == 0) && !swaped && !revert) {
        // Revert sort
        sortBy(section, true, 0);
    }else if(swaped) {
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
		list();
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

	list();
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

		setModified(false);
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
	if (ok) setModified(false);

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
	if (!isModified()) return true;

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
		execPopup(this, popup, listView->viewport()->mapToGlobal(pos));
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

	if ((n < 0) || (n >= pl.count())) {
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
		if (current_item < 0) current_item = 0;
		playItem(current_item);
	}
}

void TPlaylist::playDirectory(const QString &dir) {

	clear();
	addDirectory(dir);
	sortBy(1);
	// sortBy() can change current_item and modified
	setCurrentItem(0);
	setModified(false);
	latest_dir = dir;
	startPlay();
}

void TPlaylist::newMediaLoaded() {

	if (!pref->auto_add_to_playlist) {
		qDebug("Gui::TPlaylist::newMediaLoaded: add to playlist disabled by user");
		return;
	}

	QString filename = core->mdat.filename;
	QString current_filename;
	if (current_item >= 0 && current_item < pl.count()) {
		current_filename = pl[current_item].filename();
	}
	if (filename == current_filename) {
		qDebug("Gui::TPlaylist::newMediaLoaded: new file is current item");
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
			qDebug("Gui::TPlaylist::newMediaLoaded: new file is from current disc");
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
		// Add current file. getMediaInfo will fill in name and duration.
		addItem(filename, "", 0);

		// Add associated files to playlist
		if (core->mdat.selected_type == TMediaData::TYPE_FILE) {
			qDebug() << "Gui::TPlaylist::newMediaLoaded: searching for files to add to playlist for"
					 << filename;
			QStringList files_to_add = Helper::filesForPlaylist(filename, pref->media_to_add_to_playlist);
			if (files_to_add.isEmpty()) {
				qDebug("Gui::TPlaylist::newMediaLoaded: none found");
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

	qDebug() << "Gui::TPlaylist::newMediaLoaded: created new playlist with" << count()
			 << "items for" << filename;
}

void TPlaylist::getMediaInfo() {
	qDebug("Gui::TPlaylist::getMediaInfo");

	// Already have info for dics
	if (core->mdat.detectedDisc()) {
		return;
	}

	QString filename = core->mdat.filename;
	double duration = core->mdat.duration;
	QString artist = core->mdat.meta_data.value("ARTIST");

	QString name = core->mdat.meta_data.value("NAME", core->mdat.stream_title);

	#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	filename = Helper::changeSlashes(filename);
	#endif

	if (name.isEmpty()) {
		QFileInfo fi(filename);
		if (fi.exists()) {
			// Local file
			name = fi.fileName();
		} else {
			// Stream
			name = filename;
		}
	}
	if (!artist.isEmpty()) name = artist + " - " + name;

	for (int n = 0; n < pl.count(); n++) {
		TPlaylistItem& item = pl[n];
		if (item.filename() == filename) {
			// Protect playlist by only updating items with duration 0
			if (item.duration() == 0) {
				if (!name.isEmpty() && !item.edited()) {
					item.setName(name);
				}
				item.setDuration(duration);
			}
		}
	}

	updateView();
}

void TPlaylist::mediaEOF() {
	qDebug("Gui::Tplaylist::mediaEOF");

	if (automatically_play_next) {
		playNext();
	}
}

void TPlaylist::playerSwitchedTitle(int id) {
	qDebug("Gui::TPlaylist::playerSwitchedTitle: %d", id);

	id -= core->mdat.titles.firstID();
	if (id >= 0 && id < pl.count()) {
		setCurrentItem(id);
		pl[id].setPlayed(true);
	}
}

// Add current file to playlist
void TPlaylist::addCurrentFile() {
	qDebug("Gui::TPlaylist::addCurrentFile");
	if (!core->mdat.filename.isEmpty()) {
		addItem(core->mdat.filename, "", 0);
		getMediaInfo();
	}
}

void TPlaylist::addFile(const QString &filename, bool get_info) {
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
		} else {
			TMediaData media_data;

#if USE_INFOPROVIDER
			if (get_info) {
				TInfoProvider::getInfo(filename, media_data);
			}
#endif

			addItem(filename, media_data.displayName(), media_data.duration);
		}

		latest_dir = fi.absolutePath();
	} else {
		addItem(filename, "", 0);
	}
}

void TPlaylist::addDirectory(const QString &dir, bool get_info) {
	qDebug() << "Gui::TPlaylist::addDirectory:" << dir;

	static TExtensions ext;
	static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

	emit displayMessage(adding_msg + " " + dir, 0);

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
					addDirectory(filename, get_info);
			} else if (rx_ext.indexIn(fi.suffix()) >= 0) {
				addFile(filename, get_info);
			}
		}
		++it;
	}

	emit displayMessage("", 0);
}

void TPlaylist::addFileOrDir(const QString &filename, bool get_info) {

	if (QFileInfo(filename).isDir()) {
		addDirectory(filename, get_info);
	} else {
		addFile(filename, get_info);
	}
}

void TPlaylist::addFiles(const QStringList &files, bool get_info) {
	qDebug("Gui::TPlaylist::addFiles");

#if USE_INFOPROVIDER
	setCursor(Qt::WaitCursor);
#endif

	QStringList::ConstIterator it = files.constBegin();
	while(it != files.constEnd()) {
		addFileOrDir(*it, get_info);
		++it;
	}

#if USE_INFOPROVIDER
	unsetCursor();
#endif

	updateView();

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
		addDirectory(s, true);
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
			setModified(true);
		}
	}


    if (first_selected < current_item) {
        current_item -= number_previous_item;
    }

	if (isEmpty()) setModified(false);
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
	/*
	pl.clear();
	updateView();
	setModified(false);
	*/
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
	setModified(true);
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

void TPlaylist::editCurrentItem() {
	int current = listView->currentRow();
	if (current > -1) editItem(current);
}

void TPlaylist::editItem(int item) {
	QString current_name = pl[item].name();
	if (current_name.isEmpty()) current_name = pl[item].filename();

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
		setModified(true);
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

void TPlaylist::maybeSaveSettings() {
	qDebug("Gui::TPlaylist::maybeSaveSettings");

	if (modified)
		saveSettings();
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

	set->setValue("auto_get_info", automatically_get_info);
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

	automatically_get_info = set->value("auto_get_info", automatically_get_info).toBool();
	recursive_add_directory = set->value("recursive_add_directory", recursive_add_directory).toBool();
	save_playlist_in_config = set->value("save_playlist_in_config", save_playlist_in_config).toBool();
	play_files_from_start = set->value("play_files_from_start", play_files_from_start).toBool();
	automatically_play_next = set->value("automatically_play_next", automatically_play_next).toBool();

	row_spacing = set->value("row_spacing", row_spacing).toInt();

	latest_dir = set->value("latest_dir", latest_dir).toString();

	set->endGroup();

	if (save_playlist_in_config) {
		//Load latest list
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
		setModified(set->value("modified", false).toBool());
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

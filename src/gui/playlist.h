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


#ifndef _GUI_PLAYLIST_H_
#define _GUI_PLAYLIST_H_

#include <QList>
#include <QStringList>
#include <QWidget>
#include "gui/action/action.h"
#include "gui/tablewidget.h"

class QToolBar;
class TCore;
class QMenu;
class QSettings;
class QToolButton;
class QTimer;

namespace Gui {

class TPlaylistItem {

public:
	TPlaylistItem();
	TPlaylistItem(const QString &filename, const QString &name, double duration);
	virtual ~TPlaylistItem() {}

	void setFilename(const QString &filename) { _filename = filename; }
	void setName(const QString &name) { _name = name; }
	void setDuration(double duration) { _duration = duration; }
	void setPlayed(bool b) { _played = b; }
	void setMarkForDeletion(bool b) { _deleted = b; }
	void setEdited(bool b) { _edited = b; }

	QString directory() const { return _directory; }
	QString filename() const { return _filename; }
	QString name() const { return _name; }
	double duration() const { return _duration; }
	bool played() const { return _played; }
	bool markedForDeletion() const { return _deleted; }
	bool edited() const { return _edited; }

private:
	QString _directory, _filename, _name;
	double _duration;
	bool _played, _deleted, _edited;
};

class TPlaylist : public QWidget {
	Q_OBJECT

public:
	enum AutoGetInfo { NoGetInfo = 0, GetInfo = 1, UserDefined = 2 };

	typedef QList<TPlaylistItem> TPlaylistItemList;

	TPlaylist(QWidget* parent, TCore* c, Qt::WindowFlags f = Qt::Window);
	virtual ~TPlaylist();

	int count();
	bool isEmpty();
	bool directoryRecursion() const { return recursive_add_directory; }
	bool autoGetInfo() const { return automatically_get_info; }
	bool savePlaylistOnExit() const { return save_playlist_in_config; }
	bool playFilesFromStart() const { return play_files_from_start; }
	const TPlaylistItemList* playlist() const { return &pl; }
	int currentItem() const { return current_item; }

	void clear();
	void list();

	void loadSettings();
	void saveSettings();
	void retranslateStrings();

public slots:
	// Start playing, from item 0 if shuffle is off, or from
	// a random item otherwise
	void startPlay();

	void playItem(int n);
	void playDirectory(const QString& dir);

	virtual void playNext();
	virtual void playPrev();

	virtual void resumePlay();

	virtual void removeSelected();
	virtual void removeAll();
	virtual void remove(int);

	virtual void moveItemUp(int);
	virtual void moveItemDown(int);

	virtual void addCurrentFile();
	virtual void addFiles();
	virtual void addDirectory();
	virtual void addUrls();

	virtual void addFile(const QString& filename, bool get_info = false);
	virtual void addFiles(const QStringList& files, bool get_info = false);
	void addFileOrDir(const QString& filename, bool get_info = false);

	// Adds a directory, maybe with recursion (depends on user config)
	virtual void addDirectory(const QString& dir, bool get_info = false);

	// EDIT BY NEO -->
	virtual void sortBy(int section);
	// <--

	virtual void deleteSelectedFileFromDisk();

	virtual bool maybeSave();
    virtual void load();
    virtual bool save();

	virtual void load_m3u(const QString& file, bool clear = true, bool play = true);
	virtual bool save_m3u(QString file);

	virtual void load_pls(const QString& file, bool clear = true, bool play = true);
	virtual bool save_pls(QString file);

	virtual void newMediaLoaded();
	virtual void getMediaInfo();
	virtual void mediaEOF();
	void playerSwitchedTitle(int id);

	// Preferences
	void setDirectoryRecursion(bool b) { recursive_add_directory = b; }
	void setAutoGetInfo(bool b) { automatically_get_info = b; }
	void setSavePlaylistOnExit(bool b) { save_playlist_in_config = b; }
	void setPlayFilesFromStart(bool b) { play_files_from_start = b; }

signals:
	void playlistEnded();
	void visibilityChanged(bool visible);
	void displayMessage(const QString&, int);

protected:
	void addItem(QString filename, QString name, double duration);
	void setCurrentItem(int current);
	void clearPlayedTag();
	int chooseRandomItem();
	void swapItems(int item1, int item2);
	// EDIT BY NEO -->
	void sortBy(int section, bool revert, int count);
	// <--
	QString lastDir();
	void updateView();

	void createTable();
	void createActions();
	void createToolbar();

	virtual void dragEnterEvent(QDragEnterEvent*) ;
	virtual void dropEvent (QDropEvent*);
	virtual void hideEvent (QHideEvent*);
	virtual void showEvent (QShowEvent*);
	virtual void closeEvent(QCloseEvent* e);

protected slots:
	virtual void playCurrent();
	virtual void itemDoubleClicked(int row);
	virtual void showContextMenu(const QPoint & pos);
	virtual void upItem();
	virtual void downItem();
	virtual void editCurrentItem();
	virtual void editItem(int item);

protected:
	TPlaylistItemList pl;
	int current_item;

	QString playlist_path;
	QString latest_dir;

	TCore* core;
	QMenu* add_menu;
	QMenu* remove_menu;
	QMenu* popup;

	TTableWidget* listView;

	QToolBar* toolbar;
	QToolButton* add_button;
	QToolButton* remove_button;

	TAction* openAct;
	TAction* saveAct;
	TAction* playAct;
	TAction* prevAct;
	TAction* nextAct;
	TAction* repeatAct;
	TAction* shuffleAct;

	TAction* moveUpAct;
	TAction* moveDownAct;
	TAction* editAct;

	TAction* addCurrentAct;
	TAction* addFilesAct;
	TAction* addDirectoryAct;
	TAction* addUrlsAct;

	TAction* removeSelectedAct;
	TAction* removeAllAct;

	TAction* deleteSelectedFileFromDiskAct;

private:
	bool modified;

	//Preferences
	bool recursive_add_directory;
	bool automatically_get_info;
	bool save_playlist_in_config;
	bool play_files_from_start;
	bool automatically_play_next;
	int row_spacing;
};

} // namespace Gui

#endif // _GUI_PLAYLIST_H_


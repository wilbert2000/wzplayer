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


#ifndef _PLAYLIST_H_
#define _PLAYLIST_H_

#include <QList>
#include <QStringList>
#include <QWidget>

class PlaylistItem {

public:
	PlaylistItem();
	PlaylistItem(const QString &filename, const QString &name, double duration);
	~PlaylistItem() {};

	void setFilename(const QString &filename) { _filename = filename; }
	void setName(const QString &name) { _name = name; }
	void setDuration(double duration) { _duration = duration; }
	void setPlayed(bool b) { _played = b; }
	void setMarkForDeletion(bool b) { _deleted = b; }
	void setEdited(bool b) { _edited = b; }

	QString filename() { return _filename; }
	QString name() { return _name; }
	double duration() { return _duration; }
	bool played() { return _played; }
	bool markedForDeletion() { return _deleted; }
	bool edited() { return _edited; }

private:
	QString _filename, _name;
	double _duration;
	bool _played, _deleted, _edited;
};

class MyTableWidget;
class QToolBar;
class MyAction;
class Core;
class QMenu;
class QSettings;
class QToolButton;
class QTimer;

class Playlist : public QWidget
{
	Q_OBJECT

public:
	enum AutoGetInfo { NoGetInfo = 0, GetInfo = 1, UserDefined = 2 };

	Playlist( Core *c, QWidget * parent = 0, Qt::WindowFlags f = Qt::Window );
	~Playlist();

	int count();
	bool isEmpty();
	QString print(QString seperator);

	bool isModified() { return modified; }

	void clear();
	void list();

public slots:
	// Start playing, from item 0 if shuffle is off, or from
	// a random item otherwise
	void startPlay();

	void playItem(int n);
	void playDirectory(const QString &dir);

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

	virtual void addFile(const QString &filename, bool get_info = false);
	virtual void addFiles(const QStringList &files, bool get_info = false);
	void addFileOrDir(const QString &filename, bool get_info = false);

	// Adds a directory, maybe with recursion (depends on user config)
	virtual void addDirectory(const QString &dir, bool get_info = false);

	// EDIT BY NEO -->
	virtual void sortBy(int section);
	// <--

	virtual void deleteSelectedFileFromDisk();

	virtual bool maybeSave();
    virtual void load();
    virtual bool save();
	virtual void saveSettings();

	virtual void load_m3u(const QString &file, bool clear = true, bool play = true);
	virtual bool save_m3u(QString file);

	virtual void load_pls(const QString &file, bool clear = true, bool play = true);
	virtual bool save_pls(QString file);

	virtual void newMediaLoaded();
	virtual void getMediaInfo();
	void playerSwitchedTitle(int id);

	void setModified(bool);

	// Preferences
	void setDirectoryRecursion(bool b) { recursive_add_directory = b; }
	void setAutoGetInfo(bool b) { automatically_get_info = b; }
	void setSavePlaylistOnExit(bool b) { save_playlist_in_config = b; }
	void setPlayFilesFromStart(bool b) { play_files_from_start = b; }
	void setIgnorePlayerErrors(bool b) { ignore_player_errors = b; }

public:
	bool directoryRecursion() { return recursive_add_directory; }
	bool autoGetInfo() { return automatically_get_info; }
	bool savePlaylistOnExit() { return save_playlist_in_config; }
	bool playFilesFromStart() { return play_files_from_start; }
	bool ignorePlayerErrors() { return ignore_player_errors; }

	QList<PlaylistItem> playlist() {return pl;}

/*
public:
	MyAction * playPrevAct() { return prevAct; };
	MyAction * playNextAct() { return nextAct; };
*/

signals:
	void playlistEnded();
	void visibilityChanged(bool visible);
	void modifiedChanged(bool);

protected:
	void addItem(const QString &filename, QString name, double duration);
	void setCurrentItem(int current);
	void clearPlayedTag();
	int chooseRandomItem();
	void swapItems(int item1, int item2 );
	// EDIT BY NEO -->
	void sortBy(int section, bool revert, int count);
	// <--
	QString lastDir();
	void updateView();

protected slots:
	virtual void playCurrent();
	virtual void itemDoubleClicked(int row);
	virtual void showPopup(const QPoint & pos);
	virtual void upItem();
	virtual void downItem();
	virtual void editCurrentItem();
	virtual void editItem(int item);

	virtual void loadSettings();
	virtual void maybeSaveSettings();

protected:
	void createTable();
	void createActions();
	void createToolbar();

protected:
	void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;
	virtual void dragEnterEvent( QDragEnterEvent * ) ;
	virtual void dropEvent ( QDropEvent * );
	virtual void hideEvent ( QHideEvent * );
	virtual void showEvent ( QShowEvent * );
	virtual void closeEvent( QCloseEvent * e );

protected:
	typedef QList <PlaylistItem> PlaylistItemList;
	PlaylistItemList pl;
	int current_item;

	QString playlist_path;
	QString latest_dir;

	Core * core;
	QMenu * add_menu;
	QMenu * remove_menu;
	QMenu * popup;

	MyTableWidget * listView;

	QToolBar * toolbar;
	QToolButton * add_button;
	QToolButton * remove_button;

	MyAction * openAct;
	MyAction * saveAct;
	MyAction * playAct;
	MyAction * prevAct;
	MyAction * nextAct;
	MyAction * repeatAct;
	MyAction * shuffleAct;

	MyAction * moveUpAct;
	MyAction * moveDownAct;
	MyAction * editAct;

	MyAction * addCurrentAct;
	MyAction * addFilesAct;
	MyAction * addDirectoryAct;
	MyAction * addUrlsAct;

	MyAction * removeSelectedAct;
	MyAction * removeAllAct;

	MyAction * deleteSelectedFileFromDiskAct;

private:
	bool modified;
	QTimer * save_timer;

	//Preferences
	bool recursive_add_directory;
	bool automatically_get_info;
	bool save_playlist_in_config;
	bool play_files_from_start;
	int row_spacing;

	bool automatically_play_next;
	bool ignore_player_errors;
};


#endif


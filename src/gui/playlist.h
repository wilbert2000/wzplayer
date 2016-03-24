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


#ifndef GUI_PLAYLIST_H
#define GUI_PLAYLIST_H

#include <QList>
#include <QStringList>
#include <QWidget>


class QToolBar;
class TCore;
class QMenu;
class QSettings;
class QToolButton;
class QTimer;

namespace Gui {

class TTableWidget;

namespace Action {
class TAction;
}

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
	typedef QList<TPlaylistItem> TPlaylistItemList;

	TPlaylist(QWidget* parent, TCore* c);
	virtual ~TPlaylist();

	int count() const;
	bool isEmpty() const;
	bool directoryRecursion() const { return recursive_add_directory; }
	bool savePlaylistOnExit() const { return save_playlist_in_config; }
	bool playFilesFromStart() const { return play_files_from_start; }

	void appendFiles(QStringList& files) const;
	int currentItem() const { return current_item; }

	void clear();

	void loadSettings();
	void saveSettings();
	void retranslateStrings();

public slots:
	// Start playing, from item 0 if shuffle is off,
	// or from a random item otherwise
	void startPlay();

	void playItem(int n);
	void playDirectory(const QString& dir);

	void playNext();
	void playPrev();

	void resumePlay();

	void removeSelected();
	void removeAll();
	void remove(int);

	void moveItemUp(int);
	void moveItemDown(int);

	void addCurrentFile();
	void addFiles();
	void addDirectory();
	void addUrls();

	void addFile(const QString& filename);
	void addFiles(const QStringList& files);
	void addFileOrDir(const QString& filename);

	// Adds a directory, maybe with recursion (depends on user config)
	void addDirectory(const QString& dir);


	void deleteSelectedFileFromDisk();

	bool maybeSave();
	void load();
	bool save();

	void load_m3u(const QString& file, bool clear = true, bool play = true);
	bool save_m3u(QString file);

	void load_pls(const QString& file, bool clear = true, bool play = true);
	bool save_pls(QString file);

	void onNewMediaStartedPlaying();
	void onMediaLoaded();
	void mediaEOF();
	void playerSwitchedTitle(int id);

	// Preferences
	void setDirectoryRecursion(bool b) { recursive_add_directory = b; }
	void setSavePlaylistOnExit(bool b) { save_playlist_in_config = b; }
	void setPlayFilesFromStart(bool b) { play_files_from_start = b; }

signals:
	void playlistEnded();
	void visibilityChanged(bool visible);
	void displayMessage(const QString&, int);

protected:
	virtual void dragEnterEvent(QDragEnterEvent*) ;
	virtual void dropEvent (QDropEvent*);
	virtual void hideEvent (QHideEvent*);
	virtual void showEvent (QShowEvent*);
	virtual void closeEvent(QCloseEvent* e);

protected slots:
	void playCurrent();
	void itemDoubleClicked(int row);
	void showContextMenu(const QPoint & pos);
	void upItem();
	void downItem();
	void copyCurrentItem();
	void editCurrentItem();
	void editItem(int item);

private:
	enum TColID {
		COL_PLAY = 0,
		COL_NAME = 1,
		COL_TIME = 2,
		COL_COUNT = 3
	};

	int current_item;
	TCore* core;
	TPlaylistItemList pl;
	TTableWidget* listView;

	QMenu* add_menu;
	QMenu* remove_menu;
	QMenu* popup;

	QToolBar* toolbar;
	QToolButton* add_button;
	QToolButton* remove_button;

	Action::TAction* openAct;
	Action::TAction* saveAct;
	Action::TAction* playAct;
	Action::TAction* prevAct;
	Action::TAction* nextAct;
	Action::TAction* repeatAct;
	Action::TAction* shuffleAct;

	Action::TAction* moveUpAct;
	Action::TAction* moveDownAct;

	Action::TAction* addCurrentAct;
	Action::TAction* addFilesAct;
	Action::TAction* addDirectoryAct;
	Action::TAction* addUrlsAct;

	Action::TAction* copyAct;
	Action::TAction* editAct;
	Action::TAction* removeSelectedAct;
	Action::TAction* removeAllAct;

	Action::TAction* deleteSelectedFileFromDiskAct;

	// Preferences
	bool recursive_add_directory;
	bool save_playlist_in_config;
	bool play_files_from_start;
	bool automatically_play_next;
	int row_spacing;

	bool modified;
	QString playlist_path;
	QString latest_dir;

	void addItem(QString filename, QString name, double duration);
	int chooseRandomItem();
	void clearPlayedTag();
	void createTable();
	void createActions(QWidget* parent);
	void createToolbar();
	QString lastDir();
	void setCurrentItem(int current);
	void swapItems(int item1, int item2);
	void updateView();

private slots:
	void sortBy(int section);
	void sortBy(int section, bool revert, int count);
};

} // namespace Gui

#endif // GUI_PLAYLIST_H


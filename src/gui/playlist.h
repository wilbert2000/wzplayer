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
#ifndef GUI_PLAYLIST_H
#define GUI_PLAYLIST_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QList>
#include <QStringList>

#include "gui/playlistwidget.h"


class QToolBar;
class QMenu;
class QSettings;
class QToolButton;
class QTimer;
class QItemSelection;

class TCore;

namespace Gui {

class TBase;

namespace Action {
class TAction;
class TMenu;
class TMenuInOut;
}

class TPlaylist : public QWidget {
	Q_OBJECT
public:
    TPlaylist(TBase* mw, TCore* c);
	virtual ~TPlaylist();

	// Start playing, from item 0 if shuffle is off,
	// or from a random item otherwise
	void startPlay();
    void playItem(TPlaylistWidgetItem* item);
	void playDirectory(const QString& dir);

    QString playingFile() const { return playlistWidget->playingFile(); }
    TPlaylistWidgetItem* findFilename(const QString& filename) {
        return playlistWidget->findFilename(filename);
    }

	void clear();
    void addFiles(const QStringList& files, bool insert = false);
	void getFilesAppend(QStringList& files) const;

	// Preferences
	bool directoryRecursion() const { return recursive_add_directory; }
	void setDirectoryRecursion(bool b) { recursive_add_directory = b; }

	bool maybeSave();
	void loadSettings();
	void saveSettings();
	void retranslateStrings();

    Action::TMenuInOut* getInOutMenu() const { return inOutMenu; }

public slots:
	void playNext();
	void playPrev();

	void load();
	bool save();

signals:
	void playlistEnded();
    void enablePrevNextChanged();
    void visibilityChanged(bool visible);
	void displayMessage(const QString&, int);
    void displayMessageOnOSD(const QString&, int);
    void windowTitleChanged();

protected:
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);
	virtual void hideEvent (QHideEvent*);
	virtual void showEvent (QShowEvent*);
	virtual void closeEvent(QCloseEvent* e);

private:
    TBase* main_window;
	TCore* core;
    TPlaylistWidget* playlistWidget;

    Action::TMenu* add_menu;
    Action::TMenu* remove_menu;
	QMenu* popup;

	QToolBar* toolbar;
	QToolButton* add_button;
	QToolButton* remove_button;

	Action::TAction* openAct;
	Action::TAction* saveAct;
    Action::TAction* playOrPauseAct;
    Action::TAction* stopAct;
	Action::TAction* prevAct;
	Action::TAction* nextAct;
	Action::TAction* repeatAct;
	Action::TAction* shuffleAct;

	Action::TAction* addCurrentAct;
	Action::TAction* addFilesAct;
	Action::TAction* addDirectoryAct;
	Action::TAction* addUrlsAct;

    Action::TAction* cutAct;
    Action::TAction* copyAct;
    Action::TAction* pasteAct;
    Action::TAction* editAct;
	Action::TAction* removeSelectedAct;
	Action::TAction* removeSelectedFromDiskAct;
	Action::TAction* removeAllAct;

    Action::TMenuInOut* inOutMenu;

	// Preferences
	bool recursive_add_directory;

    bool notify_sel_changed;

    bool modified;
    QString title;
    QString playlist_filename;
    QString playlist_path;

    void createTree();
    void createActions();
	void createToolbar();

    void msg(const QString& s);

    void cleanAndAddItem(QString filename, QString name, double duration);

    void addFile(QTreeWidgetItem* parent, const QString& filename);
    void addFileOrDir(QTreeWidgetItem* parent, const QString& filename);
    void addDirectory(QTreeWidgetItem* parent, const QString& dir);

    TPlaylistWidgetItem* chooseRandomItem() const;

    void swapItems(int item1, int item2);

    bool deleteFileFromDisk(const QString& filename);

	void loadM3u(const QString& file, bool clear = true, bool play = true);
	bool saveM3u(QString file);

	void loadIni(const QString& file, bool clear = true, bool play = true);

    void setModified(bool mod = true);
    void setWinTitle(QString s = 0);
    void setPlaylistFilename(const QString& name);

private slots:
	void showContextMenu(const QPoint& pos);

    void onPlayerError();

    void playOrPause();

    void addCurrentFile();
	void addFiles();
	void addDirectory();
	void addUrls();

	void removeSelected(bool deleteFromDisk = false);
	void removeSelectedFromDisk();
	void removeAll();

    void editCurrentItem();
    void editItem(TPlaylistWidgetItem* item);

    void copySelected();
    void paste();
    void enablePaste();
    void cut();

    void enableActions();

    void onItemActivated(QTreeWidgetItem* item, int);
    void onDropRow(int, int);
    void onVisibilityChanged(bool);
    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);
    void onStartPlayingNewMedia();
	void onTitleTrackChanged(int id);
	void onMediaEOF();
	void resumePlay();
};

} // namespace Gui

#endif // GUI_PLAYLIST_H


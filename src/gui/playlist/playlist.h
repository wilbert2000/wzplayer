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
#ifndef GUI_PLAYLIST_PLAYLIST_H
#define GUI_PLAYLIST_PLAYLIST_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QList>
#include <QStringList>

#include "wzdebug.h"
#include "gui/action/menu/menu.h"


class QToolBar;
class QMenu;
class QToolButton;


namespace Gui {

class TMainWindow;

namespace Action {
class TAction;
namespace Menu {
class TMenuInOut;
}
}

namespace Playlist {

class TPlaylistWidget;
class TPlaylistWidgetItem;
class TAddFilesThread;

class TAddRemovedMenu : public Action::Menu::TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit TAddRemovedMenu(QWidget* parent,
                             TMainWindow* w,
                             TPlaylistWidget* plWidget);
    virtual ~TAddRemovedMenu();

signals:
    void addRemovedItem(QString s);

protected:
    virtual void onAboutToShow();

private:
    TPlaylistWidget* playlistWidget;
    TPlaylistWidgetItem* item;

private slots:
    void onTriggered(QAction* action);
};


class TPlaylist : public QWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylist(QWidget* parent, TMainWindow* mw);
    virtual ~TPlaylist();

    void openPlaylist(const QString& filename);
    void playDirectory(const QString& dir);

    bool isPlaylistEnabled() const;
    QString playingFile() const;
    TPlaylistWidgetItem* findFilename(const QString& filename) const;
    TPlaylistWidgetItem* currentPlaylistWidgetItem() const;
    bool hasItems() const;
    bool hasPlayingItem() const;

    void clear();
    void addFiles(const QStringList& files,
                  bool startPlay = false,
                  QTreeWidgetItem* target = 0,
                  const QString& fileToPlay = "",
                  bool searchForItems = false);
    void getFilesToPlay(QStringList& files) const;
    void abortThread();

    bool maybeSave();
    void loadSettings();
    void saveSettings();
    void retranslateStrings();

    Action::Menu::TMenuInOut* getInOutMenu() const { return inOutMenu; }

public slots:
    void stop();

    void editName();
    void newFolder();
    void findPlayingItem();

    void cut();
    void copySelected();
    void paste();

    void removeSelected(bool deleteFromDisk = false);
    void removeSelectedFromDisk();
    void removeAll();

signals:
    void playlistFinished();
    void enablePrevNextChanged();
    void playlistTitleChanged(QString title);

protected:
    virtual void dragEnterEvent(QDragEnterEvent*);
    virtual void dropEvent(QDropEvent*);

private:
    Action::Menu::TMenu* add_menu;
    TAddRemovedMenu* add_removed_menu;
    Action::Menu::TMenu* remove_menu;
    QMenu* contextMenu;

    QToolBar* toolbar;
    QToolButton* add_button;
    QToolButton* remove_button;

    Action::TAction* openAct;
    Action::TAction* saveAct;
    Action::TAction* saveAsAct;
    Action::TAction* openDirectoryAct;
    Action::TAction* refreshAct;

    Action::TAction* playAct;
    Action::TAction* playNewAct;
    Action::TAction* pauseAct;
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

    Action::Menu::TMenuInOut* inOutMenu;

    TMainWindow* main_window;
    TPlaylistWidget* playlistWidget;

    QString filename;

    TAddFilesThread* thread;
    QStringList addFilesFiles;
    TPlaylistWidgetItem* addFilesTarget;
    QString addFilesFileToPlay;
    bool addFilesStartPlay;
    bool addFilesSearchItems;
    bool restartThread;

    bool disable_enableActions;
    bool reachedEndOfPlaylist;


    void createTree();
    void createActions();
    void createToolbar();

    void addFilesStartThread();
    void startPlay();
    void playItem(TPlaylistWidgetItem* item);

    void setPlaylistTitle();

    TPlaylistWidgetItem* getRandomItem() const;
    bool haveUnplayedItems() const;

    void copySelection(const QString& actionName);
    bool removeFromDisk(const QString& filename,
                            const QString& playingFile);

    bool saveM3uFolder(TPlaylistWidgetItem* folder,
                       const QString& path,
                       QTextStream& stream,
                       bool linkFolders,
                       bool& savedMetaData);
    bool saveM3u(TPlaylistWidgetItem* folder,
                 const QString& filename,
                 bool wzplaylist);
    bool saveM3u(const QString& filename, bool linkFolders);

private slots:
    void open();
    bool save();
    bool saveAs();
    void openFolder();

    void addCurrentFile();
    void addFilesDialog();
    void addDirectory();
    void addUrls();
    void addRemovedItem(QString s);

    void refresh();

    void play();
    void playOrPause();
    void playNext(bool loop_playlist = true);
    void playPrev();
    void openInNewWindow();
    void resumePlay();

    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);

    void enableActions();
    void showContextMenu(const QPoint& pos);

    void onItemActivated(QTreeWidgetItem* item, int);
    void onPlayerError();
    void onModifiedChanged();
    void onNewMediaStartedPlaying();
    void onTitleTrackChanged(int id);
    void onMediaEOF();
    void onThreadFinished();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLIST_H


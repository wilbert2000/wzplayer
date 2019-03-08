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
#include "wzdebug.h"


class QToolBar;
class QToolButton;
class QTreeWidgetItem;


namespace Gui {

class TMainWindow;

namespace Action {
class TAction;
namespace Menu {
class TMenu;
}
}

namespace Playlist {

class TPlaylistWidget;
class TPlaylistItem;
class TAddFilesThread;


class TPlaylist : public QWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylist(QWidget* parent, TMainWindow* mw);
    virtual ~TPlaylist();

    QString playingFile() const;

    void add(const QStringList& files,
                  bool startPlay = false,
                  TPlaylistItem* target = 0,
                  const QString& fileToPlay = "");
    void getFilesToPlay(QStringList& files) const;
    void abortThread();

    bool maybeSave();
    void loadSettings();
    void saveSettings();

public slots:
    void open(const QString &fileName, const QString& name = QString());
    void openFiles(const QStringList& files, const QString& fileToPlay = "");
    void openFileDialog();
    void openDirectoryDialog();

    void stop();

    void addRemovedItem(const QString& s);

signals:
    void playlistFinished();
    void enablePrevNextChanged();
    void playlistTitleChanged(QString title);

protected:
    virtual void dragEnterEvent(QDragEnterEvent*) override;
    virtual void dropEvent(QDropEvent*) override;

private:
    TMainWindow* main_window;
    TPlaylistWidget* playlistWidget;
    QToolBar* toolbar;
    QToolButton* add_button;
    QToolButton* remove_button;

    Action::TAction* openPlaylistAct;
    Action::TAction* saveAct;
    Action::TAction* saveAsAct;
    Action::TAction* refreshAct;
    Action::TAction* browseDirAct;

    Action::TAction* stopAct;
    Action::TAction* playAct;
    Action::TAction* playOrPauseAct;
    Action::TAction* playNewAct;
    Action::TAction* pauseAct;
    Action::TAction* playNextAct;
    Action::TAction* playPrevAct;
    Action::TAction* repeatAct;
    Action::TAction* shuffleAct;

    Action::TAction* editNameAct;
    Action::TAction* newFolderAct;
    Action::TAction* findPlayingAct;

    Action::TAction* cutAct;
    Action::TAction* copyAct;
    Action::TAction* pasteAct;

    Action::Menu::TMenu* playlistAddMenu;
    Action::TAction* addPlayingFileAct;

    Action::Menu::TMenu* playlistRemoveMenu;
    Action::TAction* removeSelectedAct;
    Action::TAction* removeSelectedFromDiskAct;
    Action::TAction* removeAllAct;


    QString filename;

    TAddFilesThread* thread;
    QStringList addFiles;
    TPlaylistItem* addTarget;
    QString addFileToPlay;
    bool addStartPlay;
    bool restartThread;

    int disableEnableActions;
    bool reachedEndOfPlaylist;


    void createTree();
    void createActions();

    void openPlaylist(const QString& filename);
    void openDirectory(const QString& dir);

    void clear();
    void addStartThread();
    void startPlay();
    void playItem(TPlaylistItem* item, bool keepPaused = false);

    void setPlaylistTitle();

    TPlaylistItem* getRandomItem() const;
    bool haveUnplayedItems() const;

    void copySelection(const QString& actionName);

    bool saveM3uFolder(TPlaylistItem* folder,
                       const QString& path,
                       QTextStream& stream,
                       bool linkFolders,
                       bool& savedMetaData);
    bool saveM3u(TPlaylistItem* folder,
                 const QString& filename,
                 bool wzplaylist);
    bool saveM3u(const QString& filename, bool linkFolders);

private slots:
    void askOpenPlaylist();

    bool save();
    bool saveAs();
    void refresh();
    void browseDir();

    void addPlayingFile();
    void addFilesDialog();
    void addDirectory();
    void addUrls();

    void removeSelected(bool deleteFromDisk = false);
    void removeSelectedFromDisk();
    void removeAll();

    void play();
    void playOrPause();
    void playNext(bool loop_playlist = true);
    void playPrev();
    void openInNewWindow();
    void resumePlay();

    void findPlayingItem();
    void newFolder();

    void cut();
    void copySelected();
    void paste();

    void enablePaste();
    void enableRemoveFromDiskAction();
    void enableRemoveMenu();
    void enableActions();

    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);

    void onItemActivated(QTreeWidgetItem* i, int);
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


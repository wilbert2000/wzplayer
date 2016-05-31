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
#include "gui/playlist/playlistwidget.h"
#include "config.h"


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

namespace Playlist {

class TAddFilesThread;


class TPlaylist : public QWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TPlaylist(TBase* mw, TCore* c);
    virtual ~TPlaylist();

    void openPlaylist(const QString& filename);

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
    void addFiles(const QStringList& files,
                  bool startPlay = false,
                  QTreeWidgetItem* target = 0,
                  const QString& fileToPlay = "",
                  bool searchForItems = false);
    void getFilesToPlay(QStringList& files) const;

    bool maybeSave();
    void loadSettings();
    void saveSettings();
    void retranslateStrings();

    Action::TMenuInOut* getInOutMenu() const { return inOutMenu; }

public slots:
    void playNext(bool allow_reshuffle = true);
    void playPrev();

    void open();
    bool save();
    bool saveAs();

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
    Action::TMenu* add_menu;
    Action::TMenu* remove_menu;
    QMenu* popup;

    QToolBar* toolbar;
    QToolButton* add_button;
    QToolButton* remove_button;

    Action::TAction* openAct;
    Action::TAction* saveAct;
    Action::TAction* saveAsAct;
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

    Action::TAction* cutAct;
    Action::TAction* copyAct;
    Action::TAction* pasteAct;
    Action::TAction* editAct;

    Action::TAction* refreshAct;

    Action::TAction* removeSelectedAct;
    Action::TAction* removeSelectedFromDiskAct;
    Action::TAction* removeAllAct;

    Action::TMenuInOut* inOutMenu;

    TBase* main_window;
    TCore* core;
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


    void createTree();
    void createActions();
    void createToolbar();

    void abortThread();
    void addFilesStartThread();

    void msg(const QString& s, int duration = TConfig::MESSAGE_DURATION);
    void setWinTitle();

    TPlaylistWidgetItem* getRandomItem() const;
    bool haveUnplayedItems() const;

    void swapItems(int item1, int item2);

    bool deleteFileFromDisk(const QString& filename, const QString& playingFile);

    bool saveM3uFolder(TPlaylistWidgetItem* folder,
                       const QString& path,
                       QTextStream& stream,
                       bool linkFolders);
    bool saveM3u(TPlaylistWidgetItem* folder,
                 const QString& filename,
                 bool wzplaylist);
    bool saveM3u(const QString& filename, bool linkFolders);
    bool savePls(const QString& filename);

private slots:
    void showContextMenu(const QPoint& pos);

    void onPlayerError();

    void play();
    void playOrPause();
    void stop();

    void addCurrentFile();
    void addFiles();
    void addDirectory();
    void addUrls();

    void removeSelected(bool deleteFromDisk = false);
    void removeSelectedFromDisk();
    void removeAll();

    void editCurrentItem();
    void editItem(TPlaylistWidgetItem* item);
    void onModifiedChanged();

    // TODO: Copied needs translation
    void copySelected(const QString& actionName = "Copied");
    void paste();
    void enablePaste();
    void cut();
    void refresh();
    void openInNewWindow();

    void enableActions();

    void onItemActivated(QTreeWidgetItem* item, int);
    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);
    void onNewMediaStartedPlaying();
    void onTitleTrackChanged(int id);
    void onMediaEOF();
    void resumePlay();
    void onThreadFinished();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLIST_H


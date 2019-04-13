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

#include "gui/playlist/plist.h"
#include "wzdebug.h"


class TDiscName;

namespace Gui {

class TMainWindow;
class TDockWidget;

namespace Action {
class TAction;
}

namespace Playlist {

class TPlaylistItem;


class TPlaylist : public TPList {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylist(TDockWidget* parent, TMainWindow* mw);

    Action::TAction* playNextAct;
    Action::TAction* playPrevAct;

    QString playingFile() const;
    QString getPlayingTitle(bool addModified = false,
                            bool useStreamingTitle = true) const;
    void getFilesToPlay(QStringList& files) const;
    bool hasPlayableItems() const;

    void openDisc(const TDiscName& disc);
    virtual void startPlay() override;

    virtual void loadSettings() override;
    virtual void saveSettings() override;

public slots:
    void open(const QString &fileName, const QString& name = QString());
    void openFiles(const QStringList& files, const QString& fileToPlay = "");
    void openFileDialog();
    void openDirectoryDialog();

    virtual void stop() override;
    void playOrPause();

    virtual void enableActions() override;
    virtual void findPlayingItem() override;

signals:
    void playlistFinished();

protected:
    virtual void dragEnterEvent(QDragEnterEvent*) override;
    virtual void dropEvent(QDropEvent*) override;

    virtual void clear(bool clearFilename = true) override;
    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) override;

protected slots:
    virtual void refresh() override;

private:
    Action::TAction* repeatAct;
    Action::TAction* shuffleAct;

    bool reachedEndOfPlaylist;
    QString dvdTitle;
    QString dvdSerial;

    void createActions();
    void createToolbar();

    TPlaylistItem* getRandomItem() const;
    bool haveUnplayedItems() const;

    void openDirectory(const QString& dir);

    void onNewMediaStartedPlayingUpdatePlayingItem();
    void updatePlayingItem();

private slots:
    void playNext(bool loop_playlist = true);
    void playPrev();

    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);

    void onPlayerError();
    void onNewMediaStartedPlaying();
    void onTitleTrackChanged(int id);
    void onDurationChanged(int ms);
    void onMediaEOF();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLIST_H


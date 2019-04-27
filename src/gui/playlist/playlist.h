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
#include "player/state.h"
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
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylist(TDockWidget* parent);

    QString playingFile() const;
    QString getPlayingTitle(bool addModified = false,
                            bool useStreamingTitle = true) const;
    void getFilesToPlay(QStringList& files) const;

    void openDisc(const TDiscName& disc);

    virtual void loadSettings() override;
    virtual void saveSettings() override;

public slots:
    void open(const QString &fileName, const QString& name = QString());
    void openFiles(const QStringList& files, const QString& fileToPlay = "");
    void openFileDialog();
    void openDirectoryDialog();

    virtual void stop() override;
    void playPause();

    virtual void enableActions() override;

signals:
    void playlistFinished();

protected:
    virtual void clear(bool clearFilename = true) override;
    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) override;

protected slots:
    virtual void refresh() override;

private:
    QString dvdTitle;
    QString dvdSerial;

    void createToolbar();

    bool haveUnplayedItems() const;

    void openDirectory(const QString& dir);

    void onNewFileStartedPlaying();
    bool onNewDiscStartedPlaying();

private slots:
    void onRepeatToggled(bool toggled);
    void onShuffleToggled(bool toggled);

    void onPlayerError();
    void onStateChanged(Player::TState state);
    void onNewMediaStartedPlaying();
    void onTitleTrackChanged(int id);
    void onDurationChanged(int ms);
    void onMediaEOF();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLIST_H


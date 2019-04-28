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

#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/playlistitem.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/msg.h"
#include "gui/filedialog.h"
#include "player/player.h"
#include "wzfiles.h"
#include "images.h"
#include "extensions.h"
#include "iconprovider.h"
#include "name.h"

#include <QToolBar>
#include <QMimeData>
#include <QMessageBox>


namespace Gui {

using namespace Action;

namespace Playlist {

TPlaylist::TPlaylist(TDockWidget* parent) :
    TPList(parent, "playlist", "pl", tr("Playlist")) {

    // Repeat
    repeatAct = new TAction(mainWindow, "pl_repeat", tr("Repeat playlist"),
                            "", Qt::CTRL | Qt::Key_Backslash);
    repeatAct->setCheckable(true);
    connect(repeatAct, &TAction::triggered, this, &TPlaylist::onRepeatToggled);

    // Shuffle
    shuffleAct = new TAction(mainWindow, "pl_shuffle", tr("Shuffle playlist"),
                             "shuffle", Qt::ALT | Qt::Key_Backslash);
    shuffleAct->setCheckable(true);
    connect(shuffleAct, &TAction::triggered,
            this, &TPlaylist::onShuffleToggled);

    createToolbar();

    connect(player, &Player::TPlayer::playerError,
            this, &TPlaylist::onPlayerError);
    connect(player, &Player::TPlayer::stateChanged,
            this, &TPlaylist::onStateChanged);
    connect(player, &Player::TPlayer::newMediaStartedPlaying,
            this, &TPlaylist::onNewMediaStartedPlaying);
    connect(player, &Player::TPlayer::titleTrackChanged,
            this, &TPlaylist::onTitleTrackChanged);
    connect(player, &Player::TPlayer::durationMSChanged,
            this, &TPlaylist::onDurationChanged);
    connect(player, &Player::TPlayer::mediaEOF,
            this, &TPlaylist::onMediaEOF, Qt::QueuedConnection);
    connect(player, &Player::TPlayer::noFileToPlay,
            this, &TPlaylist::startPlay, Qt::QueuedConnection);


    connect(playlistWidget, &TPlaylistWidget::nothingToPlay,
            this, &TPlaylist::onNothingToPlay,
            Qt::QueuedConnection);
    connect(playlistWidget, &TPlaylistWidget::latestDirChanged,
            this, &TPlaylist::onLatestDirChanged);

    // Random seed
    QTime t;
    t.start();
    qsrand(t.hour() * 3600 + t.minute() * 60 + t.second());
}

void TPlaylist::onNothingToPlay(QString msg) {
    QMessageBox::information(this, tr("Nothing to play"), msg);
}

void TPlaylist::onLatestDirChanged(QString dir) {
    Settings::pref->last_dir = dir;
}

void TPlaylist::onRootFilenameChanged(QString rootFilename) {

    TPList::onRootFilenameChanged(rootFilename);
    Settings::pref->addRecent(playlistFilename,
                              playlistWidget->root()->fname());
}

void TPlaylist::createToolbar() {

    QStringList actions;
    actions << "pl_open"
            << "pl_save"
            << "pl_save_as"
            << "separator"
            << "pl_add_menu"
            << "pl_remove_menu"
            << "separator"
            << "pl_refresh"
            << "pl_browse_dir"
            << "separator"
            << "pl_shuffle"
            << "pl_repeat";
    toolbar->setDefaultActions(actions);
}


void TPlaylist::getFilesToPlay(QStringList& files) const {

    TPlaylistItem* root = playlistWidget->root();
    if (root == 0) {
        return;
    }

    QString fn = root->filename();
    if (!fn.isEmpty()) {
        files.append(fn);
        return;
    }

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        files.append(i->filename());
        it++;
    }
}

void TPlaylist::stop() {
    WZINFO("State " + player->stateToString());

    TPList::stop();
    player->stop();
}

void TPlaylist::playItem(TPlaylistItem* item, bool keepPaused) {

    reachedEndOfPlaylist = false;
    while (item && item->isFolder()) {
        item = playlistWidget->getNextPlaylistItem(item);
    }
    if (item) {
        WZD << item->filename();
        playlistWidget->setPlayingItem(item, item->state());
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
            player->setStartPausedOnce();
        }
        player->open(item->filename(), getPlayingTitle(false, false));
    } else {
        WZDEBUG("End of playlist");
        stop();
        reachedEndOfPlaylist = true;
        msg(tr("End of playlist"), 0);
        emit playlistFinished();
    }
}

void TPlaylist::playPause() {
    WZDEBUG("State " + player->stateToString());

    switch (player->state()) {
        case Player::STATE_PLAYING: player->pause(); break;
        case Player::STATE_PAUSED: player->play(); break;

        case Player::STATE_STOPPING:
        case Player::STATE_RESTARTING:
        case Player::STATE_LOADING:
            break;

        case Player::STATE_STOPPED:
        default:
            playEx();
            break;
    }
}

bool TPlaylist::haveUnplayedItems() const {

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (!i->isFolder() && !i->played() && i->state() != PSTATE_FAILED) {
            return true;
        }
        ++it;
    }

    return false;
}

QString TPlaylist::playingFile() const {
    return playlistWidget->playingFile();
}

QString TPlaylist::getPlayingTitle(bool addModified,
                                   bool useStreamingTitle) const {

    bool isRoot = false;
    TPlaylistItem* item = playlistWidget->playingItem;
    if (!item) {
        item = playlistWidget->root();
        isRoot = true;
    }

    QString title;
    if (useStreamingTitle
            && player->mdat.detected_type == TMediaData::TYPE_STREAM
            && !player->mdat.title.isEmpty()) {
        title = player->mdat.name();
    } else {
        title = item->editName();

        // Add disc title, root already has it added
        if (item->isDisc() && !isRoot) {
            QString s = TName::cleanTitle(dvdTitle);
            if (!s.isEmpty()) {
                title = QString("%1 - %2").arg(s).arg(title);
            }
        }
    }

    if (addModified && item->modified()) {
        title += "*";
    }
    return title;
}

void TPlaylist::onStateChanged(Player::TState state) {

    TPlaylistItem* item = playlistWidget->playingItem;
    if (!item) {
        WZTRACE("No playing item");
        return;
    }
    WZTRACE(player->stateToString());

    switch (state) {
        case Player::STATE_STOPPED:
            if (item->state() != PSTATE_STOPPED && item->state() != PSTATE_FAILED) {
                playlistWidget->setPlayingItem(item, PSTATE_STOPPED);
            }
            break;
        case Player::STATE_PLAYING:
        case Player::STATE_PAUSED:
            if (item->state() != PSTATE_PLAYING) {
                playlistWidget->setPlayingItem(item, PSTATE_PLAYING);
            }
            break;
        case Player::STATE_STOPPING:
            break;
        case Player::STATE_RESTARTING:
            if (item->state() != PSTATE_LOADING) {
                playlistWidget->setPlayingItem(item, PSTATE_LOADING);
            }
            break;
        case Player::STATE_LOADING:
            // The player->open(url) call can change the URL. It sets
            // player->mdat.filename just before switching state to loading.
            // If we do not copy the url here, onNewMediaStartedPlaying() will
            // start a new playlist, clearing the current playlist.
            if (player->mdat.selected_type == TMediaData::TYPE_FILE
                    && !player->mdat.filename.isEmpty()
                    && player->mdat.filename != item->filename()
                    && !playlistWidget->hasSingleItem()) {
                WZWARN(QString("File name playing item '%1' does not match"
                               " file name player '%2' in state loading.")
                       .arg(item->filename()).arg(player->mdat.filename));

                // Savest to just start a new playlist. Alternatively:
                // item->setFilename(player->mdat.filename);
                // item->setModified();
            }

            if (item->state() != PSTATE_LOADING) {
                playlistWidget->setPlayingItem(item, PSTATE_LOADING);
            }
            break;
    } // switch (state)
}

void TPlaylist::enableActions() {

    if (disableEnableActions) {
        WZTRACE("Disabled");
        return;
    }
    disableEnableActions++;

    WZTRACE("State " + player->stateToString());
    findPlayingAct->setEnabled(playlistWidget->playingItem);
    TPList::enableActions();

    disableEnableActions--;
}

void TPlaylist::onPlayerError() {

    if (player->state() != Player::STATE_STOPPING) {
        TPlaylistItem* item = playlistWidget->playingItem;
        if (item && item->filename() == player->mdat.filename) {
            playlistWidget->setPlayingItem(item, PSTATE_FAILED);
            playlistWidget->setCurrentItem(item);
            playlistWidget->scrollToItem(item);
        }
    }
}

void TPlaylist::clear(bool clearFilename) {

    dvdTitle = "";
    dvdSerial = "";
    TPList::clear(clearFilename);
}

void TPlaylist::onNewFileStartedPlaying() {

    TPlaylistItem* playingItem = playlistWidget->playingItem;
    if (!playingItem) {
        return;
    }
    const TMediaData* md = &player->mdat;
    bool modified = false;

    // Update item name
    if (!playingItem->edited()) {
        QString name = md->name();
        if (name != playingItem->baseName()) {
            WZDEBUG(QString("Updating name from '%1' to '%2'")
                    .arg(playingItem->baseName()).arg(name));
            playingItem->setName(name, playingItem->extension(), false);
            modified = true;
        }
    }

    // Update item duration
    if (md->duration_ms > 0
            && !md->image
            && md->duration_ms != playingItem->durationMS()) {
        WZDEBUG(QString("Updating duration from %1 ms to %2 ms")
                .arg(playingItem->durationMS()).arg(md->duration_ms));
        if (qAbs(md->duration_ms - playingItem->durationMS())
                > TConfig::DURATION_MODIFIED_TRESHOLD) {
            modified = true;
        }
        playingItem->setDurationMS(md->duration_ms);
    }

    if (playlistWidget->hasSingleItem()) {
        playlistWidget->clearModified();
    } else if (modified) {
        playingItem->setModified();
    } else {
        WZDEBUG("Item considered uptodate");
    }

    // Could set state playingItem to PSTATE_PLAYING now, but wait for player
    // to change state to playing to not trigger additional calls to
    // enableActions().
    // playlistWidget->setPlayingItem(playingItem, PSTATE_PLAYING);

    return;
}

bool TPlaylist::onNewDiscStartedPlaying() {

    if (playlistWidget->playingItem == 0) {
        WZDEBUG("No playing item");
        return false;
    }

    const TMediaData* md = &player->mdat;
    TDiscName curDisc(playlistWidget->playingItem->filename());
    if (curDisc.protocol == "dvdnav" && Settings::pref->isMPV()) {
        curDisc.protocol = "dvd";
    }
    if (curDisc.valid
            && curDisc.title != 0
            && curDisc.protocol == md->disc.protocol
            && curDisc.device == md->disc.device) {

        if (!dvdSerial.isEmpty()) {
            if (dvdSerial == md->dvd_disc_serial) {
                WZD << "Item is from current disc with serial" << dvdSerial;
                return true;
            }
            WZD << "Serial player" << md->dvd_disc_serial
                << "mismatches serial playlist" << dvdSerial
                << "- Loading new disc playlist";
            return false;
        }

        // No serial
        if (!dvdTitle.isEmpty()) {
            if (dvdTitle == md->title) {
                WZD << "Item is from current disc with no serial and matching"
                       " title" << dvdTitle;
                return true;
            }
            WZD << "Title player" << md->title
                << "mismatches title from playlist" << dvdTitle
                << "- Loading new disc playlist";
            return false;
            }

        // No serial, no title
        if (md->titles.count() == playlistWidget->countItems()) {
            WZDEBUG("Assuming item is from current disc without serial and"
                    " without title");
            return true;
        }

        WZDEBUG("Title counts mismatch - Loading new disc playlist");
        return false;
    }

    WZDEBUG("Playing item is not a disc - Loading new disc playlist");
    return false;
}

void TPlaylist::onNewMediaStartedPlaying() {

    const TMediaData* md = &player->mdat;
    QString filename = md->filename;

    if (md->disc.valid) {
        if (onNewDiscStartedPlaying()) {
            return;
        }
    } else if (playlistWidget->playingItem
               && filename == playlistWidget->playingItem->filename()) {
        // Handle current item started playing
        onNewFileStartedPlaying();
        // Pause a single image
        if (player->mdat.image && playlistWidget->hasSingleItem()) {
            mainWindow->runActionsLater("pause", false, true);
        }
        return;
    } else {
        // Find it in playlist?
        // No for play in new window with single instance.
        // Yes for preserving current playlist whenever possible.
    }

    // Create new playlist
    WZT << "Creating new playlist for" << filename;
    clear();

    if (md->disc.valid) {
        // Save title and serial to catch changed disc. Cleared by clear().
        dvdTitle = md->title;
        dvdSerial = md->dvd_disc_serial;

        // Add disc titles without sorting
        playlistWidget->disableSort();
        TPlaylistItem* root = playlistWidget->root();
        root->setFilename(filename, md->name());
        // setFilename() won't mark discs and isos as folder (they will not
        // play if marked as folder). Mark them as folder here.
        // No need to update icon as it is not visible.
        root->setFolder(true);

        TDiscName disc = md->disc;
        // Add menu for DVDNAV
        if (disc.disc() == TDiscName::DVDNAV) {
            disc.title = -1;
            TPlaylistItem* item = new TPlaylistItem(root,
                                                    disc.toString(true),
                                                    disc.displayName(),
                                                    0, // duration
                                                    false /* protext name */);
            if (md->titles.getSelectedID() < 0) {
                item->setDurationMS(md->duration_ms);
                playlistWidget->setPlayingItem(item, PSTATE_PLAYING);
            }
        }

        foreach(const Maps::TTitleData& title, md->titles) {
            disc.title = title.getID();
            TPlaylistItem* i = new TPlaylistItem(root,
                                                 disc.toString(false),
                                                 title.getDisplayName(false),
                                                 title.getDuration(),
                                                 false /* protext name */);
            if (title.getID() == md->titles.getSelectedID()) {
                playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
            }
        } // if (md->disc.valid)
    } else {
        // Add current file
        TPlaylistItem* current = new TPlaylistItem(playlistWidget->root(),
                                                   filename,
                                                   md->name(),
                                                   md->duration_ms,
                                                   false);
        playlistWidget->setPlayingItem(current, PSTATE_PLAYING);
    }

    setPLaylistTitle();
    WZTRACE("Created new playlist for '" + filename + "'");
}

void TPlaylist::onMediaEOF() {
    WZTRACE("");
    playNext(false);
}

void TPlaylist::onTitleTrackChanged(int id) {
    WZTRACE(QString::number(id));

    // Search for id
    TDiscName disc = player->mdat.disc;
    disc.title = id;
    QString filename = disc.toString(true);

    TPlaylistItem* item = playlistWidget->findFilename(filename);
    if (item) {
        if (player->mdat.duration_ms > 0 || id == -1) {
            item->setDurationMS(player->mdat.duration_ms);
        }
        playlistWidget->setPlayingItem(item, PSTATE_PLAYING);
    } else {
        WZWARN(QString("Title id %1 with filename '%2' not found in playlist")
               .arg(id).arg(filename));
    }
}

void TPlaylist::onDurationChanged(int ms) {

    TPlaylistItem* item = playlistWidget->playingItem;
    if (item) {
        if (item->isDisc()) {
            int selectedTitle = player->mdat.titles.getSelectedID();
            int itemTitle = TDiscName(item->filename()).title;
            if (selectedTitle == itemTitle) {
                if (ms > 0 || itemTitle == -1) {
                    WZTRACE(QString("Updating duration title %1 with name '%2'"
                                    " from %3 ms to %4 ms")
                            .arg(itemTitle)
                            .arg(item->baseName())
                            .arg(item->durationMS())
                            .arg(ms));
                    item->setDurationMSEmit(ms);
                } else {
                    WZTRACE(QString("Keeping duration item title %1 with name"
                                    " '%2' and duration %3 ms instead of %4 ms")
                            .arg(itemTitle)
                            .arg(item->baseName())
                            .arg(item->durationMS())
                            .arg(ms));
                }
            } else {
                WZTRACE(QString("Selected title %1 does not match item title %2"
                                " with name '%3'")
                        .arg(selectedTitle)
                        .arg(itemTitle)
                        .arg(item->baseName()));
            }
        } else if (ms > 0 && !player->mdat.image) {
            WZTRACE(QString("Updating duration '%1' from %2 ms to %3 ms")
                    .arg(item->baseName()).arg(item->durationMS()).arg(ms));
            if (qAbs(ms - item->durationMS())
                    > TConfig::DURATION_MODIFIED_TRESHOLD) {
                item->setModified();
            }
            item->setDurationMSEmit(ms);
        } else {
            WZTRACE(QString("Keeping duration %1 ms instead of %2 ms for '%3'")
                    .arg(item->durationMS())
                    .arg(ms)
                    .arg(item->baseName()));
        }
    } else {
        WZTRACE("No playing item");
    }
}

void TPlaylist::refresh() {

    if (!playlistFilename.isEmpty() && maybeSave()) {
        QString playingFile;
        if (player->statePOP()) {
            playingFile = playlistWidget->playingFile();
            if (!playingFile.isEmpty()) {
                player->saveRestartState();
            }
        }
        clear(false);
        playlistWidget->addFiles(QStringList() << playlistFilename,
                                 0, // target
                                 -1, // target index
                                 !playingFile.isEmpty(), // start playing
                                 playingFile);
    }
}

void TPlaylist::open(const QString &fileName, const QString& name) {
    WZDEBUG("'" + fileName + "'");

    if (fileName.isEmpty()) {
        WZDEBUG("Nothing to play");
        return;
    }
    if (!maybeSave()) {
        return;
    }

    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            openDirectory(fi.absoluteFilePath());
            return;
        }
        if (extensions.isPlaylist(fi)) {
            openPlaylist(fi.absoluteFilePath());
            return;
        }
        Settings::pref->last_dir = fi.absolutePath();
    }

    // Find in playlist
    TPlaylistItem* item = playlistWidget->findFilename(fileName);
    if (item) {
        WZDEBUG("Found item in playlist");
        playItem(item);
        return;
    }

    WZDEBUG("Starting new playlist");
    clear();
    bool edited = !name.isEmpty() && name != TName::nameForURL(fileName);
    playItem(new TPlaylistItem(playlistWidget->root(),
                               fileName,
                               name,
                               0,
                               edited));
    WZDEBUG("Done");
}

void TPlaylist::openFileDialog() {
    WZDEBUG("");

    QString s = TFileDialog::getOpenFileName(
        this,
        tr("Choose a file"),
        Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;"
        + tr("Video") + extensions.video().forFilter() + ";;"
        + tr("Audio") + extensions.audio().forFilter() + ";;"
        + tr("Playlists") + extensions.playlists().forFilter() + ";;"
        + tr("Images") + extensions.images().forFilter() + ";;"
        + tr("All files") +" (*.*)");

    if (!s.isEmpty()) {
        open(s);
    }
}


void TPlaylist::openFiles(const QStringList& files, const QString& fileToPlay) {
    WZDEBUG("");

    if (files.empty()) {
        WZDEBUG("No files in list to open");
        return;
    }

    if (maybeSave()) {
        clear();
        playlistWidget->addFiles(files, 0, -1, true, fileToPlay);
    }
}

void TPlaylist::openDirectory(const QString& dir) {
    WZDEBUG("'" + dir + "'");

    Settings::pref->last_dir = dir;

    if (TWZFiles::directoryContainsDVD(dir)) {
        // Calls clear()
        openDisc(TDiscName(dir, Settings::pref->useDVDNAV()));
    } else {
        clear();
        playlistWidget->addFiles(QStringList() << dir, 0, -1, true);
    }
}

void TPlaylist::openDirectoryDialog() {
    WZDEBUG("");

    if (maybeSave()) {
        QString s = TFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"), Settings::pref->last_dir);
        if (!s.isEmpty()) {
            openDirectory(s);
        }
    }
}

void TPlaylist::openDisc(const TDiscName& disc) {

    if (maybeSave()) {
        clear();
        player->openDisc(disc);
    }
}

void TPlaylist::onRepeatToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Repeat playlist set"));
    else
        msgOSD(tr("Repeat playlist cleared"));
}

void TPlaylist::onShuffleToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Shuffle playlist set"));
    else
        msgOSD(tr("Shuffle playlist cleared"));
}

void TPlaylist::saveSettings() {

    TPList::saveSettings();
    Settings::pref->beginGroup(objectName());
    Settings::pref->setValue("repeat", repeatAct->isChecked());
    Settings::pref->setValue("shuffle", shuffleAct->isChecked());
    Settings::pref->endGroup();
}

void TPlaylist::loadSettings() {

    TPList::loadSettings();
    Settings::pref->beginGroup(objectName());
    repeatAct->setChecked(
        Settings::pref->value("repeat", repeatAct->isChecked()).toBool());
    shuffleAct->setChecked(
        Settings::pref->value("shuffle", shuffleAct->isChecked()).toBool());
    Settings::pref->endGroup();
}

} // namespace Playlist
} // namespace Gui

#include "moc_playlist.cpp"

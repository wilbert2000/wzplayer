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


/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
 * Copyright 2012  Bernd Buschinski <b.buschinski@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include "player/mpris2/mediaplayer2player.h"
#include "player/mpris2/mpris2.h"
#include "player/player.h"
#include "gui/playlist/playlist.h"
#include "gui/mainwindow.h"

#include <QCryptographicHash>


namespace Player {
namespace Mpris2 {

static QByteArray makeTrackId(const QString& source) {

    return ("/org/" + TConfig::PROGRAM_ID + "/" + TConfig::PROGRAM_ID
            + "/tid_").toLocal8Bit()
            + QCryptographicHash::hash(source.toLocal8Bit(),
                                       QCryptographicHash::Sha1).toHex();
}


TMediaPlayer2Player::TMediaPlayer2Player(Gui::TMainWindow* gui, QObject* parent)
    : QDBusAbstractAdaptor(parent),
    m_playlist(gui->getPlaylist()) {

    connect(player, SIGNAL(stateChanged(Player::TState)),
            this, SLOT(stateUpdated()));
    connect(player, SIGNAL(mediaInfoChanged()),
            this, SLOT(emitMetadataChange()));
    connect(player, SIGNAL(volumeChanged(int)),
            this, SLOT(volumeChanged()));
}

TMediaPlayer2Player::~TMediaPlayer2Player() {
}

bool TMediaPlayer2Player::CanGoNext() const {
    return true;
}

void TMediaPlayer2Player::Next() const {
    m_playlist->playNext();
}

bool TMediaPlayer2Player::CanGoPrevious() const {
    return true;
}

void TMediaPlayer2Player::Previous() const {
    m_playlist->playPrev();
}

bool TMediaPlayer2Player::CanPause() const {
    return true;
}

void TMediaPlayer2Player::Pause() const {
    player->pause();
}

void TMediaPlayer2Player::PlayPause() const {
    player->playOrPause();
}

void TMediaPlayer2Player::Stop() const {
    player->stop();
}

bool TMediaPlayer2Player::CanPlay() const {
    return true;
}

void TMediaPlayer2Player::Play() const {
    player->play();
}

void TMediaPlayer2Player::SetPosition(const QDBusObjectPath& TrackId,
                                     qlonglong Position) const {

    if (TrackId.path().toLocal8Bit() == makeTrackId(player->mdat.filename))
        player->seekTime(Position / 1000000);
}

void TMediaPlayer2Player::OpenUri(QString uri) const {
    player->open(uri);
}

QString TMediaPlayer2Player::PlaybackStatus() const {
    return player->stateToString();
}

QString TMediaPlayer2Player::LoopStatus() const {
    return "None";
}

void TMediaPlayer2Player::setLoopStatus(const QString& loopStatus) const {
    Q_UNUSED(loopStatus)
}

double TMediaPlayer2Player::Rate() const {
    return player->mset.speed;
}

void TMediaPlayer2Player::setRate(double rate) const {
    player->setSpeed(rate);
}

bool TMediaPlayer2Player::Shuffle() const {
    return false;
}

void TMediaPlayer2Player::setShuffle(bool shuffle) const {
    Q_UNUSED(shuffle)
}

QVariantMap TMediaPlayer2Player::Metadata() const {

    QVariantMap metaData;

    if (!player->mdat.initialized)
        return metaData;

    TMediaData* md = &player->mdat;
    metaData["mpris:trackid"] = QVariant::fromValue<QDBusObjectPath>(
        QDBusObjectPath(makeTrackId(md->filename).constData()));
    metaData["mpris:length"] = md->duration * 1000000;

	if (md->selected_type == TMediaData::TYPE_STREAM)
		metaData["xesam:url"] = md->stream_url;
	else
		metaData["xesam:url"] = md->filename;

	if (md->meta_data.contains("album"))
		metaData["xesam:album"] = md->meta_data["album"];
	if (!md->title.isEmpty())
		metaData["xesam:title"] = md->title;
	else if (md->meta_data.contains("title"))
		metaData["xesam:title"] = md->meta_data["title"];
	else if (md->meta_data.contains("name"))
		metaData["xesam:title"] = md->meta_data["name"];
	else if (!md->filename.isEmpty()) {
		QFileInfo fileInfo(md->filename);
		metaData["xesam:title"] = fileInfo.fileName();
	}
	if (md->meta_data.contains("artist"))
		metaData["xesam:artist"] = md->meta_data["artist"];
	if (md->meta_data.contains("genre"))
		metaData["xesam:genre"] = md->meta_data["genre"];

	return metaData;
}

double TMediaPlayer2Player::Volume() const {
    return static_cast<double>(player->getVolume() / 100.0);
}

void TMediaPlayer2Player::setVolume(double volume) const {
    player->setVolume(static_cast<int>(volume*100));
}

qlonglong TMediaPlayer2Player::Position() const {
    return static_cast<qlonglong>(player->mset.current_sec * 1000000);
}

double TMediaPlayer2Player::MinimumRate() const {
    return 0.01;
}

double TMediaPlayer2Player::MaximumRate() const {
    return 100.0;
}

bool TMediaPlayer2Player::CanSeek() const {
    return true;
}

void TMediaPlayer2Player::Seek(qlonglong Offset) const {
    player->seekRelative(Offset / 1000000);
}

bool TMediaPlayer2Player::CanControl() const {
    return true;
}

void TMediaPlayer2Player::tick(qint64 newPos) {

//   if (newPos - oldPos > tickInterval + 250 || newPos < oldPos)
//       emit Seeked(newPos * 1000);

    oldPos = newPos;
}

void TMediaPlayer2Player::emitMetadataChange() const {

    QVariantMap properties;
    properties["Metadata"] = Metadata();
	properties["CanSeek"] = CanSeek();
	TMpris2::signalPropertiesChange(this, properties);
}

void TMediaPlayer2Player::stateUpdated() const {

    QVariantMap properties;
    properties["PlaybackStatus"] = PlaybackStatus();
    properties["CanPause"] = CanPause();
    TMpris2::signalPropertiesChange(this, properties);
}

void TMediaPlayer2Player::totalTimeChanged() const {

    QVariantMap properties;
    properties["Metadata"] = Metadata();
    TMpris2::signalPropertiesChange(this, properties);
}

void TMediaPlayer2Player::seekableChanged(bool seekable) const {

    QVariantMap properties;
    properties["CanSeek"] = seekable;
    TMpris2::signalPropertiesChange(this, properties);
}

void TMediaPlayer2Player::volumeChanged() const {

    QVariantMap properties;
    properties["Volume"] = Volume();
    TMpris2::signalPropertiesChange(this, properties);
}

} // namespace Mpris2
} // namespace Player

#include "moc_mediaplayer2player.cpp"

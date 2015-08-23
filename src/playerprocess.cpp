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

#include "playerprocess.h"
#include <QFileInfo>
#include <QDebug>

#ifdef MPV_SUPPORT
#include "mpvprocess.h"
#endif

#ifdef MPLAYER_SUPPORT
#include "mplayerprocess.h"
#endif


PlayerProcess::PlayerProcess(MediaData *mdata, QRegExp *r_eof)
	: MyProcess(0)
	, md(mdata)
	, notified_player_is_running(false)
	, video_tracks_changed(false)
	, audio_tracks_changed(false)
	, subtitle_tracks_changed(false)
	, fps()
	, line_count(0)
	, received_end_of_file(false)
	, rx_eof(r_eof)
{
	qRegisterMetaType<SubTracks>("SubTracks");
	qRegisterMetaType<Tracks>("Tracks");
	qRegisterMetaType<Chapters>("Chapters");

	connect( this, SIGNAL(error(QProcess::ProcessError)),
			 this, SLOT(gotError(QProcess::ProcessError)) );

	connect( this, SIGNAL(finished(int,QProcess::ExitStatus)),
			 this, SLOT(processFinished(int,QProcess::ExitStatus)) );

	connect( this, SIGNAL(lineAvailable(QByteArray)),
			 this, SLOT(parseBytes(QByteArray)) );
}

void PlayerProcess::writeToStdin(QString text) {
	if (isRunning()) {
		qDebug("PlayerProcess::writeToStdin: %s", text.toUtf8().constData());
		#ifdef Q_OS_WIN
		write( text.toUtf8() + "\n");
		#else
		write( text.toLocal8Bit() + "\n");
		#endif
	} else {
		qWarning("PlayerProcess::writeToStdin: process not running");
	}
}

PlayerProcess * PlayerProcess::createPlayerProcess(const QString &player_bin, MediaData *md) {

	PlayerProcess * proc = 0;

#if defined(MPV_SUPPORT) && defined(MPLAYER_SUPPORT)
	if (PlayerID::player(player_bin) == PlayerID::MPLAYER) {
		qDebug() << "PlayerProcess::createPlayerProcess: creating MplayerProcess";
		proc = new MplayerProcess(md);
	} else {
		qDebug() << "PlayerProcess::createPlayerProcess: creating MPVProcess";
		proc = new MPVProcess(md);
	}
#else
	#ifdef MPV_SUPPORT
	proc = new MPVProcess(md);
	#endif
	#ifdef MPLAYER_SUPPORT
	proc = new MplayerProcess(md);
	#endif
#endif

	return proc;
}

bool PlayerProcess::startPlayer() {

	prev_line = "";

	notified_player_is_running = false;

	waiting_for_answers = 0;
	waiting_for_answers_safe_guard = 50;

	received_end_of_file = false;

	dwidth = 0;
	dheight = 0;

	fps = 0.0;
	prev_frame = -1;

	md->reset();

	// Start the player process
	start();
	// and wait for it to come up
	return waitForStarted();
}

void PlayerProcess::gotError(QProcess::ProcessError error) {
	qWarning("PlayerProcess::gotError: %d", (int) error);
}

// Slot called when the process is finished
void PlayerProcess::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	qDebug("PlayerProcess::processFinished: exitCode: %d, status: %d", exitCode, (int) exitStatus);

	// Send this signal before the endoffile one, otherwise
	// the playlist will start to play next file before all
	// objects are notified that the process has exited.
	emit processExited();
	if (received_end_of_file)
		emit receivedEndOfFile();
}

// Convert line of bytes to QString.
void PlayerProcess::parseBytes(QByteArray ba) {

#if COLOR_OUTPUT_SUPPORT
	QString line = ColorUtils::stripColorsTags(QString::fromLocal8Bit(ba));
#else
	#ifdef Q_OS_WIN
	QString line = QString::fromUtf8(ba);
	#else
	QString line = QString::fromLocal8Bit(ba);
	#endif
#endif

	// Parse QString
	if (!parseLine(line)) {
		qDebug("PlayerProcess::parseBytes: ignored");
	}

	line_count++;
	if (line_count % 5000 == 0) {
		qDebug("PlayerProcess::parseBytes: parsed %'d lines", line_count);
	}
}

void PlayerProcess::notifyChanges() {

	// Only called for changes after fully loaded

	if (video_tracks_changed) {
		video_tracks_changed = false;
		qDebug("PlayerProcess::notifyChanges: emit videoTracksChanged");
		emit videoTracksChanged();
	}
	if (audio_tracks_changed) {
		audio_tracks_changed = false;
		qDebug("PlayerProcess::notifyChanges: emit audioTracksChanged");
		emit audioTracksChanged();
	}
	if (subtitle_tracks_changed) {
		subtitle_tracks_changed = false;
		qDebug("PlayerProcess::notifyChanges: emit subtitleTracksChanged");
		emit subtitleTracksChanged();
	}
}

bool PlayerProcess::waitForAnswers() {

	if (waiting_for_answers > 0) {
		waiting_for_answers_safe_guard--;
		if (waiting_for_answers_safe_guard > 0)
			return true;

		qWarning("MplayerProcess::waitForAnswers: did not receive answers in time. Stopped waitng.");
		waiting_for_answers = 0;
	}

	return false;
}

void PlayerProcess::playingStarted() {

	notified_player_is_running = true;

	video_tracks_changed = false;
	audio_tracks_changed = false;
	subtitle_tracks_changed = false;

	// Have any video?
	if (dwidth <= 0) {
		md->novideo = true;
		qDebug("PlayerProcess::startPlaying: emit receivedNoVideo()");
		emit receivedNoVideo();
	} else {
		qDebug("PlayerProcess::startPlaying: emit receivedWindowResolution()");
		emit receivedWindowResolution(dwidth, dheight);
	}

	qDebug("PlayerProcess::startPlaying: emit playerFullyLoaded()");
	emit playerFullyLoaded();

	// Clear frame counter if no fps
	if (fps == 0.0) {
		emit receivedCurrentFrame(0);
	}
}

bool PlayerProcess::parseLine(QString &line) {

	// Trim line
	line = line.trimmed();
	if (line.isEmpty())
		return true;

	// Protect against spurious messages.
	// There is danger in this...
	// Scenario:
	// Query
	// Answer
	// Play a little while
	// Same query
	// Oeps, answer eaten here
	// Moreover, letting them through might alert someone to
	// do something about them...
	if (line == prev_line)
		return true;
	prev_line = line;

	qDebug("PlayerProcess::parseLine: '%s'", line.toUtf8().data() );

	emit lineAvailable(line);

	// End of file
	if (rx_eof->indexIn(line) > -1)  {
		qDebug("PlayerProcess::parseLine: detected end of file");

		// TODO:
		if (!received_end_of_file) {
			received_end_of_file = true;

			// In case of playing VCDs or DVDs, maybe the first title
			// is not playable, so the GUI doesn't get the info about
			// available titles. So if we received the end of file
			// first let's pretend the file has started so the GUI can have
			// the data.
			if ( !notified_player_is_running) {
				notified_player_is_running = true;
				emit playerFullyLoaded();
			}

			// Send signal in processFinished(),
			// once the process is finished, not now!
			// emit receivedEndOfFile();
		}
		return true;
	}

	return false;
}

bool PlayerProcess::parseAudioProperty(const QString &name, const QString &value) {

	if (name == "BITRATE") {
		md->audio_bitrate = value.toInt();
		qDebug("PlayerProcess::parseAudioProperty: audio_bitrate set to %d", md->audio_bitrate);
		emit receivedAudioBitrate(md->audio_bitrate);
		return true;
	}
	if (name == "FORMAT") {
		md->audio_format = value;
		qDebug() << "PlayerProcess::parseAudioProperty: audio_format set to" << md->audio_format;
		return true;
	}
	if (name == "RATE") {
		md->audio_rate = value.toInt();
		qDebug("PlayerProcess::parseAudioProperty: audio_rate set to %d", md->audio_rate);
		return true;
	}
	if (name == "NCH") {
		md->audio_nch = value.toInt();
		qDebug("PlayerProcess::parseAudioProperty: audio_nch set to %d", md->audio_nch);
		return true;
	}
	if (name == "CODEC") {
		md->audio_codec = value;
		qDebug() << "PlayerProcess::parseAudioProperty: audio_codec set to" << md->audio_codec;
		return true;
	}

	return false;
}

bool PlayerProcess::parseVideoProperty(const QString &name, const QString &value) {

	if (name == "WIDTH") {
		md->video_width = value.toInt();
		qDebug("PlayerProcess::parseVideoProperty: video_width set to %d", md->video_width);
		return true;
	}
	if (name == "HEIGHT") {
		md->video_height = value.toInt();
		qDebug("PlayerProcess::parseVideoProperty: video_height set to %d", md->video_height);
		return true;
	}
	if (name == "ASPECT") {
		md->video_aspect = value.toDouble();
		if (md->video_aspect == 0.0 && md->video_height != 0) {
			md->video_aspect = (double) md->video_width / md->video_height;
		}
		qDebug("PlayerProcess::parseVideoProperty: video_aspect set to %f", md->video_aspect);
		return true;
	}
	if (name == "FPS") {
		md->video_fps = value;
		fps = value.toDouble();
		qDebug() << "PlayerProcess::parseVideoProperty: video_fps set to" << fps;
		return true;
	}
	if (name == "BITRATE") {
		md->video_bitrate = value.toInt();
		qDebug("PlayerProcess::parseVideoProperty: video_bitrate set to %d", md->video_bitrate);
		emit receivedVideoBitrate(md->video_bitrate);
		return true;
	}
	if (name == "FORMAT") {
		md->video_format = value;
		qDebug() << "PlayerProcess::parseVideoProperty: video_format set to" << md->video_format;
		return true;
	}
	if (name == "CODEC") {
		md->video_codec = value;
		qDebug() << "PlayerProcess::parseVideoProperty: video_codec set to" << md->video_codec;
		return true;
	}

	return false;
}

bool PlayerProcess::parseMetaDataProperty(const QString &name, const QString &value) {

	if (value.isEmpty())
		return false;

	if (name == "TITLE") {
		md->clip_name = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_name set to" << md->clip_name;
		return true;
	}
	if (name == "ARTIST") {
		md->clip_artist = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_artist set to" << md->clip_artist;
		return true;
	}
	if (name == "ALBUM") {
		md->clip_album = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_album set to" << md->clip_album;
		return true;
	}
	if (name == "GENRE") {
		md->clip_genre = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_genre set to" << md->clip_genre;
		return true;
	}
	if (name == "DATE") {
		md->clip_date = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_date set to" << md->clip_date;
		return true;
	}
	if (name == "TRACK") {
		md->clip_track = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_track set to" << md->clip_track;
		return true;
	}
	if (name == "COPYRIGHT") {
		md->clip_copyright = value;
		qDebug() << "PlayerProcess::parseMetaDataProperty: clip_copyright set to" << md->clip_copyright;
		return true;
	}

	return false;
}

bool PlayerProcess::parseProperty(const QString &name, const QString &value) {

	if (name == "LENGTH") {
		double duration = value.toDouble();
		if (qAbs(duration - md->duration) > 0.001) {
			md->duration = duration;
			qDebug("PlayerProcess::parseProperty: emit durationChanged");
			emit durationChanged(duration);
		}
		return true;
	}
	if (name == "DEMUXER") {
		md->demuxer = value;
		qDebug() << "PlayerProcess::parseProperty: demuxer set to" << md->demuxer;
		return true;
	}
	if (name == "CHAPTERS") {
		md->n_chapters = value.toInt();

#ifdef TOO_CHAPTERS_WORKAROUND
		if (md->n_chapters > 1000) {
			qWarning("PlayerProcess::parseProperty: ignoring too many chapters: %d", md->n_chapters);
			md->n_chapters = 0;
		}
#endif

		qDebug("PlayerProcess::parseProperty: n_chapters set to %d", md->n_chapters);
		return true;
	}

	return false;
}



#include "moc_playerprocess.cpp"

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


PlayerProcess::PlayerProcess(PlayerID::Player pid, MediaData *mdata, QRegExp *r_eof)
	: MyProcess(0)
	, player_id(pid)
	, md(mdata)
	, notified_player_is_running(false)
	, video_tracks_changed(false)
	, audio_tracks_changed(false)
	, subtitle_tracks_changed(false)
	, line_count(0)
	, received_end_of_file(false)
	, rx_eof(r_eof)
{
	qRegisterMetaType<SubTracks>("SubTracks");
	qRegisterMetaType<Tracks>("Tracks");
	qRegisterMetaType<Chapters>("Chapters");

	connect( this, SIGNAL(error(QProcess::ProcessError)),
			 this, SLOT(processError(QProcess::ProcessError)) );

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

	notified_player_is_running = false;

	waiting_for_answers = 0;
	waiting_for_answers_safe_guard = 50;

	received_end_of_file = false;

	prev_frame = -11111;

	// Clear media data, false don't clear filename and type
	md->reset(false);

	// Start the player process
	start();
	// and wait for it to come up
	return waitForStarted();
}

void PlayerProcess::processError(QProcess::ProcessError error) {
	qWarning("PlayerProcess::processError: %d", (int) error);

}

// Slot called when the process is finished
void PlayerProcess::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	qDebug("PlayerProcess::processFinished: exitCode: %d, status: %d", exitCode, (int) exitStatus);

	// Send this signal before the endoffile one, otherwise
	// the playlist will start to play next file before all
	// objects are notified that the process has exited.

	emit processExited(exitCode == 0 && exitStatus == QProcess::NormalExit);

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

	// Clear notifications
	video_tracks_changed = false;
	audio_tracks_changed = false;
	subtitle_tracks_changed = false;

	// emit resolution unqueued
	qDebug("PlayerProcess::playingStarted: emit receivedVideoOutResolution()");
	emit receivedVideoOutResolution(md->video_out_width, md->video_out_height);

	// emit playerFullyLoaded queued
	qDebug("PlayerProcess::playingStarted: queued emit playerFullyLoaded()");
	emit playerFullyLoaded();
}

void PlayerProcess::notifyDuration(double duration) {

	// Duration changed?
	if (duration > 0 && qAbs(duration - md->duration) > 0.001) {
		qDebug("MPVProcess::notifyDuration: duration changed from %f to %f", md->duration, duration);
		md->duration = duration;
		qDebug("MPVProcess::notifyDuration: emit durationChanged(%f)", duration);
		emit durationChanged(duration);
	}
}

void PlayerProcess::correctDuration(double sec) {
	Q_UNUSED(sec)
	// Only needed by mplayer.
}

 // 2^33 / 90 kHz
static const double ts_rollover = 8589934592.0 / 90000.0;

double PlayerProcess::guiTimeToPlayerTime(double sec) {

	sec += md->start_sec;

	// Handle MPEG-TS PTS timestamp rollover
	if (sec >= ts_rollover && md->demuxer == "mpegts") {
		sec -= ts_rollover;
	}

	return sec;
}

void PlayerProcess::notifyTime(double time_sec, const QString &line) {

	// Store video timestamp
	md->time_sec = time_sec;

	// Substract start time grounding start time at 0
	time_sec -= md->start_sec;

	// Handle MPEG-TS PTS timestamp rollover
	if (time_sec < 0 && md->demuxer == "mpegts") {
		time_sec += ts_rollover;
	}

	// Only for mplayer
	correctDuration(time_sec);

	// Pass timestamp to GUI
	emit receivedCurrentSec(time_sec);

	// Ask children for frame
	int frame = getFrame(time_sec, line);

	// Pass frame to GUI
	if (frame != prev_frame) {
		prev_frame = frame;
		emit receivedCurrentFrame( frame );
	}
}

bool PlayerProcess::parseStatusLine(double time_sec, double duration, QRegExp &rx, QString &line) {
	Q_UNUSED(rx)

	// Store timestamp of first status line
	if (!md->start_sec_set) {
		md->start_sec_set = true;
		if (md->start_sec_prop_set) {
			md->start_sec = md->start_sec_prop;
			qDebug("PlayerProcess::parseStatusLine: selected start time container %f (status %f)",
				   md->start_sec, time_sec);
		} else {
			md->start_sec = time_sec;
			qDebug("PlayerProcess::parseStatusLine: selected start time status %f", time_sec);
		}
	}

	// Any pending questions?
	// Cancel the remainder of this line and get the answers.
	if (waitForAnswers())
		return true;

	notifyDuration(duration);

	notifyTime(time_sec, line);

	// Parse the line just a litlle bit longer
	return false;
}

bool PlayerProcess::parseLine(QString &line) {

	// Trim line
	line = line.trimmed();
	if (line.isEmpty())
		return true;

	// Output line to console
	qDebug("PlayerProcess::parseLine: '%s'", line.toUtf8().data() );

	// Output line to logs
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

		// Done looking at this line
		return true;
	}

	// Like to be parsed a little longer
	return false;
}

bool PlayerProcess::parseAudioProperty(const QString &name, const QString &value) {

	if (name == "BITRATE") {
		md->audio_bitrate = value.toInt();
		qDebug("PlayerProcess::parseAudioProperty: audio_bitrate set to %d", md->audio_bitrate);
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
		md->video_fps = value.toDouble();
		qDebug("PlayerProcess::parseVideoProperty: video_fps set to %f", md->video_fps);
		return true;
	}
	if (name == "BITRATE") {
		md->video_bitrate = value.toInt();
		qDebug("PlayerProcess::parseVideoProperty: video_bitrate set to %d", md->video_bitrate);
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

bool PlayerProcess::parseMetaDataProperty(QString name, QString value) {

	name = name.toUpper();
	value = value.trimmed();

	if (value.isEmpty()) {
		qDebug("PlayerProcess::parseMetaDataProperty: value empty");
		return false;
	}

	md->meta_data[name] = value;
	qDebug() << "PlayerProcess::parseMetaDataProperty:" << name << "set to" << value;
	return true;
}

bool PlayerProcess::parseProperty(const QString &name, const QString &value) {

	if (name == "START_TIME") {
		if (value.isEmpty()) {
			qDebug("PlayerProcess::parseProperty: start time not set");
		} else {
			md->start_sec_prop_set = true;
			md->start_sec_prop = value.toDouble();
			qDebug("PlayerProcess::parseProperty: start_sec_prop set to %f",
				   md->start_sec_prop);
		}
		return true;
	}
	if (name == "LENGTH") {
		notifyDuration(value.toDouble());
		return true;
	}
	if (name == "DEMUXER") {
		md->demuxer = value;
		qDebug() << "PlayerProcess::parseProperty: demuxer set to" << md->demuxer;
		return true;
	}
	if (name == "CHAPTERS") {
		md->n_chapters = value.toInt();

		if (md->n_chapters > 1000) {
			qWarning("PlayerProcess::parseProperty: ignoring too many chapters: %d", md->n_chapters);
			md->n_chapters = 0;
		}

		qDebug("PlayerProcess::parseProperty: n_chapters set to %d", md->n_chapters);
		return true;
	}

	return false;
}

void PlayerProcess::seek(double secs, int mode, bool precise, bool currently_paused) {

	// Convert time to player time
	secs = guiTimeToPlayerTime(secs);
	seekPlayerTime(secs, mode, precise, currently_paused);
}

#include "moc_playerprocess.cpp"

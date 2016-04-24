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

#include "proc/playerprocess.h"

#include <QDebug>
#include <QPoint>
#include <QDir>
#include <QFileInfo>

#include "proc/exitmsg.h"
#include "settings/aspectratio.h"
#include "settings/preferences.h"

#include "proc/mpvprocess.h"
#include "proc/mplayerprocess.h"


namespace Proc {

const int waiting_for_answers_safe_guard_init = 100;


TPlayerProcess::TPlayerProcess(QObject* parent, TMediaData* mdata) :
    TProcess(parent),
    md(mdata),
    notified_player_is_running(false),
    received_end_of_file(false),
    quit_send(false),
    line_count(0) {

	//qRegisterMetaType<SubTracks>("SubTracks");
	//qRegisterMetaType<Maps::TTracks>("Tracks");
	//qRegisterMetaType<Chapters>("Chapters");

	connect(this, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onFinished(int,QProcess::ExitStatus)));

	connect(this, SIGNAL(lineAvailable(QByteArray)),
			this, SLOT(parseBytes(QByteArray)));
}

void TPlayerProcess::writeToStdin(QString text, bool log) {

    if (log) {
        qDebug() << "Proc::TPlayerProcess::writeToStdin:" << text;
    }

	if (isRunning()) {

#ifdef Q_OS_WIN
		write(text.toUtf8() + "\n");
#else
		write(text.toLocal8Bit() + "\n");
#endif

	} else {
        qWarning("Proc::TPlayerProcess::writeToStdin: process not in running state");
	}
}

TPlayerProcess* TPlayerProcess::createPlayerProcess(QObject* parent, TMediaData* md) {

	TPlayerProcess* proc;
	if (Settings::pref->isMPlayer()) {
		qDebug() << "Proc::TPlayerProcess::createPlayerProcess: creating TMPlayerProcess";
		proc = new TMPlayerProcess(parent, md);
	} else {
		qDebug() << "Proc::TPlayerProcess::createPlayerProcess: creating TMPVProcess";
		proc = new TMPVProcess(parent, md);
	}

	return proc;
}

bool TPlayerProcess::startPlayer() {

	exit_code_override = 0;
	TExitMsg::setExitCodeMsg("");

	notified_player_is_running = false;

	waiting_for_answers = 0;
	waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;

	received_end_of_file = false;
	quit_send = false;

	prev_frame = -11111;

    // TODO: check, also cleared by TCore::close()
	// Clear media data
	*md = TMediaData(*md);

	// Start the player process
	start();
	// and wait for it to come up
	return waitForStarted();
}

// Slot called when the process is finished
void TPlayerProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug("Proc::TPlayerProcess::onFinished: exitCode: %d, override: %d, status: %d",
           exitCode, exit_code_override, exitStatus);

    if (exit_code_override) {
        exitCode = exit_code_override;
    }
    if (exitCode) {
        received_end_of_file = false;
    }

    emit processFinished(exitCode == 0 && exitStatus == QProcess::NormalExit,
                         exitCode, received_end_of_file);
}

// Convert line of bytes to QString.
void TPlayerProcess::parseBytes(QByteArray ba) {

	static QTime line_time;
	if (line_count == 0) {
		line_time.start();
	}

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
		qDebug("Proc::TPlayerProcess::parseBytes: ignored");
	}

	line_count++;
	if (line_count % 10000 == 0) {
		qDebug("Proc::TPlayerProcess::parseBytes: parsed %'d lines at %f lines per second",
			   line_count, (line_count * 1000.0) / line_time.elapsed());
	}
}

void TPlayerProcess::playingStarted() {
	qDebug("Proc::TPlayerProcess::playingStarted");

	notified_player_is_running = true;

	emit receivedVideoOut();
	emit receivedVideoTracks();
	emit receivedAudioTracks();
	emit receivedSubtitleTracks();
	emit receivedTitleTracks();

	qDebug("Proc::TPlayerProcess::playingStarted: emit playerFullyLoaded()");
	emit playerFullyLoaded();
}

void TPlayerProcess::notifyTitleTrackChanged(int new_title) {

	int selected = md->titles.getSelectedID();
	if (new_title != selected) {
		qDebug("Proc::TPlayerProcess::notifyTitleTrackChanged: title changed from %d to %d",
			   selected, new_title);
		md->titles.setSelectedID(new_title);
		if (notified_player_is_running) {
			qDebug("Proc::TPlayerProcess::notifyTitleTrackChanged: emit receivedTitleTrackChanged()");
			emit receivedTitleTrackChanged(new_title);
		}
	} else {
		qDebug("Proc::TPlayerProcess::notifyTitleTrackChanged: current title already set to %d",
			   new_title);
	}
}

void TPlayerProcess::notifyDuration(double duration) {

	// Duration changed?
	if (qAbs(duration - md->duration) > 0.001) {
		qDebug("Proc::TPlayerProcess::notifyDuration: duration changed from %f to %f", md->duration, duration);
		md->duration = duration;
		qDebug("Proc::TPlayerProcess::notifyDuration: emit durationChanged(%f)", duration);
		emit durationChanged(duration);
	}
}

void TPlayerProcess::checkTime(double sec) {
	Q_UNUSED(sec)
}

 // 2^33 / 90 kHz
static const double ts_rollover = 8589934592.0 / 90000.0;

double TPlayerProcess::guiTimeToPlayerTime(double sec) {

	sec += md->start_sec;

	// Handle MPEG-TS PTS timestamp rollover
	if (md->mpegts && sec >= ts_rollover) {
		sec -= ts_rollover;
	}

	return sec;
}

double TPlayerProcess::playerTimeToGuiTime(double sec) {

	// Substract start time grounding start time at 0
	sec -= md->start_sec;

	// Handle MPEG-TS PTS timestamp rollover
	if (md->mpegts && sec < 0) {
		sec += ts_rollover;
	}

	return sec;
}

void TPlayerProcess::notifyTime(double time_sec, const QString& line) {

	// Store video timestamp
	md->time_sec = time_sec;

	time_sec = playerTimeToGuiTime(time_sec);

	// Give descendants a look at the time
	checkTime(time_sec);

	// Pass timestamp to GUI
    emit receivedPosition(time_sec);

	// Ask children for frame
	int frame = getFrame(time_sec, line);

	// Pass frame to GUI
	if (frame != prev_frame) {
		prev_frame = frame;
        emit receivedFrame(frame);
	}
}

bool TPlayerProcess::waitForAnswers() {

	if (waiting_for_answers > 0) {
		waiting_for_answers_safe_guard--;
		if (waiting_for_answers_safe_guard > 0)
			return true;

		qWarning("Proc::TPlayerProcess::waitForAnswers: did not receive answers in time. Stopped waitng.");
		waiting_for_answers = 0;
		waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;
	}

	return false;
}

bool TPlayerProcess::parseStatusLine(double time_sec, double duration, QRegExp& rx, QString& line) {
	Q_UNUSED(rx)

	// Any pending questions?
	// Cancel the remainder of this line and get the answers.
	if (waitForAnswers())
		return true;

	// For mplayer duration is always 0
	if (duration > 0)
		notifyDuration(duration);

	notifyTime(time_sec, line);

	// Parse the line just a litlle bit longer
	return false;
}

void TPlayerProcess::quit(int exit_code) {
	qDebug("Proc::TPlayerProcess::quit");

    if (!quit_send) {
        quit_send = true;
        writeToStdin("quit " + QString::number(exit_code));
    }
}

bool TPlayerProcess::parseLine(QString& line) {

	static QRegExp rx_vo("^VO: \\[([^\\]]*)\\] (\\d+)x(\\d+)( => (\\d+)x(\\d+))?");
	static QRegExp rx_eof("^Exiting... \\(End of file\\)|^ID_EXIT=EOF");
	static QRegExp rx_no_disk(".*WARN.*No medium found.*", Qt::CaseInsensitive);

	// TODO: remove trim. Be carefull some regexp depend on it...
	// Trim line
	line = line.trimmed();
	if (line.isEmpty())
		return true;

	// Output line to console++
	qDebug("Proc::TPlayerProcess::parseLine: '%s'", line.toUtf8().data());

	if (quit_send) {
		qDebug("Proc::TPlayerProcess::parseLine: ignored, waiting for quit to arrive");
		return true;
	}

	// VO
	if (rx_vo.indexIn(line) >= 0) {
		return parseVO(rx_vo.cap(1),
					   rx_vo.cap(2).toInt(), rx_vo.cap(3).toInt(),
					   rx_vo.cap(5).toInt(), rx_vo.cap(6).toInt());
	}

	if (rx_no_disk.indexIn(line) >= 0) {
		qWarning("Proc::TPlayerProcess::parseLine: no disc in device");
		quit(TExitMsg::ERR_NO_DISC);
		return true;
	}

	// End of file
	if (rx_eof.indexIn(line) >= 0)  {
		qDebug("Proc::TPlayerProcess::parseLine: detected end of file");
		received_end_of_file = true;
		return true;
	}

	// Like to be parsed a little longer
	return false;
}

bool TPlayerProcess::parseVO(const QString& vo, int sw, int sh, int dw, int dh) {

	md->vo = vo;
	md->video_width = sw;
	md->video_height = sh;
	if (dw == 0) {
		md->video_out_width = sw;
		md->video_out_height = sh;
	} else {
		md->video_out_width = dw;
		md->video_out_height = dh;
	}

	qDebug() << "Proc::TMPVProcess::parseLine: VO" << md->vo
			 << md->video_width << "x" << md->video_height << "=>"
			 << md->video_out_width << "x" << md->video_out_height;

	if (notified_player_is_running) {
		emit receivedVideoOut();
	}

	return true;
}

bool TPlayerProcess::parseVideoProperty(const QString& name, const QString& value) {

	/* Replaced by parseVO()
	if (name == "WIDTH") {
		md->video_width = value.toInt();
		qDebug("Proc::TPlayerProcess::parseVideoProperty: video_width set to %d",
			   md->video_width);
		return true;
	}
	if (name == "HEIGHT") {
		md->video_height = value.toInt();
		qDebug("Proc::TPlayerProcess::parseVideoProperty: video_height set to %d",
			   md->video_height);
		return true;
	}
	*/
	if (name == "ASPECT") {
		md->video_aspect = value;
		qDebug() << "Proc::TPlayerProcess::parseAspectRatio: video aspect ratio set to"
				 << md->video_aspect;
		return true;
	}
	if (name == "FPS") {
		md->video_fps = value.toDouble();
		qDebug("Proc::TPlayerProcess::parseVideoProperty: video_fps set to %f",
			   md->video_fps);
		return true;
	}
	if (name == "BITRATE") {
		md->video_bitrate = value.toInt();
		qDebug("Proc::TPlayerProcess::parseVideoProperty: video_bitrate set to %d",
			   md->video_bitrate);
		return true;
	}
	if (name == "FORMAT") {
		md->video_format = value;
		qDebug() << "Proc::TPlayerProcess::parseVideoProperty: video_format set to"
				 << md->video_format;
		return true;
	}
	if (name == "CODEC") {
		md->video_codec = value;
		qDebug() << "Proc::TPlayerProcess::parseVideoProperty: video_codec set to"
				 << md->video_codec;
		return true;
	}

	return false;
}

bool TPlayerProcess::parseAudioProperty(const QString& name, const QString& value) {

	if (name == "BITRATE") {
		md->audio_bitrate = value.toInt();
		qDebug("Proc::TPlayerProcess::parseAudioProperty: audio_bitrate set to %d", md->audio_bitrate);
		return true;
	}
	if (name == "FORMAT") {
		md->audio_format = value;
		qDebug() << "Proc::TPlayerProcess::parseAudioProperty: audio_format set to" << md->audio_format;
		return true;
	}
	if (name == "RATE") {
		md->audio_rate = value.toInt();
		qDebug("Proc::TPlayerProcess::parseAudioProperty: audio_rate set to %d", md->audio_rate);
		return true;
	}
	if (name == "NCH") {
		md->audio_nch = value.toInt();
		qDebug("Proc::TPlayerProcess::parseAudioProperty: audio_nch set to %d", md->audio_nch);
		return true;
	}
	if (name == "CODEC") {
		md->audio_codec = value;
		qDebug() << "Proc::TPlayerProcess::parseAudioProperty: audio_codec set to" << md->audio_codec;
		return true;
	}

	return false;
}

bool TPlayerProcess::parseAngle(const QString& value) {

	static QRegExp rx_angles("(\\d+)/(\\d+)");

	if (value.startsWith("$")) {
		return false;
	}
	if (rx_angles.indexIn(value) >= 0) {
		md->angle = rx_angles.cap(1).toInt();
		md->angles = rx_angles.cap(2).toInt();
	} else {
		md->angle = 0;
		md->angles = 0;
	}
	qDebug("Proc::TPlayerProcess::parseAngle: selected angle %d/%d",
		   md->angle, md->angles);

	if (notified_player_is_running) {
		qDebug("Proc::TPlayerProcess::parseAngle: emit receivedAngles()");
		emit receivedAngles();
	}

	return true;
}

bool TPlayerProcess::parseProperty(const QString& name, const QString& value) {

	if (name == "START_TIME") {
		if (value.isEmpty() || value == "unknown") {
			qDebug("Proc::TPlayerProcess::parseProperty: start time unknown");
		} else {
			md->start_sec_set = true;
			md->start_sec = value.toDouble();
			qDebug("Proc::TPlayerProcess::parseProperty: start time set to %f",
				   md->start_sec);
		}
		return true;
	}
	if (name == "LENGTH") {
		notifyDuration(value.toDouble());
		return true;
	}
	if (name == "DEMUXER") {
		md->demuxer = value;
		qDebug() << "Proc::TPlayerProcess::parseProperty: demuxer set to" << md->demuxer;
		// TODO: mpeg TS detection
		if (md->demuxer == "mpegts") {
			md->mpegts = true;
			qDebug("Proc::TPlayerProcess::parseProperty: detected mpegts");
		}
		return true;
	}
	if (name == "ANGLE_EX") {
		return parseAngle(value);
	}

	return false;
}

bool TPlayerProcess::parseMetaDataProperty(QString name, QString value) {

	name = name.trimmed();
	value = value.trimmed();
	md->meta_data[name] = value;
	qDebug() << "Proc::TPlayerProcess::parseMetaDataProperty:" << name << "set to" << value;
	return true;
}

void TPlayerProcess::seek(double secs, int mode, bool precise, bool currently_paused) {

	// Convert time to player time if time is absolute position in secs
	if (mode == 2) {
		secs = guiTimeToPlayerTime(secs);
	}
	seekPlayerTime(secs, mode, precise, currently_paused);
}

void TPlayerProcess::setCaptureDirectory(const QString& dir) {

	capture_filename = "";
	if (!dir.isEmpty()) {
		QFileInfo fi(dir);
		if (fi.isDir() && fi.isWritable()) {
			// Find a unique filename
			QString prefix = "capture";
			for (int n = 1; ; n++) {
				QString c = QDir::toNativeSeparators(QString("%1/%2_%3.dump").arg(dir).arg(prefix).arg(n, 4, 10, QChar('0')));
				if (!QFile::exists(c)) {
					capture_filename = c;
					return;
				}
			}
		}
	}
}

} // namespace Proc

#include "moc_playerprocess.cpp"

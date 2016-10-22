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

#include "player/process/playerprocess.h"

#include <QPoint>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "player/process/exitmsg.h"
#include "player/process/mpvprocess.h"
#include "player/process/mplayerprocess.h"
#include "settings/aspectratio.h"
#include "settings/preferences.h"


namespace Player {
namespace Process {

const int waiting_for_answers_safe_guard_init = 100;


TPlayerProcess::TPlayerProcess(QObject* parent, TMediaData* mdata) :
    TProcess(parent),
    debug(logger()),
    md(mdata),
    notified_player_is_running(false),
    received_end_of_file(false),
    quit_send(false),
    line_count(0) {

    //qRegisterMetaType<TSubTracks>("TSubTracks");
    //qRegisterMetaType<Maps::TTracks>("Tracks");
    //qRegisterMetaType<Chapters>("Chapters");

    connect(this, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onFinished(int,QProcess::ExitStatus)));
}

void TPlayerProcess::writeToStdin(const QString& text, bool log) {

    if (log) {
        logger()->debug("writeToStdin: %1", text);
    }

    if (isRunning()) {

#ifdef Q_OS_WIN
        write(text.toUtf8() + "\n");
#else
        write(text.toLocal8Bit() + "\n");
#endif

    } else {
        logger()->warn("writeToStdin: process not in running state");
    }
}

TPlayerProcess* TPlayerProcess::createPlayerProcess(QObject* parent,
                                                    TMediaData* md) {

    TPlayerProcess* process;
    Log4Qt::Logger* logger = Log4Qt::Logger::logger(
                                 "Player::Process::TPlayerProcess");
    if (Settings::pref->isMPlayer()) {
        logger->debug("createPlayerProcess: creating TMPlayerProcess");
        process = new TMPlayerProcess(parent, md);
    } else {
        logger->debug("createPlayerProcess: creating TMPVProcess");
        process = new TMPVProcess(parent, md);
    }

    return process;
}

bool TPlayerProcess::startPlayer() {

    exit_code_override = 0;
    TExitMsg::setExitCodeMsg("");

    notified_player_is_running = false;

    waiting_for_answers = 0;
    waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;

    paused = false;

    received_end_of_file = false;
    quit_send = false;

    // Start the player process
    start();
    // and wait for it to come up
    return waitForStarted();
}

// Slot called when the process is finished
void TPlayerProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    logger()->debug("onFinished: exitCode: %1, override:"
                    " %2, status: %3", exitCode, exit_code_override, exitStatus);

    if (exit_code_override) {
        exitCode = exit_code_override;
    }
    if (exitCode) {
        received_end_of_file = false;
    }

    emit processFinished(exitCode == 0 && exitStatus == QProcess::NormalExit,
                         exitCode, received_end_of_file);
}

void TPlayerProcess::playingStarted() {
    logger()->debug("playingStarted");

    notified_player_is_running = true;

    emit receivedVideoOut();
    emit receivedVideoTracks();
    emit receivedAudioTracks();
    emit receivedSubtitleTracks();
    emit receivedTitleTracks();

    logger()->debug("playingStarted: emit playerFullyLoaded()");
    emit playerFullyLoaded();
}

void TPlayerProcess::notifyTitleTrackChanged(int new_title) {

    int selected = md->titles.getSelectedID();
    if (new_title != selected) {
        logger()->debug("notifyTitleTrackChanged: title changed from %1 to %2",
                        selected, new_title);
        md->titles.setSelectedID(new_title);
        if (notified_player_is_running) {
            logger()->debug("notifyTitleTrackChanged: emit"
                            " receivedTitleTrackChanged()");
            emit receivedTitleTrackChanged(new_title);
        }
    } else {
        logger()->debug("notifyTitleTrackChanged: current title already set to"
                        " %1", new_title);
    }
}

void TPlayerProcess::notifyDuration(double duration) {

    // Duration changed?
    if (qAbs(duration - md->duration) > 0.001) {
        logger()->debug("notifyDuration: duration changed from %1 to %2",
                        QString::number(md->duration),
                        QString::number(duration));
        md->duration = duration;
        emit durationChanged(md->duration);
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

void TPlayerProcess::notifyTime(double time_sec) {

    // Store video timestamp
    md->time_sec = time_sec;

    time_sec = playerTimeToGuiTime(time_sec);

    // Give descendants a look at the time
    checkTime(time_sec);

    // Pass timestamp to GUI
    emit receivedPosition(time_sec);
}

// TODO: move to TMPVProcess
bool TPlayerProcess::waitForAnswers() {

    if (waiting_for_answers > 0) {
        waiting_for_answers_safe_guard--;
        if (waiting_for_answers_safe_guard > 0)
            return true;

        logger()->warn("waitForAnswers: did not receive answers in time. Stopped waitng.");
        waiting_for_answers = 0;
        waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;
    }

    return false;
}

void TPlayerProcess::quit(int exit_code) {
    logger()->debug("quit");

    if (!quit_send) {
        quit_send = true;
        writeToStdin("quit " + QString::number(exit_code));
    }
}

bool TPlayerProcess::parseLine(QString& line) {

    static QRegExp rx_vo("^VO: \\[([^\\]]*)\\] (\\d+)x(\\d+)( => (\\d+)x(\\d+))?");
    static QRegExp rx_eof("^Exiting... \\(End of file\\)|^ID_EXIT=EOF");
    static QRegExp rx_no_disk(".*WARN.*No medium found.*", Qt::CaseInsensitive);

    logger()->debug("parseLine: '%1'", line);

    if (quit_send) {
        logger()->debug("parseLine: ignored, waiting for quit to arrive");
        return true;
    }

    // VO
    if (rx_vo.indexIn(line) >= 0) {
        return parseVO(rx_vo.cap(1),
                       rx_vo.cap(2).toInt(), rx_vo.cap(3).toInt(),
                       rx_vo.cap(5).toInt(), rx_vo.cap(6).toInt());
    }

    if (rx_no_disk.indexIn(line) >= 0) {
        logger()->warn("parseLine: no disc in device");
        quit(TExitMsg::ERR_NO_DISC);
        return true;
    }

    // End of file
    if (rx_eof.indexIn(line) >= 0)  {
        logger()->debug("parseLine: detected end of file");
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

    logger()->debug(QString("parseVO: VO '%1' %2 x %3 => %4 x %5")
                    .arg(md->vo).arg(md->video_width).arg(md->video_height)
                    .arg(md->video_out_width).arg(md->video_out_height));

    if (notified_player_is_running) {
        emit receivedVideoOut();
    }

    return true;
}

bool TPlayerProcess::parseVideoProperty(const QString& name, const QString& value) {

    if (name == "ASPECT") {
        md->video_aspect = value;
        logger()->debug("parseVideoProperty: video aspect ratio set to '%1'",
                        md->video_aspect);
        return true;
    }
    if (name == "FPS") {
        md->video_fps = value.toDouble();
        logger()->debug("parseVideoProperty: video_fps set to %1",
                        QString::number(md->video_fps));
        return true;
    }
    if (name == "BITRATE") {
        md->video_bitrate = value.toInt();
        logger()->debug("parseVideoProperty: video_bitrate set to %1",
                        md->video_bitrate);
        if (notified_player_is_running) {
            emit videoBitRateChanged(md->video_bitrate);
        }
        return true;
    }
    if (name == "FORMAT") {
        md->video_format = value;
        logger()->debug("parseVideoProperty: video_format set to '%1'",
                        md->video_format);
        return true;
    }
    if (name == "CODEC") {
        md->video_codec = value;
        logger()->debug("parseVideoProperty: video_codec set to '%1'",
                        md->video_codec);
        return true;
    }

    return false;
}

bool TPlayerProcess::parseAudioProperty(const QString& name, const QString& value) {

    if (name == "BITRATE") {
        md->audio_bitrate = value.toInt();
        logger()->debug("parseAudioProperty: audio_bitrate set to %1",
                        md->audio_bitrate);
        if (notified_player_is_running) {
            emit audioBitRateChanged(md->audio_bitrate);
        }
        return true;
    }
    if (name == "FORMAT") {
        md->audio_format = value;
        logger()->debug("parseAudioProperty: audio_format set to '%1'",
                        md->audio_format);
        return true;
    }
    if (name == "RATE") {
        md->audio_rate = value.toInt();
        logger()->debug("parseAudioProperty: audio_rate set to %1",
                        md->audio_rate);
        return true;
    }
    if (name == "NCH") {
        md->audio_nch = value.toInt();
        logger()->debug("parseAudioProperty: audio_nch set to %1",
                        md->audio_nch);
        return true;
    }
    if (name == "CODEC") {
        md->audio_codec = value;
        logger()->debug("parseAudioProperty: audio_codec set to '%1'",
                        md->audio_codec);
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
    logger()->debug("parseAngle: selected angle %1/%2",
           md->angle, md->angles);

    if (notified_player_is_running) {
        logger()->debug("parseAngle: emit receivedAngles()");
        emit receivedAngles();
    }

    return true;
}

bool TPlayerProcess::parseProperty(const QString& name, const QString& value) {

    if (name == "START_TIME") {
        if (value.isEmpty() || value == "unknown") {
            logger()->debug("parseProperty: start time not set");
        } else {
            md->start_sec_player = value.toDouble();
            if (Settings::pref->isMPV()) {
                md->start_sec_set = true;
                md->start_sec = md->start_sec_player;
            }
            logger()->debug("parseProperty: start time set to %1",
                            QString::number(md->start_sec_player));
        }
        return true;
    }
    if (name == "LENGTH") {
        notifyDuration(value.toDouble());
        return true;
    }
    if (name == "DEMUXER") {
        md->demuxer = value;
        logger()->debug("parseProperty: demuxer set to '%1'", md->demuxer);
        // TODO: mpeg TS detection
        if (md->demuxer == "mpegts") {
            md->mpegts = true;
            logger()->debug("parseProperty: detected mpegts");
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
    logger()->debug("parseMetaDataProperty: '%1' set to '%2'", name, value);
    return true;
}

void TPlayerProcess::setImageDuration(int duration) {

    // Need at least 2 frames
    if (duration < 2) {
        duration = 2;
    } else if (duration > 999) {
        duration = 999;
    }

    int fps;
    if (Settings::pref->isMPlayer()) {
        // When MPlayer runs on 1 fps it will only respond to events once a
        // second. So increasing the framerate...
        if (duration <= 20) {
            fps = 5;
        } else if (duration <= 60) {
            fps = 2;
        } else {
            fps = 1;
        }
    } else {
        // Select 1 frame per second for MPV
        // With fps < 1 the displayed time is off
        fps = 1;
    }

    int frames = duration * fps;

    logger()->debug("setImageDuration: duration %1, frames %2, fps %3",
                    duration, frames, fps);
    if (temp_file.open()) {
        temp_file_name = temp_file.fileName();
        temp_file.resize(0);
        QTextStream text(&temp_file);
        for(int i = 0; i < frames; i++) {
            text << md->filename << "\n";
        }
        text.flush();
        temp_file.flush();
        temp_file.close();
    }

    setOption("fps", fps);
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

} // namespace Process
} // namespace Player

#include "moc_playerprocess.cpp"

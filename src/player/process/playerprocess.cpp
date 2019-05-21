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
#include "player/process/exitmsg.h"
#include "player/process/mpvprocess.h"
#include "player/process/mplayerprocess.h"
#include "settings/aspectratio.h"
#include "settings/preferences.h"
#include "wzdebug.h"

#include <QPoint>
#include <QDir>
#include <QFileInfo>
#include <QString>


LOG4QT_DECLARE_STATIC_LOGGER(logger, Player::Process::TPlayerProcess)

namespace Player {
namespace Process {

const int waiting_for_answers_safe_guard_init = 100;


TPlayerProcess* TPlayerProcess::createPlayerProcess(QObject* parent,
                                                    const QString& name,
                                                    TMediaData* md) {

    TPlayerProcess* process;
    if (Settings::pref->isMPlayer()) {
        WZDEBUG("Creating TMPlayerProcess " + name);
        process = new TMPlayerProcess(parent, name, md);
    } else {
        WZDEBUG("Creating TMPVProcess " + name);
        process = new TMPVProcess(parent, name, md);
    }

    return process;
}


TPlayerProcess::TPlayerProcess(QObject* parent,
                               const QString& name,
                               TMediaData* mdata) :
    TProcess(parent, name),
    md(mdata),
    notified_player_is_running(false),
    received_end_of_file(false),
    quit_send(false),
    line_count(0) {

    //qRegisterMetaType<TSubTracks>("TSubTracks");
    //qRegisterMetaType<Maps::TTracks>("Tracks");
    //qRegisterMetaType<Chapters>("Chapters");

    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onFinished(int, QProcess::ExitStatus)));
}

void TPlayerProcess::writeToPlayer(const QString& text, bool log) {

    if (log) {
        WZDEBUGOBJ(text);
    }

    if (received_end_of_file) {
        WZWOBJ << "Skipping write of" << text << "after eof";
    } else if (isRunning()) {

#ifdef Q_OS_WIN
        write(text.toUtf8() + "\n");
#else
        write(text.toLocal8Bit() + "\n");
#endif

    } else {
        WZWOBJ << "Process not running while trying to write" << text;
    }
}

bool TPlayerProcess::startPlayer() {

    exit_code_override = 0;
    TExitMsg::setExitCodeMsg("");

    notified_player_is_running = false;

    waiting_for_answers = 0;
    waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;

    paused = false;
    buffering = false;

    received_end_of_file = false;
    quit_send = false;

    // Start the player process
    startTime.start();
    start();
    // and wait for it to come up
    return waitForStarted();
}

// Slot called when the process is finished
void TPlayerProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    WZDOBJ << "Exit code" << exitCode
           << "override" << exit_code_override
           << "status" << exitStatus;

    if (exit_code_override) {
        exitCode = exit_code_override;
    }
    if (exitCode) {
        received_end_of_file = false;
    }

    emit processFinished(exitCode == 0 && exitStatus == QProcess::NormalExit,
                         exitCode, received_end_of_file);
}

void TPlayerProcess::notifyPlayingStarted() {
    WZDOBJ;

    // Notify duration 0
    if (md->duration_ms == 0) {
        emit durationChanged(0);
    }

    emit receivedVideoOut();
    emit receivedVideoTracks();
    emit receivedAudioTracks();
    emit receivedSubtitleTracks();
    emit receivedTitleTracks();

    notified_player_is_running = true;
    WZDEBUGOBJ("emit playingStarted()");
    emit playingStarted();
}

void TPlayerProcess::notifyTitleTrackChanged(int new_title) {

    int selected = md->titles.getSelectedID();
    if (new_title != selected) {
        WZDEBUGOBJ(QString("Title changed from %1 to %2")
                .arg(selected).arg(new_title));
        md->titles.setSelectedID(new_title);
        if (notified_player_is_running) {
            WZDEBUGOBJ("emit receivedTitleTrackChanged()");
            emit receivedTitleTrackChanged(new_title);
        }
    }
}

void TPlayerProcess::notifyDuration(double durationSec, bool forceEmit) {

    if (durationSec < 0) {
        WZWARN(QString("Received negative duration %1").arg(durationSec));
        durationSec = 0;
    }

    int oldMS = md->duration_ms;
    md->duration_ms = qRound(durationSec * 1000);

    if (oldMS != md->duration_ms || forceEmit) {
        WZDEBUGOBJ(QString("Duration updated from %1 ms to %2 ms")
                   .arg(oldMS).arg(md->duration_ms));
        emit durationChanged(md->duration_ms);
    }
}

void TPlayerProcess::checkTime(int ms) {
    Q_UNUSED(ms)
}

 // 2^33 / 90 kHz
static const double ts_rollover = 8589934592.0 / 90000.0;

double TPlayerProcess::guiTimeToPlayerTime(double sec) {

    sec += md->getStartSec();

    // Handle MPEG-TS PTS timestamp rollover
    if (md->mpegts && sec >= ts_rollover) {
        sec -= ts_rollover;
    }

    return sec;
}

int TPlayerProcess::playerTimeToGuiTime(int ms) {

    // Substract start time grounding start time at 0
    ms -= md->start_ms;

    // Handle MPEG-TS PTS timestamp rollover
    if (md->mpegts && ms < 0) {
        // TODO: check qRound()
        ms += qRound(ts_rollover * 1000);
    }

    return ms;
}

void TPlayerProcess::notifyTime(double time_sec) {

    // Store video timestamp
    md->setPosSec(time_sec);
    // Set time stamp for GUI
    md->pos_gui_ms = playerTimeToGuiTime(md->pos_ms);

    // Give descendants a look at the time
    checkTime(md->pos_gui_ms);

    // Pass timestamp to GUI
    emit receivedPositionMS(md->pos_gui_ms);
}

// TODO: move to TMPVProcess
bool TPlayerProcess::waitForAnswers() {

    if (waiting_for_answers > 0) {
        waiting_for_answers_safe_guard--;
        if (waiting_for_answers_safe_guard > 0)
            return true;

        WZWARN("Did not receive answers in time, stopped waitng");
        waiting_for_answers = 0;
        waiting_for_answers_safe_guard = waiting_for_answers_safe_guard_init;
    }

    return false;
}

void TPlayerProcess::setEOF() {

    received_end_of_file = true;
}

void TPlayerProcess::quit(int exit_code) {
    WZDEBUGOBJ("");

    if (!quit_send) {
        quit_send = true;
        writeToPlayer("quit " + QString::number(exit_code));
    }
}

bool TPlayerProcess::parseLine(QString& line) {

    static QRegExp rx_vo("^VO: \\[([^\\]]*)\\] (\\d+)x(\\d+)( => (\\d+)x(\\d+))?");
    static QRegExp rx_eof("^Exiting... \\(End of file\\)|^ID_EXIT=EOF");
    static QRegExp rx_no_disk(".*WARN.*No medium found.*", Qt::CaseInsensitive);
    static QRegExp rx_dvd_serial("Serial Number: (.*)$");

    if (quit_send) {
        return true;
    }
    WZDEBUGOBJ("\"" + line + "\"");

    // Video out driver
    if (rx_vo.indexIn(line) >= 0) {
        return parseVO(rx_vo.cap(1),
                       rx_vo.cap(2).toInt(), rx_vo.cap(3).toInt(),
                       rx_vo.cap(5).toInt(), rx_vo.cap(6).toInt());
    }

    // No disc
    if (rx_no_disk.indexIn(line) >= 0) {
        WZWARN("No disc in device");
        quit(TExitMsg::ERR_NO_DISC);
        return true;
    }

    // End of file
    if (rx_eof.indexIn(line) >= 0)  {
        WZDEBUGOBJ("Received end of file");
        setEOF();
        return true;
    }

    // DVD serial number
    if (rx_dvd_serial.indexIn(line) >= 0)  {
        md->dvd_disc_serial = rx_dvd_serial.cap(1);
        WZDEBUGOBJ("Serial set to " + md->dvd_disc_serial);
        return true;
    }

    // Continue parsing
    return false;
}

bool TPlayerProcess::parseVO(const QString& vo,
                             int sw, int sh,
                             int dw, int dh) {

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

    WZDEBUGOBJ(QString("parseVO: VO '%1' %2 x %3 => %4 x %5")
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
        WZDEBUGOBJ("video_aspect set to '" + md->video_aspect + "'");
        return true;
    }
    if (name == "FPS") {
        md->video_fps = value.toDouble();
        WZDEBUGOBJ("video_fps set to " + QString::number(md->video_fps));
        return true;
    }
    if (name == "BITRATE") {
        md->video_bitrate = value.toInt();
        WZDEBUGOBJ("video_bitrate set to " + QString::number(md->video_bitrate));
        if (notified_player_is_running) {
            emit videoBitRateChanged(md->video_bitrate);
        }
        return true;
    }
    if (name == "FORMAT") {
        md->video_format = value;
        WZDEBUGOBJ("video_format set to '" + md->video_format + "'");
        return true;
    }
    if (name == "CODEC") {
        md->video_codec = value;
        WZDEBUGOBJ("video_codec set to '" + md->video_codec + "'");
        return true;
    }
    // TODO: check MPlayer
    if (name == "COLORMATRIX") {
        md->video_colorspace = value;
        WZDEBUGOBJ("video_colorspace set to '" + md->video_colorspace + "'");
        return true;
    }
    if (name == "OUTCOLORMATRIX") {
        md->video_out_colorspace = value;
        WZDEBUGOBJ("video_out_colorspace set to '" + md->video_out_colorspace + "'");
        return true;
    }

    return false;
}

bool TPlayerProcess::parseAudioProperty(const QString& name, const QString& value) {

    if (name == "BITRATE") {
        md->audio_bitrate = value.toInt();
        WZDEBUGOBJ("audio_bitrate set to " + QString::number(md->audio_bitrate));
        if (notified_player_is_running) {
            emit audioBitRateChanged(md->audio_bitrate);
        }
        return true;
    }
    if (name == "FORMAT") {
        md->audio_format = value;
        WZDEBUGOBJ("audio_format set to '" + md->audio_format + "'");
        return true;
    }
    if (name == "RATE") {
        md->audio_rate = value.toInt();
        WZDEBUGOBJ("audio_rate set to " + QString::number(md->audio_rate));
        return true;
    }
    if (name == "NCH") {
        md->audio_nch = value.toInt();
        WZDEBUGOBJ("audio_nch set to " + QString::number(md->audio_nch));
        return true;
    }
    if (name == "CODEC") {
        md->audio_codec = value;
        WZDEBUGOBJ("audio_codec set to '" + md->audio_codec + "'");
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
    WZDEBUGOBJ(QString("selected angle %1/%2").arg(md->angle).arg(md->angles));

    if (notified_player_is_running) {
        WZDEBUGOBJ("emit receivedAngles()");
        emit receivedAngles();
    }

    return true;
}

bool TPlayerProcess::parseProperty(const QString& name, const QString& value) {

    if (name == "START_TIME") {
        if (value.isEmpty() || value == "unknown") {
            WZDEBUGOBJ("Start time not set");
        } else {
            md->start_ms_player = qRound(value.toDouble() * 1000);
            if (Settings::pref->isMPV()) {
                md->start_ms_used = true;
                md->start_ms = md->start_ms_player;
            }
            WZDEBUGOBJ(QString("Start time set to %1 ms")
                       .arg(md->start_ms_player));
        }
        return true;
    }
    if (name == "LENGTH") {
        notifyDuration(value.toDouble());
        return true;
    }
    if (name == "DEMUXER") {
        md->demuxer = value;
        WZDEBUGOBJ("Demuxer set to '" + md->demuxer + "'");
        // TODO: better mpeg TS detection
        if (md->demuxer == "mpegts") {
            md->mpegts = true;
            WZDEBUGOBJ("Detected mpegts");
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
    WZDEBUGOBJ(QString("'%1' set to '%2'").arg(name).arg(value));
    return true;
}

void TPlayerProcess::setImageDuration(int durationSec) {

    int fps, frames;
    if (Settings::pref->isMPlayer()) {
        // Need at least 2 frames
        if (durationSec < 2) {
            durationSec = 2;
        } else if (durationSec > 999) {
            durationSec = 999;
        }
        // When MPlayer runs on 1 fps it will only respond to events once a
        // second. So increasing the framerate...
        if (durationSec <= 20) {
            fps = 5;
        } else if (durationSec <= 60) {
            fps = 2;
        } else {
            fps = 1;
        }
        frames = durationSec * fps;
    } else {
        // Need at least 2 frames
        if (durationSec < 1) {
            durationSec = 1;
        } else if (durationSec > 999) {
            durationSec = 999;
        }
        // With fps > 1, the displayed duration is off by factor fps
        // With 1 fps, the playtime is one frame too short
        // With fps < 1 the displayed time is off
        fps = 1;
        frames = (durationSec * fps) + 1;
    }

    if (temp_file.open()) {
        temp_file_name = temp_file.fileName();
        WZDEBUGOBJ(QString("Writing %1 frames to '%2'. Duration %3, fps %4.")
                .arg(frames).arg(temp_file_name).arg(durationSec).arg(fps));
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

void TPlayerProcess::seek(double secs, int mode, bool keyframes,
                          bool currently_paused) {

    // Convert time to player time if time is absolute position in secs
    if (mode == 2) {
        secs = guiTimeToPlayerTime(secs);
    }
    seekPlayerTime(secs, mode, keyframes, currently_paused);
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

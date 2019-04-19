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

#include "player/process/mpvprocess.h"

#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QApplication>

#include "config.h"
#include "player/process/exitmsg.h"
#include "player/process/playerprocess.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "player/info/playerinfo.h"
#include "mediadata.h"
#include "name.h"


namespace Player {
namespace Process {

TMPVProcess::TMPVProcess(QObject* parent,
                         const QString& name,
                         TMediaData* mdata) :
    TPlayerProcess(parent, name, mdata) {
}

bool TMPVProcess::startPlayer() {

    received_buffering = false;
    received_title_not_found = false;
    request_bit_rate_info = !md->image;
    quit_at_end_of_title = false;

    return TPlayerProcess::startPlayer();
}

bool TMPVProcess::parseVideoTrack(int id, QString name, bool selected) {

    // Note lang "". Track info has lang.
    if (md->videos.updateTrack(id, "", name, selected)) {
        if (notified_player_is_running)
            emit receivedVideoTracks();
        return true;
    }
    return false;
}

bool TMPVProcess::parseAudioTrack(int id,
                                  const QString& lang,
                                  QString name,
                                  bool selected) {

    if (md->audios.updateTrack(id, lang, name, selected)) {
        if (notified_player_is_running) {
            emit receivedAudioTracks();
        }
        return true;
    }
    return false;
}

bool TMPVProcess::parseSubtitleTrack(int id,
                                    const QString &lang,
                                    QString name,
                                    QString type,
                                    bool selected) {

    if (type.startsWith("*) (")) {
        type = type.mid(4);
    }
    if (name.isEmpty() && !type.isEmpty()) {
        name = type;
    }

    SubData::Type sub_type;
    QString filename;
    if (type.contains("external", Qt::CaseInsensitive)) {
        sub_type = SubData::File;
        filename = sub_file;
    } else {
        sub_type = SubData::Sub;
    }

    SubData::Type sec_type = md->subs.selectedSecondaryType();
    int sec_ID = md->subs.selectedSecondaryID();
    bool sec_selected = sec_type != SubData::None && sec_ID >= 0;

    if (sub_type == sec_type && id == sec_ID) {
        WZDEBUGOBJ("found secondary subtitle track");
        // Secondary sub, don't select the primary sub
        selected = false;
    }

    if (md->subs.update(sub_type, id, sec_type, sec_ID,
                        lang, name, filename,
                        selected, sec_selected)) {
        if (notified_player_is_running)
            emit receivedSubtitleTracks();
        return true;
    }

    return false;
}

bool TMPVProcess::parseProperty(const QString& name, const QString& value) {

/*
    if (name == "TRACKS_COUNT") {
        int tracks = value.toInt();
        logger()->debug("parseProperty: requesting track info for %1 tracks",
                        tracks);
        for (int n = 0; n < tracks; n++) {
            writeToPlayer(QString("print_text \"TRACK_INFO_%1="
                "${track-list/%1/type} "
                "${track-list/%1/id} "
                "${track-list/%1/selected} "
                "'${track-list/%1/lang:}' "
                "'${track-list/%1/title:}'\"").arg(n));
        }
        return true;
    }
*/

    if (name == "TITLES") {
        int n_titles = value.toInt();
        WZDEBUGOBJ("Creating " + QString::number(n_titles) + " titles");
        for (int idx = 0; idx < n_titles; idx++) {
            md->titles.addID(idx + 1);
            writeToPlayer(QString("print_text \"INFO_TITLE_LENGTH=%1"
                                 " ${=disc-title-list/%1/length:-1}\"")
                         .arg(idx));
        }
        waiting_for_answers += n_titles;
        return true;
    }

    if (name == "TITLE_LENGTH") {
        static QRegExp rx_title_length("^(\\d+) (.*)");
        if (rx_title_length.indexIn(value) >= 0) {
            int idx = rx_title_length.cap(1).toInt();
            // if "" player does not know or support prop
            if (!rx_title_length.cap(2).isEmpty()) {
                double length = rx_title_length.cap(2).toDouble();
                md->titles.addDuration(idx + 1, length);
            }
        }
        waiting_for_answers--;
        return true;
    }

    if (name == "CHAPTERS") {
        int n_chapters = value.toInt();
        WZDEBUGOBJ("Requesting start and titel of " + QString::number(n_chapters)
                + " chapter(s)");
        for (int n = 0; n < n_chapters; n++) {
            writeToPlayer(QString("print_text \"CHAPTER_%1="
                                 "${=chapter-list/%1/time:}"
                                 " '${chapter-list/%1/title:}'\"").arg(n));
        }
        waiting_for_answers += n_chapters;
        return true;
    }

    if (name == "MEDIA_TITLE") {
        if (md->image) {
            WZDEBUGOBJ("Ignoring image title");
        } else if (md->disc.valid) {
            // For a disc, always set the title, unless it is "cdrom"
            if (value != "cdrom") {
                md->title = value;
                WZDEBUGOBJ("Title set to '" + md->title + "'");
            }
        } else {
            QString name = TName::nameForURL(md->filename);
            QString ext = QFileInfo(md->filename).suffix();
            if (value == name || value == name + "." + ext) {
                WZDEBUGOBJ("Ignoring title matching file name");
            } else {
                md->title = value;
                WZDEBUGOBJ("Title set to '" + md->title + "'");
            }
        }
        return true;
    }

    return TPlayerProcess::parseProperty(name, value);
}

bool TMPVProcess::parseMetaDataList(QString list) {

    // TODO: no idea how MPV escapes a ", so for now this will
    // prob. break if the meta data contains a "
    static QRegExp rx("\\{\"key\"\\:\"([^\"]*)\",\"value\"\\:\"([^\"]*)\"\\}");

    while (rx.indexIn(list) >= 0) {
        QString key = rx.cap(1);
        QString value = rx.cap(2);
        parseMetaDataProperty(key, value);
        list = list.mid(22 + key.length() + value.length());
    }

    return true;
}

bool TMPVProcess::parseChapter(int id, double start, QString title) {

    waiting_for_answers--;
    md->chapters.addChapter(id, title, start);
    WZDEBUGOBJ("Added chapter id " + QString::number(id)
            + " starting at " + QString::number(start)
            + " with title '" + title + "'");
    return true;
}

void TMPVProcess::requestChapterInfo() {
    writeToPlayer("print_text \"INFO_CHAPTERS=${=chapters:}\"");
}

void TMPVProcess::fixTitle() {

    // The problem with MPV is title switching. It only prints the VTS that
    // is selected, but not the title.
    // To work around it let the playlist handle title switching and kill the
    // player in parseLine() or checkTime() when it reaches the end of a title.

    // Note: see also setFixedOptions() for msg-level command line option.
    // Note: by now get title ID property with
    // writeToPlayer("print_text INFO_DISC_TITLE=${=disc-title:}");
    // is valid.

    int title = md->disc.title;
    if (title == 0) {
        // See setMedia() for reason why 0 is 1
        title = 1;
    }

    // If we did not receive a title not found, accept the requested title as
    // the selected title. First and upmost this handles titles being reported
    // as VTS by DVDNAV, but it also makes it possible to sequentially play all
    // titles, needed because MPV does not support menus.
    if (!received_title_not_found) {
        WZDEBUGOBJ(QString("Notify requested title %1").arg(title));
        notifyTitleTrackChanged(title);
        return;
    }

    WZWARNOBJ("Requested title " + QString(title) + " not found");

    // Let the playlist try the next title if a valid title was requested and
    // there is more than 1 title.
    if (md->titles.count() > 1 && title <= md->titles.count()) {
        // Need to notify the requested title, otherwise the playlist will
        // select the second title instead of the title after this one.
        notifyTitleTrackChanged(title);
        // Ask player to quit
        quit(0);
        // Pass eof to trigger playNext() in playlist
        received_end_of_file = true;
        return;
    }

    // Accept defeat
    quit(TExitMsg::ERR_TITLE_NOT_FOUND);
}

void TMPVProcess::checkTime(int ms) {

    switch (md->detected_type) {
        case TMediaData::TYPE_FILE:
        case TMediaData::TYPE_STREAM:
        case TMediaData::TYPE_TV:
        case TMediaData::TYPE_UNKNOWN:
            break;
        case TMediaData::TYPE_CDDA:
        case TMediaData::TYPE_VCD: {
            // Note: using md->pos_sec_ms instaed of sec, to use player time
            int chapter = md->chapters.idForTime(md->getPosSec(), false);
            int title;
            if (chapter < 0) {
                title = -1;
            } else {
                title = chapter - md->chapters.firstID() + md->titles.firstID();
            }
            notifyTitleTrackChanged(title);
        }
            break;
        case TMediaData::TYPE_DVD:
        case TMediaData::TYPE_DVDNAV:
        case TMediaData::TYPE_BLURAY:
            if (notified_player_is_running
                    && md->duration_ms > 0
                    && ms >= md->duration_ms
                    && !quit_send) {
                WZDEBUGOBJ(QString("Time %1 ms is past end of title %2 ms."
                                   " Quitting.")
                           .arg(ms).arg(md->duration_ms));
                quit(0);
                received_end_of_file =  true;
            }
    }
}

bool TMPVProcess::parseTitleSwitched(QString disc_type, int title) {
    Q_UNUSED(title)

    // MPV uses dvdnav to play DVDs, but without support for menus
    if (disc_type == "dvdnav") {
        md->detected_type = TMediaData::TYPE_DVD;
    } else {
        md->detected_type = md->stringToType(disc_type);
    }

    if (disc_type != "cdda" && disc_type != "vcd") {
        // When a title ends and hits a menu MPV can go haywire on invalid
        // video time stamps. By setting quit_at_end_of_title, parseLine() will
        // release it from its suffering when the title ends by sending a quit
        // and fake an eof, so the playlist can play the next item.
        if (notified_player_is_running && !quit_at_end_of_title) {
            // Set ms to wait before quitting. Cannnot rely on timestamp video,
            // because it can switch before the end of the title is reached.
            // Notes on margins:
            // - checkTime() quits when time > duration
            // - Current time can lag behind
            // - Menus or jumps back in time tend to be triggered at the end
            //   of a title
            // - Quit needs time to arrive
            quit_at_end_of_title = true;
            quit_at_end_of_title_ms = md->duration_ms - md->pos_gui_ms;
            // Quit right away if less than 400 ms to go
            if (quit_at_end_of_title_ms <= 400) {
                WZDEBUGOBJ("Quitting at end of title");
                quit(0);
                received_end_of_file =  true;
            } else {
                // Quit when quit_at_end_of_title_ms elapsed
                quit_at_end_of_title_ms -= 400;
                quit_at_end_of_title_time.start();
                WZDEBUGOBJ("Marked title to quit in "
                        + QString::number(quit_at_end_of_title_ms) + " ms");
            }
        }
    }

    return true;
}

bool TMPVProcess::parseTitleNotFound(const QString&) {

    // Requested title means the original title. The currently selected title
    // seems still valid and is the last selected title during its search
    // through the disc.
    received_title_not_found = true;
    return true;
}

void TMPVProcess::save() {
}

void TMPVProcess::convertChaptersToTitles() {

    // Just for safety...
    if (md->titles.count() > 0) {
        WZWARNOBJ("Found unexpected titles");
        return;
    }
    if (md->chapters.count() == 1) {
        // Playing a single track
        int firstChapter = md->chapters.firstID();
        md->titles.addTrack(md->titles.getSelectedID(),
                            md->chapters[firstChapter].getName(),
                            md->getDurationSec());
        md->chapters.setSelectedID(firstChapter);
    } else {
        // Playing all tracks
        Maps::TChapters::TChapterIterator i = md->chapters.getIterator();
        if (i.hasNext()) {
            i.next();
            Maps::TChapterData prev_chapter = i.value();
            while (i.hasNext()) {
                i.next();
                Maps::TChapterData chapter = i.value();
                double duration = chapter.getStart() - prev_chapter.getStart();
                md->titles.addTrack(prev_chapter.getID() + 1,
                                    prev_chapter.getName(),
                                    duration);
                prev_chapter = chapter;
            }
            // Note: md->duration no longer provided, just in case it reappears
            // TODO: Request duration
            md->titles.addTrack(prev_chapter.getID() + 1,
                                prev_chapter.getName(),
                                md->getDurationSec() - prev_chapter.getStart());
        }
    }

    WZDEBUGOBJ("Created " + QString::number(md->titles.count()) + " titles");
}

void TMPVProcess::playingStarted() {
    WZDEBUGOBJ("");

    // MPV can give negative times for TS without giving a start time.
    // Correct them by setting the start time.
    if (!md->start_ms_used && md->pos_ms < 0) {
        WZDOBJ << "Setting negative start time" << md->pos_ms << "ms";
        md->start_ms = md->pos_ms;
        // No longer need rollover protection (though not set for MPV anyway).
        md->mpegts = false;
        notifyTime(md->getPosSec());
    }

    if (TMediaData::isCD(md->detected_type)) {
        // Convert chapters to titles for CD
        convertChaptersToTitles();
    } else if (md->detectedDisc()) {
        // Workaround titles being reported as VTS
        fixTitle();
    }

    TPlayerProcess::playingStarted();
}

void TMPVProcess::requestBitrateInfo() {
    writeToPlayer("print_text VIDEO_BITRATE=${=video-bitrate}");
    writeToPlayer("print_text AUDIO_BITRATE=${=audio-bitrate}");
}

bool TMPVProcess::parseStatusLine(const QRegExp& rx) {
    // Parse custom status line
    // rx("^T:([0-9\\.-]*)/([0-9\\.-]+) P:(yes|no) B:(yes|no) I:(yes|no)");
    // T:${=time-pos}/${=duration:${=length:0}} P:${=pause}
    // B:${=paused-for-cache} I:${=core-idle}

    paused = rx.cap(3) == "yes";

    notifyDuration(rx.cap(2).toDouble());
    notifyTime(rx.cap(1).toDouble());

    // Any pending questions?
    if (waitForAnswers()) {
        return true;
    }

    if (!notified_player_is_running) {
        // First and only run of state playing or paused
        // Base class sets notified_player_is_running to true
        playingStarted();

        if (paused) {
            emit receivedPause();
        }
        return true;
    }

    // Don't emit signal receivedPause(), it is not needed for MPV
    if (!paused) {
        // Parse status flags
        bool buff = rx.cap(4) == "yes";
        bool idle = rx.cap(5) == "yes";

        if (buff || idle) {
            buffering = true;
            emit receivedBuffering();
            return true;
        }

        if (request_bit_rate_info
                && md->pos_gui_ms > 11000
                && time.elapsed() > 11000) {
            request_bit_rate_info = false;
            requestBitrateInfo();
        }
    }

    if (buffering) {
        buffering = false;
        emit receivedBufferingEnded();
    }

    return true;
}

bool TMPVProcess::parseLine(QString& line) {

    // Custom status line. Make sure it matches!
    static QRegExp rx_status("^T:([0-9\\.-]*)/([0-9\\.-]+) P:(yes|no)"
                             " B:(yes|no) I:(yes|no)");

    // Tracks:
    static QRegExp rx_video_track("^(.*)Video\\s+--vid=(\\d+)(.*)");
    static QRegExp rx_audio_track("^(.*)Audio\\s+--aid=(\\d+)(\\s+--alang=([a-zA-Z]+))?(.*)");
    static QRegExp rx_subtitle_track("^(.*)Subs\\s+--sid=(\\d+)(\\s+--slang=([a-zA-Z]+))?(\\s+'(.*)')?(\\s+\\((.*)\\))?");

    static QRegExp rx_ao("^AO: \\[(.*)\\]");

    static QRegExp rx_video_property("^VIDEO_([A-Z]+)=\\s*(.*)");
    static QRegExp rx_audio_property("^AUDIO_([A-Z]+)=\\s*(.*)");

    static QRegExp rx_meta_data("^METADATA_LIST=(.*)");

    static QRegExp rx_chapter("^CHAPTER_(\\d+)=([0-9\\.-]+) '(.*)'");

    static QRegExp rx_title_switch("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*switched to (track|title):?\\s+(-?\\d+)",
                                   Qt::CaseInsensitive);
    static QRegExp rx_title_not_found("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*(track|title) not found",
                                   Qt::CaseInsensitive);

    static QRegExp rx_stream_title("icy-title: (.*)");

    static QRegExp rx_property("^INFO_([A-Z_]+)=\\s*(.*)");

    // Messages to show in statusline
    static QRegExp rx_message("^(\\[ytdl_hook|libdvdread: Get key)");

    // Errors
    static QRegExp rx_file_open("^\\[file\\] Cannot open file '.*': (.*)");
    static QRegExp rx_failed_open("^Failed to open (.*)\\.$");
    static QRegExp rx_failed_format("^Failed to recognize file format");
    static QRegExp rx_error_http_403("HTTP error 403 ");
    static QRegExp rx_error_http_404("HTTP error 404 ");
    static QRegExp rx_error("error", Qt::CaseInsensitive);

    static QRegExp rx_verbose("^\\[(statusline|term-msg|cplayer)\\] (.*)");


    // Check to see if a DVD title needs to be terminated
    if (quit_at_end_of_title
            && !quit_send
            && quit_at_end_of_title_time.elapsed() >= quit_at_end_of_title_ms) {

        quit_at_end_of_title = false;
        WZDEBUGOBJ("Quitting title");
        quit(0);
        received_end_of_file =  true;
        return true;
    }

    // Remove sender when using verbose
    if (rx_verbose.indexIn(line) >= 0) {
        line = rx_verbose.cap(2);
    }

    // Parse custom status line
    if (rx_status.indexIn(line) >= 0) {
        return parseStatusLine(rx_status);
    }

    // Let parent have a look at it
    if (TPlayerProcess::parseLine(line))
        return true;

    if (rx_message.indexIn(line) >= 0) {
        emit receivedMessage(line);
        return true;
    }

    // Video id, codec, name and selected
    // If enabled, track info does give lang
    if (rx_video_track.indexIn(line) >= 0) {
        return parseVideoTrack(rx_video_track.cap(2).toInt(),
                               rx_video_track.cap(3).trimmed(),
                               !rx_video_track.cap(1).trimmed().isEmpty());
    }

    // Audio id, lang, codec, name and selected
    if (rx_audio_track.indexIn(line) >= 0) {
        return parseAudioTrack(rx_audio_track.cap(2).toInt(),
                               rx_audio_track.cap(4),
                               rx_audio_track.cap(5).trimmed(),
                               !rx_audio_track.cap(1).trimmed().isEmpty());
    }

    // Subtitles id, lang, name, type and selected
    if (rx_subtitle_track.indexIn(line) >= 0) {
        return parseSubtitleTrack(rx_subtitle_track.cap(2).toInt(),
                                  rx_subtitle_track.cap(4),
                                  rx_subtitle_track.cap(6).trimmed(),
                                  rx_subtitle_track.cap(8),
                                  !rx_subtitle_track.cap(1).trimmed().isEmpty());
    }

    // AO
    if (rx_ao.indexIn(line) >= 0) {
        md->ao = rx_ao.cap(1);
        WZDEBUGOBJ("Audio driver '" + md->ao + "'");
        return true;
    }

    // Video codec
    // Fall back to generic VIDEO_CODEC in TPlayerProcess::parseVideoProperty()
    // if match fails.
    if (line.startsWith("VIDEO_CODEC=")) {
        int i = line.indexOf(" (");
        if (i >= 0) {
            md->video_codec = line.left(i).mid(12);
            md->video_codec_description = line.mid(i + 2);
            md->video_codec_description.chop(1);
            WZDEBUGOBJ("video_codec set to '" + md->video_codec + "'");
            WZDEBUGOBJ("video_codec_description set to '"
                    + md->video_codec_description + "'");
            return true;
        }
    }

    // Video property VIDEO_name and value
    if (rx_video_property.indexIn(line) >= 0) {
        return parseVideoProperty(rx_video_property.cap(1),
                                  rx_video_property.cap(2));
    }

    // Audio codec
    // Fall back to generic AUDIO_CODEC in TPlayerProcess::parseAudioProperty()
    // if match fails.
    if (line.startsWith("AUDIO_CODEC=")) {
        int i = line.indexOf(" (");
        if (i >= 0) {
            md->audio_codec = line.left(i).mid(12);
            md->audio_codec_description = line.mid(i + 2);
            md->audio_codec_description.chop(1);
            WZDEBUGOBJ("audio_codec set to '" + md->audio_codec + "'");
            WZDEBUGOBJ("audio_codec_description set to '"
                    + md->audio_codec_description + "'");
            return true;
        }
    }

    // Audio property AUDIO_name and value
    if (rx_audio_property.indexIn(line) >= 0) {
        return parseAudioProperty(rx_audio_property.cap(1),
                                  rx_audio_property.cap(2));
    }

    // Chapter id, time and title
    if (rx_chapter.indexIn(line) >= 0) {
        return parseChapter(rx_chapter.cap(1).toInt(),
                            rx_chapter.cap(2).toDouble(),
                            rx_chapter.cap(3).trimmed());
    }

    // Property INFO_name and value
    if (rx_property.indexIn(line) >= 0) {
        return parseProperty(rx_property.cap(1), rx_property.cap(2));
    }

    // Meta data METADATA_name and value
    if (rx_meta_data.indexIn(line) >= 0) {
        return parseMetaDataList(rx_meta_data.cap(1));
    }

    // Switch title
    if (rx_title_switch.indexIn(line) >= 0) {
        return parseTitleSwitched(rx_title_switch.cap(1).toLower(),
                                  rx_title_switch.cap(3).toInt());
    }

    // Title not found
    if (rx_title_not_found.indexIn(line) >= 0) {
        return parseTitleNotFound(rx_title_not_found.cap(1));
    }

    // Stream title
    if (rx_stream_title.indexIn(line) >= 0) {
        md->detected_type = TMediaData::TYPE_STREAM;
        QString s = rx_stream_title.cap(1);
        md->title = s;
        WZDEBUGOBJ("Stream title set to '" + md->title + "'");
        emit receivedStreamTitle();
        return true;
    }

    // Errors
    if (rx_file_open.indexIn(line) >= 0) {
        WZDEBUGOBJ("Storing file open failed");
        exit_code_override = TExitMsg::ERR_FILE_OPEN;
        TExitMsg::setExitCodeMsg(rx_file_open.cap(1));
        return true;
    }
    if (rx_failed_open.indexIn(line) >= 0) {
        if (exit_code_override == 0 && rx_failed_open.cap(1) == md->filename) {
            WZDEBUGOBJ("Storing open failed");
            exit_code_override = TExitMsg::ERR_OPEN;
        } else {
            WZDEBUGOBJ("Skipped open failed");
        }
    }
    if (rx_failed_format.indexIn(line) >= 0) {
        WZDEBUGOBJ("Stored unrecognized file format");
        exit_code_override = TExitMsg::ERR_FILE_FORMAT;
    }
    if (rx_error_http_403.indexIn(line) >= 0) {
        WZDEBUGOBJ("Stored HTTP 403");
        exit_code_override = TExitMsg::ERR_HTTP_403;
        return true;
    }
    if (rx_error_http_404.indexIn(line) >= 0) {
        WZDEBUGOBJ("Stored HTTP 404");
        exit_code_override = TExitMsg::ERR_HTTP_404;
        return true;
    }
    if (rx_error.indexIn(line) >= 0) {
        emit receivedMessage(line);
        return true;
    }

    return false;
}

// Start of what used to be mpvoptions.cpp and was pulled in with an include

void TMPVProcess::setMedia(const QString& media) {
    args << "--term-playing-msg="
        "VIDEO_ASPECT=${video-aspect}\n"
        "VIDEO_FPS=${=container-fps}\n"
//      "VIDEO_BITRATE=${=video-bitrate}\n"
        "VIDEO_FORMAT=${=video-format}\n"
        "VIDEO_CODEC=${=video-codec}\n"
        "VIDEO_COLORMATRIX=${=colormatrix}\n"
        "VIDEO_OUTCOLORMATRIX=${=video-out-params/colormatrix}\n"

//      "AUDIO_BITRATE=${=audio-bitrate}\n"
        "AUDIO_FORMAT=${=audio-codec-name}\n"
        "AUDIO_CODEC=${=audio-codec}\n"
        "AUDIO_RATE=${=audio-params/samplerate}\n"
        "AUDIO_NCH=${=audio-params/channel-count}\n"

        "INFO_START_TIME=${=time-start:}\n"
        "INFO_LENGTH=${=duration:${=length}}\n"
        "INFO_DEMUXER=${=current-demuxer}\n"

        "INFO_TITLES=${=disc-titles}\n"
        "INFO_CHAPTERS=${=chapters}\n"
        "INFO_ANGLE_EX=${angle}\n"
//      "INFO_TRACKS_COUNT=${=track-list/count}\n"

        "METADATA_LIST=${=metadata/list:}\n"
        "INFO_MEDIA_TITLE=${=media-title:}\n";

    args << "--term-status-msg=T:${=time-pos}/${=duration:${=length:0}}"
            " P:${=pause} B:${=paused-for-cache} I:${=core-idle}";

    // MPV interprets the ID in a DVD URL as index [0..#titles-1] instead of
    // [1..#titles]. Sigh. When no title is given it plays the longest title it
    // can find. Need to change no title to 0, otherwise fixTitle() won't work.
    // CDs work as expected, don't know about bluray, but assuming it's the
    // same.
    QString url = media;
    TDiscName disc(media);
    if (disc.valid
        && (disc.protocol == "dvd"
            || disc.protocol == "dvdnav"
            || disc.protocol == "br")) {
        if (disc.title > 0) {
            disc.title--;
        }
        url = disc.toString(false, true);
    } else if (md->image) {
        url = "mf://@" + temp_file_name;
    }

    args << url;

    capturing = false;
}

void TMPVProcess::setFixedOptions() {

    args << "--no-config";
    args << "--no-quiet";
    args << "--terminal";
    args << "--no-msg-color";
    args << "--input-file=/dev/stdin";
    //arg << "--no-osc";
    //arg << "--msg-level=vd=v";

    // Need cplayer msg level 6 to catch DVDNAV, NEW TITLE
    // if (md->selected_type == TMediaData::TYPE_DVD) {
    //    args << "-msg-level" << "cplayer=v";
    // }
}

void TMPVProcess::disableInput() {

    args << "--no-input-default-bindings";
    args << "--input-vo-keyboard=no"; // mpv < 0.21 --input-x11-keyboard=no
    args << "--no-input-cursor";
    args << "--cursor-autohide=no";
}

bool TMPVProcess::isOptionAvailable(const QString& option) {

    Player::Info::TPlayerInfo* ir = Player::Info::TPlayerInfo::obj();
    ir->getInfo();
    return ir->optionList().contains(option);
}

void TMPVProcess::addVFIfAvailable(const QString& vf, const QString& value) {

    Player::Info::TPlayerInfo* ir = Player::Info::TPlayerInfo::obj();
    ir->getInfo();
    if (ir->vfList().contains(vf)) {
        QString s = "--vf-add=" + vf;
        if (!value.isEmpty()) {
            s += "=" + value;
        }
        args << s;
        WZDEBUGOBJ("Added video filter '" + s + "'");
    } else {
        WZINFOOBJ("Video filter '" + vf + "' is not available");
    }
}

void TMPVProcess::setOption(const QString& name, const QVariant& value) {

    // Options without translation
    if (name == "wid"
        || name == "vo"
        || name == "aid"
        || name == "vid"
        || name == "volume"
        || name == "ass-styles"
        || name == "ass-force-style"
        || name == "embeddedfonts"
        || name == "osd-scale-by-window"
        || name == "osd-scale"
        || name == "speed"
        || name == "contrast"
        || name == "brightness"
        || name == "hue"
        || name == "saturation"
        || name == "gamma"
        || name == "colormatrix"
        || name == "monitorpixelaspect"
        || name == "monitoraspect"
        || name == "mc"
        || name == "framedrop"
        || name == "hwdec"
        || name == "autosync"
        || name == "dvd-device"
        || name == "cdrom-device"
        || name == "demuxer"
        || name == "shuffle"
        || name == "frames"
        || name == "hwdec-codecs"
        || name == "pause"
        || name == "fps") {

        QString s = "--" + name;
        if (!value.isNull())
            s += "=" + value.toString();
        args << s;
    } else if (name == "aspect") {
        QString s = value.toString();
        if (!s.isEmpty()) {
            if (s == "0") {
                args << "--no-video-aspect";
            } else {
                args << "--video-aspect";
                args << s;
            }
        }
    } else if (name == "cache") {
        int cache = value.toInt();
        if (cache > 31) {
            args << "--cache=" + value.toString();
        } else {
            args << "--cache=no";
        }
    } else if (name == "ss") {
        args << "--start=" + value.toString();
    } else if (name == "endpos") {
        args << "--length=" + value.toString();
    } else if (name == "loop") {
        QString o = value.toString();
        if (o == "0") {
            o = "inf";
        }
        args << "--loop=" + o;
    } else if (name == "ass") {
        args << "--sub-ass";
    } else if (name == "noass") {
        args << "--no-sub-ass";
    } else if (name == "ass-line-spacing") {
        args << "--sub-ass-line-spacing=" + value.toString();
    } else if (name == "nosub") {
        args << "--no-sub";
    } else if (name == "sub-fuzziness") {
        QString v;
        switch (value.toInt()) {
            case 1: v = "fuzzy"; break;
            case 2: v = "all"; break;
            default: v = "exact";
        }
        args << "--sub-auto=" + v;
    } else if (name == "audiofile") {
        args << "--audio-file=" + value.toString();
    } else if (name == "delay") {
        args << "--audio-delay=" + value.toString();
    } else if (name == "subdelay") {
        args << "--sub-delay=" + value.toString();
    } else if (name == "sid") {
        args << "--sid=" + value.toString();
    } else if (name == "sub") {
        sub_file = value.toString();
        args << "--sub-file=" + sub_file;
    } else if (name == "subpos") {
        args << "--sub-pos=" + value.toString();
    } else if (name == "font") {
        args << "--osd-font=" + value.toString();
    } else if (name == "subcp") {
        args << "--sub-codepage=" + value.toString();
    } else if (name == "osdlevel") {
        args << "--osd-level=" + value.toString();
    } else if (name == "sws") {
        args << "--sws-scaler=lanczos";
    } else if (name == "channels") {
        args << "--audio-channels=" + value.toString();
    } else if (name == "sub-scale"
               || name == "subfont-text-scale"
               || name == "ass-font-scale") {
        QString scale = value.toString();
        if (scale != "1")
            args << "--sub-scale=" + scale;
    } else if (name == "correct-pts") {
        bool b = value.toBool();
        if (b) args << "--correct-pts"; else args << "--no-correct-pts";
    } else if (name == "idx") {
        args << "--index=default";
    } else if (name == "subfps") {
        args << "--sub-fps=" + value.toString();
    } else if (name == "forcedsubsonly") {
        args << "--sub-forced-only";
    } else if (name == "dvdangle") {
        args << "--dvd-angle=" + value.toString();
    } else if (name == "screenshot_template") {
        args << "--screenshot-template=" + value.toString();
    } else if (name == "screenshot_format") {
        args << "--screenshot-format=" + value.toString();
    } else if (name == "keepaspect" || name == "fs") {
        bool b = value.toBool();
        if (b) args << "--" + name; else args << "--no-" + name;
    } else if (name == "ao") {
        QString o = value.toString();
        if (o.startsWith("alsa:device=")) {
            QString device = o.mid(12);
            device = device.replace("=", ":").replace(".", ",");
            o = "alsa:device=[" + device + "]";
        }
        args << "--ao=" + o;
    } else if (name == "afm") {
        QString s = value.toString();
        if (s == "hwac3")
            args << "--ad=spdif:ac3,spdif:dts";
    } else if (name == "verbose") {
        args << "-v";
        args << "--ytdl-raw-options=verbose="; // pass --verbose to youtube-dl
    } else if (name == "mute") {
        args << "--mute=yes";
    } else if (name == "vf-add") {
        if (!value.isNull())
            args << "--vf-add=" + value.toString();
    } else if (name == "af-add") {
        if (!value.isNull()) {
            args << "--af-add=" + value.toString();
        }
    } else if (name == "prefer-ipv4") {
        args << "--ytdl-raw-options=force-ipv4=";
    } else if (name == "prefer-ipv6") {
        args << "--ytdl-raw-options=force-ipv6=";
    } else {
        WZINFOOBJ("ignoring option name '" + name
               + "' value '" + value.toString() + "'");
    }
}

void TMPVProcess::addUserOption(const QString& option) {
    args << option;
}

void TMPVProcess::addVideoFilter(const QString& filter_name,
                                 const QVariant& value) {

    QString option = value.toString();

    if ((filter_name == "harddup") || (filter_name == "hue")) {
        // ignore
    } else if (filter_name == "eq2") {
        args << "--vf-add=eq";
    } else if (filter_name == "blur") {
        addVFIfAvailable("lavfi", "[unsharp=la=-1.5:ca=-1.5]");
    } else if (filter_name == "sharpen") {
        addVFIfAvailable("lavfi", "[unsharp=la=1.5:ca=1.5]");
    } else if (filter_name == "noise") {
        addVFIfAvailable("lavfi", "[noise=alls=9:allf=t]");
    } else if (filter_name == "deblock") {
        addVFIfAvailable("lavfi", "[pp=" + option +"]");
    } else if (filter_name == "dering") {
        addVFIfAvailable("lavfi", "[pp=dr]");
    } else if (filter_name == "phase") {
        addVFIfAvailable("lavfi", "[phase=" + option +"]");
    } else if (filter_name == "postprocessing") {
        addVFIfAvailable("lavfi", "[pp]");
    } else if (filter_name == "hqdn3d") {
        QString o;
        if (!option.isEmpty())
            o = "=" + option;
        addVFIfAvailable("lavfi", "[hqdn3d" + o +"]");
    } else if (filter_name == "yadif") {
        if (option == "1") {
            args << "--vf-add=yadif=field";
        } else {
            args << "--vf-add=yadif";
        }
    } else if (filter_name == "kerndeint") {
        addVFIfAvailable("lavfi", "[kerndeint=" + option +"]");
    } else if (filter_name == "lb" || filter_name == "l5") {
        addVFIfAvailable("lavfi", "[pp=" + filter_name +"]");
    } else if (filter_name == "subs_on_screenshots") {
        // Ignore
    } else if (filter_name == "screenshot") {
        if (!screenshot_dir.isEmpty()
            && isOptionAvailable("--screenshot-directory")) {
            args << "--screenshot-directory="
                    + QDir::toNativeSeparators(screenshot_dir);
        }
    } else if (filter_name == "rotate") {
        args << "--vf-add=rotate=" + option;
    } else {
        if (filter_name == "pp") {
            QString s;
            if (option.isEmpty()) {
                s = "[pp]";
            } else {
                s = "[pp=" + option + "]";
            }
            addVFIfAvailable("lavfi", s);
        } else if (filter_name == "extrastereo") {
            args << "--af-add=lavfi=[extrastereo]";
        } else if (filter_name == "karaoke") {
            /* Not supported anymore, ignore */
        } else {
            QString s = filter_name;
            if (!option.isEmpty())
                s += "=" + option;
            args << "--vf-add=" + s;
        }
    }
}

void TMPVProcess::addStereo3DFilter(const QString& in, const QString& out) {
    args << "--vf-add=stereo3d=" + in + ":" + out;
}

void TMPVProcess::addAudioFilter(const QString& filter_name,
                                 const QVariant& value) {

    QString option = value.toString();

    if (filter_name == "volnorm") {
        QString s = "drc";
        if (!option.isEmpty()) s += "=" + option;
        args << "--af-add=" + s;
    } else if (filter_name == "channels") {
        if (option == "2:2:0:1:0:0") args << "--af-add=channels=2:[0-1,0-0]";
        else
        if (option == "2:2:1:0:1:1") args << "--af-add=channels=2:[1-0,1-1]";
        else
        if (option == "2:2:0:1:1:0") args << "--af-add=channels=2:[0-1,1-0]";
    } else if (filter_name == "pan") {
        if (option == "1:0.5:0.5") {
            args << "--af-add=pan=1:[0.5,0.5]";
        }
    } else if (filter_name == "equalizer") {
        previous_audio_equalizer = option;
        args << "--af-add=equalizer=" + option;
    } else {
        QString s = filter_name;
        if (!option.isEmpty()) s += "=" + option;
        args << "--af-add=" + s;
    }
}

void TMPVProcess::setVolume(int v) {
    writeToPlayer("set volume " + QString::number(v));
}

void TMPVProcess::setOSDLevel(int level) {
    writeToPlayer("no-osd set osd-level " + QString::number(level));
}

void TMPVProcess::setAudio(int ID) {
    writeToPlayer("set aid " + QString::number(ID));
}

void TMPVProcess::setVideo(int ID) {
    writeToPlayer("set vid " + QString::number(ID));
}

void TMPVProcess::setSubtitle(SubData::Type type, int ID) {

    writeToPlayer("set sid " + QString::number(ID));
    md->subs.setSelected(type, ID);
    emit receivedSubtitleTrackChanged();
}

void TMPVProcess::disableSubtitles() {

    writeToPlayer("set sid no");
    md->subs.clearSelected();
    emit receivedSubtitleTrackChanged();
}

void TMPVProcess::setSecondarySubtitle(SubData::Type type, int ID) {

    md->subs.setSelectedSecondary(type, ID);
    writeToPlayer("set secondary-sid " + QString::number(ID));
}

void TMPVProcess::disableSecondarySubtitles() {

    md->subs.setSelectedSecondary(SubData::None, -1);
    writeToPlayer("set secondary-sid no");
}

void TMPVProcess::setSubtitlesVisibility(bool b) {
    writeToPlayer(QString("set sub-visibility %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::seekPlayerTime(double secs,
                                 int mode,
                                 bool keyframes,
                                 bool currently_paused) {
    Q_UNUSED(currently_paused)

    QString s = "seek " + QString::number(secs) + " ";
    switch (mode) {
        case 0 : s += "relative "; break;
        case 1 : s += "absolute-percent "; break;
        case 2 : s += "absolute "; break;
    }
    if (keyframes) s += "keyframes"; else s += "exact";
    writeToPlayer(s);
}

void TMPVProcess::mute(bool b) {
    writeToPlayer(QString("set mute %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setPause(bool b) {
    writeToPlayer(QString("set pause %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::frameStep() {
    writeToPlayer("frame_step");
}

void TMPVProcess::frameBackStep() {
    writeToPlayer("frame_back_step");
}

void TMPVProcess::showOSDText(const QString& text, int duration, int level) {
    QString str = QString("show_text \"%1\" %2 %3")
                  .arg(text).arg(duration).arg(level);
    writeToPlayer(str);
}

void TMPVProcess::showFilenameOnOSD() {
    writeToPlayer("show_text \"${filename}\" 2000 0");
}

void TMPVProcess::showTimeOnOSD() {
    writeToPlayer("show_text \"${time-pos} / ${length:0} (${percent-pos}%)\" 2000 0");
}

void TMPVProcess::setContrast(int value) {
    writeToPlayer("set contrast " + QString::number(value));
}

void TMPVProcess::setBrightness(int value) {
    writeToPlayer("set brightness " + QString::number(value));
}

void TMPVProcess::setHue(int value) {
    writeToPlayer("set hue " + QString::number(value));
}

void TMPVProcess::setSaturation(int value) {
    writeToPlayer("set saturation " + QString::number(value));
}

void TMPVProcess::setGamma(int value) {
    writeToPlayer("set gamma " + QString::number(value));
}

void TMPVProcess::setChapter(int ID) {
    writeToPlayer("set chapter " + QString::number(ID));
}

void TMPVProcess::nextChapter(int delta) {
    writeToPlayer("add chapter " + QString::number(delta));
}

void TMPVProcess::setAngle(int ID) {
    writeToPlayer("set angle " + QString::number(ID));
    writeToPlayer("print_text INFO_ANGLE_EX=${angle}");
}

void TMPVProcess::nextAngle() {
    writeToPlayer("cycle angle");
    writeToPlayer("print_text INFO_ANGLE_EX=${angle}");
}

void TMPVProcess::setExternalSubtitleFile(const QString& filename) {

    writeToPlayer("sub_add \""+ filename +"\"");
    // Remeber filename to add to subs when MPV is done with it
    sub_file = filename;
}

void TMPVProcess::setSubPos(int pos) {
    writeToPlayer("set sub-pos " + QString::number(pos));
}

void TMPVProcess::setSubScale(double value) {
    writeToPlayer("set sub-scale " + QString::number(value));
}

void TMPVProcess::setSubStep(int value) {
    writeToPlayer("sub_step " + QString::number(value));
}

void TMPVProcess::seekSub(int value) {
    writeToPlayer("sub-seek " + QString::number(value));
}

void TMPVProcess::setSubForcedOnly(bool b) {
    writeToPlayer(QString("set sub-forced-only %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setSpeed(double value) {
    writeToPlayer("set speed " + QString::number(value));
}

void TMPVProcess::enableKaraoke(bool) {
    logger()->warn("enableKaraoke: filter karaoke not supported");
}

void TMPVProcess::enableExtrastereo(bool b) {

    if (b)
        writeToPlayer("af add lavfi=[extrastereo]");
    else
        writeToPlayer("af del lavfi=[extrastereo]");
 }

void TMPVProcess::enableVolnorm(bool b, const QString& option) {

    if (b) {
        writeToPlayer("af add drc=" + option);
    } else {
        writeToPlayer("af del drc=" + option);
    }
}

void TMPVProcess::setAudioEqualizer(const QString& values) {

    if (values == previous_audio_equalizer) {
        return;
    }
    if (!previous_audio_equalizer.isEmpty()) {
        writeToPlayer("af del equalizer=" + previous_audio_equalizer);
    }
    writeToPlayer("af add equalizer=" + values);
    previous_audio_equalizer = values;
}

void TMPVProcess::setAudioDelay(double delay) {
    writeToPlayer("set audio-delay " + QString::number(delay));
}

void TMPVProcess::setSubDelay(double delay) {
    writeToPlayer("set sub-delay " + QString::number(delay));
}

void TMPVProcess::setLoop(int v) {
    QString o;
    switch (v) {
        case -1: o = "no"; break;
        case 0: o = "inf"; break;
        default: o = QString::number(v);
    }
    writeToPlayer(QString("set loop %1").arg(o));
}

void TMPVProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
    writeToPlayer(QString("screenshot %1 %2")
                  .arg(include_subtitles ? "subtitles" : "video")
                  .arg(t == Single ? "single" : "each-frame"));
}

void TMPVProcess::switchCapturing() {

    if (!capture_filename.isEmpty()) {
        QString f;
        if (capturing) {
            f = "";
        } else {
            f = capture_filename;
#ifdef Q_OS_WIN
            // Escape backslash
            f = f.replace("\\", "\\\\");
#endif
        }
        writeToPlayer("set stream-capture \"" + f + "\"");
        capturing = !capturing;
    }
}

void TMPVProcess::setTitle(int ID) {
    writeToPlayer("set disc-title " + QString::number(ID));
}

void TMPVProcess::discSetMousePos(int, int) {

    // MPV versions later than 18 july 2015 no longer support menus

    // writeToPlayer(QString("discnav mouse_move %1 %2").arg(x).arg(y));
    // mouse_move doesn't accept options :?

    // For some reason this doesn't work either...
    // So it's not possible to select options in the dvd menus just
    // because there's no way to pass the mouse position to mpv, or it
    // ignores it.
    // writeToPlayer(QString("mouse %1 %2").arg(x).arg(y));
    // writeToPlayer("discnav mouse_move");
}

void TMPVProcess::discButtonPressed(const QString& button_name) {
    writeToPlayer("discnav " + button_name);
}

void TMPVProcess::setAspect(double aspect) {
    writeToPlayer("set video-aspect " + QString::number(aspect));
}

void TMPVProcess::toggleDeinterlace() {
    writeToPlayer("cycle deinterlace");
}

void TMPVProcess::setOSDScale(double value) {
    writeToPlayer("set osd-scale " + QString::number(value));
}

void TMPVProcess::setVideoFilter(const QString& filter,
                           bool enable,
                           const QVariant& option) {
    WZDEBUGOBJ("filter '" + filter + "', enable " + QString::number(enable)
            + ", option " + option.toString());

    QString f;
    if (filter == "format") {
        f = "format=" + option.toString();
    } else if (filter == "letterbox") {
        f = QString("expand=aspect=%1").arg(option.toDouble());
    } else if (filter == "noise") {
        f = "lavfi=[noise=alls=9:allf=t]";
    } else if (filter == "blur") {
        f = "lavfi=[unsharp=la=-1.5:ca=-1.5]";
    } else if (filter == "sharpen") {
        f = "lavfi=[unsharp=la=1.5:ca=1.5]";
    } else if (filter == "deblock") {
        f = "lavfi=[pp=" + option.toString() +"]";
    } else if (filter == "dering") {
        f = "lavfi=[pp=dr]";
    } else if (filter == "phase") {
        f = "lavfi=[phase=" + option.toString() +"]";
    } else if (filter == "postprocessing") {
        f = "lavfi=[pp]";
    } else if (filter == "hqdn3d") {
        QString o = option.toString();
        if (!o.isEmpty()) o = "=" + o;
        f = "lavfi=[hqdn3d" + o +"]";
    } else if (filter == "rotate") {
        f = "rotate=" + option.toString();
    } else if (filter == "flip" || filter == "mirror") {
        f = filter;
    } else if (filter == "scale" || filter == "gradfun") {
        f = filter;
        QString o = option.toString();
        if (!o.isEmpty()) f += "=" + o;
    } else if (filter == "lb" || filter == "l5") {
        f = "lavfi=[pp=" + filter +"]";
    } else if (filter == "yadif") {
        if (option.toString() == "1") {
            f = "yadif=field";
        } else {
            f = "yadif";
        }
    } else if (filter == "kerndeint") {
        f = "lavfi=[kerndeint=" + option.toString() +"]";
    } else {
        WZDEBUGOBJ("unknown filter '" + filter + "'");
    }

    if (!f.isEmpty()) {
        writeToPlayer(QString("vf %1 \"%2\"")
                      .arg(enable ? "add" : "del").arg(f));
    }
}

void TMPVProcess::setStereo3DFilter(bool enable,
                                       const QString& in,
                                       const QString& out) {
    QString filter = "stereo3d=" + in + ":" + out;
    writeToPlayer(QString("vf %1 \"%2\"")
                  .arg(enable ? "add" : "del").arg(filter));
}

void TMPVProcess::setSubStyles(const Settings::TAssStyles& styles,
                               const QString&) {

    using namespace Settings;
    QString font = styles.fontname;
    //arg << "--sub-text-font=" + font.replace(" ", "");
    args << "--sub-text-font=" + font;
    args << "--sub-text-color=#"
            + TColorUtils::colorToRRGGBB(styles.primarycolor);

    if (styles.borderstyle == TAssStyles::Outline) {
        args << "--sub-text-shadow-color=#"
                + TColorUtils::colorToRRGGBB(styles.backcolor);
    } else {
        args << "--sub-text-back-color=#"
                + TColorUtils::colorToRRGGBB(styles.outlinecolor);
    }
    args << "--sub-text-border-color=#"
            + TColorUtils::colorToRRGGBB(styles.outlinecolor);

    args << "--sub-text-border-size=" + QString::number(styles.outline * 2.5);
    args << "--sub-text-shadow-offset=" + QString::number(styles.shadow * 2.5);

    if (isOptionAvailable("--sub-text-font-size")) {
        args << "--sub-text-font-size="
                + QString::number(styles.fontsize * 2.5);
    }
    if (isOptionAvailable("--sub-text-bold")) {
        args << QString("--sub-text-bold=%1").arg(styles.bold ? "yes" : "no");
    }

    QString halign;
    switch (styles.halignment) {
        case TAssStyles::Left: halign = "left"; break;
        case TAssStyles::Right: halign = "right"; break;
    }

    QString valign;
    switch (styles.valignment) {
        case TAssStyles::VCenter: valign = "center"; break;
        case TAssStyles::Top: valign = "top"; break;
    }

    if (!halign.isEmpty() || !valign.isEmpty()) {
        if (isOptionAvailable("--sub-text-align-x")) {
            if (!halign.isEmpty()) args << "--sub-text-align-x=" + halign;
            if (!valign.isEmpty()) args << "--sub-text-align-y=" + valign;
        }
    }
}

} // namespace Process
} // namespace Player


#include "moc_mpvprocess.cpp"

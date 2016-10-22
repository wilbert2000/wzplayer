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

#include "player/process/mplayerprocess.h"

#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QApplication>
#include <QTimer>

#include "config.h"
#include "player/process/exitmsg.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "subtracks.h"
#include "maps/titletracks.h"

using namespace Settings;

namespace Player {
namespace Process {

const double FRAME_BACKSTEP_DEFAULT_STEP = 0.1;
const double FRAME_BACKSTEP_DISABLED = 3600000;

bool TMPlayerProcess::restore_dvdnav = false;
int TMPlayerProcess::dvdnav_vts_to_restore;
int TMPlayerProcess::dvdnav_title_to_restore_vts;
int TMPlayerProcess::dvdnav_title_to_restore;
double TMPlayerProcess::dvdnav_time_to_restore;


TMPlayerProcess::TMPlayerProcess(QObject* parent, TMediaData* mdata) :
	TPlayerProcess(parent, mdata),
    debug(logger()),
    mute_option_set(false) {
}

TMPlayerProcess::~TMPlayerProcess() {
}

void TMPlayerProcess::clearSubSources() {

	sub_source = -1;
	sub_file = false;
	sub_vob = false;
	sub_demux = false;
	sub_file_id = -1;
}

bool TMPlayerProcess::startPlayer() {

    start_frame_set = false;
    start_frame = 0;

	zoom = 1;
	pan_x = 0;
	pan_y = 0;

	clearSubSources();
	frame_backstep_time_start = FRAME_BACKSTEP_DISABLED;
	clip_info_id = -1;

	return TPlayerProcess::startPlayer();
}

void TMPlayerProcess::getSelectedSubtitles() {
    logger()->debug("getSelectedSubtitles");

	if (md->subs.count() > 0) {
        writeToPlayer("get_property sub_source");
		if (sub_file)
            writeToPlayer("get_property sub_file");
		if (sub_vob)
            writeToPlayer("get_property sub_vob");
		if (sub_demux)
            writeToPlayer("get_property sub_demux");
	}
}

void TMPlayerProcess::getSelectedTracks() {
    logger()->debug("getSelectedTracks");

	if (md->videos.count() > 0)
        writeToPlayer("get_property switch_video");
	if (md->audios.count() > 0)
        writeToPlayer("get_property switch_audio");
	getSelectedSubtitles();
}

void TMPlayerProcess::getSelectedAngle() {

	if (md->videos.count() > 0) {
        logger()->debug("getSelectedAngle");
        // Need "angle/number of angles", hence use run instead of
		// get_property angle, which only gives the current angle
        writeToPlayer("run \"echo ID_ANGLE_EX=${angle}\"");
	}
}

bool TMPlayerProcess::parseVideoProperty(const QString& name,
                                         const QString& value) {

	if (name == "ID") {
		int id = value.toInt();
		if (md->videos.contains(id)) {
            logger()->debug("parseVideoProperty: found video track id %1", id);
			get_selected_video_track = true;
		} else {
			md->videos.addID(id);
			video_tracks_changed = true;
            logger()->debug("parseVideoProperty: added video track id %1", id);
		}
		return true;
	}

	if (name == "TRACK") {
		static QRegExp rx_track("(\\d+)");
		if (rx_track.indexIn(value) >= 0) {
			return setVideoTrack(rx_track.cap(1).toInt());
		}
		if (value == "disabled") {
            logger()->debug("parseVideoProperty: no video track");
			return true;
		}
        logger()->warn("parseVideoProperty: failed to parse video track ID '"
                     + value + "'");
		return false;
	}

	return TPlayerProcess::parseVideoProperty(name, value);
}

bool TMPlayerProcess::parseAudioProperty(const QString& name,
                                         const QString& value) {

	// Audio ID
	if (name == "ID") {
		int id = value.toInt();
		if (md->audios.contains(id)) {
            logger()->debug("parseAudioProperty: found audio track id %1", id);
			get_selected_audio_track = true;
		} else {
			md->audios.addID(id);
			audio_tracks_changed = true;
            logger()->debug("parseAudioProperty: added audio track id %1", id);
		}
		return true;
	}

	if (name == "TRACK") {
		static QRegExp rx_track("(\\d+)");
		if (rx_track.indexIn(value) >= 0) {
			return setAudioTrack(rx_track.cap(1).toInt());
		}
		if (value == "disabled") {
            logger()->debug("parseAudioProperty: no audio track");
			return true;
		}
		if (!value.startsWith("$")) {
            logger()->warn("parseAudioProperty: failed to parse audio track ID"
                           " '%1'", value);
		}
		return false;
	}

	return TPlayerProcess::parseAudioProperty(name, value);
}

bool TMPlayerProcess::parseSubID(const QString& type, int id) {

	// Add new id or a sub got selected

	SubData::Type sub_type;
	if (type == "FILE_SUB") {
		sub_type = SubData::File;
		sub_file = true;
		// Remember id in case there is a filename comming
		sub_file_id = id;
	} else if (type == "VOBSUB") {
		sub_type = SubData::Vob;
		sub_vob = true;
	} else {
		sub_type = SubData::Sub;
		sub_demux = true;
	}

	if (md->subs.find(sub_type, id) < 0) {
		md->subs.add(sub_type, id);
		subtitles_changed = true;
        logger()->debug("parseSubID: created subtitle id %1 type '%2'",
                        QString::number(id), type);
	} else {
        logger()->debug("parseSubID: found subtitle id %1 type '%2'",
                      QString::number(id), type);
		get_selected_subtitle = true;
	}

	return true;
}

bool TMPlayerProcess::parseSubTrack(const QString& type,
                                    int id,
                                    const QString& name,
                                    const QString& value) {

	SubData::Type sub_type;
	if (type == "VSID")	{
		sub_type = SubData::Vob;
		sub_vob = true;
	} else {
		sub_type = SubData::Sub;
		sub_demux = true;
	}

	if (md->subs.find(sub_type, id) < 0) {
        logger()->debug("parseSubTrack: adding new subtitle id %1", id);
		md->subs.add(sub_type, id);
	}

	if (name == "NAME")
		md->subs.changeName(sub_type, id, value);
	else
		md->subs.changeLang(sub_type, id, value);
	subtitles_changed = true;

    debug << "parseSubTrack: updated subtitle id" << id
          << "type" << type << "field" << name << "to" << value << debug;
	return true;
}

bool TMPlayerProcess::setVideoTrack(int id) {

    logger()->debug("setVideoTrack: selecting video track with id %1", id);
	md->videos.setSelectedID(id);
	if (notified_player_is_running) {
		emit receivedVideoTrackChanged(id);
	}
	return true;
}

bool TMPlayerProcess::setAudioTrack(int id) {

    logger()->debug("setAudioTrack: selecting audio track with id %1", id);
	md->audios.setSelectedID(id);
	if (notified_player_is_running) {
		emit receivedAudioTrackChanged(id);
	}
	return true;
}

bool TMPlayerProcess::parseAnswer(const QString& name, const QString& value) {

	if (name == "LENGTH") {
		notifyDuration(value.toDouble());
		return true;
	}

	int i = value.toInt();

	// Video track
	if (name == "SWITCH_VIDEO") {
		return setVideoTrack(i);
	}

	// Audio track
	if (name == "SWITCH_AUDIO") {
		return setAudioTrack(i);
	}

	// Subtitle track
	if (name == "SUB_SOURCE") {
        logger()->debug("parseAnswer: subtitle source set to %1", i);
		sub_source = i;
		if (i < 0 && md->subs.selectedID() >= 0) {
			md->subs.clearSelected();
			emit receivedSubtitleTrackChanged();
		}
		return true;
	}

	if (name == "SUB_DEMUX") {
		if (sub_source == SubData::Sub) {
            logger()->debug("parseAnswer: selected subtitle track id %1 from"
                            " demuxer", i);
			md->subs.setSelected(SubData::Sub, i);
			emit receivedSubtitleTrackChanged();
		} else {
            logger()->debug("parseAnswer: did not select subs from demuxer");
		}
		return true;
	}

	if (name == "SUB_VOB") {
		if (sub_source == SubData::Vob) {
            logger()->debug("parseAnswer: selected VOB subtitle track id %1",
                            i);
			md->subs.setSelected(SubData::Vob, i);
			emit receivedSubtitleTrackChanged();
		} else {
            logger()->debug("parseAnswer: did not select VOB subtitles");
		}
		return true;
	}

	if (name == "SUB_FILE") {
		if (sub_source == SubData::File) {
            logger()->debug("parseAnswer: selected subtitle track id %1 from"
                            " external file", i);
			md->subs.setSelected(SubData::File, i);
			emit receivedSubtitleTrackChanged();
		} else {
            logger()->debug("parseAnswer: did not select external subtitles");
		}
		return true;
	}

	if (name != "ERROR") {
        logger()->warn("parseAnswer: unexpected answer '" + name
                     + "' = '" + value + "'");
	}

	return false;
}

bool TMPlayerProcess::parseClipInfoName(int id, const QString& name) {

	clip_info_id = id;
	clip_info_name = name;
	return true;
}

bool TMPlayerProcess::parseClipInfoValue(int id, const QString& value) {

	bool result;
	if (id == clip_info_id) {
		result = parseMetaDataProperty(clip_info_name, value);
	} else {
        logger()->warn("TMPlayerProcess::parseClipInfoValue: unexpected value"
                       " id %1", id);
		result = false;
	}
	clip_info_id = -1;
	clip_info_name = "";
	return result;
}

bool TMPlayerProcess::dvdnavVTSChanged(int vts) {
    logger()->debug("dvdnavVTSChanged: selecting VTS %1", vts);

	md->detected_type = TMediaData::TYPE_DVDNAV;
	md->titles.setSelectedVTS(vts);

	if (notified_player_is_running) {

		// Videos
		// Videos don't get reannounced

		// Audios
        logger()->debug("dvdnavVTSChanged: clearing audio tracks");
		md->audios = Maps::TTracks();
		audio_tracks_changed = true;

		// Subs
		// Don't need clear, though can get updated...
		// clearSubSources();
		// md->subs.clear();
		// subtitles_changed = true;
	}

	return true;
}

bool TMPlayerProcess::dvdnavTitleChanged(int title) {
    logger()->debug("dvdnavTitleChanged: title changed from %1 to %1",
		   md->titles.getSelectedID(), title);

	// Reset start time and time
	md->start_sec = 0;
    md->start_sec_player = 0;
    md->start_sec_set = false;
    notifyTime(0);

	if (title <= 0) {
		// Menu: clear selected title, duration and chapters
		title = -1;
		md->titles.setSelectedID(title);
		md->duration = 0;
		md->chapters = Maps::TChapters();
	} else {
		// Title: select it and mark it with the current VTS
		md->titles.setSelectedTitle(title);
		// Get duration and chapters
		const Maps::TTitleData title_data = md->titles[title];
		md->duration = title_data.getDuration();
		md->chapters = title_data.chapters;

		// The duration from the title TOC is not always reliable,
		// so verify duration
        writeToPlayer("get_property length");
	}

    logger()->debug("dvdnavTitleChanged: emit durationChanged(" +
                    QString::number(md->duration) + ")");
	emit durationChanged(md->duration);

	if (notified_player_is_running) {
		getSelectedAngle();
		emit receivedTitleTrackChanged(title);
		emit receivedChapters();
	}

	return true;
}

bool TMPlayerProcess::dvdnavTitleIsMenu() {
    logger()->debug("dvdnavTitleIsMenu");

	if (notified_player_is_running) {
        notifyTime(0);
		notifyDuration(0);
		// Menus can have a length...
        writeToPlayer("get_property length");
	}

	return true;
}

void TMPlayerProcess::dvdnavSave() {

	// For DVDNAV remember the current video title set, title and pos
	// TODO: should clear restore_dvdnav on player crash
	if (md->detected_type == TMediaData::TYPE_DVDNAV) {
		restore_dvdnav = true;
		dvdnav_vts_to_restore = md->titles.getSelectedVTS();
		dvdnav_title_to_restore_vts = md->titles.getSelectedID();
		if (dvdnav_title_to_restore_vts < 0) {
			// For a menu we need a title to be able to restore the VTS the
			// menu belongs to, because there is no command to select a VTS.
			// When no title is found (-1), we just see how far we get.
			dvdnav_title_to_restore_vts = md->titles.findTitleForVTS(
				dvdnav_vts_to_restore);
		}
		dvdnav_title_to_restore = md->titles.getSelectedID();
		dvdnav_time_to_restore = md->time_sec;
		if (dvdnav_time_to_restore > md->duration) {
			dvdnav_time_to_restore = 0;
		}

		// Open the disc, not just the current title
		md->disc.title = 0;
		md->filename = md->disc.toString();
		md->selected_type = TMediaData::TYPE_DVDNAV;
        logger()->debug("dvdnavSave: saved state '" + md->filename + "'");
	} else {
		restore_dvdnav = false;
	}
}

void TMPlayerProcess::save() {
	dvdnavSave();
}

void TMPlayerProcess::dvdnavRestoreTime() {
    logger()->debug("dvdNavRestoreTime: restoring time %1",
                    dvdnav_time_to_restore);
	// seek time, abs, exact, currently paused
	seekPlayerTime(dvdnav_time_to_restore - 5, 2, true, false);
}

void TMPlayerProcess::dvdnavRestore() {
    logger()->debug("dvdnavRestore");

	restore_dvdnav = false;
	bool restore_title = true;
	bool did_set_title = false;

	// VTS
	if (dvdnav_vts_to_restore != md->titles.getSelectedVTS()) {
		if (dvdnav_title_to_restore_vts > 0) {
            logger()->debug("dvdnavRestore: restoring VTS %1 with title %1",
				   dvdnav_vts_to_restore, dvdnav_title_to_restore_vts);
			setTitle(dvdnav_title_to_restore_vts);
			did_set_title = true;
			if (dvdnav_title_to_restore_vts == dvdnav_title_to_restore) {
				restore_title = false;
			}
		} else {
            logger()->debug("dvdnavRestore: don't have a title to restore VTS"
                            " %1, canceling restore",
				   dvdnav_vts_to_restore);
			return;
		}
	}

	if (dvdnav_title_to_restore <= 0) {
		// Menu
		if (did_set_title || md->titles.getSelectedID() > 0) {
            logger()->debug("dvdnavRestore: restoring menu");
			discButtonPressed("menu");
		}
	} else {
		// Title
        if (restore_title
            && dvdnav_title_to_restore != md->titles.getSelectedID()) {
            logger()->debug("dvdnavRestore: restoring title %1",
                            dvdnav_title_to_restore);
			setTitle(dvdnav_title_to_restore);
		}
		if (dvdnav_time_to_restore > 20) {
			QTimer::singleShot(500, this, SLOT(dvdnavRestoreTime()));
		}
	}
}

// Title changed for non DVDNAV disc
bool TMPlayerProcess::titleChanged(TMediaData::Type type, int title) {
    logger()->debug("titleChanged: title %1", title);

	md->detected_type = type;
	notifyTitleTrackChanged(title);

	return true;
}

bool TMPlayerProcess::parseProperty(const QString& name, const QString& value) {

	// Track changed
	if (name == "CDDA_TRACK") {
		return titleChanged(TMediaData::TYPE_CDDA, value.toInt());
	}
	if (name == "VCD_TRACK") {
		return titleChanged(TMediaData::TYPE_VCD, value.toInt());
	}

	// DVD/Bluray title changed. DVDNAV uses its own reg expr
	if (name == "DVD_CURRENT_TITLE") {
		return titleChanged(TMediaData::TYPE_DVD, value.toInt());
	}
	if (name == "BLURAY_CURRENT_TITLE") {
		return titleChanged(TMediaData::TYPE_BLURAY, value.toInt());
	}

	// Subtitle filename
	if (name == "FILE_SUB_FILENAME") {
		if (sub_file_id >= 0) {
            logger()->debug("parseProperty: set filename sub id %1 to '%2'",
                            QString::number(sub_file_id), value);
			md->subs.changeFilename(SubData::File, sub_file_id, value);
			subtitles_changed = true;
			return true;
		}
        logger()->warn("parseProperty: unexpected subtitle filename '%1'",
                       value);
		return false;
	}

	// DVD title
	if (name == "DVD_VOLUME_ID") {
        md->title = value;
        logger()->debug("parseProperty: title set to '%1'", md->title);
		return true;
	}

	// DVD disc ID
	if (name == "DVD_DISC_ID") {
		md->dvd_id = value;
        logger()->debug("parseProperty: DVD ID set to '%1'", md->dvd_id);
		return true;
	}

	return TPlayerProcess::parseProperty(name, value);
}

bool TMPlayerProcess::parseChapter(int id, const QString& type,
                                   const QString& value) {

	if(type == "START") {
		double time = value.toDouble()/1000;
		md->chapters.addStart(id, time);
        logger()->debug("parseChapter: Chapter ID %1 starts at: %2", id, time);
	} else if(type == "END") {
		double time = value.toDouble()/1000;
		md->chapters.addEnd(id, time);
        logger()->debug("parseChapter: Chapter ID %1 ends at: %2", id, time);
	} else {
		md->chapters.addName(id, value);
        logger()->debug("parseChapter: Chapter ID %1 name: '%2'",
                        id, value.toUtf8().data());
	}

	return true;
}

bool TMPlayerProcess::parseCDTrack(const QString& type,
                                   int id,
                                   const QString& length) {

	static QRegExp rx_length("(\\d+):(\\d+):(\\d+)");

	double duration = 0;
	if (rx_length.indexIn(length) >= 0) {
		duration = rx_length.cap(1).toInt() * 60;
		duration += rx_length.cap(2).toInt();
		// MSF is 1/75 of second
		duration += ((double) rx_length.cap(3).toInt())/75;
	}

	md->titles.addDuration(id, duration, true);

    logger()->debug("parseCDTrack: added " + type + " track with duration"
                    + QString::number(duration));
	return true;
}

bool TMPlayerProcess::parseTitleLength(int id, const QString& value) {

	// DVD/Bluray title length
	md->titles.addDuration(id, value.toDouble());
    logger()->debug("parseTitleLength: length for title "
                    + QString::number(id) + " set to '" + value + "'");
	return true;
}

bool TMPlayerProcess::parseTitleChapters(Maps::TChapters& chapters,
                                         const QString& chaps) {

	static QRegExp rx_time("(\\d\\d):(\\d\\d):(\\d\\d)(.(\\d\\d\\d))?");

	int i;
	int idx = 0;
	int from = 0;
	while ((i = chaps.indexOf(",", from)) > 0) {
		QString s = chaps.mid(from, i - from);
		if (rx_time.indexIn(s) >= 0) {
			double time = rx_time.cap(1).toInt() * 3600
						  + rx_time.cap(2).toInt() * 60
						  + rx_time.cap(3).toInt()
						  + rx_time.cap(5).toDouble()/1000;
			chapters.addStart(idx, time);
		}
		from = i + 1;
		idx++;
	}

    logger()->debug("parseTitleChapters: added %1 chapters", chapters.count());
	return true;
}

bool TMPlayerProcess::parsePause() {

    if (md->time_sec > frame_backstep_time_start) {
        logger()->debug("parsePause: retrying frameBackStep at %1 looking for"
                        " %2",
                        QString::number(md->time_sec),
                        QString::number(frame_backstep_time_start));
        frameBackStep();
        return true;
    }
    frame_backstep_time_start = FRAME_BACKSTEP_DISABLED;

    paused = true;

    logger()->debug("parsePause: emit receivedPause()");
    emit receivedPause();

    return true;
}

void TMPlayerProcess::convertTitlesToChapters() {

	// Just for safety, don't overwrite
	if (md->chapters.count() > 0)
		return;

	int first_title_id = md->titles.firstID();

	Maps::TTitleTracks::TTitleTrackIterator i = md->titles.getIterator();
	double start = 0;
    foreach(const Maps::TTitleData title, md->titles) {
        md->chapters.addChapter(title.getID() - first_title_id, title.getName(),
                                start);
		start += title.getDuration();
	}

	if (md->chapters.count() > 0) {
		md->chapters.setSelectedID(md->titles.getSelectedID() - first_title_id);
	}

    logger()->debug("convertTitlesToChapters: added %1 chapers",
		   md->chapters.count());
}

void TMPlayerProcess::notifyChanges() {

	if (video_tracks_changed) {
		video_tracks_changed = false;
        logger()->debug("notifyChanges: emit receivedVideoTracks()");
		emit receivedVideoTracks();
		get_selected_video_track = true;
	}
	if (get_selected_video_track) {
		get_selected_video_track = false;
        writeToPlayer("get_property switch_video");
	}
	if (audio_tracks_changed) {
		audio_tracks_changed = false;
        logger()->debug("notifyChanges: emit receivedAudioTracks()");
		emit receivedAudioTracks();
		get_selected_audio_track = true;
	}
	if (get_selected_audio_track) {
		get_selected_audio_track = false;
        writeToPlayer("get_property switch_audio");
	}
	if (subtitles_changed) {
		subtitles_changed = false;
        logger()->debug("notifyChanges: emit receivedSubtitleTracks()");
		emit receivedSubtitleTracks();
		get_selected_subtitle = true;
	}
	if (get_selected_subtitle) {
		get_selected_subtitle = false;
		getSelectedSubtitles();
	}
}

void TMPlayerProcess::playingStarted() {
    logger()->debug("playingStarted");

	// Set mute here because mplayer doesn't have an option
	// to set mute from the command line
	if (mute_option_set) {
		mute_option_set = false;
		mute(true);
	}

	// Clear notifications
	video_tracks_changed = false;
	get_selected_video_track = false;
	audio_tracks_changed = false;
	get_selected_audio_track = false;
	subtitles_changed = false;
	get_selected_subtitle = false;

	// Reset the check duration timer
	check_duration_time = md->time_sec;
	if (md->detectedDisc()) {
		// Don't check disc
		check_duration_time_diff = 360000;
	} else {
		check_duration_time_diff = 1;
	}

	if (md->duration == 0 && md->detected_type != TMediaData::TYPE_DVDNAV ) {
		// See if the duration is known by now
        writeToPlayer("get_property length");
	}

	// Get selected subtitles
	getSelectedSubtitles();

	// Create chapters from titles for vcd or audio CD
	if (TMediaData::isCD(md->detected_type)) {
		convertTitlesToChapters();
	}

	// Restore DVDNAV
	if (restore_dvdnav) {
		dvdnavRestore();
	}

	// Get the GUI going
	TPlayerProcess::playingStarted();
}

void TMPlayerProcess::parseFrame(double& s, const QString& line) {

    static QRegExp rx_frame("(\\d+)\\/");

    // Check for frame in status line. Available types:
    // 1 - no frames
    // 2 - exact frames, determining the time stamp
    // 3 - played frames, always incrementing

    md->fuzzy_time = QString::fromUtf8("\u00B1"); // +/- char;
    if (md->video_fps > 0 && rx_frame.indexIn(line) >= 0) {
        int f = rx_frame.cap(1).toInt();
        if (f > 0) {
            if (!start_frame_set) {
                start_frame_set = true;
                start_frame = f;
                logger()->debug("parseFrame: start frame set to %1. l: %2",
                                start_frame, line);
                frame_off_by_one = 0.05 + 1 / md->video_fps;
            }

            double sf = (f - start_frame) / md->video_fps;
            double d = sf - s;
            double da = qAbs(d);

            if (da <= 0.05) {
                s = sf;
                md->fuzzy_time = "=";
            } else if (da <= frame_off_by_one) {
                if (logger()->isTraceEnabled()) {
                    logger()->trace(QString("parseFrame: frame %1, start frame"
                        " %2, off by one. d: %3, l: %4")
                        .arg(f).arg(start_frame).arg(d).arg(line));
                }
                if (d < 0) {
                    start_frame--;
                    md->fuzzy_time = "<";
                } else {
                    start_frame++;
                    md->fuzzy_time = ">";
                }
            } else {
                // Resync start frame
                int nsf = f - qRound(s * md->video_fps) + 1;
                if (logger()->isTraceEnabled()) {
                    logger()->trace(QString("parseFrame: frame %1 is out of"
                        " sync, updating start frame from %2 to %3."
                        " s frame: %4, s: %5, d: %6, l: %7")
                       .arg(f).arg(start_frame).arg(nsf)
                       .arg(sf).arg(s).arg(d).arg(line));
                }
                start_frame = nsf;
            }
        }
    }
}

bool TMPlayerProcess::parseStatusLine(double secs, const QString& line) {

    parseFrame(secs, line);

    notifyTime(secs);

	if (notified_player_is_running) {
		// Normal way to go, playing, except for the first frame
		notifyChanges();

		// Check for changes in duration once in a while.
		// Abs, to protect against time wrappers like TS.
        if (!paused && qAbs(secs - check_duration_time)
            > check_duration_time_diff) {
			// Ask for length
            writeToPlayer("get_property length");
			// Wait another while
            check_duration_time = secs;
			// Just a little longer
			check_duration_time_diff *= 4;
		}
	} else {
        // First and only run of state playing.
        // Base sets notified_player_is_running.
		playingStarted();
	}

	return true;
}

bool TMPlayerProcess::parseLine(QString& line) {

	// Status line
    static QRegExp rx_av("^A: .* V: +([0-9.\\-,:]*) A");
    static QRegExp rx_a_or_v("^[AV]: *([0-9.\\-,:]+)");

	// Answers to queries
	static QRegExp rx_answer("^ANS_(.+)=(.*)");

	// Audio driver
	static QRegExp rx_ao("^AO: \\[(.*)\\]");
	// Video and audio tracks
	static QRegExp rx_video_track("^ID_VID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
	static QRegExp rx_audio_track("^ID_AID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
    static QRegExp rx_audio_track_alt("^audio stream: \\d+ format: (.*)"
                                      " language: (.*) aid: (\\d+)");
	// Video and audio properties
	static QRegExp rx_video_prop("^ID_VIDEO_([A-Z_]+)\\s*=\\s*(.*)");
	static QRegExp rx_audio_prop("^ID_AUDIO_([A-Z_]+)\\s*=\\s*(.*)");

	// Subtitles
	static QRegExp rx_sub_id("^ID_(SUBTITLE|FILE_SUB|VOBSUB)_ID=(\\d+)");
	static QRegExp rx_sub_track("^ID_(SID|VSID)_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");

	// Chapters
	static QRegExp rx_chapters("^ID_CHAPTER_(\\d+)_(START|END|NAME)=(.*)");
	static QRegExp rx_mkvchapters("\\[mkv\\] Chapter (\\d+) from");

	// CD tracks
	static QRegExp rx_cd_track("^ID_(CDDA|VCD)_TRACK_(\\d+)_MSF=(.*)");

	// DVD/BLURAY titles
	static QRegExp rx_title_length("^ID_(DVD|BLURAY)_TITLE_(\\d+)_LENGTH=(.*)");
	// DVD/BLURAY chapters
	static QRegExp rx_title_chapters("^CHAPTERS: (.*)");

	// DVDNAV
	static QRegExp rx_dvdread_vts_count("^libdvdread: Found (\\d+) VTS");
	static QRegExp rx_dvdnav_switched_vts("^DVDNAV, switched to title: (\\d+)");
	static QRegExp rx_dvdnav_new_title("^DVDNAV, NEW TITLE (\\d+)");
	static QRegExp rx_dvdnav_title_is_menu("^DVDNAV_TITLE_IS_MENU");
	static QRegExp rx_dvdnav_chapters("^TITLE (\\d+), CHAPTERS: (.*)");

    static QRegExp rx_kill_line(

        /* DVDNAV messages that kill the log */
        /* scaling mouse move, because off -msglevel cplayer=6 */
        "^(rescaled coordinates"
        /* Emitted on DVDNAV menus when image not mpeg2 compliant */
        "|\\[mpeg2video .*Invalid horizontal or vertical size value"
        "|\\[ASPECT\\] Warning: No suitable new res found)"

        /* TS Transport stream program ID */
#ifndef PROGRAM_SWITCH
        "|PROGRAM_ID="
#endif
        ")"
    );

	// Clip info
	static QRegExp rx_clip_info_name("^ID_CLIP_INFO_NAME(\\d+)=(.+)");
	static QRegExp rx_clip_info_value("^ID_CLIP_INFO_VALUE(\\d+)=(.*)");

	// Stream title and url
	static QRegExp rx_stream_title("StreamTitle='(.*)';");
    static QRegExp rx_stream_title_and_url("StreamTitle='(.*)';"
                                           "StreamUrl='(.*)';");

	// Screen shot
	static QRegExp rx_screenshot("^\\*\\*\\* screenshot '(.*)'");

	// Program switch
#if PROGRAM_SWITCH
	static QRegExp rx_program("^PROGRAM_ID=(\\d+)");
#endif

	// Catch all props
	static QRegExp rx_prop("^ID_([A-Z_]+)\\s*=\\s*(.*)");

	// Errors
	static QRegExp rx_error_open("^Failed to open (.*).");
	static QRegExp rx_error_http_403("Server returned 403:");
	static QRegExp rx_error_http_404("Server returned 404:");
	static QRegExp rx_error_no_stream_found("^No stream found to handle url ");

	// Font cache
    static QRegExp rx_fontcache("^\\[ass\\] Updating font"
                                " cache|^\\[ass\\] Init");

	// General messages to pass on to core
	static QRegExp rx_message("^(Playing "
							  "|Cache "
							  "|Generating "
							  "|Connecting "
							  "|Resolving "
							  "|Scanning "
							  "|libdvdread: Get key )");


    // Parse A: V: status line
    if (rx_av.indexIn(line) >= 0) {
        return parseStatusLine(rx_av.cap(1).toDouble(), line);
    }
    if (rx_a_or_v.indexIn(line) >= 0) {
        return parseStatusLine(rx_a_or_v.cap(1).toDouble(), line);
    }

    // Messages that kill the log
    if (rx_kill_line.indexIn(line) >= 0)
		return true;

	// First ask mom
	if (TPlayerProcess::parseLine(line))
		return true;

	// Pause
	if (line == "ID_PAUSED") {
		return parsePause();
	}

	// Answers ANS_name=value
	if (rx_answer.indexIn(line) >= 0) {
		return parseAnswer(rx_answer.cap(1).toUpper(), rx_answer.cap(2));
	}

	// AO driver
	if (rx_ao.indexIn(line) >= 0) {
		md->ao = rx_ao.cap(1);
        logger()->debug("parseLine: audio driver '%1'", md->ao);
		return true;
	}

	// Video track ID, (NAME|LANG), value
	if (rx_video_track.indexIn(line) >= 0) {
		bool changed = md->videos.updateTrack(rx_video_track.cap(1).toInt(),
											  rx_video_track.cap(2),
											  rx_video_track.cap(3));
		if (changed) video_tracks_changed = true;
		return changed;
	}

	// Audio track ID, (NAME|LANG), value
	if (rx_audio_track.indexIn(line) >= 0) {
		bool changed = md->audios.updateTrack(rx_audio_track.cap(1).toInt(),
											  rx_audio_track.cap(2),
											  rx_audio_track.cap(3));
		if (changed) audio_tracks_changed = true;
		return changed;
	}

	// Audio track alt ID, lang and format
	if (rx_audio_track_alt.indexIn(line) >= 0) {
		int id = rx_audio_track_alt.cap(3).toInt();
		bool selected = md->audios.getSelectedID() == id;
		bool changed = md->audios.updateTrack(id, rx_audio_track_alt.cap(2),
											  rx_audio_track_alt.cap(1),
											  selected);
		if (changed) audio_tracks_changed = true;
		return changed;
	}

	// Subtitle ID
	if (rx_sub_id.indexIn(line) >= 0) {
		return parseSubID(rx_sub_id.cap(1), rx_sub_id.cap(2).toInt());
	}

	// Subtitle track (SID|VSID), id, (LANG|NAME) and value
	if (rx_sub_track.indexIn(line) >= 0) {
		return parseSubTrack(rx_sub_track.cap(1), rx_sub_track.cap(2).toInt(),
							 rx_sub_track.cap(3), rx_sub_track.cap(4));
	}

	// Video property ID_VIDEO_name and value
	if (rx_video_prop.indexIn(line) >= 0) {
		return parseVideoProperty(rx_video_prop.cap(1),rx_video_prop.cap(2));
	}

	// Audio property ID_AUDIO_name and value
	if (rx_audio_prop.indexIn(line) >= 0) {
		return parseAudioProperty(rx_audio_prop.cap(1), rx_audio_prop.cap(2));
	}

	// Chapters
	if (rx_chapters.indexIn(line) >= 0) {
		return parseChapter(rx_chapters.cap(1).toInt(),
							rx_chapters.cap(2),
							rx_chapters.cap(3).trimmed());
	}

	// Matroshka chapters
	if (rx_mkvchapters.indexIn(line) >= 0) {
		int c = rx_mkvchapters.cap(1).toInt();
        logger()->debug("parseLine: adding mkv chapter %1", c);
		md->chapters.addID(c);
		return true;
	}

	// Audio/Video CD tracks
	if (rx_cd_track.indexIn(line) >= 0) {
		return parseCDTrack(rx_cd_track.cap(1),
							rx_cd_track.cap(2).toInt(),
							rx_cd_track.cap(3));
	}

	// DVD/Bluray title length
	if (rx_title_length.indexIn(line) >= 0) {
		return parseTitleLength(rx_title_length.cap(2).toInt(),
								rx_title_length.cap(3));
	}

	// DVD/Bluray chapters for title only stored in md->chapters
	if (rx_title_chapters.indexIn(line) >= 0) {
		return parseTitleChapters(md->chapters, rx_title_chapters.cap(1));
	}

	// DVDNAV chapters for title stored in md->titles[title].chapters
	if (rx_dvdnav_chapters.indexIn(line) >= 0) {
		int title = rx_dvdnav_chapters.cap(1).toInt();
		if (md->titles.contains(title))
			return parseTitleChapters(md->titles[title].chapters,
									  rx_dvdnav_chapters.cap(2));
        logger()->warn("parseLine: unexpected title '%1'", title);
		return false;
	}
	if (rx_dvdnav_switched_vts.indexIn(line) >= 0) {
		return dvdnavVTSChanged(rx_dvdnav_switched_vts.cap(1).toInt());
	}
	if (rx_dvdnav_new_title.indexIn(line) >= 0) {
		return dvdnavTitleChanged(rx_dvdnav_new_title.cap(1).toInt());
	}
	if (rx_dvdnav_title_is_menu.indexIn(line) >= 0) {
		return dvdnavTitleIsMenu();
	}
	if (rx_dvdread_vts_count.indexIn(line) >= 0) {
		int count = rx_dvdread_vts_count.cap(1).toInt();
		md->titles.setVTSCount(count);
        logger()->debug("parseLine: VTS count set to %1", count);
		return true;
	}

	// Clip info
	if (rx_clip_info_name.indexIn(line) >= 0) {
		return parseClipInfoName(rx_clip_info_name.cap(1).toInt(),
								 rx_clip_info_name.cap(2));
	}
	if (rx_clip_info_value.indexIn(line) >= 0) {
		return parseClipInfoValue(rx_clip_info_value.cap(1).toInt(),
								  rx_clip_info_value.cap(2));
	}

	// Stream title
	if (rx_stream_title_and_url.indexIn(line) >= 0) {
		QString s = rx_stream_title_and_url.cap(1);
		QString url = rx_stream_title_and_url.cap(2);
        logger()->debug("parseLine: stream title: '%1'", s);
        logger()->debug("parseLine: stream_url: '%1'", url);
		md->detected_type = TMediaData::TYPE_STREAM;
        md->title = s;
		md->stream_url = url;
		emit receivedStreamTitle();
		return true;
	}

	if (rx_stream_title.indexIn(line) >= 0) {
		QString s = rx_stream_title.cap(1);
        logger()->debug("parseLine: stream title '%1'", s);
		md->detected_type = TMediaData::TYPE_STREAM;
        md->title = s;
		emit receivedStreamTitle();
		return true;
	}

	// Screenshot
	if (rx_screenshot.indexIn(line) >= 0) {
		QString shot = rx_screenshot.cap(1);
        logger()->debug("parseLine: screenshot: '%1'", shot);
		emit receivedScreenshot(shot);
		return true;
	}

#if PROGRAM_SWITCH
	// Program switch
	if (rx_program.indexIn(line) >= 0) {
		int ID = rx_program.cap(1).toInt();
		md->programs.addID(ID);
        logger()->debug("parseLine: Added program: ID: %1", ID);
		return true;
	}
#endif

	// Catch all property ID_name = value
	if (rx_prop.indexIn(line) >= 0) {
		return parseProperty(rx_prop.cap(1), rx_prop.cap(2));
	}

	// Errors
	if (rx_error_open.indexIn(line) >= 0) {
		if (exit_code_override == 0 && rx_error_open.cap(1) == md->filename) {
            logger()->debug("parseLine: storing open failed");
			exit_code_override = TExitMsg::ERR_OPEN;
		} else {
            logger()->debug("parseLine: skipped open failed");
		}
		return true;
	}
	if (rx_error_http_403.indexIn(line) >= 0) {
        logger()->debug("parseLine: storing HTTP 403");
		exit_code_override = TExitMsg::ERR_HTTP_403;
		return true;
	}
	if (rx_error_http_404.indexIn(line) >= 0) {
        logger()->debug("parseLine: storing HTTP 404");
		exit_code_override = TExitMsg::ERR_HTTP_404;
		return true;
	}
	if (rx_error_no_stream_found.indexIn(line) >= 0) {
		if (exit_code_override == 0) {
            logger()->debug("parseLine: storing no stream");
			exit_code_override = TExitMsg::ERR_NO_STREAM_FOUND;
		} else {
            logger()->debug("parseLine: skipped no stream");
		}
		return true;
	}

	// Font cache
	if (rx_fontcache.indexIn(line) >= 0) {
        logger()->debug("parseLine: emit receivedUpdatingFontCache");
		emit receivedUpdatingFontCache();
		return true;
	}

	// Messages to display
	if (rx_message.indexIn(line) >= 0) {
        logger()->info("parseLine: '%1'", line);
		emit receivedMessage(line);
		return true;
	}

	return false;
}

void TMPlayerProcess::setMedia(const QString& media) {

	// TODO: Add sub_source?
    args << "-playing-msg"
		<< "ID_VIDEO_TRACK=${switch_video}\n"
		   "ID_AUDIO_TRACK=${switch_audio}\n"
		   "ID_ANGLE_EX=${angle}\n";

    if (md->image) {
        args << "mf://@" + temp_file_name;
    } else {
        args << media;
    }
}

void TMPlayerProcess::setFixedOptions() {
    args << "-noquiet"
		<< "-slave"
		<< "-identify"
		<< "-panscanrange" << QString::number(1 - TConfig::ZOOM_MAX);

    // Need cplayer msg level 6 to catch DVDNAV, NEW TITLE
	if (md->selected_type == TMediaData::TYPE_DVDNAV) {
        args << "-msglevel" << "cplayer=6";
	}
}

void TMPlayerProcess::disableInput() {
    args << "-nomouseinput";

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
    args << "-input" << "nodefault-bindings:conf=/dev/null";
#endif
}

void TMPlayerProcess::setCaptureDirectory(const QString& dir) {

	TPlayerProcess::setCaptureDirectory(dir);
	if (!capture_filename.isEmpty()) {
        args << "-capture" << "-dumpfile" << capture_filename;
	}
}

void TMPlayerProcess::setOption(const QString& name, const QVariant& value) {

	if (name == "cache") {
		int cache = value.toInt();
		if (cache > 31) {
            args << "-cache" << value.toString();
		} else {
            args << "-nocache";
		}
	} else if (name == "framedrop") {
		QString o = value.toString();
		if (o.contains("vo"))
            args << "-framedrop";
		if (o.contains("decoder"))
            args << "-hardframedrop";
	} else if (name == "osd-scale") {
		QString scale = value.toString();
		if (scale != "6") {
            args << "-subfont-osd-scale" << scale;
		}
	} else if (name == "verbose") {
        args << "-v";
	} else if (name == "mute") {
		// Emulate mute, executed by playingStarted()
		mute_option_set = true;
	} else if (name == "keepaspect"
			   || name == "fs"
			   || name == "flip-hebrew"
			   || name == "correct-pts"
			   || name == "fontconfig") {
		bool b = value.toBool();
		if (b) {
            args << "-" + name;
		} else {
            args << "-no" + name;
		}
	} else if (name == "aspect") {
		QString s = value.toString();
		if (!s.isEmpty()) {
			if (s == "0") {
                args << "-noaspect";
			} else {
                args << "-aspect";
                args << s;
			}
		}
	} else {
        args << "-" + name;
		if (!value.isNull()) {
            args << value.toString();
		}
	}
}

void TMPlayerProcess::addUserOption(const QString& option) {
    args << option;
}

void TMPlayerProcess::addVF(const QString& filter_name, const QVariant& value) {

	QString option = value.toString();

	if (filter_name == "blur" || filter_name == "sharpen") {
        args << "-vf-add" << "unsharp=" + option;
	} else if (filter_name == "deblock") {
        args << "-vf-add" << "pp=" + option;
	} else if (filter_name == "dering") {
        args << "-vf-add" << "pp=dr";
	} else if (filter_name == "postprocessing") {
        args << "-vf-add" << "pp";
	} else if (filter_name == "lb" || filter_name == "l5") {
        args << "-vf-add" << "pp=" + filter_name;
	} else if (filter_name == "subs_on_screenshots") {
		if (option == "ass") {
            args << "-vf-add" << "ass";
		} else {
            args << "-vf-add" << "expand=osd=1";
		}
	} else if (filter_name == "screenshot") {
		QString f = "screenshot";
		if (!screenshot_dir.isEmpty()) {
			f += "="+ QDir::toNativeSeparators(screenshot_dir + "/shot");
		}
        args << "-vf-add" << f;
	} else if (filter_name == "flip") {
		// expand + flip doesn't work well, a workaround is to add another
		// filter between them, so that's why harddup is here
        args << "-vf-add" << "harddup,flip";
	} else if (filter_name == "expand") {
        args << "-vf-add" << "expand=" + option + ",harddup";
        // Note: on some videos (h264 for instance) the subtitles doesn't
        // disappear, appearing the new ones on top of the old ones. It seems
        // adding another filter after expand fixes the problem. I chose harddup
        // 'cos I think it will be harmless in mplayer.
	} else {
		QString s = filter_name;
		if (!option.isEmpty())
			s += "=" + option;
        args << "-vf-add" << s;
	}
}

void TMPlayerProcess::addStereo3DFilter(const QString& in, const QString& out) {
	QString filter = "stereo3d=" + in + ":" + out;
	filter += ",scale"; // In my PC it doesn't work without scale :?
    args << "-vf-add" << filter;
}

void TMPlayerProcess::addAF(const QString& filter_name, const QVariant& value) {
	QString s = filter_name;
	if (!value.isNull()) s += "=" + value.toString();
    args << "-af-add" << s;
}

void TMPlayerProcess::setVolume(int v) {
    writeToPlayer("pausing_keep_force volume " + QString::number(v) + " 1");
}

void TMPlayerProcess::setOSDLevel(int level) {
    writeToPlayer("pausing_keep osd " + QString::number(level));
}

void TMPlayerProcess::setAudio(int ID) {
    writeToPlayer("switch_audio " + QString::number(ID));
}

void TMPlayerProcess::setVideo(int ID) {
    writeToPlayer("set_property switch_video " + QString::number(ID));
}

void TMPlayerProcess::setSubtitle(SubData::Type type, int ID) {

	switch (type) {
		case SubData::Vob:
            writeToPlayer("sub_vob " + QString::number(ID));
			break;
		case SubData::Sub:
            writeToPlayer("sub_demux " + QString::number(ID));
			break;
		case SubData::File:
            writeToPlayer("sub_file " + QString::number(ID));
			break;
		default: {
            logger()->warn("setSubtitle: unknown type!");
			return;
		}
	}

	md->subs.setSelected(type, ID);
	emit receivedSubtitleTrackChanged();
}

void TMPlayerProcess::disableSubtitles() {

    writeToPlayer("sub_source -1");

	md->subs.clearSelected();
	emit receivedSubtitleTrackChanged();
}

void TMPlayerProcess::setSubtitlesVisibility(bool b) {
    writeToPlayer(QString("sub_visibility %1").arg(b ? 1 : 0));
}

void TMPlayerProcess::seekPlayerTime(double secs,
                                     int mode,
                                     bool precise,
                                     bool currently_paused) {
	// seek <value> [type]
	// Seek to some place in the movie.
	// 0 is a relative seek of +/- <value> seconds (default).
	// 1 is a seek to <value> % in the movie.
	// 2 is a seek to an absolute position of <value> seconds.

	QString s = QString("seek %1 %2").arg(secs).arg(mode);
	if (precise) s += " 1"; else s += " -1";

    // pausing_keep does strange things with seek, so need to use pausing
    // instead, hence the leakage of currently_paused.
	if (currently_paused)
		s = "pausing " + s;
    paused = currently_paused;

    writeToPlayer(s);
}

void TMPlayerProcess::mute(bool b) {
    writeToPlayer("pausing_keep_force mute " + QString::number(b ? 1 : 0));
}

void TMPlayerProcess::setPause(bool pause) {

    paused = pause;
    if (pause) writeToPlayer("pausing pause");
    // else writeToPlayer("resume pause"); is buggy
    // maybe use "pausing xxx\npause"
    else writeToPlayer("pause");
}

void TMPlayerProcess::frameStep() {
    writeToPlayer("frame_step");
}

void TMPlayerProcess::frameBackStep() {

	if (frame_backstep_time_start == FRAME_BACKSTEP_DISABLED) {
        if (md->video_fps <= 0 || md->video_fps > 70) {
            frame_backstep_step = FRAME_BACKSTEP_DEFAULT_STEP;
        } else {
            frame_backstep_step = 1 / md->video_fps;
        }
        frame_backstep_time_start = md->time_sec - frame_backstep_step;
		frame_backstep_time_requested = frame_backstep_time_start;
	} else {
        // Retry call from parsePause()
        if (md->video_fps <= 0 || md->video_fps > 70) {
            frame_backstep_step += FRAME_BACKSTEP_DEFAULT_STEP;
        } else {
            frame_backstep_step += 1 / md->video_fps;
        }
        frame_backstep_time_requested -= frame_backstep_step;
	}
    if (frame_backstep_time_requested < 0) {
        frame_backstep_time_requested = 0;
    }
    logger()->debug("frameBackStep: emulating frame back step. Trying %1",
                    QString::number(frame_backstep_time_requested));

	seekPlayerTime(frame_backstep_time_requested, // time to seek
				   2,		// seek absolute
				   true,	// seek precise
				   true);	// currently paused

	// Don't retry when hitting zero
	if (frame_backstep_time_requested <= md->start_sec
		|| frame_backstep_time_requested <= 0) {
		frame_backstep_time_start = FRAME_BACKSTEP_DISABLED;
	}
}

void TMPlayerProcess::showOSDText(const QString& text,
                                  int duration,
                                  int level) {

	QString s = "pausing_keep_force osd_show_text \"" + text + "\" "
			+ QString::number(duration) + " " + QString::number(level);

    writeToPlayer(s);
}

void TMPlayerProcess::showFilenameOnOSD() {
    writeToPlayer("pausing_keep osd_show_property_text \"${filename}\" "
                 + QString::number(TConfig::MESSAGE_DURATION)
                 + " 0");
}

void TMPlayerProcess::showTimeOnOSD() {
    writeToPlayer("pausing_keep osd_show_property_text \"${time_pos} / ${length}"
                 " (${percent_pos}%)\" "
                 + QString::number(TConfig::MESSAGE_DURATION)
                 + " 0");
}

void TMPlayerProcess::setContrast(int value) {
    writeToPlayer("pausing_keep contrast " + QString::number(value) + " 1");
}

void TMPlayerProcess::setBrightness(int value) {
    writeToPlayer("pausing_keep brightness " + QString::number(value) + " 1");
}

void TMPlayerProcess::setHue(int value) {
    writeToPlayer("pausing_keep hue " + QString::number(value) + " 1");
}

void TMPlayerProcess::setSaturation(int value) {
    writeToPlayer("pausing_keep saturation " + QString::number(value) + " 1");
}

void TMPlayerProcess::setGamma(int value) {
    writeToPlayer("pausing_keep gamma " + QString::number(value) + " 1");
}

void TMPlayerProcess::setChapter(int ID) {
    writeToPlayer("seek_chapter " + QString::number(ID) +" 1");
}

void TMPlayerProcess::nextChapter(int delta) {
    writeToPlayer("seek_chapter " + QString::number(delta) +" 0");
}

void TMPlayerProcess::setAngle(int angle) {
    writeToPlayer("switch_angle " + QString::number(angle - 1));
	// Switch angle does not always succeed, so verify new angle
	getSelectedAngle();
}

void TMPlayerProcess::nextAngle() {
	// switch_angle -1 swicthes to next angle too
    writeToPlayer("switch_angle");
	getSelectedAngle();
}

void TMPlayerProcess::setExternalSubtitleFile(const QString& filename) {

	// Load it
    writeToPlayer("sub_load \""+ filename +"\"");
	// Select files as sub source
    writeToPlayer("sub_source 0");
}

void TMPlayerProcess::setSubPos(int pos) {
    writeToPlayer("sub_pos " + QString::number(pos) + " 1");
}

void TMPlayerProcess::setSubScale(double value) {
    writeToPlayer("sub_scale " + QString::number(value) + " 1");
}

void TMPlayerProcess::setSubStep(int value) {
    writeToPlayer("sub_step " + QString::number(value));
}

void TMPlayerProcess::seekSub(int) {
	/* Not supported */
    showOSDText(tr("seekSub is not supported by MPlayer"),
                TConfig::MESSAGE_DURATION, 1);
}

void TMPlayerProcess::setSubForcedOnly(bool b) {
    writeToPlayer(QString("forced_subs_only %1").arg(b ? 1 : 0));
}

void TMPlayerProcess::setSpeed(double value) {
    writeToPlayer("speed_set " + QString::number(value));
}

void TMPlayerProcess::enableKaraoke(bool b) {
    if (b) writeToPlayer("af_add karaoke"); else writeToPlayer("af_del karaoke");
}

void TMPlayerProcess::enableExtrastereo(bool b) {

    if (b) writeToPlayer("af_add extrastereo");
    else writeToPlayer("af_del extrastereo");
}

void TMPlayerProcess::enableVolnorm(bool b, const QString& option) {

    if (b) writeToPlayer("af_add volnorm=" + option);
    else writeToPlayer("af_del volnorm");
}

void TMPlayerProcess::setAudioEqualizer(const QString& values) {
    writeToPlayer("af_cmdline equalizer " + values);
}

void TMPlayerProcess::setAudioDelay(double delay) {
    writeToPlayer("pausing_keep_force audio_delay "
                 + QString::number(delay) +" 1");
}

void TMPlayerProcess::setSubDelay(double delay) {
    writeToPlayer("pausing_keep_force sub_delay "
                 + QString::number(delay) +" 1");
}

void TMPlayerProcess::setLoop(int v) {
    writeToPlayer(QString("loop %1 1").arg(v));
}

void TMPlayerProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	Q_UNUSED(include_subtitles)

	if (t == Single) {
        writeToPlayer("pausing_keep_force screenshot 0");
	} else {
        writeToPlayer("screenshot 1");
	}
}

void TMPlayerProcess::switchCapturing() {
    writeToPlayer("capturing");
}

void TMPlayerProcess::setTitle(int ID) {

    writeToPlayer("switch_title " + QString::number(ID));

	// Changing title on a menu without duration does not work :(
	// This hack seems to solve it.
	// TODO: find out what is going on and fix
	if (md->titles.getSelectedID() < 0 && md->duration <= 0) {
        logger()->debug("setTitle: fixing menu");
		// First go to menu of this VTS
		discButtonPressed("menu");
		// Select and hope...
		discButtonPressed("select");
		// And set the title again
        writeToPlayer("switch_title " + QString::number(ID));
	}
}

void TMPlayerProcess::discSetMousePos(int x, int y) {
    writeToPlayer(QString("set_mouse_pos %1 %2").arg(x).arg(y), false);
}

void TMPlayerProcess::discButtonPressed(const QString& button_name) {
    writeToPlayer("dvdnav " + button_name);
}

void TMPlayerProcess::setAspect(double aspect) {
    writeToPlayer("switch_ratio " + QString::number(aspect));
}

void TMPlayerProcess::setZoomAndPan(double zoom, double, double, int) {
	// -panscanrange is set in setFixedOptions() to allow for ZOOM_MAX zoom.

	if (notified_player_is_running) {
		// Zoom < 1 does not work.
		if (zoom < 1) {
			zoom = 1;
		}
		if (zoom != this->zoom) {
			this->zoom = zoom;
			// Map 1 - ZOOM_MAX to 0 - 1
			zoom = (zoom - 1) / (TConfig::ZOOM_MAX - 1);
            writeToPlayer("pausing_keep_force panscan "
                         + QString::number(zoom) + " 1");
		}
	}
}

#if PROGRAM_SWITCH
void TMPlayerProcess::setTSProgram(int ID) {
    writeToPlayer("set_property switch_program " + QString::number(ID));
	// TODO: check
	getSelectedTracks();
}
#endif

void TMPlayerProcess::toggleDeinterlace() {
    writeToPlayer("step_property deinterlace");
}

void TMPlayerProcess::setOSDScale(double) {
	// not supported
    //writeToPlayer("set_property subfont-osd-scale " + QString::number(value));
}

void TMPlayerProcess::changeVF(const QString&, bool, const QVariant&) {
	// not supported
}

void TMPlayerProcess::changeStereo3DFilter(bool,
                                           const QString&,
                                           const QString&) {
	// not supported
}

void TMPlayerProcess::setSubStyles(const TAssStyles& styles,
                                   const QString& assStylesFile) {
	if (assStylesFile.isEmpty()) {
        logger()->warn("setSubStyles: assStylesFile is invalid");
		return;
	}

	// Load the styles.ass file
	if (!QFile::exists(assStylesFile)) {
		// If file doesn't exist, create it
		styles.exportStyles(assStylesFile);
	}
	if (QFile::exists(assStylesFile)) {
		setOption("ass-styles", assStylesFile);
	} else {
        logger()->warn("setSubStyles: '" + assStylesFile + "' doesn't exist");
	}
}

} // namespace Process
} // namespace Player

#include "moc_mplayerprocess.cpp"

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

#include "proc/mplayerprocess.h"

#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QApplication>

#include "config.h"
#include "errormsg.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "subtracks.h"
#include "maps/titletracks.h"

using namespace Settings;

namespace Proc {

const double FRAME_BACKSTEP_TIME = 0.1;
const double FRAME_BACKSTEP_DISABLED = 3600000;

TMPlayerProcess::TMPlayerProcess(QObject* parent, TMediaData* mdata)
	: TPlayerProcess(parent, mdata)
	, mute_option_set(false) {
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

	clearSubSources();
	frame_backstep_time_start = FRAME_BACKSTEP_DISABLED;
	clip_info_id = -1;
	title_needs_update = false;
	title_hint = -2;

	return TPlayerProcess::startPlayer();
}

void TMPlayerProcess::getSelectedSubtitles() {
	qDebug("Proc::TMPlayerProcess::getSelectedSubtitles");

	if (md->subs.count() > 0) {
		writeToStdin("get_property sub_source");
		if (sub_file)
			writeToStdin("get_property sub_file");
		if (sub_vob)
			writeToStdin("get_property sub_vob");
		if (sub_demux)
			writeToStdin("get_property sub_demux");
	}
}

void TMPlayerProcess::getSelectedTracks() {
	qDebug("Proc::TMPlayerProcess::getSelectedTracks");

	if (md->videos.count() > 0)
		writeToStdin("get_property switch_video");
	if (md->audios.count() > 0)
		writeToStdin("get_property switch_audio");
	getSelectedSubtitles();
}

void TMPlayerProcess::getSelectedAngle() {

	if (md->videos.count() > 0) {
		// Need "angle/number of angles", hence use run instead of
		// get_property angle, which only gives the current angle
		writeToStdin("run \"echo ID_ANGLE=${angle}\"");
	}
}

bool TMPlayerProcess::parseVideoProperty(const QString& name, const QString& value) {

	if (name == "ID") {
		int id = value.toInt();
		if (md->videos.contains(id)) {
			qDebug("Proc::TMPlayerProcess::parseVideoProperty: found video track id %d", id);
			get_selected_video_track = true;
		} else {
			md->videos.addID(id);
			video_tracks_changed = true;
			qDebug("Proc::TMPlayerProcess::parseVideoProperty: added video track id %d", id);
		}
		return true;
	}

	if (name == "TRACK_EX") {
		static QRegExp rx_track(".*(\\d+)");
		if (rx_track.indexIn(value) >= 0) {
			return setVideoTrack(rx_track.cap(1).toInt());
		}
		if (value != "disabled") {
			qWarning() << "Proc::TMPlayerProcess:parseVideoProperty: failed to parse video track ID"
					   << value;
		}
		return false;
	}

	return TPlayerProcess::parseVideoProperty(name, value);
}

bool TMPlayerProcess::parseAudioProperty(const QString& name, const QString& value) {

	// Audio ID
	if (name == "ID") {
		int id = value.toInt();
		if (md->audios.contains(id)) {
			qDebug("Proc::TMPlayerProcess::parseAudioProperty: found audio track id %d", id);
			get_selected_audio_track = true;
		} else {
			md->audios.addID(id);
			audio_tracks_changed = true;
			qDebug("Proc::TMPlayerProcess::parseAudioProperty: added audio track id %d", id);
		}
		return true;
	}

	if (name == "TRACK_EX") {
		static QRegExp rx_track(".*(\\d+)");
		if (rx_track.indexIn(value) >= 0) {
			return setAudioTrack(rx_track.cap(1).toInt());
		}
		if (value != "disabled") {
			qWarning() << "Proc::TMPlayerProcess:parseAudioProperty: failed to parse audio track ID"
					   << value;
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
		qDebug() << "Proc::TMPlayerProcess::parseSubID: created subtitle id"
				 << id << "type" << type;
	} else {
		qDebug() << "Proc::TMPlayerProcess::parseSubID: found subtitle id"
				 << id << "type" << type;
		get_selected_subtitle = true;
	}

	return true;
}

bool TMPlayerProcess::parseSubTrack(const QString& type, int id, const QString& name, const QString& value) {

	SubData::Type sub_type;
	if (type == "VSID")	{
		sub_type = SubData::Vob;
		sub_vob = true;
	} else {
		sub_type = SubData::Sub;
		sub_demux = true;
	}

	if (md->subs.find(sub_type, id) < 0) {
		qDebug("Proc::TMPlayerProcess::parseSubTrack: adding new subtitle id %d", id);
		md->subs.add(sub_type, id);
	}

	if (name == "NAME")
		md->subs.changeName(sub_type, id, value);
	else
		md->subs.changeLang(sub_type, id, value);
	subtitles_changed = true;

	qDebug() << "Proc::TMPlayerProcess::parseSubTrack: updated subtitle id" << id
			 << "type" << type << "field" << name << "to" << value;
	return true;
}

bool TMPlayerProcess::setVideoTrack(int id) {

	if (id != md->videos.getSelectedID()) {
		qDebug("Proc::TMPlayerProcess::setVideoTrack: selecting video track with id %d", id);
		md->videos.setSelectedID(id);
		if (notified_player_is_running) {
			emit receivedVideoTrackChanged(id);
		}
		return true;
	}

	qDebug("Proc::TMPlayerProcess::setVideoTrack: video track with id %d already selected", id);
	return false;
}

bool TMPlayerProcess::setAudioTrack(int id) {

	if (id != md->audios.getSelectedID()) {
		qDebug("Proc::TMPlayerProcess::setAudioTrack: selecting audio track with id %d", id);
		md->audios.setSelectedID(id);
		if (notified_player_is_running) {
			emit receivedAudioTrackChanged(id);
		}
		return true;
	}

	qDebug("Proc::TMPlayerProcess::setAudioTrack: audio track with id %d already selected", id);
	return false;
}

bool TMPlayerProcess::parseAnswer(const QString& name, const QString& value) {

	if (name == "LENGTH") {
		double duration = value.toDouble();
		if (title_needs_update) {
			updateTitleFromDuration(duration);
		} else {
			notifyDuration(duration);
		}
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
		qDebug("Proc::TMPlayerProcess::parseAnswer: subtitle source set to %d", i);
		sub_source = i;
		if (i < 0 && md->subs.selectedID() >= 0) {
			md->subs.clearSelected();
			emit receivedSubtitleTrackChanged();
		}
		return true;
	}

	if (name == "SUB_DEMUX") {
		if (sub_source == SubData::Sub) {
			qDebug("Proc::TMPlayerProcess::parseAnswer: selected subtitle track id %d from demuxer", i);
			md->subs.setSelected(SubData::Sub, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("Proc::TMPlayerProcess::parseAnswer: did not select subtitles from demuxer");
		}
		return true;
	}

	if (name == "SUB_VOB") {
		if (sub_source == SubData::Vob) {
			qDebug("Proc::TMPlayerProcess::parseAnswer: selected VOB subtitle track id %d", i);
			md->subs.setSelected(SubData::Vob, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("Proc::TMPlayerProcess::parseAnswer: did not select VOB subtitles");
		}
		return true;
	}

	if (name == "SUB_FILE") {
		if (sub_source == SubData::File) {
			qDebug("Proc::TMPlayerProcess::parseAnswer: selected subtitle track id %d from external file", i);
			md->subs.setSelected(SubData::File, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("Proc::TMPlayerProcess::parseAnswer: did not select external subtitles");
		}
		return true;
	}

	if (name != "ERROR") {
		qWarning() << "Proc::TMPlayerProcess::parseAnswer: unexpected answer"
				   << name << "=" << value;
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
		result = parseMetaDataProperty(clip_info_name, value, false);
	} else {
		qWarning("TMPlayerProcess::parseClipInfoValue: unexpected value id %d", id);
		result = false;
	}
	clip_info_id = -1;
	clip_info_name = "";
	return result;
}

void TMPlayerProcess::clearStartTime() {
	qDebug("Proc::TMPlayerProcess::clearStartTime");

	// Reset start time. Assuming we get ID_START_TIME if title has one,
	// though never seen it, except for the first title at startup.
	md->start_sec = 0;
	md->start_sec_set = false;
}

void TMPlayerProcess::dvdnavTitleChanged(int oldTitle) {
	qDebug("Proc::TMPlayerProcess::dvdnavTitleChanged: title changed from %d to %d VTS %d",
		   oldTitle, md->titles.getSelectedID(), md->titles.getSelectedVTS());

	if (notified_player_is_running) {
		emit receivedTitleTrackChanged(md->titles.getSelectedID());
	}

	Maps::TTitleData title = md->titles[md->titles.getSelectedID()];
	notifyDuration(title.getDuration());

	// Chapters
	md->chapters = title.chapters;
	if (notified_player_is_running) {
		emit receivedChapters();
	}

	// Angles already handled by calling getSelectedAngle() in dvdnavGetTitle()
}

void TMPlayerProcess::updateTitleFromDuration(double duration) {

	title_needs_update = false;
	int old_title = md->titles.getSelectedID();
	if (md->titles.setTitleFromDuration(duration, title_hint)) {
		title_hint = -2;
		dvdnavTitleChanged(old_title);
	} else {
		title_hint = -2;
	}
}

bool TMPlayerProcess::failedToGetLength() {

	if (md->title_is_menu) {
		qDebug("Proc::TMPlayerProcess::failedToGetLength: this menu has no length");
		if (title_needs_update) {
			title_needs_update = false;
			title_hint = -2;
			// Set selected title if none selected (first time), else leave it as is.
			if (md->titles.getSelectedID() < 0) {
				md->titles.setSelectedID(1);
				if (notified_player_is_running) {
					emit receivedTitleTrackChanged(1);
				}
			}
		}
		notifyTime(0, "");
		notifyDuration(0);
		return true;
	}

	return false;
}

void TMPlayerProcess::dvdnavGetTitle() {

	clearStartTime();

	if (md->titles.count() == md->titles.getVTSCount()) {
		// 1 title per VTS
		int old_title = md->titles.getSelectedID();
		md->titles.setSelectedID(md->titles.getSelectedVTS());
		dvdnavTitleChanged(old_title);
	} else {
		// Use duration to map from vts to title
		qDebug("Proc::TMPlayerProcess::dvdnavGetTitle: VTS %d title needs update by duration",
			   md->titles.getSelectedVTS());
		title_needs_update = true;
		writeToStdin("get_property length");
	}

	// Update angles
	if (notified_player_is_running) {
		getSelectedAngle();
	}
}

bool TMPlayerProcess::parseTitleIsMenu() {
	qDebug("Proc::TMPlayerProcess::parseTitleIsMenu");

	md->title_is_menu = true;
	dvdnavGetTitle();

	// Menus can have a length. If the menu has no length we get
	// 'Failed to get value of property' and clear the duration.
	if (!title_needs_update)
		writeToStdin("get_property length");

	return true;
}

bool TMPlayerProcess::parseTitleIsMovie() {
	qDebug("Proc::TMPlayerProcess::parseTitleIsMovie");

	md->title_is_menu = false;
	dvdnavGetTitle();

	return true;
}

bool TMPlayerProcess::vtsChanged(int vts) {
	qDebug("Proc::TMPlayerProcess::vtsChanged: selecting VTS %d", vts);

	md->detected_type = TMediaData::TYPE_DVDNAV;
	md->titles.setSelectedVTS(vts);

	if (notified_player_is_running) {

		// Videos
		// Videos don't get reannounced

		// Audios
		qDebug("Proc::TMPlayerProcess::vtsChanged: clearing audio tracks");
		md->audios = Maps::TTracks();
		audio_tracks_changed = true;

		// Subs
		// Sometimes don't get reannounced...
		// clearSubSources();
		// md->subs.clear();
		// subtitles_changed = true;
	}

	return true;
}

bool TMPlayerProcess::titleChanged(TMediaData::Type type, int title) {
	qDebug("Proc::TMPlayerProcess::titleChanged: title %d", title);

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
			qDebug() << "Proc::TMPlayerProcess::parseProperty: set filename sub id"
					 << sub_file_id << "to" << value;
			md->subs.changeFilename(SubData::File, sub_file_id, value);
			subtitles_changed = true;
			return true;
		}
		qWarning() << "Proc::TMPlayerProcess::parseProperty: unexpected subtitle filename"
				   << value;
		return false;
	}

	// DVD disc ID (DVD_VOLUME_ID is not the same)
	if (name == "DVD_DISC_ID") {
		md->dvd_id = value;
		qDebug("Proc::TMPlayerProcess::parseProperty: DVD ID set to '%s'", md->dvd_id.toUtf8().data());
		return true;
	}

	return TPlayerProcess::parseProperty(name, value);
}

bool TMPlayerProcess::parseChapter(int id, const QString& type, const QString& value) {

	if(type == "START") {
		double time = value.toDouble()/1000;
		md->chapters.addStart(id, time);
		qDebug("Proc::TMPlayerProcess::parseChapter: Chapter ID %d starts at: %g", id, time);
	} else if(type == "END") {
		double time = value.toDouble()/1000;
		md->chapters.addEnd(id, time);
		qDebug("Proc::TMPlayerProcess::parseChapter: Chapter ID %d ends at: %g", id, time);
	} else {
		md->chapters.addName(id, value);
		qDebug("Proc::TMPlayerProcess::parseChapter: Chapter ID %d name: '%s'", id, value.toUtf8().data());
	}

	return true;
}

bool TMPlayerProcess::parseCDTrack(const QString& type, int id, const QString& length) {

	static QRegExp rx_length("(\\d+):(\\d+):(\\d+)");

	double duration = 0;
	if (rx_length.indexIn(length) >= 0) {
		duration = rx_length.cap(1).toInt() * 60;
		duration += rx_length.cap(2).toInt();
		// MSF is 1/75 of second
		duration += ((double) rx_length.cap(3).toInt())/75;
	}

	md->titles.addDuration(id, duration, true);

	qDebug() << "Proc::TMPlayerProcess::parseCDTrack: added" << type << "track with duration" << duration;
	return true;
}

bool TMPlayerProcess::parseTitleLength(int id, const QString& value) {

	// DVD/Bluray title length
	md->titles.addDuration(id, value.toDouble());
	qDebug() << "Proc::TMPlayerProcess::parseTitleLength: length for title"
			 << id << "set to" << value;
	return true;
}

bool TMPlayerProcess::parseTitleChapters(Maps::TChapters& chapters, const QString& chaps) {

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

	qDebug("Proc::TMPlayerProcess::parseTitleChapters: added %d chapters", chapters.count());
	return true;
}

bool TMPlayerProcess::parseVO(const QString& driver, int w, int h) {

	md->vo = driver;
	md->video_out_width = w;
	md->video_out_height = h;

	qDebug() << "Proc::TMPlayerProcess::parseVO: video out" << driver << w << "x" << h;
	return true;
}

bool TMPlayerProcess::parsePause() {

	if (md->time_sec > frame_backstep_time_start) {
		qDebug("Proc::TMPlayerProcess::parsePause(): retrying frameBackStep() at %f", md->time_sec);
		frameBackStep();
		return true;
	}
	frame_backstep_time_start = FRAME_BACKSTEP_DISABLED;

	qDebug("Proc::TMPlayerProcess::parsePause: emit receivedPause()");
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
	while (i.hasNext()) {
		i.next();
		Maps::TTitleData title = i.value();
		md->chapters.addChapter(title.getID() - first_title_id, title.getName(), start);
		start += title.getDuration();
	}

	if (md->chapters.count() > 0) {
		md->chapters.setSelectedID(md->titles.getSelectedID() - first_title_id);
	}

	qDebug("Proc::TMPlayerProcess::convertTitlesToChapters: added %d chapers",
		   md->chapters.count());
}

void TMPlayerProcess::checkTime(double sec) {
	Q_UNUSED(sec)
}

int TMPlayerProcess::getFrame(double sec, const QString& line) {
	Q_UNUSED(sec)

	// Check for frame in status line
	static QRegExp rx_frame("^[AV]:.* (\\d+)\\/.\\d+");
	if (rx_frame.indexIn(line) >= 0) {
		return rx_frame.cap(1).toInt();
	}

	// Status line timestamp resolution is only 0.1, so can be off by 10%
	// return qRound(sec * fps);
	return 0;
}

void TMPlayerProcess::notifyChanges() {

	if (video_tracks_changed) {
		video_tracks_changed = false;
		qDebug("Proc::TMPlayerProcess::notifyChanges: emit receivedVideoTracks()");
		emit receivedVideoTracks();
		get_selected_video_track = true;
	}
	if (get_selected_video_track) {
		get_selected_video_track = false;
		writeToStdin("get_property switch_video");
	}
	if (audio_tracks_changed) {
		audio_tracks_changed = false;
		qDebug("Proc::TMPlayerProcess::notifyChanges: emit receivedAudioTracks()");
		emit receivedAudioTracks();
		get_selected_audio_track = true;
	}
	if (get_selected_audio_track) {
		get_selected_audio_track = false;
		writeToStdin("get_property switch_audio");
	}
	if (subtitles_changed) {
		subtitles_changed = false;
		qDebug("Proc::TMPlayerProcess::notifyChanges: emit receivedSubtitleTracks()");
		emit receivedSubtitleTracks();
		get_selected_subtitle = true;
	}
	if (get_selected_subtitle) {
		get_selected_subtitle = false;
		getSelectedSubtitles();
	}
}

void TMPlayerProcess::playingStarted() {
	qDebug("Proc::TMPlayerProcess::playingStarted");

	// Not used yet...
	want_pause = false;

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

	if (!md->title_is_menu && md->duration == 0) {
		// Use duration from selected title if duration 0
		int title = md->titles.getSelectedID();
		if (title >= 0)
			notifyDuration(md->titles[title].getDuration());

		// See if the duration is known by now
		writeToStdin("get_property length");
	}

	// Get selected subtitles
	getSelectedSubtitles();

	// Create chapters from titles for vcd or audio CD
	if (TMediaData::isCD(md->detected_type)) {
		convertTitlesToChapters();
	}

	// Get the GUI going
	TPlayerProcess::playingStarted();
}

bool TMPlayerProcess::parseStatusLine(double time_sec, double duration, QRegExp& rx, QString& line) {

	if (TPlayerProcess::parseStatusLine(time_sec, duration, rx, line))
		return true;

	if (notified_player_is_running) {
		// Normal way to go, playing, except for the first frame
		notifyChanges();

		// Check for changes in duration once in a while.
		// Abs, to protect against time wrappers like TS.
		if (qAbs(time_sec - check_duration_time) > check_duration_time_diff) {
			// Ask for length
			writeToStdin("get_property length");
			// Wait another while
			check_duration_time = time_sec;
			// Just a little longer
			check_duration_time_diff *= 4;
		}
	} else {
		// First and only run of state playing. Base sets notified_player_is_running.
		playingStarted();
	}

	return true;
}

bool TMPlayerProcess::parseLine(QString& line) {

	// Status line
	static QRegExp rx_av("^[AV]: *([0-9,:.-]+)");

	// Answers to queries
	static QRegExp rx_answer("^ANS_(.+)=(.*)");

	// Video driver and resolution after filters and aspect applied
	static QRegExp rx_vo("^VO: \\[(.*)\\] \\d+x\\d+ => (\\d+)x(\\d+)");
	// Audio driver
	static QRegExp rx_ao("^AO: \\[(.*)\\]");
	// Video and audio tracks
	static QRegExp rx_video_track("^ID_VID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
	static QRegExp rx_audio_track("^ID_AID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
	static QRegExp rx_audio_track_alt("^audio stream: \\d+ format: (.*) language: (.*) aid: (\\d+)");
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

	// DVDNAV chapters for every title
	static QRegExp rx_dvdnav_chapters("^TITLE (\\d+), CHAPTERS: (.*)");
	static QRegExp rx_dvdnav_switch_title("^DVDNAV, switched to title: (\\d+)");
	static QRegExp rx_dvdnav_title_is_menu("^DVDNAV_TITLE_IS_MENU");
	static QRegExp rx_dvdnav_title_is_movie("^DVDNAV_TITLE_IS_MOVIE");
	static QRegExp rx_dvdread_vts_count("Found (\\d+) VTS", Qt::CaseInsensitive);

	// Clip info
	static QRegExp rx_clip_info_name("^ID_CLIP_INFO_NAME(\\d+)=(.+)");
	static QRegExp rx_clip_info_value("^ID_CLIP_INFO_VALUE(\\d+)=(.*)");

	// Stream title and url
	static QRegExp rx_stream_title("^.*StreamTitle='(.*)';");
	static QRegExp rx_stream_title_and_url("^.*StreamTitle='(.*)';StreamUrl='(.*)';");

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
	static QRegExp rx_fontcache("^\\[ass\\] Updating font cache|^\\[ass\\] Init");

	// General messages to pass on to core
	static QRegExp rx_message("^(Playing "
							  "|Cache "
							  "|Generating "
							  "|Connecting "
							  "|Resolving "
							  "|Scanning )");


	// Parse A: V: status line
	if (rx_av.indexIn(line) >= 0) {
		return parseStatusLine(rx_av.cap(1).toDouble(), 0, rx_av, line);
	}

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

	// VO driver and resolution after aspect and filters applied
	if (rx_vo.indexIn(line) >= 0) {
		return parseVO(rx_vo.cap(1), rx_vo.cap(2).toInt(),
					   rx_vo.cap(3).toInt());
	}

	// AO driver
	if (rx_ao.indexIn(line) >= 0) {
		md->ao = rx_ao.cap(1);
		qDebug() << "Proc::TMPlayerProcess::parseLine: audio driver" << md->ao;
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
		qDebug("Proc::TMPlayerProcess::parseLine: adding mkv chapter %d", c);
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
		qWarning("Proc::TMPlayerProcess::parseLine: unexpected title %d", title);
		return false;
	}
	if (rx_dvdnav_switch_title.indexIn(line) >= 0) {
		return vtsChanged(rx_dvdnav_switch_title.cap(1).toInt());
	}
	if (rx_dvdnav_title_is_menu.indexIn(line) >= 0) {
		return parseTitleIsMenu();
	}
	if (line == "Failed to get value of property 'length'.") {
		return failedToGetLength();
	}
	if (rx_dvdnav_title_is_movie.indexIn(line) >= 0) {
		return parseTitleIsMovie();
	}
	if (rx_dvdread_vts_count.indexIn(line) >= 0) {
		int count = rx_dvdread_vts_count.cap(1).toInt();
		md->titles.setVTSCount(count);
		qDebug("Proc::TMPlayerProcess::parseLine: VTS count set to %d", count);
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
		qDebug("Proc::TMPlayerProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
		qDebug("Proc::TMPlayerProcess::parseLine: stream_url: '%s'", url.toUtf8().data());
		md->detected_type = TMediaData::TYPE_STREAM;
		md->stream_title = s;
		md->stream_url = url;
		emit receivedStreamTitleAndUrl(s, url);
		return true;
	}

	if (rx_stream_title.indexIn(line) >= 0) {
		QString s = rx_stream_title.cap(1);
		qDebug("Proc::TMPlayerProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
		md->detected_type = TMediaData::TYPE_STREAM;
		md->stream_title = s;
		emit receivedStreamTitle(s);
		return true;
	}

	// Screenshot
	if (rx_screenshot.indexIn(line) >= 0) {
		QString shot = rx_screenshot.cap(1);
		qDebug("Proc::TMPlayerProcess::parseLine: screenshot: '%s'", shot.toUtf8().data());
		emit receivedScreenshot(shot);
		return true;
	}

#if PROGRAM_SWITCH
	// Program switch
	if (rx_program.indexIn(line) >= 0) {
		int ID = rx_program.cap(1).toInt();
		md->programs.addID(ID);
		qDebug("Proc::TMPlayerProcess::parseLine: Added program: ID: %d", ID);
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
			qDebug("Proc::TMPlayerProcess::parseLine: storing open failed");
			exit_code_override = TErrorMsg::ERR_OPEN;
		} else {
			qDebug("Proc::TMPlayerProcess::parseLine: skipped open failed");
		}
		return true;
	}
	if (rx_error_http_403.indexIn(line) >= 0) {
		qDebug("Proc::TMPlayerProcess::parseLine: storing HTTP 403");
		exit_code_override = TErrorMsg::ERR_HTTP_403;
		return true;
	}
	if (rx_error_http_404.indexIn(line) >= 0) {
		qDebug("Proc::TMPlayerProcess::parseLine: storing HTTP 404");
		exit_code_override = TErrorMsg::ERR_HTTP_404;
		return true;
	}
	if (rx_error_no_stream_found.indexIn(line) >= 0) {
		if (exit_code_override == 0) {
			qDebug("Proc::TMPlayerProcess::parseLine: storing no stream");
			exit_code_override = TErrorMsg::ERR_NO_STREAM_FOUND;
		} else {
			qDebug("Proc::TMPlayerProcess::parseLine: skipped no stream");
		}
		return true;
	}

	// Font cache
	if (rx_fontcache.indexIn(line) >= 0) {
		qDebug("Proc::TMPlayerProcess::parseLine: emit receivedUpdatingFontCache");
		emit receivedUpdatingFontCache();
		return true;
	}

	// Messages to display
	if (rx_message.indexIn(line) >= 0) {
		emit receivedMessage(line);
		return true;
	}

	return false;
}


// Start of what used to be mplayeroptions.cpp

void TMPlayerProcess::setMedia(const QString& media, bool is_playlist) {

	// TODO: Add sub_source?
	arg << "-playing-msg"
		<< "ID_VIDEO_TRACK_EX=${switch_video}\n"
		   "ID_AUDIO_TRACK_EX=${switch_audio}\n"
		   "ID_ANGLE=${angle}\n";
	if (is_playlist)
		arg << "-playlist";
	arg << media;
}

void TMPlayerProcess::setFixedOptions() {
	arg << "-noquiet" << "-slave" << "-identify";
}

void TMPlayerProcess::disableInput() {
	arg << "-nomouseinput";

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	arg << "-input" << "nodefault-bindings:conf=/dev/null";
#endif
}

void TMPlayerProcess::setCaptureDirectory(const QString& dir) {

	TPlayerProcess::setCaptureDirectory(dir);
	if (!capture_filename.isEmpty()) {
		arg << "-capture" << "-dumpfile" << capture_filename;
	}
}

void TMPlayerProcess::setOption(const QString& name, const QVariant& value) {

	if (name == "cache") {
		int cache = value.toInt();
		if (cache > 31) {
			arg << "-cache" << value.toString();
		} else {
			arg << "-nocache";
		}
	} else if (name == "framedrop") {
		QString o = value.toString();
		if (o.contains("vo"))
			arg << "-framedrop";
		if (o.contains("decoder"))
			arg << "-hardframedrop";
	} else if (name == "osd-scale") {
		QString scale = value.toString();
		if (scale != "6") {
			arg << "-subfont-osd-scale" << scale;
		}
	} else if (name == "verbose") {
		arg << "-v";
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
			arg << "-" + name;
		} else {
			arg << "-no" + name;
		}
	} else {
		arg << "-" + name;
		if (!value.isNull()) {
			arg << value.toString();
		}
	}
}

void TMPlayerProcess::addUserOption(const QString& option) {
	arg << option;
}

void TMPlayerProcess::addVF(const QString& filter_name, const QVariant& value) {

	QString option = value.toString();

	if (filter_name == "blur" || filter_name == "sharpen") {
		arg << "-vf-add" << "unsharp=" + option;
	} else if (filter_name == "deblock") {
		arg << "-vf-add" << "pp=" + option;
	} else if (filter_name == "dering") {
		arg << "-vf-add" << "pp=dr";
	} else if (filter_name == "postprocessing") {
		arg << "-vf-add" << "pp";
	} else if (filter_name == "lb" || filter_name == "l5") {
		arg << "-vf-add" << "pp=" + filter_name;
	} else if (filter_name == "subs_on_screenshots") {
		if (option == "ass") {
			arg << "-vf-add" << "ass";
		} else {
			arg << "-vf-add" << "expand=osd=1";
		}
	} else if (filter_name == "screenshot") {
		QString f = "screenshot";
		if (!screenshot_dir.isEmpty()) {
			f += "="+ QDir::toNativeSeparators(screenshot_dir + "/shot");
		}
		arg << "-vf-add" << f;
	} else if (filter_name == "flip") {
		// expand + flip doesn't work well, a workaround is to add another
		// filter between them, so that's why harddup is here
		arg << "-vf-add" << "harddup,flip";
	} else if (filter_name == "expand") {
		arg << "-vf-add" << "expand=" + option + ",harddup";
		// Note: on some videos (h264 for instance) the subtitles doesn't disappear,
		// appearing the new ones on top of the old ones. It seems adding another
		// filter after expand fixes the problem. I chose harddup 'cos I think
		// it will be harmless in mplayer.
	} else {
		QString s = filter_name;
		if (!option.isEmpty())
			s += "=" + option;
		arg << "-vf-add" << s;
	}
}

void TMPlayerProcess::addStereo3DFilter(const QString& in, const QString& out) {
	QString filter = "stereo3d=" + in + ":" + out;
	filter += ",scale"; // In my PC it doesn't work without scale :?
	arg << "-vf-add" << filter;
}

void TMPlayerProcess::addAF(const QString& filter_name, const QVariant& value) {
	QString s = filter_name;
	if (!value.isNull()) s += "=" + value.toString();
	arg << "-af-add" << s;
}

void TMPlayerProcess::setVolume(int v) {
	writeToStdin("pausing_keep_force volume " + QString::number(v) + " 1");
}

void TMPlayerProcess::setOSDLevel(int level) {
	writeToStdin("pausing_keep osd " + QString::number(level));
}

void TMPlayerProcess::setAudio(int ID) {
	writeToStdin("switch_audio " + QString::number(ID));
}

void TMPlayerProcess::setVideo(int ID) {
	writeToStdin("set_property switch_video " + QString::number(ID));
}

void TMPlayerProcess::setSubtitle(SubData::Type type, int ID) {

	switch (type) {
		case SubData::Vob:
			writeToStdin("sub_vob " + QString::number(ID));
			break;
		case SubData::Sub:
			writeToStdin("sub_demux " + QString::number(ID));
			break;
		case SubData::File:
			writeToStdin("sub_file " + QString::number(ID));
			break;
		default: {
			qWarning("Proc::TMPlayerProcess::setSubtitle: unknown type!");
			return;
		}
	}

	md->subs.setSelected(type, ID);
	emit receivedSubtitleTrackChanged();
}

void TMPlayerProcess::disableSubtitles() {

	writeToStdin("sub_source -1");

	md->subs.clearSelected();
	emit receivedSubtitleTrackChanged();
}

void TMPlayerProcess::setSubtitlesVisibility(bool b) {
	writeToStdin(QString("sub_visibility %1").arg(b ? 1 : 0));
}

void TMPlayerProcess::seekPlayerTime(double secs, int mode, bool precise, bool currently_paused) {
	//seek <value> [type]
	//    Seek to some place in the movie.
	//        0 is a relative seek of +/- <value> seconds (default).
	//        1 is a seek to <value> % in the movie.
	//        2 is a seek to an absolute position of <value> seconds.

	QString s = QString("seek %1 %2").arg(secs).arg(mode);
	if (precise) s += " 1"; else s += " -1";

	// pausing_keep does strange things with seek, so need to use pausing instead,
	// hence the leakage of currently_paused.
	if (currently_paused)
		s = "pausing " + s;
	want_pause = currently_paused;

	writeToStdin(s);
}

void TMPlayerProcess::mute(bool b) {
	writeToStdin("pausing_keep_force mute " + QString::number(b ? 1 : 0));
}

void TMPlayerProcess::setPause(bool pause) {

	want_pause = pause;
	if (pause) writeToStdin("pausing pause"); // pauses
	else writeToStdin("pause"); // pauses / unpauses
}

void TMPlayerProcess::frameStep() {
	writeToStdin("frame_step");
}

void TMPlayerProcess::frameBackStep() {

	if (frame_backstep_time_start == FRAME_BACKSTEP_DISABLED) {
		frame_backstep_time_start = md->time_sec - FRAME_BACKSTEP_TIME;
		frame_backstep_time_requested = frame_backstep_time_start;
	} else {
		// Retry call from parsePause()
		frame_backstep_time_requested -= FRAME_BACKSTEP_TIME;
	}
	if (frame_backstep_time_requested < 0) frame_backstep_time_requested = 0;
	qDebug("Proc::TMPlayerProcess::frameBackStep: emulating unsupported function. Trying %f",
		   frame_backstep_time_requested);

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

void TMPlayerProcess::showOSDText(const QString& text, int duration, int level) {

	QString s = "pausing_keep_force osd_show_text \"" + text + "\" "
			+ QString::number(duration) + " " + QString::number(level);

	writeToStdin(s);
}

void TMPlayerProcess::showFilenameOnOSD() {
	writeToStdin("osd_show_property_text \"${filename}\" 2000 0");
}

void TMPlayerProcess::showTimeOnOSD() {
	writeToStdin("osd_show_property_text \"${time_pos} / ${length} (${percent_pos}%)\" 2000 0");
}

void TMPlayerProcess::setContrast(int value) {
	writeToStdin("pausing_keep contrast " + QString::number(value) + " 1");
}

void TMPlayerProcess::setBrightness(int value) {
	writeToStdin("pausing_keep brightness " + QString::number(value) + " 1");
}

void TMPlayerProcess::setHue(int value) {
	writeToStdin("pausing_keep hue " + QString::number(value) + " 1");
}

void TMPlayerProcess::setSaturation(int value) {
	writeToStdin("pausing_keep saturation " + QString::number(value) + " 1");
}

void TMPlayerProcess::setGamma(int value) {
	writeToStdin("pausing_keep gamma " + QString::number(value) + " 1");
}

void TMPlayerProcess::setChapter(int ID) {
	writeToStdin("seek_chapter " + QString::number(ID) +" 1");
}

void TMPlayerProcess::setAngle(int ID) {
	writeToStdin("switch_angle " + QString::number(ID));
	// Switch angle does not always succeed, so verify new angle
	getSelectedAngle();
}

void TMPlayerProcess::nextAngle() {
	// switch_angle -1 swicthes to next angle too
	writeToStdin("switch_angle");
	getSelectedAngle();
}

void TMPlayerProcess::setExternalSubtitleFile(const QString& filename) {

	// Load it
	writeToStdin("sub_load \""+ filename +"\"");
	// Select files as sub source
	writeToStdin("sub_source 0");
}

void TMPlayerProcess::setSubPos(int pos) {
	writeToStdin("sub_pos " + QString::number(pos) + " 1");
}

void TMPlayerProcess::setSubScale(double value) {
	writeToStdin("sub_scale " + QString::number(value) + " 1");
}

void TMPlayerProcess::setSubStep(int value) {
	writeToStdin("sub_step " + QString::number(value));
}

void TMPlayerProcess::seekSub(int) {
	/* Not supported */
	showOSDText(tr("This option is not supported by MPlayer"), TConfig::MESSAGE_DURATION, 1);
}

void TMPlayerProcess::setSubForcedOnly(bool b) {
	writeToStdin(QString("forced_subs_only %1").arg(b ? 1 : 0));
}

void TMPlayerProcess::setSpeed(double value) {
	writeToStdin("speed_set " + QString::number(value));
}

void TMPlayerProcess::enableKaraoke(bool b) {
	if (b) writeToStdin("af_add karaoke"); else writeToStdin("af_del karaoke");
}

void TMPlayerProcess::enableExtrastereo(bool b) {
	if (b) writeToStdin("af_add extrastereo"); else writeToStdin("af_del extrastereo");
}

void TMPlayerProcess::enableVolnorm(bool b, const QString& option) {
	if (b) writeToStdin("af_add volnorm=" + option); else writeToStdin("af_del volnorm");
}

void TMPlayerProcess::setAudioEqualizer(const QString& values) {
	writeToStdin("af_cmdline equalizer " + values);
}

void TMPlayerProcess::setAudioDelay(double delay) {
	writeToStdin("pausing_keep_force audio_delay " + QString::number(delay) +" 1");
}

void TMPlayerProcess::setSubDelay(double delay) {
	writeToStdin("pausing_keep_force sub_delay " + QString::number(delay) +" 1");
}

void TMPlayerProcess::setLoop(int v) {
	writeToStdin(QString("loop %1 1").arg(v));
}

void TMPlayerProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	Q_UNUSED(include_subtitles)

	if (t == Single) {
		writeToStdin("pausing_keep_force screenshot 0");
	} else {
		writeToStdin("screenshot 1");
	}
}

void TMPlayerProcess::switchCapturing() {
	writeToStdin("capturing");
}

void TMPlayerProcess::setTitle(int ID) {
	title_hint = ID;
	writeToStdin("switch_title " + QString::number(ID));
}

void TMPlayerProcess::discSetMousePos(int x, int y) {
	writeToStdin(QString("set_mouse_pos %1 %2").arg(x).arg(y), false);
}

void TMPlayerProcess::discButtonPressed(const QString& button_name) {
	writeToStdin("dvdnav " + button_name);
}

void TMPlayerProcess::setAspect(double aspect) {
	writeToStdin("switch_ratio " + QString::number(aspect));
}

void TMPlayerProcess::setFullscreen(bool b) {
	writeToStdin(QString("vo_fullscreen %1").arg(b ? "1" : "0"));
}

#if PROGRAM_SWITCH
void TMPlayerProcess::setTSProgram(int ID) {
	writeToStdin("set_property switch_program " + QString::number(ID));
	// TODO: check
	getSelectedTracks();
}
#endif

void TMPlayerProcess::toggleDeinterlace() {
	writeToStdin("step_property deinterlace");
}

void TMPlayerProcess::setOSDPos(const QPoint&, int) {
	// not supported
}

void TMPlayerProcess::setOSDScale(double) {
	// not supported
	//writeToStdin("set_property subfont-osd-scale " + QString::number(value));
}

void TMPlayerProcess::changeVF(const QString&, bool, const QVariant&) {
	// not supported
}

void TMPlayerProcess::changeStereo3DFilter(bool, const QString&, const QString&) {
	// not supported
}

void TMPlayerProcess::setSubStyles(const TAssStyles & styles, const QString& assStylesFile) {
	if (assStylesFile.isEmpty()) {
		qWarning("Proc::TMPlayerProcess::setSubStyles: assStylesFile is invalid");
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
		qWarning("Proc::TMPlayerProcess::setSubStyles: '%s' doesn't exist", assStylesFile.toUtf8().constData());
	}
}

} // namespace Proc

#include "moc_mplayerprocess.cpp"

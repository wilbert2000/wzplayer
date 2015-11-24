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

#include "proc/mpvprocess.h"

#include <QDebug>
#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QApplication>

#include "proc/playerprocess.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "inforeader.h"


namespace Proc {

// Max pos accepted by MPV for OSD margins used by setOSDPos.
// TODO: get from player too
static const QPoint max_osd_pos(300, 600);

TMPVProcess::TMPVProcess(QObject* parent, TMediaData* mdata)
	: TPlayerProcess(parent, mdata)
	, verbose(false)
	, osd_pos()
	, osd_centered_x(false)
	, osd_centered_y(false) {
}

TMPVProcess::~TMPVProcess() {
}

bool TMPVProcess::startPlayer() {

	received_buffering = false;

	selected_title = -1;
	received_title_not_found = false;
	title_swictched = false;
	quit_at_end_of_title = false;

	request_bit_rate_info = true;

	osd_centered_x = false;
	osd_centered_y = false;

	return TPlayerProcess::startPlayer();
}

bool TMPVProcess::parseVideoTrack(int id,
								 const QString &codec,
								 QString name,
								 bool selected) {

	// Note lang "". Track info has lang.

	if (name.isEmpty() && !codec.isEmpty()) {
		name = "(" + codec + ")";
	}
	if (md->videos.updateTrack(id, "", name, selected)) {
		if (notified_player_is_running)
			emit receivedVideoTrackInfo();
		return true;
	}
	return false;
}

bool TMPVProcess::parseAudioTrack(int id, const QString &lang, const QString &codec, QString name, bool selected) {

	if (name.isEmpty() && !codec.isEmpty()) {
		name = "(" + codec + ")";
	}
	if (md->audios.updateTrack(id, lang, name, selected)) {
		if (notified_player_is_running)
			emit receivedAudioTrackInfo();
		return true;
	}
	return false;
}

bool TMPVProcess::parseSubtitleTrack(int id,
									const QString &lang,
									QString name,
									const QString& type,
									bool selected) {

	if (name.isEmpty() && !type.isEmpty()) {
		name = "(" + type + ")";
	}

	SubData::Type sub_type;
	QString filename;
	if (type.contains("external", Qt::CaseInsensitive)) {
		sub_type = SubData::File;
		filename = sub_file;
	} else {
		sub_type = SubData::Sub;
	}

	if (md->subs.update(sub_type, id, lang, name, filename, selected)) {
		if (notified_player_is_running)
			emit receivedSubtitleTrackInfo();
		return true;
	}

	return false;
}


bool TMPVProcess::parseProperty(const QString& name, const QString& value) {

	if (name == "MPV_VERSION") {
		mpv_version = value;
		qDebug() << "Proc::TMPVProcess::parseProperty: mpv_version set to" << mpv_version;
		return true;
	}

/*
	if (name == "TRACKS_COUNT") {
		int tracks = value.toInt();
		qDebug("Proc::TMPVProcess::parseProperty: requesting track info for %d tracks", tracks);
		for (int n = 0; n < tracks; n++) {
			writeToStdin(QString("print_text \"TRACK_INFO_%1="
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
		qDebug("Proc::TMPVProcess::parseProperty: creating %d titles", n_titles);
		for (int idx = 0; idx < n_titles; idx++) {
			md->titles.addID(idx + 1);
			writeToStdin(QString("print_text \"INFO_TITLE_LENGTH=%1 ${=disc-title-list/%1/length:-1}\"").arg(idx));
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
		qDebug("Proc::TMPVProcess::parseProperty: requesting start and title of %d chapter(s)", n_chapters);
		for (int n = 0; n < n_chapters; n++) {
			writeToStdin(QString("print_text \"CHAPTER_%1=${=chapter-list/%1/time:} '${chapter-list/%1/title:}'\"").arg(n));
		}
		waiting_for_answers += n_chapters;
		return true;
	}

	if (name == "MEDIA_TITLE") {
		if (!value.isEmpty() && value != "mp4" && !value.startsWith("mp4&")) {
			md->meta_data["NAME"] = value;
			qDebug() << "Proc::TMPVProcess::parseProperty: set meta_data[\"NAME\"] to" << value;
		}
		return true;
	}

	if (name == "OSD_X") {
		default_osd_pos.rx() = value.toInt();
		osd_pos.rx() = default_osd_pos.x();
		qDebug("Proc::TMPVProcess::parseProperty: set OSD x margin to %d", default_osd_pos.x());
		return true;
	}

	if (name == "OSD_Y") {
		default_osd_pos.ry() = value.toInt();
		osd_pos.ry() = default_osd_pos.y();
		qDebug("Proc::TMPVProcess::parseProperty: set OSD y margin to %d", default_osd_pos.y());
		return true;
	}

	return TPlayerProcess::parseProperty(name, value);
}

bool TMPVProcess::parseChapter(int id, double start, QString title) {

	waiting_for_answers--;
	md->chapters.addChapter(id, title, start);
	qDebug() << "Proc::TMPVProcess::parseChapter: added chapter id" << id
			 << "starting at" << start << "with title" << title;
	return true;
}

void TMPVProcess::requestChapterInfo() {
	writeToStdin("print_text \"INFO_CHAPTERS=${=chapters:}\"");
}

void TMPVProcess::fixTitle() {

	TDiscData disc = TDiscName::split(md->filename);
	if (disc.title == 0) disc.title = 1;

	// Accept the requested title as the selected title, if we did not receive
	// a title not found. First and upmost this handles titles being reported
	// as VTS by DVDNAV, but it also makes it possible to sequentially play all
	// titles, needed because MPV does not support menus.
	if (!received_title_not_found) {
		if (disc.title == selected_title) {
			qDebug("Proc::TMPVProcess::fixTitle: found requested title %d", disc.title);
		} else {
			qDebug("Proc::TMPVProcess::fixTitle: selecting title %d, player reports it is playing VTS %d",
				   disc.title, selected_title);
		}
		notifyTitleTrackChanged(disc.title);
		return;
	}

	qWarning("Proc::TMPVProcess::fixTitle: requested title %d not found or really short", disc.title);

	// Let the playlist try the next title if a valid title was requested and
	// there is more than 1 title.
	if (disc.title <= md->titles.count() && md->titles.count() > 1) {
		// Need to set the selected title, otherwise the playlist will select
		// the second title instead of the title after this one.
		notifyTitleTrackChanged(disc.title);
		// Pass eof to trigger playNext() in playlist
		received_end_of_file = true;
		// Ask player to quit
		quit(0);
		return;
	}

	// Accept defeat
	quit(1);
}

void TMPVProcess::checkTime(double sec) {

	if (title_swictched && sec >= title_switch_time) {
		title_swictched = false;
		qDebug("Proc::TMPVProcess::checkTime: sending track changed");
		notifyTitleTrackChanged(selected_title);
	}
}

bool TMPVProcess::parseTitleSwitched(QString disc_type, int title) {

	// MPV uses dvdnav to play DVDs, but without support for menus
	if (disc_type == "dvdnav") {
		md->detected_type = TMediaData::TYPE_DVD;
	} else {
		md->detected_type = md->stringToType(disc_type);
	}

	// Due to caching it still can take a while before the previous title
	// really ends, so store the title and the time to swicth and let
	// checkTime() do the swithing when the moment arrives.
	selected_title = title;

	if (disc_type == "cdda" || disc_type == "vcd") {
		if (notified_player_is_running) {
			int chapter = title - md->titles.firstID() + md->chapters.firstID();
			title_switch_time = md->chapters[chapter].getStart();
			if (title_switch_time <= md->time_sec + 0.5) {
				qDebug("Proc::TMPVProcess::parseTitleSwitched: switched to track %d", title);
				notifyTitleTrackChanged(title);
			} else {
				// Switch when the time comes
				title_swictched = true;
				title_switch_time -= 0.4;
				qDebug("Proc::TMPVProcess::parseTitleSwitched: saved track changed to %d at %f", title, title_switch_time);
			}
		} else {
			notifyTitleTrackChanged(title);
		}
	} else {
		// When a title ends and hits a menu MPV can go haywire on invalid
		// video time stamps. By setting quit_at_end_of_title, parseLine() will
		// release it from its suffering when the title ends by sending a quit
		// and fake an eof, so the playlist can play the next item.
		if (notified_player_is_running && !quit_at_end_of_title) {
			quit_at_end_of_title = true;
			// Set ms to wait before quitting. Cannnot rely on timestamp video,
			// because it can switch before the end of the title is reached.
			// A note on margins:
			// - Current md->time_sec can be behind
			// - Menus tend to be triggered on the last second of video
			// - Quit needs time to arrive
			quit_at_end_of_title_ms = (int) ((md->duration - md->time_sec) * 1000);
			// Quit right away if less than 400 ms to go.
			if (quit_at_end_of_title_ms <= 400) {
				qDebug("Proc::TMPVProcess::parseTitleSwitched: quitting at end of title");
				received_end_of_file =  true;
				quit(0);
			} else {
				// Quit when quit_at_end_of_title_ms elapsed
				quit_at_end_of_title_ms -= 400;
				quit_at_end_of_title_time.start();
				qDebug("Proc::TMPVProcess::parseTitleSwitched: marked title to quit in %d ms",
					   quit_at_end_of_title_ms);
			}
		}
	}

	return true;
}

bool TMPVProcess::parseTitleNotFound(const QString &disc_type) {
	Q_UNUSED(disc_type)

	// qWarning("Proc::TMPVProcess::parseTitleNotFound: requested title not found");

	// Requested title means the original title. The currently selected title
	// seems still valid and is the last selected title during its search through
	// the disc.

	// Ask which one is selected. Seems to always deliver -1, probably mpv just doesn't know?
	// writeToStdin("print_text \"[" + disc_type + "] switched to title: ${disc-title:-1}\"");

	received_title_not_found = true;

	return true;
}


int TMPVProcess::getFrame(double time_sec, const QString& line) {
	Q_UNUSED(line)

	// Emulate frames.
	return qRound(time_sec * md->video_fps);
}

void TMPVProcess::convertChaptersToTitles() {

	// Just for safety...
	if (md->titles.count() > 0) {
		qWarning("Proc::TMPVProcess::convertChaptersToTitles: found unexpected titles");
		return;
	}
	if (md->chapters.count() == 1) {
		// Playing a single track
		int firstChapter = md->chapters.firstID();
		md->titles.addTrack(md->titles.getSelectedID(),
							md->chapters[firstChapter].getName(),
							md->duration);
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
			md->titles.addTrack(prev_chapter.getID() + 1,
								prev_chapter.getName(),
								md->duration - prev_chapter.getStart());
		}
	}

	qDebug("Proc::TMPVProcess::convertChaptersToTitles: created %d titles",
		   md->titles.count());
}

void TMPVProcess::playingStarted() {
	qDebug("Proc::TMPVProcess::playingStarted");

	// MPV can give negative times for TS without giving a start time.
	// Correct them by setting the start time.
	if (!md->start_sec_set && md->time_sec < 0) {
		qDebug("Proc::TMPVProcess::playingStarted: setting negative start time %f", md->time_sec);
		md->start_sec = md->time_sec;
		// No longer need rollover protection (though not set for MPV anyway).
		md->mpegts = false;
		notifyTime(md->time_sec, "");
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
	writeToStdin("print_text VIDEO_BITRATE=${=video-bitrate}");
	writeToStdin("print_text AUDIO_BITRATE=${=audio-bitrate}");
}

bool TMPVProcess::parseStatusLine(double time_sec, double duration, QRegExp& rx, QString& line) {
	// Parse custom status line
	// STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}

	if (TPlayerProcess::parseStatusLine(time_sec, duration, rx, line))
		return true;

	// Parse status flags
	bool paused = rx.cap(3) == "yes";
	bool buffering = rx.cap(4) == "yes";
	bool idle = rx.cap(5) == "yes";

	if (paused) {
		//qDebug("Proc::TMPVProcess::parseStatusLine: paused");
		emit receivedPause();
		return true;
	}

	if (buffering or idle) {
		//qDebug("Proc::TMPVProcess::parseStatusLine: buffering");
		received_buffering = true;
		emit receivedBuffering();
		return true;
	}

	// Playing
	if (notified_player_is_running) {
		// Normal way to go: playing, except for first frame
		if (received_buffering) {
			received_buffering = false;
			emit receivedBufferingEnded();
		}

		if (request_bit_rate_info && time_sec > 11) {
			request_bit_rate_info = false;
			requestBitrateInfo();
		}

		return true;
	}

	// First and only run of state playing
	// Base class sets notified_player_is_running to true
	playingStarted();

	return true;
}

bool TMPVProcess::parseLine(QString& line) {

	// Custom status line. Make sure it matches!
	static QRegExp rx_status("^STATUS: ([0-9\\.-]+) / ([0-9\\.-]+) P: (yes|no) B: (yes|no) I: (yes|no)");
	static QRegExp rx_message("^(Playing:|\\[ytdl_hook\\])");

	// TODO: check video and audio track name.
	// Subs suggest name comes before codec...
	static QRegExp rx_video_track("^(.*)Video\\s+--vid=(\\d+)(\\s+\\((.*)\\))?(\\s+'(.*)')?");
	static QRegExp rx_audio_track("^(.*)Audio\\s+--aid=(\\d+)(\\s+--alang=([a-zA-Z]+))?(\\s+\\((.*)\\))?(\\s+'(.*)')?");
	static QRegExp rx_subtitle_track("^(.*)Subs\\s+--sid=(\\d+)(\\s+--slang=([a-zA-Z]+))?(\\s+'(.*)')?(\\s+\\((.*)\\))?");

	static QRegExp rx_dsize("^VIDEO_DSIZE=(\\d+)x(\\d+)");
	static QRegExp rx_vo("^VO: \\[(.*)\\]");
	static QRegExp rx_ao("^AO: \\[(.*)\\]");

	static QRegExp rx_video_codec("^VIDEO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_video_property("^VIDEO_([A-Z]+)=\\s*(.*)");
	static QRegExp rx_audio_codec("^AUDIO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_audio_property("^AUDIO_([A-Z]+)=\\s*(.*)");

	static QRegExp rx_meta_data("^METADATA_([A-Z]+)=\\s*(.*)");

	static QRegExp rx_chapter("^CHAPTER_(\\d+)=([0-9\\.-]+) '(.*)'");

	static QRegExp rx_title_switch("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*switched to (track|title):?\\s+(-?\\d+)",
								   Qt::CaseInsensitive);
	static QRegExp rx_title_not_found("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*(track|title) not found",
								   Qt::CaseInsensitive);

	static QRegExp rx_stream_title("icy-title: (.*)");

	static QRegExp rx_property("^INFO_([A-Z_]+)=\\s*(.*)");
	static QRegExp rx_forbidden("HTTP error 403 Forbidden");


	if (quit_at_end_of_title && !quit_send
		&& quit_at_end_of_title_time.elapsed() >= quit_at_end_of_title_ms) {
		qDebug("Proc::TMPVProcess::parseLine: %d ms elapsed, quitting title",
			   quit_at_end_of_title_ms);
		quit_at_end_of_title = false;
		received_end_of_file =  true;
		quit(0);
		return true;
	}

	if (verbose) {
		line = line.replace("[statusline] ", "");
		line = line.replace("[cplayer] ", "");
	}

	// Parse custom status line
	if (rx_status.indexIn(line) >= 0) {
		return parseStatusLine(rx_status.cap(1).toDouble(),
							   rx_status.cap(2).toDouble(),
							   rx_status, line);
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
							   rx_video_track.cap(4),
							   rx_video_track.cap(6).trimmed(),
							   rx_video_track.cap(1) != "");
	}

	// Audio id, lang, codec, name and selected
	if (rx_audio_track.indexIn(line) >= 0) {
		return parseAudioTrack(rx_audio_track.cap(2).toInt(),
							   rx_audio_track.cap(4),
							   rx_audio_track.cap(6),
							   rx_audio_track.cap(8).trimmed(),
							   rx_audio_track.cap(1) != "");
	}

	// Subtitles id, lang, name, type and selected
	if (rx_subtitle_track.indexIn(line) >= 0) {
		return parseSubtitleTrack(rx_subtitle_track.cap(2).toInt(),
								  rx_subtitle_track.cap(4),
								  rx_subtitle_track.cap(6).trimmed(),
								  rx_subtitle_track.cap(8),
								  rx_subtitle_track.cap(1) != "");
	}

	// VO
	if (rx_vo.indexIn(line) >= 0) {
		QString vo = rx_vo.cap(1);
		qDebug() << "MVPProcess::parseLine: emit receivedVO(" << vo << ")";
		emit receivedVO(vo);

		// Ask for video out resolution
		writeToStdin("print_text VIDEO_DSIZE=${=dwidth}x${=dheight}");
		waiting_for_answers++;

		return true;
	}

	// Video out size w x h
	if (rx_dsize.indexIn(line) >= 0) {
		waiting_for_answers--;
		md->video_out_width = rx_dsize.cap(1).toInt();
		md->video_out_height = rx_dsize.cap(2).toInt();
		qDebug("Proc::TMPVProcess::parseLine: set video out size to %d x %d",
			   md->video_out_width, md->video_out_height);
		return true;
	}

	// AO
	if (rx_ao.indexIn(line) >= 0) {
		QString ao = rx_ao.cap(1);
		qDebug() << "MVPProcess::parseLine: emit receivedAO(" << ao << ")";
		emit receivedAO(ao);
		return true;
	}

	// Video codec best match.
	// Fall back to generic VIDEO_CODEC if match fails.
	if (rx_video_codec.indexIn(line) >= 0) {
		md->video_codec = rx_video_codec.cap(2);
		qDebug() << "Proc::TMPVProcess::parseLine: video_codec set to" << md->video_codec;
		return true;
	}

	// Video property VIDEO_name and value
	if (rx_video_property.indexIn(line) >= 0) {
		return parseVideoProperty(rx_video_property.cap(1),
								  rx_video_property.cap(2));
	}

	// Audio codec best match
	// Fall back to generic AUDIO_CODEC if match fails.
	if (rx_audio_codec.indexIn(line) >= 0) {
		md->audio_codec = rx_audio_codec.cap(2);
		qDebug() << "Proc::TMPVProcess::parseLine: audio_codec set to" << md->audio_codec;
		return true;
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
		return parseMetaDataProperty(rx_meta_data.cap(1), rx_meta_data.cap(2));
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

	if (rx_stream_title.indexIn(line) > -1) {
		QString s = rx_stream_title.cap(1);
		qDebug("Proc::TMPVProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
		md->stream_title = s;
		emit receivedStreamTitle(s);
		return true;
	}

	// HTTP error 403 Forbidden
	if (rx_forbidden.indexIn(line) >= 0) {
		qDebug("MVPProcess::parseLine: 403 forbidden");
		emit receivedForbiddenText();
		return true;
	}

	return false;
}

// Start of what used to be mpvoptions.cpp and was pulled in with an include

void TMPVProcess::setMedia(const QString& media, bool is_playlist) {
	arg << "--term-playing-msg="
		"INFO_MPV_VERSION=${=mpv-version:}\n"

		"VIDEO_WIDTH=${=width}\n"
		"VIDEO_HEIGHT=${=height}\n"
		"VIDEO_ASPECT=${=video-aspect}\n"
//		"VIDEO_DSIZE=${=dwidth}x${=dheight}\n"
		"VIDEO_FPS=${=fps}\n"
//		"VIDEO_BITRATE=${=video-bitrate}\n"
		"VIDEO_FORMAT=${=video-format}\n"
		"VIDEO_CODEC=${=video-codec}\n"

//		"AUDIO_BITRATE=${=audio-bitrate}\n"
		"AUDIO_FORMAT=${=audio-codec-name:${=audio-format}}\n"
		"AUDIO_CODEC=${=audio-codec}\n"
		"AUDIO_RATE=${=audio-params/samplerate:${=audio-samplerate}}\n"
		"AUDIO_NCH=${=audio-params/channel-count:${=audio-channels}}\n"

		"INFO_START_TIME=${=time-start:}\n"
		"INFO_LENGTH=${=duration:${=length}}\n"
		"INFO_DEMUXER=${=demuxer}\n"

		"INFO_TITLES=${=disc-titles}\n"
		"INFO_CHAPTERS=${=chapters}\n"
//		"INFO_TRACKS_COUNT=${=track-list/count}\n"

		// TODO: check name, author, comment etc.
		"METADATA_TITLE=${metadata/by-key/title:}\n"
		"METADATA_ARTIST=${metadata/by-key/artist:}\n"
		"METADATA_ALBUM=${metadata/by-key/album:}\n"
		"METADATA_GENRE=${metadata/by-key/genre:}\n"
		"METADATA_DATE=${metadata/by-key/date:}\n"
		"METADATA_TRACK=${metadata/by-key/track:}\n"
		"METADATA_COPYRIGHT=${metadata/by-key/copyright:}\n"

		"INFO_MEDIA_TITLE=${=media-title:}\n"

		// OSD position used by setOSDPos. Docs MPV: (25, 22)
		"INFO_OSD_X=${=options/osd-margin-x:}\n"
		"INFO_OSD_Y=${=options/osd-margin-y:}\n";

	arg << "--term-status-msg=STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}";

	// MPV interprets the ID in a DVD URL as index [0..#titles-1] instead of
	// [1..#titles]. Maybe one day they gonna fix it and this will break. Sigh.
	// When no title is given it plays the longest title it can find.
	// Need to change no title to 0, otherwise fixTitle() won't work.
	// CDs work as expected, don't know about bluray, but assuming it's the same.
	QString url = media;
	bool valid_disc;
	TDiscData disc = TDiscName::split(media, &valid_disc);
	if (valid_disc
		&& (disc.protocol == "dvd" || disc.protocol == "dvdnav"
			|| disc.protocol == "br")) {
		if (disc.title > 0)
			disc.title--;
		url = TDiscName::join(disc, true);
	}

	if (is_playlist) {
		arg << "--playlist=" + url;
	} else {
		arg << url;
	}

#ifdef CAPTURE_STREAM
	capturing = false;
#endif
}

void TMPVProcess::setFixedOptions() {
	arg << "--no-config";
	arg << "--no-quiet";
	arg << "--terminal";
	arg << "--no-msg-color";
	arg << "--input-file=/dev/stdin";
	//arg << "--no-osc";
	//arg << "--msg-level=vd=v";
}

void TMPVProcess::disableInput() {
	arg << "--no-input-default-bindings";
	arg << "--input-x11-keyboard=no";
	arg << "--no-input-cursor";
	arg << "--cursor-autohide=no";
}

bool TMPVProcess::isOptionAvailable(const QString& option) {

	InfoReader* ir = InfoReader::obj();
	ir->getInfo();
	//qDebug() << "Proc::TMPVProcess::isOptionAvailable: option_list:" << ir->optionList();
	return ir->optionList().contains(option);
}

void TMPVProcess::addVFIfAvailable(const QString& vf, const QString& value) {

	InfoReader* ir = InfoReader::obj();
	ir->getInfo();
	if (ir->vfList().contains(vf)) {
		QString s = "--vf-add=" + vf;
		if (!value.isEmpty()) s += "=" + value;
		arg << s;
	} else {
		QString f = vf +"="+ value;
		qDebug("Proc::TMPVProcess::addVFIfAvailable: filter %s is not used because it's not available", f.toLatin1().constData());
	}
}

void TMPVProcess::messageFilterNotSupported(const QString& filter_name) {
	QString text = tr("the '%1' filter is not supported by mpv").arg(filter_name);
	writeToStdin(QString("show_text \"%1\" 3000").arg(text));
}

void TMPVProcess::setOption(const QString& option_name, const QVariant& value) {
	if (option_name == "cache") {
		int cache = value.toInt();
		if (cache > 31) {
			arg << "--cache=" + value.toString();
		} else {
			arg << "--cache=no";
		}
	}
	else
	if (option_name == "ss") {
		arg << "--start=" + value.toString();
	}
	else
	if (option_name == "endpos") {
		arg << "--length=" + value.toString();
	}
	else
	if (option_name == "loop") {
		QString o = value.toString();
		if (o == "0") o = "inf";
		arg << "--loop=" + o;
	}
	else
	if (option_name == "ass") {
		arg << "--sub-ass";
	}
	else
	if (option_name == "noass") {
		arg << "--no-sub-ass";
	}
	else
	if (option_name == "nosub") {
		arg << "--no-sub";
	}
	else
	if (option_name == "sub-fuzziness") {
		QString v;
		switch (value.toInt()) {
			case 1: v = "fuzzy"; break;
			case 2: v = "all"; break;
			default: v = "exact";
		}
		arg << "--sub-auto=" + v;
	}
	else
	if (option_name == "audiofile") {
		arg << "--audio-file=" + value.toString();
	}
	else
	if (option_name == "delay") {
		arg << "--audio-delay=" + value.toString();
	}
	else
	if (option_name == "subdelay") {
		arg << "--sub-delay=" + value.toString();
	}
	else
	if (option_name == "sid") {
		arg << "--sid=" + value.toString();
	}
	else
	if (option_name == "sub") {
		sub_file = value.toString();
		arg << "--sub-file=" + sub_file;
	}
	else
	if (option_name == "subpos") {
		arg << "--sub-pos=" + value.toString();
	}
	else
	if (option_name == "font") {
		arg << "--osd-font=" + value.toString();
	}
	else
	if (option_name == "subcp") {
		QString cp = value.toString();
		if (!cp.startsWith("enca")) cp = "utf8:" + cp;
		arg << "--sub-codepage=" + cp;
	}
	else
	if (option_name == "osdlevel") {
		arg << "--osd-level=" + value.toString();
	}
	else
	if (option_name == "sws") {
		arg << "--sws-scaler=lanczos";
	}
	else
	if (option_name == "channels") {
		arg << "--audio-channels=" + value.toString();
	}
	else
	if (option_name == "sub-scale" || option_name == "subfont-text-scale" || option_name == "ass-font-scale") {
		QString scale = value.toString();
		if (scale != "1") arg << "--sub-scale=" + scale;
	}
	else
	if (option_name == "stop-xscreensaver") {
		bool stop_ss = value.toBool();
		if (stop_ss) arg << "--stop-screensaver"; else arg << "--no-stop-screensaver";
	}
	else
	if (option_name == "correct-pts") {
		bool b = value.toBool();
		if (b) arg << "--correct-pts"; else arg << "--no-correct-pts";
	}
	else
	if (option_name == "idx") {
		arg << "--index=default";
	}
	else
	if (option_name == "softvol") {
		arg << "--softvol=yes";
	}
	else
	if (option_name == "softvol-max") {
		int v = value.toInt();
		if (v < 100) v = 100;
		arg << "--softvol-max=" + QString::number(v);
	}
	else
	if (option_name == "subfps") {
		arg << "--sub-fps=" + value.toString();
	}
	else
	if (option_name == "forcedsubsonly") {
		arg << "--sub-forced-only";
	}
	else
	if (option_name == "prefer-ipv4" || option_name == "prefer-ipv6" ||
		option_name == "dr" || option_name == "double" ||
		option_name == "adapter" || option_name == "edl" ||
		option_name == "slices" || option_name == "colorkey" ||
		option_name == "subcc" || option_name == "vobsub" ||
		option_name == "zoom" || option_name == "flip-hebrew" ||
		option_name == "autoq")
	{
		// Ignore
	}
	else
	if (option_name == "tsprog") {
		// Unsupported
	}
	else
	if (option_name == "dvdangle") {
		/*
		arg << "--dvd-angle=" + value.toString();
		*/
	}
	else
	if (option_name == "screenshot_template") {
		arg << "--screenshot-template=" + value.toString();
	}
	else
	if (option_name == "screenshot_format") {
		arg << "--screenshot-format=" + value.toString();
	}
	else
	if (option_name == "threads") {
		arg << "--vd-lavc-threads=" + value.toString();
	}
	else
	if (option_name == "skiploopfilter") {
		arg << "--vd-lavc-skiploopfilter=all";
	}
	else
	if (option_name == "keepaspect" || option_name == "fs") {
		bool b = value.toBool();
		if (b) arg << "--" + option_name; else arg << "--no-" + option_name;
	}
	else
	if (option_name == "ao") {
		QString o = value.toString();
		if (o.startsWith("alsa:device=")) {
			QString device = o.mid(12);
			//qDebug() << "Proc::TMPVProcess::setOption: alsa device:" << device;
			device = device.replace("=", ":").replace(".", ",");
			o = "alsa:device=[" + device + "]";
		}
		arg << "--ao=" + o;
	}
	else
	if (option_name == "vc") {
		qDebug() << "Proc::TMPVProcess::setOption: video codec ignored";
	}
	else
	if (option_name == "ac") {
		qDebug() << "Proc::TMPVProcess::setOption: audio codec ignored";
	}
	else
	if (option_name == "afm") {
		QString s = value.toString();
		if (s == "hwac3") arg << "--ad=spdif:ac3,spdif:dts";
	}
	else
	if (option_name == "fontconfig") {
		// Not supported, nor needed anymore
	}
	else
	if (option_name == "verbose") {
		arg << "-v";
		verbose = true;
	}
	else
	if (option_name == "mute") {
		arg << "--mute=yes";
	}
	else
	if (option_name == "vf-add") {
		if (!value.isNull()) arg << "--vf-add=" + value.toString();
	}
	else
	if (option_name == "af-add") {
		if (!value.isNull()) arg << "--af-add=" + value.toString();
	}
	else
	if (option_name == "wid" ||
		option_name == "vo" ||
		option_name == "aid" || option_name == "vid" ||
		option_name == "volume" ||
		option_name == "ass-styles" || option_name == "ass-force-style" ||
		option_name == "ass-line-spacing" ||
		option_name == "embeddedfonts" ||
		option_name == "osd-scale-by-window" ||
		option_name == "osd-scale" ||
		option_name == "speed" ||
		option_name == "contrast" || option_name == "brightness" ||
		option_name == "hue" || option_name == "saturation" || option_name == "gamma" ||
		option_name == "monitorpixelaspect" || option_name == "monitoraspect" ||
		option_name == "mc" ||
		option_name == "framedrop" ||
		option_name == "priority" ||
		option_name == "hwdec" ||
		option_name == "autosync" ||
		option_name == "dvd-device" || option_name == "cdrom-device" ||
		option_name == "demuxer" ||
		option_name == "shuffle" ||
		option_name == "frames" ||
		option_name == "hwdec-codecs")
	{
		QString s = "--" + option_name;
		if (!value.isNull()) s += "=" + value.toString();
		arg << s;
	}
	else
	{
		qDebug() << "Proc::TMPVProcess::setOption: unknown option:" << option_name;
	}
}

void TMPVProcess::addUserOption(const QString& option) {
	arg << option;
	if (option == "-v") {
		verbose = true;
	}
}

void TMPVProcess::addVF(const QString& filter_name, const QVariant& value) {
	QString option = value.toString();

	if ((filter_name == "harddup") || (filter_name == "hue")) {
		// ignore
	}
	else
	if (filter_name == "eq2") {
		arg << "--vf-add=eq";
	}
	else
	if (filter_name == "blur") {
		addVFIfAvailable("lavfi", "[unsharp=la=-1.5:ca=-1.5]");
	}
	else
	if (filter_name == "sharpen") {
		addVFIfAvailable("lavfi", "[unsharp=la=1.5:ca=1.5]");
	}
	else
	if (filter_name == "noise") {
		addVFIfAvailable("lavfi", "[noise=alls=9:allf=t]");
	}
	else
	if (filter_name == "deblock") {
		addVFIfAvailable("lavfi", "[pp=" + option +"]");
	}
	else
	if (filter_name == "dering") {
		addVFIfAvailable("lavfi", "[pp=dr]");
	}
	else
	if (filter_name == "phase") {
		addVFIfAvailable("lavfi", "[phase=" + option +"]");
	}
	else
	if (filter_name == "postprocessing") {
		addVFIfAvailable("lavfi", "[pp]");
	}
	else
	if (filter_name == "hqdn3d") {
		QString o;
		if (!option.isEmpty()) o = "=" + option;
		addVFIfAvailable("lavfi", "[hqdn3d" + o +"]");
	}
	else
	if (filter_name == "yadif") {
		if (option == "1") {
			arg << "--vf-add=yadif=field";
		} else {
			arg << "--vf-add=yadif";
		}
	}
	else
	if (filter_name == "kerndeint") {
		addVFIfAvailable("lavfi", "[kerndeint=" + option +"]");
	}
	else
	if (filter_name == "lb" || filter_name == "l5") {
		addVFIfAvailable("lavfi", "[pp=" + filter_name +"]");
	}
	else
	if (filter_name == "subs_on_screenshots") {
		// Ignore
	}
	else
	if (filter_name == "screenshot") {
		if (!screenshot_dir.isEmpty() && isOptionAvailable("--screenshot-directory")) {
			arg << "--screenshot-directory=" + QDir::toNativeSeparators(screenshot_dir);
		}
	}
	else
	if (filter_name == "rotate") {
		if (option == "0") {
			arg << "--vf-add=rotate=270,flip";
		}
		else
		if (option == "1") {
			arg << "--vf-add=rotate=90";
		}
		else
		if (option == "2") {
			arg << "--vf-add=rotate=270";
		}
		else
		if (option == "3") {
			arg << "--vf-add=rotate=90,flip";
		}
	}
	else {
		if (filter_name == "pp") {
			QString s;
			if (option.isEmpty()) s = "[pp]"; else s = "[pp=" + option + "]";
			addVFIfAvailable("lavfi", s);
		} else if (filter_name == "extrastereo" || filter_name == "karaoke") {
			/* Not supported anymore, ignore */
		} else {
			QString s = filter_name;
			if (!option.isEmpty()) s += "=" + option;
			arg << "--vf-add=" + s;
		}
	}
}

void TMPVProcess::addStereo3DFilter(const QString& in, const QString& out) {
	arg << "--vf-add=stereo3d=" + in + ":" + out;
}

void TMPVProcess::addAF(const QString& filter_name, const QVariant& value) {
	QString option = value.toString();

	if (filter_name == "volnorm") {
		QString s = "drc";
		if (!option.isEmpty()) s += "=" + option;
		arg << "--af-add=" + s;
	}
	else
	if (filter_name == "channels") {
		if (option == "2:2:0:1:0:0") arg << "--af-add=channels=2:[0-1,0-0]";
		else
		if (option == "2:2:1:0:1:1") arg << "--af-add=channels=2:[1-0,1-1]";
		else
		if (option == "2:2:0:1:1:0") arg << "--af-add=channels=2:[0-1,1-0]";
	}
	else
	if (filter_name == "pan") {
		if (option == "1:0.5:0.5") {
			arg << "--af-add=pan=1:[0.5,0.5]";
		}
	}
	else
	if (filter_name == "equalizer") {
		previous_eq = option;
		arg << "--af-add=equalizer=" + option;
	}
	else {
		QString s = filter_name;
		if (!option.isEmpty()) s += "=" + option;
		arg << "--af-add=" + s;
	}
}

void TMPVProcess::setVolume(int v) {
	writeToStdin("set volume " + QString::number(v));
}

void TMPVProcess::setOSDLevel(int level) {
	writeToStdin("osd " + QString::number(level));
}

void TMPVProcess::setAudio(int ID) {
	writeToStdin("set aid " + QString::number(ID));
}

void TMPVProcess::setVideo(int ID) {
	writeToStdin("set vid " + QString::number(ID));
}

void TMPVProcess::setSubtitle(SubData::Type type, int ID) {

	writeToStdin("set sid " + QString::number(ID));
	md->subs.setSelected(type, ID);
	emit receivedSubtitleTrackChanged();
}

void TMPVProcess::disableSubtitles() {

	writeToStdin("set sid no");
	md->subs.clearSelected();
	emit receivedSubtitleTrackChanged();
}

void TMPVProcess::setSecondarySubtitle(int ID) {
	writeToStdin("set secondary-sid " + QString::number(ID));
	md->subs.setSelectedSecondaryID(ID);
	//emit receivedSecondarySubtitleTrackChanged(ID);
}

void TMPVProcess::disableSecondarySubtitles() {
	writeToStdin("set secondary-sid no");
	md->subs.setSelectedSecondaryID(-1);
	//emit receivedSecondarySubtitleTrackChanged(ID);
}

void TMPVProcess::setSubtitlesVisibility(bool b) {
	writeToStdin(QString("set sub-visibility %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::seekPlayerTime(double secs, int mode, bool precise, bool currently_paused) {
	Q_UNUSED(currently_paused)

	QString s = "seek " + QString::number(secs) + " ";
	switch (mode) {
		case 0 : s += "relative "; break;
		case 1 : s += "absolute-percent "; break;
		case 2 : s += "absolute "; break;
	}
	if (precise) s += "exact"; else s += "keyframes";
	writeToStdin(s);
}

void TMPVProcess::mute(bool b) {
	writeToStdin(QString("set mute %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setPause(bool b) {
	writeToStdin(QString("set pause %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::frameStep() {
	writeToStdin("frame_step");
}

void TMPVProcess::frameBackStep() {
	writeToStdin("frame_back_step");
}

void TMPVProcess::showOSDText(const QString& text, int duration, int level) {
	QString str = QString("show_text \"%1\" %2 %3").arg(text).arg(duration).arg(level);
	writeToStdin(str);
}

void TMPVProcess::showFilenameOnOSD() {
	writeToStdin("show_text \"${filename}\" 2000 0");
}

void TMPVProcess::showTimeOnOSD() {
	writeToStdin("show_text \"${time-pos} / ${length:0} (${percent-pos}%)\" 2000 0");
}

void TMPVProcess::setContrast(int value) {
	writeToStdin("set contrast " + QString::number(value));
}

void TMPVProcess::setBrightness(int value) {
	writeToStdin("set brightness " + QString::number(value));
}

void TMPVProcess::setHue(int value) {
	writeToStdin("set hue " + QString::number(value));
}

void TMPVProcess::setSaturation(int value) {
	writeToStdin("set saturation " + QString::number(value));
}

void TMPVProcess::setGamma(int value) {
	writeToStdin("set gamma " + QString::number(value));
}

void TMPVProcess::setChapter(int ID) {
	writeToStdin("set chapter " + QString::number(ID));
}

void TMPVProcess::setExternalSubtitleFile(const QString& filename) {

	writeToStdin("sub_add \""+ filename +"\"");
	// Remeber filename to add to subs when MPV is done with it
	sub_file = filename;
}

void TMPVProcess::setSubPos(int pos) {
	writeToStdin("set sub-pos " + QString::number(pos));
}

void TMPVProcess::setSubScale(double value) {
	writeToStdin("set sub-scale " + QString::number(value));
}

void TMPVProcess::setSubStep(int value) {
	writeToStdin("sub_step " + QString::number(value));
}

void TMPVProcess::seekSub(int value) {
	writeToStdin("sub-seek " + QString::number(value));
}

void TMPVProcess::setSubForcedOnly(bool b) {
	writeToStdin(QString("set sub-forced-only %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setSpeed(double value) {
	writeToStdin("set speed " + QString::number(value));
}

#ifdef MPLAYER_SUPPORT
void TMPVProcess::enableKaraoke(bool) {
	/*
	if (b) writeToStdin("af add karaoke"); else writeToStdin("af del karaoke");
	*/
	messageFilterNotSupported("karaoke");
}

void TMPVProcess::enableExtrastereo(bool) {
	/*
	if (b) writeToStdin("af add extrastereo"); else writeToStdin("af del extrastereo");
	*/
	messageFilterNotSupported("extrastereo");
}
#endif

void TMPVProcess::enableVolnorm(bool b, const QString& option) {
	if (b) writeToStdin("af add drc=" + option); else writeToStdin("af del drc=" + option);
}

void TMPVProcess::setAudioEqualizer(const QString& values) {
	if (values == previous_eq) return;

	if (!previous_eq.isEmpty()) {
		writeToStdin("af del equalizer=" + previous_eq);
	}
	writeToStdin("af add equalizer=" + values);
	previous_eq = values;
}

void TMPVProcess::setAudioDelay(double delay) {
	writeToStdin("set audio-delay " + QString::number(delay));
}

void TMPVProcess::setSubDelay(double delay) {
	writeToStdin("set sub-delay " + QString::number(delay));
}

void TMPVProcess::setLoop(int v) {
	QString o;
	switch (v) {
		case -1: o = "no"; break;
		case 0: o = "inf"; break;
		default: o = QString::number(v);
	}
	writeToStdin(QString("set loop %1").arg(o));
}

void TMPVProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	writeToStdin(QString("screenshot %1 %2").arg(include_subtitles ? "subtitles" : "video").arg(t == Single ? "single" : "each-frame"));
}

#ifdef CAPTURE_STREAM
void TMPVProcess::switchCapturing() {

	if (!capture_filename.isEmpty()) {
		if (!capturing) {
			QString f = capture_filename;
#ifdef Q_OS_WIN
			// I hate Windows
			f = f.replace("\\", "\\\\");
#endif
			writeToStdin("set stream-capture \"" + f + "\"");
		} else {
			writeToStdin("set stream-capture \"\"");
		}
		capturing = !capturing;
	}
}
#endif

void TMPVProcess::setTitle(int ID) {
	writeToStdin("set disc-title " + QString::number(ID));
}

void TMPVProcess::discSetMousePos(int x, int y) {
	Q_UNUSED(x)
	Q_UNUSED(y)

	// MPV versions later than 18 july 2015 no longer support menus

	// qDebug("Proc::TMPVProcess::discSetMousePos: %d %d", x, y);
	// writeToStdin(QString("discnav mouse_move %1 %2").arg(x).arg(y));
	// mouse_move doesn't accept options :?

	// For some reason this doesn't work either...
	// So it's not possible to select options in the dvd menus just
	// because there's no way to pass the mouse position to mpv, or it
	// ignores it.
	// writeToStdin(QString("mouse %1 %2").arg(x).arg(y));
	// writeToStdin("discnav mouse_move");
}

void TMPVProcess::discButtonPressed(const QString& button_name) {
	writeToStdin("discnav " + button_name);
}

void TMPVProcess::setAspect(double aspect) {
	writeToStdin("set video-aspect " + QString::number(aspect));
}

void TMPVProcess::setFullscreen(bool b) {
	writeToStdin(QString("set fullscreen %1").arg(b ? "yes" : "no"));
}

#if PROGRAM_SWITCH
void TMPVProcess::setTSProgram(int ID) {
	qDebug("Proc::TMPVProcess::setTSProgram: function not supported");
}
#endif

void TMPVProcess::toggleDeinterlace() {
	writeToStdin("cycle deinterlace");
}

void TMPVProcess::setOSDPos(const QPoint& pos, int current_osd_level) {
	// mpv has no way to set the OSD position,
	// so this hack uses osd-margin to emulate it.

	if (default_osd_pos.x() == 0) {
		// Old player, not supporting OSD margins
		return;
	}

	// options/osd-margin-x Integer (0 to 300) (default: 25)
	// options/osd-margin-y Integer (0 to 600) (default: 22)
	// options/osd-align-x and y from version 0.9.0 onwards
	// osd-duration=<time in ms> (default 1000)

	bool clr_osd = false;

	// Handle y first
	if (pos.y() > max_osd_pos.y()) {
		// Max margin is 600. Beyond that center osd and hope for the best
		if (!osd_centered_y) {
			writeToStdin("set options/osd-align-y center");
			clr_osd = true;
			osd_centered_y = true;
		}
		// Reset margin to default
		if (osd_pos.y() != default_osd_pos.y()) {
			osd_pos.ry() = default_osd_pos.y();
			writeToStdin("set options/osd-margin-y " + QString::number(osd_pos.y()));
			clr_osd = true;
		}
	} else {
		// Reset alignment hack if centered
		if (osd_centered_y) {
			osd_centered_y = false;
			writeToStdin("set options/osd-align-y top");
			clr_osd = true;
		}

		int y = pos.y();
		if (y < default_osd_pos.y()) {
			y = default_osd_pos.y();
		}

		if (y != osd_pos.y()) {
			osd_pos.ry() = y;
			writeToStdin("set options/osd-margin-y " + QString::number(y));
			clr_osd = true;
		}
	}

	// Handle x
	if (pos.x() > max_osd_pos.x()) {
		// Hack: center osd and hope for the best
		if (!osd_centered_x) {
			writeToStdin("set options/osd-align-x center");
			osd_centered_x = true;
			clr_osd = true;
		}
		// Reset margin
		if (osd_pos.x() != default_osd_pos.x()) {
			osd_pos.rx() = default_osd_pos.x();
			writeToStdin("set options/osd-margin-x "
				+ QString::number(osd_pos.x()));
			clr_osd = true;
		}
	} else {
		// Reset alignment hack if centered
		if (osd_centered_x) {
			osd_centered_x = false;
			writeToStdin("set options/osd-align-x left");
			clr_osd = true;
		}

		int x = pos.x();
		if (x < default_osd_pos.x()) {
			x = default_osd_pos.x();
		}

		if (x != osd_pos.x()) {
			osd_pos.rx() = x;
			writeToStdin("set options/osd-margin-x " + QString::number(x));
			clr_osd = true;
		}
	}

	if (clr_osd) {
		writeToStdin("show_text \"\" 0 " + QString::number(current_osd_level));
	}
}

void TMPVProcess::setOSDScale(double value) {
	writeToStdin("set osd-scale " + QString::number(value));
}

void TMPVProcess::changeVF(const QString& filter, bool enable, const QVariant& option) {
	qDebug() << "Proc::TMPVProcess::changeVF:" << filter << enable;

	QString f;
	if (filter == "letterbox") {
		f = QString("expand=aspect=%1").arg(option.toDouble());
	}
	else
	if (filter == "noise") {
		f = "lavfi=[noise=alls=9:allf=t]";
	}
	else
	if (filter == "blur") {
		f = "lavfi=[unsharp=la=-1.5:ca=-1.5]";
	}
	else
	if (filter == "sharpen") {
		f = "lavfi=[unsharp=la=1.5:ca=1.5]";
	}
	else
	if (filter == "deblock") {
		f = "lavfi=[pp=" + option.toString() +"]";
	}
	else
	if (filter == "dering") {
		f = "lavfi=[pp=dr]";
	}
	else
	if (filter == "phase") {
		f = "lavfi=[phase=" + option.toString() +"]";
	}
	else
	if (filter == "postprocessing") {
		f = "lavfi=[pp]";
	}
	else
	if (filter == "hqdn3d") {
		QString o = option.toString();
		if (!o.isEmpty()) o = "=" + o;
		f = "lavfi=[hqdn3d" + o +"]";
	}
	else
	if (filter == "rotate") {
		QString o = option.toString();
		if (o == "0") {
			f = "rotate=270,flip";
		}
		else
		if (o == "1") {
			f = "rotate=90";
		}
		else
		if (o == "2") {
			f = "rotate=270";
		}
		else
		if (o == "3") {
			f = "rotate=90,flip";
		}
	}
	else
	if (filter == "flip" || filter == "mirror") {
		f = filter;
	}
	else
	if (filter == "scale" || filter == "gradfun") {
		f = filter;
		QString o = option.toString();
		if (!o.isEmpty()) f += "=" + o;
	}
	else
	if (filter == "lb" || filter == "l5") {
		f = "lavfi=[pp=" + filter +"]";
	}
	else
	if (filter == "yadif") {
		if (option.toString() == "1") {
			f = "yadif=field";
		} else {
			f = "yadif";
		}
	}
	else
	if (filter == "kerndeint") {
		f = "lavfi=[kerndeint=" + option.toString() +"]";
	}
	else {
		qDebug() << "Proc::TMPVProcess::changeVF: unknown filter:" << filter;
	}

	if (!f.isEmpty()) {
		writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(f));
	}
}

void TMPVProcess::changeStereo3DFilter(bool enable, const QString& in, const QString& out) {
	QString filter = "stereo3d=" + in + ":" + out;
	writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(filter));
}

void TMPVProcess::setSubStyles(const Settings::TAssStyles& styles, const QString&) {

	using namespace Settings;
	QString font = styles.fontname;
	//arg << "--sub-text-font=" + font.replace(" ", "");
	arg << "--sub-text-font=" + font;
	arg << "--sub-text-color=#" + ColorUtils::colorToRRGGBB(styles.primarycolor);

	if (styles.borderstyle == TAssStyles::Outline) {
		arg << "--sub-text-shadow-color=#" + ColorUtils::colorToRRGGBB(styles.backcolor);
	} else {
		arg << "--sub-text-back-color=#" + ColorUtils::colorToRRGGBB(styles.outlinecolor);
	}
	arg << "--sub-text-border-color=#" + ColorUtils::colorToRRGGBB(styles.outlinecolor);

	arg << "--sub-text-border-size=" + QString::number(styles.outline * 2.5);
	arg << "--sub-text-shadow-offset=" + QString::number(styles.shadow * 2.5);

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
			if (!halign.isEmpty()) arg << "--sub-text-align-x=" + halign;
			if (!valign.isEmpty()) arg << "--sub-text-align-y=" + valign;
		}
	}
}

void TMPVProcess::setChannelsFile(const QString& filename) {
	arg << "--dvbin-file=" + filename;
}

} // namespace Proc


#include "moc_mpvprocess.cpp"

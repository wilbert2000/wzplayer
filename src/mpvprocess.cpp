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

#include "mpvprocess.h"
#include <QRegExp>
#include <QStringList>
#include <QApplication>
#include <QDebug>

#include "global.h"
#include "preferences.h"
#include "mplayerversion.h"
#include "colorutils.h"
#include "inforeader.h"

using namespace Global;

#define TOO_CHAPTERS_WORKAROUND

MPVProcess::MPVProcess(QObject * parent)
	: PlayerProcess(parent)
	, notified_mplayer_is_running(false)
	, received_end_of_file(false)
	, last_sub_id(-1)
	, mplayer_svn(-1) // Not found yet
	, verbose(false)
	, line_count(0)
	, fps(0.0)
#if NOTIFY_SUB_CHANGES
	, subtitle_info_received(false)
	, subtitle_info_changed(false)
#endif
#if NOTIFY_AUDIO_CHANGES
	, audio_info_changed(false)
#endif
#if NOTIFY_VIDEO_CHANGES
	, video_info_changed(false)
#endif
#if NOTIFY_CHAPTER_CHANGES
	, chapter_info_changed(false)
#endif
	, dvd_current_title(-1)
	, br_current_title(-1)
{
	player_id = PlayerID::MPV;

	connect( this, SIGNAL(lineAvailable(QByteArray)),
			 this, SLOT(parseLine(QByteArray)) );

	connect( this, SIGNAL(finished(int,QProcess::ExitStatus)), 
             this, SLOT(processFinished(int,QProcess::ExitStatus)) );

	connect( this, SIGNAL(error(QProcess::ProcessError)),
             this, SLOT(gotError(QProcess::ProcessError)) );

	//int svn = MplayerVersion::mplayerVersion("mpv unknown version (C)");
}

MPVProcess::~MPVProcess() {
}

bool MPVProcess::start() {
	md.reset();
	notified_mplayer_is_running = false;
	fps = 0.0;
	prev_frame = -1;
	last_sub_id = -1;
	mplayer_svn = -1; // Not found yet
	received_end_of_file = false;

#if NOTIFY_SUB_CHANGES
	subs.clear();
	subtitle_info_received = false;
	subtitle_info_changed = false;
#endif

#if NOTIFY_AUDIO_CHANGES
	audios.clear();
	audio_info_changed = false;
#endif

#if NOTIFY_VIDEO_CHANGES
	videos.clear();
	video_info_changed = false;
#endif

#if NOTIFY_CHAPTER_CHANGES
	chapters.clear();
	chapter_info_changed = false;
#endif

	dvd_current_title = -1;
	br_current_title = -1;

	MyProcess::start();

	return waitForStarted();
}

#if NOTIFY_VIDEO_CHANGES || NOTIFY_AUDIO_CHANGES || NOTIFY_SUB_CHANGES
void MPVProcess::parseTrackInfo(QRegExp &rx) {

	int ID = rx.cap(3).toInt();
	QString type = rx.cap(2);
	QString name = rx.cap(5);
	QString lang = rx.cap(4);
	QString selected = rx.cap(6);
	qDebug("MPVProcess::parseTrackInfo: ID: %d type: '%s' name: '%s' lang: '%s' selected: '%s'",
		   ID, type.toUtf8().constData(), name.toUtf8().constData(),
		   lang.toUtf8().constData(), selected.toUtf8().constData());

	/*
	if (lang == "(unavailable)") lang = "";
	if (name == "(unavailable)") name = "";
	*/

	if (type == "video") {
#if NOTIFY_VIDEO_CHANGES
		int idx = videos.find(ID);
		if (idx == -1) {
			video_info_changed = true;
			videos.addName(ID, name);
			videos.addLang(ID, lang);
		} else {
			// Track already existed
			if (videos.itemAt(idx).name() != name) {
				video_info_changed = true;
				videos.addName(ID, name);
			}
			if (videos.itemAt(idx).lang() != lang) {
				video_info_changed = true;
				videos.addLang(ID, lang);
			}
		}
#endif
		return;
	}

	if (type == "audio") {
#if NOTIFY_AUDIO_CHANGES
		updateAudioTrack(ID, name, lang);
#endif
		return;
	}

	if (type == "sub") {
#if NOTIFY_SUB_CHANGES
		updateSubtitleTrack(ID, name, lang);
#endif
	}
}
#endif

void MPVProcess::parseVideoProperty(const QString &name, const QString &value) {

	if (name == "WIDTH") {
		md.video_width = value.toInt();
		qDebug("MPVProcess::parseVideoProperty: md.video_width set to %d", md.video_width);
		return;
	}
	if (name == "HEIGHT") {
		md.video_height = value.toInt();
		qDebug("MPVProcess::parseVideoProperty: md.video_height set to %d", md.video_height);
		return;
	}
	if (name == "ASPECT") {
		md.video_aspect = value.toDouble();
		if (md.video_aspect == 0.0 && md.video_height != 0) {
			md.video_aspect = (double) md.video_width / md.video_height;
		}
		qDebug("MPVProcess::parseVideoProperty: md.video_aspect set to %f", md.video_aspect);
		return;
	}
	if (name == "FPS") {
		md.video_fps = value;
		fps = value.toDouble();
		qDebug() << "MPVProcess::parseVideoProperty: md.video_fps set to" << md.video_fps;
		return;
	}
	if (name == "BITRATE") {
		int bitrate = value.toInt();
		qDebug("MPVProcess::parseVideoProperty: emit receivedVideoBitrate(%d)", bitrate);
		emit receivedVideoBitrate(bitrate);
		return;
	}
	if (name == "FORMAT") {
		md.video_format = value;
		qDebug() << "MPVProcess::parseVideoProperty: md.video_format set to" << md.video_format;
		return;
	}
	// If no video, the video codec regexp does not match
	if (name == "CODEC") {
		md.video_codec = value;
		qDebug() << "MPVProcess::parseVideoProperty: md.video_codec set to" << md.video_codec;
		return;
	}

	qWarning("MVPProcess::parseVideoProperty: unexpected property INFO_VIDEO_%s=%s",
		name.toUtf8().constData(), value.toUtf8().constData());
}

void MPVProcess::parseAudioProperty(const QString &name, const QString &value) {

	if (name == "BITRATE") {
		int bitrate = value.toInt();
		qDebug("MPVProcess::parseAudioProperty: emit receivedAudioBitrate(%d)", bitrate);
		emit receivedAudioBitrate(bitrate);
		return;
	}
	if (name == "FORMAT") {
		md.audio_format = value;
		qDebug() << "MPVProcess::parseAudioProperty: md.audio_format set to" << md.audio_format;
		return;
	}
	if (name == "RATE") {
		md.audio_rate = value.toInt();
		qDebug("MPVProcess::parseAudioProperty: md.audio_rate set to %d", md.audio_rate);
		return;
	}
	if (name == "NCH") {
		md.audio_nch = value.toInt();
		qDebug("MPVProcess::parseAudioProperty: md.audio_nch set to %d", md.audio_nch);
		return;
	}
	// If no audio, the audio codec regexp does not match
	if (name == "CODEC") {
		md.audio_codec = value;
		qDebug() << "MPVProcess::parseAudioProperty: md.audio_codec set to" << md.audio_codec;
		return;
	}

	qWarning("MVPProcess::parseAudioProperty: unexpected property INFO_AUDIO_%s=%s",
		name.toUtf8().constData(), value.toUtf8().constData());
}


void MPVProcess::parseMetaDataProperty(const QString &name, const QString &value) {

	if (name == "TITLE") {
		if (!value.isEmpty()) md.clip_name = value;
		return;
	}
	if (name == "ARTIST") {
		if (!value.isEmpty()) md.clip_artist = value;
		return;
	}
	if (name == "ALBUM") {
		if (!value.isEmpty()) md.clip_album = value;
		return;
	}
	if (name == "GENRE") {
		if (!value.isEmpty()) md.clip_genre = value;
		return;
	}
	if (name == "DATE") {
		if (!value.isEmpty()) md.clip_date = value;
		return;
	}
	if (name == "TRACK") {
		if (!value.isEmpty()) md.clip_track = value;
		return;
	}
	if (name == "COPYRIGHT") {
		if (!value.isEmpty()) md.clip_copyright = value;
		return;
	}

	qWarning("MVPProcess::parseMetaDataProperty: unexpected property METADATA_%s=%s",
		name.toUtf8().constData(), value.toUtf8().constData());
}

void MPVProcess::parseProperty(const QString &name, const QString &value) {

	if (name == "MPV_VERSION") {
		mpv_version = value;
		qDebug() << "MPVProcess::parseProperty: mpv_version set to" << mpv_version;
		return;
	}
	if (name == "LENGTH") {
		md.duration = value.toDouble();
		qDebug("MPVProcess::parseProperty: md.duration set to %f", md.duration);
		return;
	}
	if (name == "DEMUXER") {
		md.demuxer = value;
		qDebug() << "MPVProcess::parseProperty: md.demuxer set to" << md.demuxer;
		return;
	}
	if (name == "TITLES") {
		int n_titles = value.toInt();
		for (int id = 0; id < n_titles; id++) {
			md.titles.addName(id, QString::number(id + 1));
		}
		qDebug("MPVProcess::parseProperty: added %d titles", n_titles);
		return;
	}
	if (name == "CHAPTERS") {
		md.n_chapters = value.toInt();

#ifdef TOO_CHAPTERS_WORKAROUND
		if (md.n_chapters > 1000) {
			qDebug("MPVProcess::parseProperty: warning too many chapters: %d", md.n_chapters);
			qDebug("                           chapters will be ignored");
			md.n_chapters = 0;
		}
#endif

		for (int n = 0; n < md.n_chapters; n++) {
			writeToStdin(QString("print_text INFO_CHAPTER_%1_NAME=${chapter-list/%1/title}").arg(n));
		}
		return;
	}
	if (name == "MEDIA_TITLE") {
		if (!value.isEmpty() && value != "mp4" && !value.startsWith("mp4&")) {
			md.clip_name = value;
			qDebug() << "MPVProcess::parseProperty: set md.clip_name to" << md.clip_name;
		}
		return;
	}
	if (name == "TRACKS_COUNT") {

#if NOTIFY_VIDEO_CHANGES || NOTIFY_AUDIO_CHANGES || NOTIFY_SUB_CHANGES
		int tracks = value.toInt();
		for (int n = 0; n < tracks; n++) {
			writeToStdin(QString("print_text \"INFO_TRACK_%1: "
				"${track-list/%1/type} "
				"${track-list/%1/id} "
				"'${track-list/%1/lang:}' "
				"'${track-list/%1/title:}' "
				"${track-list/%1/selected}\"").arg(n));
		}
#endif

		return;
	}

	qWarning("MVPProcess::parseProperty: unexpected property INFO_%s=%s",
		name.toUtf8().constData(), value.toUtf8().constData());
}

void MPVProcess::parseSubs(int id, const QString &lang, const QString &title) {
	qDebug("MPVProcess::parseSubs: sub id: %d, lang: '%s', name: '%s'", id, lang.toUtf8().constData(), title.toUtf8().constData());

#if NOTIFY_SUB_CHANGES
	updateSubtitleTrack(id, title, lang);
#else
	if (md.subs.find(SubData::Sub, id) == -1) {
		md.subs.add(SubData::Sub, id);
		md.subs.changeName(SubData::Sub, id, title);
		md.subs.changeLang(SubData::Sub, id, lang);
	}
#endif

}

void MPVProcess::parseChapterName(int id, QString title) {

	if (title.isEmpty()) title = QString::number(id + 1);

#if NOTIFY_CHAPTER_CHANGES
	chapters.addName(id, title);
	chapter_info_changed = true;
#else
	md.chapters.addName(id, title);
#endif

	qDebug("MPVProcess::parseLine: chapter id: %d title: '%s'",
		id, title.toUtf8().constData());
}

void MPVProcess::notifyChanges() {

#if NOTIFY_SUB_CHANGES
	if (subtitle_info_changed) {
		qDebug("MPVProcess::notifyChanges: subtitle_info_changed");
		subtitle_info_changed = false;
		subtitle_info_received = false;
		emit subtitleInfoChanged(subs);
	}
	if (subtitle_info_received) {
		qDebug("MPVProcess::notifyChanges: subtitle_info_received");
		subtitle_info_received = false;
		emit subtitleInfoReceivedAgain(subs);
	}
#endif

#if NOTIFY_AUDIO_CHANGES
	if (audio_info_changed) {
		qDebug("MPVProcess::notifyChanges: audio_info_changed");
		audio_info_changed = false;
		emit audioInfoChanged(audios);
	}
#endif

#if NOTIFY_VIDEO_CHANGES
	if (video_info_changed) {
		qDebug("MPVProcess::notifyChanges: video_info_changed");
		video_info_changed = false;
		emit videoInfoChanged(videos);
	}
#endif

#if NOTIFY_CHAPTER_CHANGES
	if (chapter_info_changed) {
		qDebug("MPVProcess::notifyChanges: chapter_info_changed");
		chapter_info_changed = false;
		emit chaptersChanged(chapters);
	}
#endif

}

void MPVProcess::notifyTimestamp(double sec) {

	// Pass current time stamp
	emit receivedCurrentSec( sec );

	// Emulate frames. mpv won't give them?
	// TODO: way to get them anyway for videos that do have frames
	if (fps != 0.0) {
		int frame = qRound(sec * fps);
		if (frame != prev_frame) {
			prev_frame = frame;
			emit receivedCurrentFrame( frame );
		}
	}
}

void MPVProcess::parseStatusLine(QRegExp &rx) {
	// Parse custom status line
	// STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}

	double sec = rx.cap(1).toDouble();
	double length = rx.cap(2).toDouble();

	bool paused = rx.cap(3) == "yes";
	bool buffering = rx.cap(4) == "yes";
	bool idle = rx.cap(5) == "yes";

	// Duration changed
	if (length != md.duration) {
		md.duration = length;
		qDebug("MPVProcess::parseStatusLine: emit receivedDuration(%f)", length);
		emit receivedDuration(length);
	}

	// Always pass current time stamp.
	notifyTimestamp( sec );

	// State flags.
	// Only fall through when playing.
	if (paused) {
		qDebug("MPVProcess::parseStatusLine: paused");
		receivedPause();
		return;
	}
	if (buffering) {
		qDebug("MPVProcess::parseStatusLine: buffering");
		receivedBuffering();
		return;
	}
	if (idle) {
		qDebug("MPVProcess::parseStatusLine: core idle");
		receivedBuffering();
		return;
	}

	// Playing
	if (notified_mplayer_is_running) {
		// Playing except for first frame. Notify AV changes.
		notifyChanges();
	} else {
		// First and only run of state playing
		notified_mplayer_is_running = true;

		// Have any video?
		if (md.video_width == 0) {
			md.novideo = true;
			qDebug("MPVProcess::parseStatusLine: emit receivedNoVideo()");
			emit receivedNoVideo();
		}

		qDebug("MPVProcess::parseStatusLine: emit mplayerFullyLoaded()");
		emit mplayerFullyLoaded();

		// Clear frame counter if no fps
		// TODO: check if not already cleared by frame counter
		if (fps == 0.0) {
			emit receivedCurrentFrame(0);
		}

		// Wait some secs to ask for bitrate
		QTimer::singleShot(12000, this, SLOT(requestBitrateInfo()));
	}
}

void MPVProcess::parseLine(QByteArray ba) {

	static QRegExp rx_mpv_status("^STATUS: ([0-9\\.-]+) / ([0-9\\.-]+) P: (yes|no) B: (yes|no) I: (yes|no)");
	static QRegExp rx_mpv_playing("^Playing:.*|^\\[ytdl_hook\\].*");

	#if !NOTIFY_VIDEO_CHANGES
	static QRegExp rx_mpv_video("^.* Video\\s+--vid=(\\d+)([ \\(\\)\\*]+)('(.*)'|)");
	#endif

	static QRegExp rx_mpv_audio("^.* Audio\\s+--aid=(\\d+)( --alang=([a-z]+)|)([ \\(\\)\\*]+)('(.*)'|)");

	static QRegExp rx_mpv_dsize("^INFO_VIDEO_DSIZE=(\\d+)x(\\d+)");
	static QRegExp rx_mpv_vo("^VO: \\[(.*)\\]");
	static QRegExp rx_mpv_ao("^AO: \\[(.*)\\]");

	static QRegExp rx_mpv_video_codec("^INFO_VIDEO_CODEC=(.*) \\[(.*)\\]");
	static QRegExp rx_mpv_video_property("^INFO_VIDEO_([A-Z]+)=(.*)");
	static QRegExp rx_mpv_audio_codec("^INFO_AUDIO_CODEC=(.*) \\[(.*)\\]");
	static QRegExp rx_mpv_audio_property("^INFO_AUDIO_([A-Z]+)=(.*)");

	static QRegExp rx_mpv_meta_data("^METADATA_([A-Z]+)=(.*)");

	static QRegExp rx_mpv_subs("^.* Subs\\s+--sid=(\\d+)( --slang=([a-z]+)|)([ \\(\\)\\*]+)('(.*)'|)");

	static QRegExp rx_mpv_trackinfo("^INFO_TRACK_(\\d+): (audio|video|sub) (\\d+) '(.*)' '(.*)' (yes|no)");
	static QRegExp rx_mpv_chaptername("^INFO_CHAPTER_(\\d+)_NAME=(.*)");

	#if DVDNAV_SUPPORT
	static QRegExp rx_mpv_switch_title("^\\[dvdnav\\] DVDNAV, switched to title: (\\d+)");
	#endif

	static QRegExp rx_mpv_property("^INFO_([A-Z_]+)=(.*)");
	static QRegExp rx_mpv_forbidden("HTTP error 403 Forbidden");
	static QRegExp rx_mpv_endoffile("^Exiting... \\(End of file\\)");


	line_count++;
	if (line_count % 10000 == 0) {
		qDebug("MPVProcess::parseLine: parsed %d lines", line_count);
	}

	if (ba.isEmpty()) return;

#if COLOR_OUTPUT_SUPPORT
	QString line = ColorUtils::stripColorsTags(QString::fromLocal8Bit(ba));
#else
	#ifdef Q_OS_WIN
	QString line = QString::fromUtf8(ba);
	#else
	QString line = QString::fromLocal8Bit(ba);
	#endif
#endif

	if (verbose) {
		line = line.replace("[statusline] ", "");
		line = line.replace("[cplayer] ", "");
	}

	// Parse custom status line
	if (rx_mpv_status.indexIn(line) > -1) {
		parseStatusLine(rx_mpv_status);
		return;
	}

	emit lineAvailable(line);

	qDebug("MPVProcess::parseLine: '%s'", line.toUtf8().data() );

	// Playing
	if (rx_mpv_playing.indexIn(line) > -1) {
		qDebug("MVPProcess::parseLine: emit receivedPlaying()");
		emit receivedPlaying();
		return;
	}

#if !NOTIFY_VIDEO_CHANGES
	// Video id and name
	if (rx_mpv_video.indexIn(line) > -1) {
		int id = rx_mpv_video.cap(1).toInt();
		QString title = rx_mpv_video.cap(4);
		qDebug("MPVProcess::parseLine: video id: %d, name: '%s'",
			   id, title.toUtf8().constData());
		md.videos.addName(id, title);\
		return;
	}
#endif

	// Audio id, lang and name
	if (rx_mpv_audio.indexIn(line) > -1) {
		int id = rx_mpv_audio.cap(1).toInt();
		QString lang = rx_mpv_audio.cap(3);
		QString title = rx_mpv_audio.cap(6);
		qDebug("MPVProcess::parseLine: audio id: %d, lang: '%s', name: '%s'",
			   id, lang.toUtf8().constData(), title.toUtf8().constData());

#if NOTIFY_AUDIO_CHANGES
		updateAudioTrack(id, title, lang);
#else
		if (md.audios.find(id) == -1) {
			md.audios.addID(id);
			md.audios.addName(id, title);
			md.audios.addLang(id, lang);
		}
#endif

		return;
	}

	// Window resolution w x h
	if (rx_mpv_dsize.indexIn(line) > -1) {
		int w = rx_mpv_dsize.cap(1).toInt();
		int h = rx_mpv_dsize.cap(2).toInt();
		qDebug("MPVProcess::parseLine: emit receivedWindowResolution(%d, %d)", w, h);
		emit receivedWindowResolution(w, h);
		return;
	}

	// VO
	if (rx_mpv_vo.indexIn(line) > -1) {
		QString vo = rx_mpv_vo.cap(1);
		qDebug() << "MVPProcess::parseLine: emit receivedVO(" << vo << ")";
		emit receivedVO(vo);
		// Ask for window resolution
		writeToStdin("print_text INFO_VIDEO_DSIZE=${=dwidth}x${=dheight}");
		return;
	}

	// AO
	if (rx_mpv_ao.indexIn(line) > -1) {
		QString ao = rx_mpv_ao.cap(1);
		qDebug() << "MVPProcess::parseLine: emit receivedAO(" << ao << ")";
		emit receivedAO(ao);
		return;
	}

	// Video codec
	if (rx_mpv_video_codec.indexIn(line) > -1) {
		md.video_codec = rx_mpv_video_codec.cap(2);
		qDebug() << "MPVProcess::parseLine: md.video_codec set to" << md.video_codec;
		return;
	}

	// Video property name and value
	if (rx_mpv_video_property.indexIn(line) > -1) {
		parseVideoProperty(rx_mpv_video_property.cap(1),
						   rx_mpv_video_property.cap(2));
		return;
	}

	// Audio codec
	if (rx_mpv_audio_codec.indexIn(line) > -1) {
		md.audio_codec = rx_mpv_audio_codec.cap(2);
		qDebug() << "MPVProcess::parseLine: md.audio_codec set to" << md.audio_codec;
		return;
	}

	// Audio property name and value
	if (rx_mpv_audio_property.indexIn(line) > -1) {
		parseAudioProperty(rx_mpv_audio_property.cap(1),
						   rx_mpv_audio_property.cap(2));
		return;
	}

	// Meta data name and value
	if (rx_mpv_meta_data.indexIn(line) > -1) {
		parseMetaDataProperty(rx_mpv_meta_data.cap(1), rx_mpv_meta_data.cap(2));
		return;
	}

	// Subtitles id, lang and title
	if (rx_mpv_subs.indexIn(line) > -1) {
		parseSubs(rx_mpv_subs.cap(1).toInt(), rx_mpv_subs.cap(3),
				  rx_mpv_subs.cap(6));
		return;
	}

#if NOTIFY_VIDEO_CHANGES || NOTIFY_AUDIO_CHANGES || NOTIFY_SUB_CHANGES
	// Track info
	if (rx_mpv_trackinfo.indexIn(line) > -1) {
		parseTrackInfo(rx_mpv_trackinfo);
		return;
	}
#endif

	// Chapter id and title
	if (rx_mpv_chaptername.indexIn(line) > -1) {
		parseChapterName(rx_mpv_chaptername.cap(1).toInt(),
						 rx_mpv_chaptername.cap(2));
		return;
	}

#if DVDNAV_SUPPORT
	if (rx_mpv_switch_title.indexIn(line) > -1) {
		int title = rx_mpv_switch_title.cap(1).toInt();
		qDebug("MPVProcess::parseLine: title changed to %d", title);
		// Ask for chapter info
		// Wait 10 secs. because it can take a while until the title start to play
		QTimer::singleShot(10000, this, SLOT(requestChapterInfo()));
		return;
	}
#endif

	// General property name and value
	if (rx_mpv_property.indexIn(line) > -1) {
		parseProperty(rx_mpv_property.cap(1), rx_mpv_property.cap(2));
		return;
	}

	// HTTP error 403 Forbidden
	if (rx_mpv_forbidden.indexIn(line) > -1) {
		qDebug("MVPProcess::parseLine: 403 forbidden");
		emit receivedForbiddenText();
		return;
	}

	// End of file
	if (rx_mpv_endoffile.indexIn(line) > -1)  {
		qDebug("MVPProcess::parseLine: detected end of file");
		if (!received_end_of_file) {
			// In case of playing VCDs or DVDs, maybe the first title
			// is not playable, so the GUI doesn't get the info about
			// available titles. So if we received the end of file
			// first let's pretend the file has started so the GUI can have
			// the data.
			if ( !notified_mplayer_is_running) {
				emit mplayerFullyLoaded();
			}
			// Send signal once the process is finished, not now!
			received_end_of_file = true;
		}
		return;
	}

	qDebug("MPVProcess::parseLine: ignored '%s'", line.toUtf8().data());
}

void MPVProcess::requestChapterInfo() {
	writeToStdin("print_text \"INFO_CHAPTERS=${=chapters}\"");
}

void MPVProcess::requestBitrateInfo() {
	writeToStdin("print_text INFO_VIDEO_BITRATE=${=video-bitrate}");
	writeToStdin("print_text INFO_AUDIO_BITRATE=${=audio-bitrate}");
}

#if NOTIFY_AUDIO_CHANGES
void MPVProcess::updateAudioTrack(int ID, const QString & name, const QString & lang) {
	qDebug("MPVProcess::updateAudioTrack: ID: %d", ID);

	int idx = audios.find(ID);
	if (idx == -1) {
		audio_info_changed = true;
		audios.addName(ID, name);
		audios.addLang(ID, lang);
	} else {
		// Track already existed
		if (audios.itemAt(idx).name() != name) {
			audio_info_changed = true;
			audios.addName(ID, name);
		}
		if (audios.itemAt(idx).lang() != lang) {
			audio_info_changed = true;
			audios.addLang(ID, lang);
		}
	}
}
#endif

#if NOTIFY_SUB_CHANGES
void MPVProcess::updateSubtitleTrack(int ID, const QString & name, const QString & lang) {
	qDebug("MPVProcess::updateSubtitleTrack: ID: %d", ID);

	int idx = subs.find(SubData::Sub, ID);
	if (idx == -1) {
		subtitle_info_changed = true;
		subs.add(SubData::Sub, ID);
		subs.changeName(SubData::Sub, ID, name);
		subs.changeLang(SubData::Sub, ID, lang);
	}
	else {
		// Track already existed
		if (subs.itemAt(idx).name() != name) {
			subtitle_info_changed = true;
			subs.changeName(SubData::Sub, ID, name);
		}
		if (subs.itemAt(idx).lang() != lang) {
			subtitle_info_changed = true;
			subs.changeLang(SubData::Sub, ID, lang);
		}
	}
}
#endif

// Called when the process is finished
void MPVProcess::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	qDebug("MPVProcess::processFinished: exitCode: %d, status: %d", exitCode, (int) exitStatus);
	// Send this signal before the endoffile one, otherwise
	// the playlist will start to play next file before all
	// objects are notified that the process has exited.
	emit processExited();
	if (received_end_of_file) emit receivedEndOfFile();
}

void MPVProcess::gotError(QProcess::ProcessError error) {
	qDebug("MPVProcess::gotError: %d", (int) error);
}


// Start of what used to be mpvoptions.cpp and was pulled in with an include

void MPVProcess::setMedia(const QString & media, bool is_playlist) {
	arg << "--term-playing-msg="
			"INFO_MPV_VERSION=${=mpv-version:}\n"

			"INFO_VIDEO_WIDTH=${=width}\n"
			"INFO_VIDEO_HEIGHT=${=height}\n"
			"INFO_VIDEO_ASPECT=${=video-aspect}\n"
//			"INFO_VIDEO_DSIZE=${=dwidth}x${=dheight}\n"
			"INFO_VIDEO_FPS=${=fps}\n"
//			"INFO_VIDEO_BITRATE=${=video-bitrate}\n"
			"INFO_VIDEO_FORMAT=${=video-format}\n"
			"INFO_VIDEO_CODEC=${=video-codec}\n"

//			"INFO_AUDIO_BITRATE=${=audio-bitrate}\n"
			"INFO_AUDIO_FORMAT=${=audio-codec-name:${=audio-format}}\n"
			"INFO_AUDIO_CODEC=${=audio-codec}\n"
			"INFO_AUDIO_RATE=${=audio-params/samplerate:${=audio-samplerate}}\n"
			"INFO_AUDIO_NCH=${=audio-params/channel-count:${=audio-channels}}\n"

			"INFO_LENGTH=${=duration:${=length}}\n"
			"INFO_DEMUXER=${=demuxer}\n"

			"INFO_TITLES=${=disc-titles}\n"
			"INFO_CHAPTERS=${=chapters}\n"
			"INFO_TRACKS_COUNT=${=track-list/count}\n"

			"METADATA_TITLE=${metadata/by-key/title:}\n"
			"METADATA_ARTIST=${metadata/by-key/artist:}\n"
			"METADATA_ALBUM=${metadata/by-key/album:}\n"
			"METADATA_GENRE=${metadata/by-key/genre:}\n"
			"METADATA_DATE=${metadata/by-key/date:}\n"
			"METADATA_TRACK=${metadata/by-key/track:}\n"
			"METADATA_COPYRIGHT=${metadata/by-key/copyright:}\n"

			"INFO_MEDIA_TITLE=${=media-title:}\n";

	arg << "--term-status-msg=STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}";

	if (is_playlist) {
		arg << "--playlist=" + media;
	} else {
		arg << media;
	}
}

void MPVProcess::setFixedOptions() {
	arg << "--no-config";
	arg << "--no-quiet";
	arg << "--terminal";
	arg << "--no-msg-color";
	arg << "--input-file=/dev/stdin";
	//arg << "--no-osc";
	//arg << "--msg-level=vd=v";
}

void MPVProcess::disableInput() {
	arg << "--no-input-default-bindings";
	arg << "--input-x11-keyboard=no";
	arg << "--no-input-cursor";
	arg << "--cursor-autohide=no";
}

bool MPVProcess::isOptionAvailable(const QString & option) {
	InfoReader * ir = InfoReader::obj(executable());
	ir->getInfo();
	//qDebug() << "MPVProcess::isOptionAvailable: option_list:" << ir->optionList();
	return ir->optionList().contains(option);
}

void MPVProcess::addVFIfAvailable(const QString & vf, const QString & value) {
	InfoReader * ir = InfoReader::obj(executable());
	ir->getInfo();
	if (ir->vfList().contains(vf)) {
		QString s = "--vf-add=" + vf;
		if (!value.isEmpty()) s += "=" + value;
		arg << s;
	} else {
		QString f = vf +"="+ value;
		qDebug("MPVProcess::addVFIfAvailable: filter %s is not used because it's not available", f.toLatin1().constData());
	}
}

void MPVProcess::setOption(const QString & option_name, const QVariant & value) {
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
	if (option_name == "sub") {
		arg << "--sub-file=" + value.toString();
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
	if (option_name == "subfont-text-scale" || option_name == "ass-font-scale") {
		arg << "--sub-scale=" + value.toString();
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
			//qDebug() << "MPVProcess::setOption: alsa device:" << device;
			device = device.replace("=", ":").replace(".", ",");
			o = "alsa:device=[" + device + "]";
		}
		arg << "--ao=" + o;
	}
	else
	if (option_name == "vc") {
		qDebug() << "MPVProcess::setOption: video codec ignored";
	}
	else
	if (option_name == "ac") {
		qDebug() << "MPVProcess::setOption: audio codec ignored";
	}
	else
	if (option_name == "afm") {
		QString s = value.toString();
		if (s == "hwac3") arg << "--ad=spdif:ac3,spdif:dts";
	}
	else
	if (option_name == "enable_streaming_sites_support") {
		if (isOptionAvailable("--ytdl")) {
			if (value.toBool()) arg << "--ytdl"; else arg << "--ytdl=no";
		}
	}
	else
	if (option_name == "fontconfig") {
		if (isOptionAvailable("--use-text-osd")) {
			bool b = value.toBool();
			if (b) arg << "--use-text-osd=yes"; else arg << "--use-text-osd=no";
		}
	}
	else
	if (option_name == "verbose") {
		arg << "-v";
		verbose = true;
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
		option_name == "frames")
	{
		QString s = "--" + option_name;
		if (!value.isNull()) s += "=" + value.toString();
		arg << s;
	}
	else
	{
		qDebug() << "MPVProcess::setOption: unknown option:" << option_name;
	}
}

void MPVProcess::addUserOption(const QString & option) {
	arg << option;
	if (option == "-v") {
		verbose = true;
	}
}

void MPVProcess::addVF(const QString & filter_name, const QVariant & value) {
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
		addVFIfAvailable("unsharp", "la=-1.5:ca=-1.5");
	}
	else
	if (filter_name == "sharpen") {
		addVFIfAvailable("unsharp", "la=1.5:ca=1.5");
	}
	else
	if (filter_name == "noise") {
		arg << "--vf-add=noise=9:pattern=yes";
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
		//arg << "--screenshot-template=%{filename:shot}-%p-%04n";
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
		} else {
			QString s = filter_name;
			if (!option.isEmpty()) s += "=" + option;
			arg << "--vf-add=" + s;
		}
	}
}

void MPVProcess::addStereo3DFilter(const QString & in, const QString & out) {
	arg << "--vf-add=stereo3d=" + in + ":" + out;
}

void MPVProcess::addAF(const QString & filter_name, const QVariant & value) {
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

void MPVProcess::quit() {
	writeToStdin("quit 0");
}

void MPVProcess::setVolume(int v) {
	writeToStdin("set volume " + QString::number(v));
}

void MPVProcess::setOSD(int o) {
	writeToStdin("osd " + QString::number(o));
}

void MPVProcess::setAudio(int ID) {
	writeToStdin("set aid " + QString::number(ID));
}

void MPVProcess::setVideo(int ID) {
	writeToStdin("set vid " + QString::number(ID));
}

void MPVProcess::setSubtitle(int type, int ID) {
	writeToStdin("set sid " + QString::number(ID));
}

void MPVProcess::disableSubtitles() {
	writeToStdin("set sid no");
}

void MPVProcess::setSecondarySubtitle(int ID) {
	writeToStdin("set secondary-sid " + QString::number(ID));
}

void MPVProcess::disableSecondarySubtitles() {
	writeToStdin("set secondary-sid no");
}

void MPVProcess::setSubtitlesVisibility(bool b) {
	writeToStdin(QString("set sub-visibility %1").arg(b ? "yes" : "no"));
}

void MPVProcess::seek(double secs, int mode, bool precise) {
	QString s = "seek " + QString::number(secs) + " ";
	switch (mode) {
		case 0 : s += "relative "; break;
		case 1 : s += "absolute-percent "; break;
		case 2 : s += "absolute "; break;
	}
	if (precise) s += "exact"; else s += "keyframes";
	writeToStdin(s);
}

void MPVProcess::mute(bool b) {
	writeToStdin(QString("set mute %1").arg(b ? "yes" : "no"));
}

void MPVProcess::setPause(bool b) {
	writeToStdin(QString("set pause %1").arg(b ? "yes" : "no"));
}

void MPVProcess::frameStep() {
	writeToStdin("frame_step");
}

void MPVProcess::frameBackStep() {
	writeToStdin("frame_back_step");
}

void MPVProcess::showOSDText(const QString & text, int duration, int level) {
	QString str = QString("show_text \"%1\" %2 %3").arg(text).arg(duration).arg(level);
	writeToStdin(str);
}

void MPVProcess::showFilenameOnOSD() {
	writeToStdin("show_text \"${filename}\" 2000 0");
}

void MPVProcess::showTimeOnOSD() {
	writeToStdin("show_text \"${time-pos} / ${length:0} (${percent-pos}%)\" 2000 0");
}

void MPVProcess::setContrast(int value) {
	writeToStdin("set contrast " + QString::number(value));
}

void MPVProcess::setBrightness(int value) {
	writeToStdin("set brightness " + QString::number(value));
}

void MPVProcess::setHue(int value) {
	writeToStdin("set hue " + QString::number(value));
}

void MPVProcess::setSaturation(int value) {
	writeToStdin("set saturation " + QString::number(value));
}

void MPVProcess::setGamma(int value) {
	writeToStdin("set gamma " + QString::number(value));
}

void MPVProcess::setChapter(int ID) {
	writeToStdin("set chapter " + QString::number(ID));
}

void MPVProcess::setExternalSubtitleFile(const QString & filename) {
	writeToStdin("sub_add \""+ filename +"\"");
	//writeToStdin("print_text ${track-list}");
	writeToStdin("print_text \"INFO_TRACKS_COUNT=${=track-list/count}\"");
}

void MPVProcess::setSubPos(int pos) {
	writeToStdin("set sub-pos " + QString::number(pos));
}

void MPVProcess::setSubScale(double value) {
	writeToStdin("set sub-scale " + QString::number(value));
}

void MPVProcess::setSubStep(int value) {
	writeToStdin("sub_step " + QString::number(value));
}

void MPVProcess::setSubForcedOnly(bool b) {
	writeToStdin(QString("set sub-forced-only %1").arg(b ? "yes" : "no"));
}

void MPVProcess::setSpeed(double value) {
	writeToStdin("set speed " + QString::number(value));
}

void MPVProcess::enableKaraoke(bool b) {
	if (b) writeToStdin("af add karaoke"); else writeToStdin("af del karaoke");
}

void MPVProcess::enableExtrastereo(bool b) {
	if (b) writeToStdin("af add extrastereo"); else writeToStdin("af del extrastereo");
}

void MPVProcess::enableVolnorm(bool b, const QString & option) {
	if (b) writeToStdin("af add drc=" + option); else writeToStdin("af del drc=" + option);
}

void MPVProcess::setAudioEqualizer(const QString & values) {
	if (values == previous_eq) return;

	if (!previous_eq.isEmpty()) {
		writeToStdin("af del equalizer=" + previous_eq);
	}
	writeToStdin("af add equalizer=" + values);
	previous_eq = values;
}

void MPVProcess::setAudioDelay(double delay) {
	writeToStdin("set audio-delay " + QString::number(delay));
}

void MPVProcess::setSubDelay(double delay) {
	writeToStdin("set sub-delay " + QString::number(delay));
}

void MPVProcess::setLoop(int v) {
	QString o;
	switch (v) {
		case -1: o = "no"; break;
		case 0: o = "inf"; break;
		default: o = QString::number(v);
	}
	writeToStdin(QString("set loop %1").arg(o));
}

void MPVProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	writeToStdin(QString("screenshot %1 %2").arg(include_subtitles ? "subtitles" : "video").arg(t == Single ? "single" : "each-frame"));
}

void MPVProcess::setTitle(int ID) {
	writeToStdin("set disc-title " + QString::number(ID));
}

#if DVDNAV_SUPPORT
void MPVProcess::discSetMousePos(int x, int y) {
	qDebug("MPVProcess::discSetMousePos: %d %d", x, y);
	//writeToStdin(QString("discnav mouse_move %1 %2").arg(x).arg(y));
	// mouse_move doesn't accept options :?

	// For some reason this doesn't work either...
	// So it's not possible to select options in the dvd menus just
	// because there's no way to pass the mouse position to mpv, or it
	// ignores it.
	writeToStdin(QString("mouse %1 %2").arg(x).arg(y));
	//writeToStdin("discnav mouse_move");
}

void MPVProcess::discButtonPressed(const QString & button_name) {
	writeToStdin("discnav " + button_name);
}
#endif

void MPVProcess::setAspect(double aspect) {
	writeToStdin("set video-aspect " + QString::number(aspect));
}

void MPVProcess::setFullscreen(bool b) {
	writeToStdin(QString("set fullscreen %1").arg(b ? "yes" : "no"));
}

#if PROGRAM_SWITCH
void MPVProcess::setTSProgram(int ID) {
	qDebug("MPVProcess::setTSProgram: function not supported");
}
#endif

void MPVProcess::toggleDeinterlace() {
	writeToStdin("cycle deinterlace");
}

void MPVProcess::askForLength() {
	writeToStdin("print_text \"INFO_LENGTH=${=length}\"");
}

void MPVProcess::setOSDScale(double value) {
	writeToStdin("set osd-scale " + QString::number(value));
}

void MPVProcess::changeVF(const QString & filter, bool enable, const QVariant & option) {
	qDebug() << "MPVProcess::changeVF:" << filter << enable;

	QString f;
	if (filter == "letterbox") {
		f = QString("expand=aspect=%1").arg(option.toDouble());
	}
	else
	if (filter == "noise") {
		f = "noise=9:pattern=yes";
	}
	else
	if (filter == "blur") {
		f = "unsharp=la=-1.5:ca=-1.5";
	}
	else
	if (filter == "sharpen") {
		f = "unsharp=la=1.5:ca=1.5";
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
	if (filter == "scale" || filter == "gradfun" || filter == "hqdn3d") {
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
		qDebug() << "MPVProcess::changeVF: unknown filter:" << filter;
	}

	if (!f.isEmpty()) {
		writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(f));
	}
}

void MPVProcess::changeStereo3DFilter(bool enable, const QString & in, const QString & out) {
	QString filter = "stereo3d=" + in + ":" + out;
	writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(filter));
}

void MPVProcess::setSubStyles(const AssStyles & styles, const QString &) {
	QString font = styles.fontname;
	arg << "--sub-text-font=" + font.replace(" ", "");
	arg << "--sub-text-color=#" + ColorUtils::colorToRRGGBB(styles.primarycolor);

	if (styles.borderstyle == AssStyles::Outline) {
		arg << "--sub-text-shadow-color=#" + ColorUtils::colorToRRGGBB(styles.backcolor);
	} else {
		arg << "--sub-text-back-color=#" + ColorUtils::colorToRRGGBB(styles.outlinecolor);
	}
	arg << "--sub-text-border-color=#" + ColorUtils::colorToRRGGBB(styles.outlinecolor);

	arg << "--sub-text-border-size=" + QString::number(styles.outline * 2.5);
	arg << "--sub-text-shadow-offset=" + QString::number(styles.shadow * 2.5);

	QString halign;
	switch (styles.halignment) {
		case AssStyles::Left: halign = "left"; break;
		case AssStyles::Right: halign = "right"; break;
	}

	QString valign;
	switch (styles.valignment) {
		case AssStyles::VCenter: valign = "center"; break;
		case AssStyles::Top: valign = "top"; break;
	}

	if (!halign.isEmpty() || !valign.isEmpty()) {
		if (isOptionAvailable("--sub-text-align-x")) {
			if (!halign.isEmpty()) arg << "--sub-text-align-x=" + halign;
			if (!valign.isEmpty()) arg << "--sub-text-align-y=" + valign;
		}
	}
}

void MPVProcess::setChannelsFile(const QString & filename) {
	arg << "--dvbin-file=" + filename;
}

#include "moc_mpvprocess.cpp"

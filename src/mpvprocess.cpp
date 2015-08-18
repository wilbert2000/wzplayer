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

// TODO: fix
static const QPoint default_osd_pos(25, 22);
static const QPoint max_osd_pos(300, 600);

// How to recognise eof
static QRegExp rx_endoffile("^Exiting... \\(End of file\\)");

MPVProcess::MPVProcess(QObject * parent)
	: PlayerProcess(parent, &rx_endoffile)
	, verbose(false)
	, subtitle_info_received(false)
	, subtitle_info_changed(false)
	, audio_info_changed(false)
#if NOTIFY_VIDEO_CHANGES
	, video_info_changed(false)
#endif
	, chapter_info_changed(false)
	, osd_pos(default_osd_pos)
	, osd_centered_x(false)
	, osd_centered_y(false)
{
	player_id = PlayerID::MPV;
}

MPVProcess::~MPVProcess() {
}

bool MPVProcess::startPlayer() {

	subs.clear();
	subtitle_info_received = false;
	subtitle_info_changed = false;

	audios.clear();
	audio_info_changed = false;

#if NOTIFY_VIDEO_CHANGES
	videos.clear();
	video_info_changed = false;
#endif

	chapters.clear();
	chapter_info_changed = false;

	osd_pos = default_osd_pos;
	osd_centered_x = false;
	osd_centered_y = false;

	return PlayerProcess::startPlayer();
}

void MPVProcess::updateAudioTrack(int ID,
								  const QString & name,
								  const QString & lang) {

	int idx = audios.find(ID);
	if (idx == -1) {
		// Not found: add it
		audio_info_changed = true;
		audios.addName(ID, name);
		audios.addLang(ID, lang);
		qDebug("MPVProcess::updateAudioTrack: added audio id: %d, name: '%s', lang: '%s'",
			   ID, name.toUtf8().constData(), lang.toUtf8().constData());
	} else {
		// Existing track
		if (audios.itemAt(idx).name() != name) {
			audio_info_changed = true;
			audios.addName(ID, name);
		}
		if (audios.itemAt(idx).lang() != lang) {
			audio_info_changed = true;
			audios.addLang(ID, lang);
		}
		qDebug("MPVProcess::updateAudioTrack: updated audio id: %d, name: '%s', lang: '%s'",
			   ID, name.toUtf8().constData(), lang.toUtf8().constData());
	}
}

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
		updateAudioTrack(ID, name, lang);
		return;
	}

	if (type == "sub") {
		updateSubtitleTrack(ID, name, lang);
	}
}

bool MPVProcess::parseProperty(const QString &name, const QString &value) {

	if (name == "MPV_VERSION") {
		mpv_version = value;
		qDebug() << "MPVProcess::parseProperty: mpv_version set to" << mpv_version;
		return true;
	}
	if (name == "TITLES") {
		int n_titles = value.toInt();
		for (int id = 0; id < n_titles; id++) {
			md.titles.addName(id, QString::number(id + 1));
		}
		qDebug("MPVProcess::parseProperty: added %d titles", n_titles);
		return true;
	}
	if (name == "MEDIA_TITLE") {
		if (!value.isEmpty() && value != "mp4" && !value.startsWith("mp4&")) {
			md.clip_name = value;
			qDebug() << "MPVProcess::parseProperty: set md.clip_name to" << md.clip_name;
		}
		return true;
	}
	if (name == "TRACKS_COUNT") {
		int tracks = value.toInt();
		qDebug("MPVProcess::parseProperty: requesting track info for %d tracks", tracks);
		for (int n = 0; n < tracks; n++) {
			writeToStdin(QString("print_text \"TRACK_INFO_%1: "
				"${track-list/%1/type} "
				"${track-list/%1/id} "
				"'${track-list/%1/lang:}' "
				"'${track-list/%1/title:}' "
				"${track-list/%1/selected}\"").arg(n));
		}
		return true;
	}

	bool parsed = PlayerProcess::parseProperty(name, value);

	if (name == "CHAPTERS") {
		// Ask for chapter titles
		if (md.n_chapters > 0)
			qDebug("MPVProcess::parseProperty: requesting chapter titles");
		for (int n = 0; n < md.n_chapters; n++) {
			writeToStdin(QString("print_text CHAPTER_%1_NAME=${chapter-list/%1/title}").arg(n));
		}
	}

	return parsed;
}

void MPVProcess::parseChapterName(int id, QString title) {

	if (title.isEmpty()) title = QString::number(id + 1);

	chapters.addName(id, title);
	chapter_info_changed = true;

	qDebug("MPVProcess::parseChapterName: added chapter id: %d title: '%s'",
		   id, title.toUtf8().constData());
}

void MPVProcess::notifyChanges() {

	if (subtitle_info_changed) {
		subtitle_info_changed = false;
		subtitle_info_received = false;
		qDebug("MPVProcess::notifyChanges: emit subtitleInfoChanged");
		emit subtitleInfoChanged(subs);
	}
	if (subtitle_info_received) {
		subtitle_info_received = false;
		qDebug("MPVProcess::notifyChanges: emit subtitleInfoReceivedAgain");
		emit subtitleInfoReceivedAgain(subs);
	}

	if (audio_info_changed) {
		audio_info_changed = false;
		qDebug("MPVProcess::notifyChanges: emit audioInfoChanged");
		emit audioInfoChanged(audios);
	}

#if NOTIFY_VIDEO_CHANGES
	if (video_info_changed) {
		video_info_changed = false;
		qDebug("MPVProcess::notifyChanges: emit videoInfoChanged");
		emit videoInfoChanged(videos);
	}
#endif

	if (chapter_info_changed) {
		chapter_info_changed = false;
		qDebug("MPVProcess::notifyChanges: emit chaptersChanged");
		emit chaptersChanged(chapters);
	}
}

void MPVProcess::notifyTimestamp(double sec) {

	// Pass current time stamp
	emit receivedCurrentSec( sec );

	// Emulate frames. mpv won't give them.
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

	// Time stamp
	double sec = rx.cap(1).toDouble();
#if DVDNAV_SUPPORT
	double length = rx.cap(2).toDouble();
#endif
	// Status flags
	bool paused = rx.cap(3) == "yes";
	bool buffering = rx.cap(4) == "yes";
	bool idle = rx.cap(5) == "yes";

#if DVDNAV_SUPPORT
	// Duration changed
	if (length != md.duration) {
		md.duration = length;
		qDebug("MPVProcess::parseStatusLine: emit receivedDuration(%f)", length);
		emit receivedDuration(length);
	}
#endif

	// Because a time stamp change can coincide with a state change, it always
	// needs to be signaled. In all cases it will be signaled first, except for
	// the first playing frame, when it is done after emit playerFullyLoaded().

	if (paused) {
		qDebug("MPVProcess::parseStatusLine: paused");
		notifyTimestamp(sec);
		receivedPause();
		return;
	}
	if (buffering) {
		qDebug("MPVProcess::parseStatusLine: buffering");
		notifyTimestamp(sec);
		receivedBuffering();
		return;
	}
	if (idle) {
		qDebug("MPVProcess::parseStatusLine: core idle");
		notifyTimestamp(sec);
		receivedBuffering();
		return;
	}

	// Playing
	if (notified_player_is_running) {
		// Playing except for first frame
		notifyTimestamp(sec);
		notifyChanges();
		return;
	}

	// First and only run of state playing
	notified_player_is_running = true;

	// Have any video?
	if (md.video_width <= 0) {
		md.novideo = true;
		qDebug("MPVProcess::parseStatusLine: emit receivedNoVideo()");
		emit receivedNoVideo();
	}

	qDebug("MPVProcess::parseStatusLine: emit playerFullyLoaded()");
	emit playerFullyLoaded();

	// Wait some secs to ask for bitrate
	QTimer::singleShot(12000, this, SLOT(requestBitrateInfo()));

	notifyTimestamp(sec);

	// Clear frame counter if no fps
	if (fps == 0.0) {
		emit receivedCurrentFrame(0);
	}
}

bool MPVProcess::parseLine(QString &line) {

	// Custom status line. Make sure it matches!
	static QRegExp rx_status("^STATUS: ([0-9\\.-]+) / ([0-9\\.-]+) P: (yes|no) B: (yes|no) I: (yes|no)");
	static QRegExp rx_playing("^Playing:.*|^\\[ytdl_hook\\].*");

	#if !NOTIFY_VIDEO_CHANGES
	static QRegExp rx_video("^.*Video --vid=(\\d+)([ \\(\\)\\*]+)('(.*)'|)");
	#endif

	static QRegExp rx_audio("^.*Audio --aid=(\\d+)( --alang=([a-z]+)|)([ \\(\\)\\*]+)('(.*)'|)");

	static QRegExp rx_dsize("^VIDEO_DSIZE=(\\d+)x(\\d+)");
	static QRegExp rx_vo("^VO: \\[(.*)\\]");
	static QRegExp rx_ao("^AO: \\[(.*)\\]");

	static QRegExp rx_video_codec("^VIDEO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_video_property("^VIDEO_([A-Z]+)=\\s*(.*)");
	static QRegExp rx_audio_codec("^AUDIO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_audio_property("^AUDIO_([A-Z]+)=\\s*(.*)");

	static QRegExp rx_meta_data("^METADATA_([A-Z]+)=\\s*(.*)");

	static QRegExp rx_subs("^.*Subs\\s+--sid=(\\d+)( --slang=([a-z]+)|)([ \\(\\)\\*]+)('(.*)'|)");

	static QRegExp rx_trackinfo("^TRACK_INFO_(\\d+): (audio|video|sub) (\\d+) '(.*)' '(.*)' (yes|no)");
	static QRegExp rx_chaptername("^CHAPTER_(\\d+)_NAME=\\s*(.*)");

	#if DVDNAV_SUPPORT
	static QRegExp rx_switch_title("^\\[dvdnav\\] DVDNAV, switched to title:\\s+(\\d+)");
	#endif

	static QRegExp rx_property("^INFO_([A-Z_]+)=\\s*(.*)");
	static QRegExp rx_forbidden("HTTP error 403 Forbidden");

	if (verbose) {
		line = line.replace("[statusline] ", "");
		line = line.replace("[cplayer] ", "");
	}

	// Parse custom status line
	if (rx_status.indexIn(line) >= 0) {
		parseStatusLine(rx_status);
		return true;
	}

	if (PlayerProcess::parseLine(line))
		return true;

	// Playing
	if (rx_playing.indexIn(line) >= 0) {
		qDebug("MVPProcess::parseLine: emit receivedPlaying()");
		emit receivedPlaying();
		return true;
	}

#if !NOTIFY_VIDEO_CHANGES
	// Video id and name
	if (rx_video.indexIn(line) >= 0) {
		int id = rx_video.cap(1).toInt();
		QString title = rx_video.cap(4).trimmed();
		md.videos.addName(id, title);
		qDebug("MPVProcess::parseLine: added video id: %d, name: '%s'",
			   id, title.toUtf8().constData());
		return true;
	}
#endif

	// Audio id, name and lang
	if (rx_audio.indexIn(line) >= 0) {
		updateAudioTrack(rx_audio.cap(1).toInt(),
						 rx_audio.cap(6),
						 rx_audio.cap(3));
		return true;
	}

	// VO
	if (rx_vo.indexIn(line) >= 0) {
		QString vo = rx_vo.cap(1);
		qDebug() << "MVPProcess::parseLine: emit receivedVO(" << vo << ")";
		emit receivedVO(vo);
		// Ask for window resolution
		writeToStdin("print_text VIDEO_DSIZE=${=dwidth}x${=dheight}");
		return true;
	}

	// Window resolution w x h
	if (rx_dsize.indexIn(line) >= 0) {
		int w = rx_dsize.cap(1).toInt();
		int h = rx_dsize.cap(2).toInt();
		qDebug("MPVProcess::parseLine: emit receivedWindowResolution(%d, %d)", w, h);
		emit receivedWindowResolution(w, h);
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
		md.video_codec = rx_video_codec.cap(2);
		qDebug() << "MPVProcess::parseLine: md.video_codec set to" << md.video_codec;
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
		md.audio_codec = rx_audio_codec.cap(2);
		qDebug() << "MPVProcess::parseLine: md.audio_codec set to" << md.audio_codec;
		return true;
	}

	// Audio property AUDIO_name and value
	if (rx_audio_property.indexIn(line) >= 0) {
		return parseAudioProperty(rx_audio_property.cap(1),
								  rx_audio_property.cap(2));
	}

	// Subtitles id, lang and title
	if (rx_subs.indexIn(line) >= 0) {
		updateSubtitleTrack(rx_subs.cap(1).toInt(),
							rx_subs.cap(6),
							rx_subs.cap(3));
		return true;
	}

	// Track info
	if (rx_trackinfo.indexIn(line) >= 0) {
		parseTrackInfo(rx_trackinfo);
		return true;
	}

	// Chapter id and title
	if (rx_chaptername.indexIn(line) >= 0) {
		parseChapterName(rx_chaptername.cap(1).toInt(),
						 rx_chaptername.cap(2));
		return true;
	}

	// Property INFO_name and value
	if (rx_property.indexIn(line) >= 0) {
		return parseProperty(rx_property.cap(1), rx_property.cap(2));
	}

	// Meta data METADATA_name and value
	if (rx_meta_data.indexIn(line) >= 0) {
		return parseMetaDataProperty(rx_meta_data.cap(1), rx_meta_data.cap(2));
	}

#if DVDNAV_SUPPORT
	if (rx_switch_title.indexIn(line) >= 0) {
		int title = rx_switch_title.cap(1).toInt();
		qDebug("MPVProcess::parseLine: title changed to %d", title);
		// Ask for chapter info
		// Wait 10 secs. because it can take a while until the title start to play
		QTimer::singleShot(10000, this, SLOT(requestChapterInfo()));
		return true;
	}
#endif

	// HTTP error 403 Forbidden
	if (rx_forbidden.indexIn(line) >= 0) {
		qDebug("MVPProcess::parseLine: 403 forbidden");
		emit receivedForbiddenText();
		return true;
	}

	return false;
}

void MPVProcess::requestChapterInfo() {
	writeToStdin("print_text \"INFO_CHAPTERS=${=chapters}\"");
}

void MPVProcess::requestBitrateInfo() {
	writeToStdin("print_text VIDEO_BITRATE=${=video-bitrate}");
	writeToStdin("print_text AUDIO_BITRATE=${=audio-bitrate}");
}


// Start of what used to be mpvoptions.cpp and was pulled in with an include

void MPVProcess::setMedia(const QString & media, bool is_playlist) {
	arg << "--term-playing-msg="
			"INFO_MPV_VERSION=${=mpv-version:}\n"

			"VIDEO_WIDTH=${=width}\n"
			"VIDEO_HEIGHT=${=height}\n"
			"VIDEO_ASPECT=${=video-aspect}\n"
//			"VIDEO_DSIZE=${=dwidth}x${=dheight}\n"
			"VIDEO_FPS=${=fps}\n"
//			"VIDEO_BITRATE=${=video-bitrate}\n"
			"VIDEO_FORMAT=${=video-format}\n"
			"VIDEO_CODEC=${=video-codec}\n"

//			"AUDIO_BITRATE=${=audio-bitrate}\n"
			"AUDIO_FORMAT=${=audio-codec-name:${=audio-format}}\n"
			"AUDIO_CODEC=${=audio-codec}\n"
			"AUDIO_RATE=${=audio-params/samplerate:${=audio-samplerate}}\n"
			"AUDIO_NCH=${=audio-params/channel-count:${=audio-channels}}\n"

			"INFO_LENGTH=${=duration:${=length}}\n"
			"INFO_DEMUXER=${=demuxer}\n"

			"INFO_TITLES=${=disc-titles}\n"
			"INFO_CHAPTERS=${=chapters}\n"
			"INFO_TRACKS_COUNT=${=track-list/count}\n"

			// TODO: check name, author, comment etc.
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

void MPVProcess::setOSDPos(const QPoint &pos) {
	// mpv has no way to set the OSD position directly,
	// so this hack uses osd-margin to emulate it.

	// TODO: versioning
	// osd-margin-y Integer (0 to 300) (default: 25)
	// osd-margin-y Integer (0 to 600) (default: 22)
	// options/osd-align-x and y from version 0.9.0 onwards

	// From mpv/options/options.c
	// OPT_FLOATRANGE("osd-bar-w", osd_bar_w, 0, 1, 100),
	// OPT_FLOATRANGE("osd-bar-h", osd_bar_h, 0, 0.1, 50),

	// TODO: how to disable OSD echo for set command

	// Handle y first
	if (pos.y() > max_osd_pos.y()) {
		// Hack: center osd and hope for the best
		if (!osd_centered_y) {
			writeToStdin("set options/osd-align-y center");
			osd_centered_y = true;
		}
		// Reset margin to default
		if (osd_pos.y() != default_osd_pos.y()) {
			osd_pos.ry() = default_osd_pos.y();
			writeToStdin("set options/osd-margin-y "
				+ QString::number(osd_pos.y()));
		}
	} else {
		// Reset alignment hack if centered
		if (osd_centered_y) {
			osd_centered_y = false;
			writeToStdin("set options/osd-align-y top");
		}

		int y = pos.y();
		if (y < default_osd_pos.y()) {
			y = default_osd_pos.y();
		}

		if (y != osd_pos.y()) {
			osd_pos.ry() = y;
			writeToStdin("set options/osd-margin-y " + QString::number(y));
		}
	}

	// Handle x
	if (pos.x() > max_osd_pos.x()) {
		// Hack: center osd and hope for the best
		if (!osd_centered_x) {
			writeToStdin("set options/osd-align-x center");
			osd_centered_x = true;
		}
		// Reset margin
		if (osd_pos.x() != default_osd_pos.x()) {
			osd_pos.rx() = default_osd_pos.x();
			writeToStdin("set options/osd-margin-x "
				+ QString::number(osd_pos.x()));
		}
	} else {
		// Reset alignment hack if centered
		if (osd_centered_x) {
			osd_centered_x = false;
			writeToStdin("set options/osd-align-x left");
		}

		int x = pos.x();
		if (x < default_osd_pos.x()) {
			x = default_osd_pos.x();
		}

		if (x != osd_pos.x()) {
			osd_pos.rx() = x;
			writeToStdin("set options/osd-margin-x " + QString::number(x));
		}
	}
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

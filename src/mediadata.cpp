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

#include "mediadata.h"
#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include "log4qt/logger.h"


LOG4QT_DECLARE_STATIC_LOGGER(logger, TMediaData)


TMediaData::TMediaData() :
	selected_type(TYPE_UNKNOWN),
	video_aspect_original(-1),
	video_hwdec(false) {
	init();
}

TMediaData::TMediaData(const TMediaData& md) :
	filename(md.filename),
	selected_type(md.selected_type),
	disc(md.disc),
	video_aspect_original(md.video_aspect_original),
	video_hwdec(md.video_hwdec) {
	init();
}

void TMediaData::init() {

	detected_type = TYPE_UNKNOWN;

	start_sec = 0;
	start_sec_set = false;
	time_sec = 0;
	duration = 0;

	mpegts = false;

	video_width = 0;
	video_height = 0;
	video_fps = 0;
	video_out_width = 0;
	video_out_height = 0;
	video_bitrate = -1;
	// video_hwdec set by constructor

	audio_bitrate = -1;
	audio_rate = 0;
	audio_nch = 0;

	angle = 0;
	angles = 0;

	initialized = false;
}

bool TMediaData::isCD(Type type) {
	return type == TYPE_CDDA || type == TYPE_VCD;
}

bool TMediaData::isDVD(Type type) {
	return type == TYPE_DVD || type == TYPE_DVDNAV;
}

bool TMediaData::isDisc(Type type) {
	return type == TYPE_DVD
			|| type == TYPE_DVDNAV
			|| type == TYPE_VCD
			|| type == TYPE_CDDA
			|| type == TYPE_BLURAY;
}

bool TMediaData::detectedDisc() const {
	return isDisc(detected_type);
}

bool TMediaData::selectedDisc() const {
	return isDisc(selected_type);
}

QString TMediaData::displayNameAddTitleOrTrack(const QString& title) const {

	if (disc.valid && disc.title > 0) {
		if (isCD(detected_type)) {
			static const char* format = QT_TRANSLATE_NOOP("TMediaData", "%1 track %2");
			return qApp->translate("TMediaData", format).arg(title).arg(QString::number(disc.title));
		}
		static const char* format = QT_TRANSLATE_NOOP("TMediaData", "%1 title %2");
		return qApp->translate("TMediaData", format).arg(title).arg(QString::number(disc.title));
	}

	return title;
}

QString TMediaData::displayName(bool show_tag) const {

	if (filename.isEmpty())
		return "";

	if (show_tag) {
		QString title = this->title;
		if (!title.isEmpty())
			return displayNameAddTitleOrTrack(title);

		title = meta_data.value("title");
		if (!title.isEmpty())
			return displayNameAddTitleOrTrack(title);

		title = meta_data.value("name");
		if (!title.isEmpty())
			return displayNameAddTitleOrTrack(title);

		if (disc.valid && disc.title > 0) {
			// See also Gui::TPlaylist::addFile() and TTitleData::getDisplayName()
			if (isCD(detected_type)) {
				return qApp->translate("Gui::TPlaylist", "Track %1").arg(QString::number(disc.title));
			}
			return qApp->translate("Gui::TPlaylist", "Title %1").arg(QString::number(disc.title));
		}
	}

	// Remove path
	QFileInfo fi(filename);
	QString fn = fi.fileName();
	if (fn.isEmpty()) {
		return filename;
	}
	return fn;
}

QString TMediaData::typeToString(Type type) {

	QString s;
	switch (type) {
	case TYPE_UNKNOWN: s = "unknown"; break;
	case TYPE_FILE: s = "file"; break;
	case TYPE_DVD: s = "dvd"; break;
	case TYPE_DVDNAV: s = "dvdnav"; break;
	case TYPE_VCD: s = "vcd"; break;
	case TYPE_CDDA: s = "cdda"; break;
	case TYPE_BLURAY: s = "br"; break;
	case TYPE_STREAM: s = "stream"; break;
	case TYPE_TV: s = "tv"; break;
	}

	return s;
}

TMediaData::Type TMediaData::stringToType(QString type) {

	type = type.toLower();

	if (type == "file")
		return TYPE_FILE;
	if (type == "dvd")
		return TYPE_DVD;
	if (type == "dvdnav")
		return TYPE_DVDNAV;
	if (type == "vcd")
		return TYPE_VCD;
	if (type == "cdda")
		return TYPE_CDDA;
	if (type == "br")
		return TYPE_BLURAY;
	if (type == "stream")
		return TYPE_STREAM;
	if (type == "tv")
		return TYPE_TV;

	return TYPE_UNKNOWN;
}

void TMediaData::list() const {
	logger()->debug("TMediaData::list");

    logger()->debug("filename: '%1'", filename);
    logger()->debug("selected type: %1", typeToString(selected_type));
    logger()->debug("detected type: %1", typeToString(detected_type));
    logger()->debug("valid disc URL: %1", disc.valid);
    logger()->debug("stream_url: '%1'", stream_url);

    logger()->debug("start: " + QString::number(start_sec));
    logger()->debug("start sec set: %1", start_sec_set);
    logger()->debug("time_sec: " + QString::number(time_sec));
    logger()->debug("duration: " + QString::number(duration));

    logger()->debug("demuxer: '%1'", demuxer);
    logger()->debug("mpegts: %1", mpegts);

    logger()->debug("video driver: '%1'", vo);
    logger()->debug("video_width: %1", video_width);
    logger()->debug("video_height: %1", video_height);
    logger()->debug("video_aspect: %1", video_aspect);
    logger()->debug("video_aspect_original: %1", video_aspect_original);
    logger()->debug("video_fps: " + QString::number(video_fps));

    logger()->debug("video_out_width: %1", video_out_width);
    logger()->debug("video_out_height: %1", video_out_height);

    logger()->debug("video_format: '%1'", video_format);
    logger()->debug("video_codec: '%1'", video_codec);
    logger()->debug("video_bitrate: %1", video_bitrate);
    logger()->debug("video_hwdec: %1", video_hwdec);
    logger()->debug("Video tracks:");
	videos.list();

    logger()->debug("audio driver: '%1'", ao);
    logger()->debug("audio_format: '%1'", audio_format);
    logger()->debug("audio_codec: '%1'", audio_codec);
    logger()->debug("audio_bitrate: %1", audio_bitrate);
    logger()->debug("audio_rate: %1", audio_rate);
    logger()->debug("audio_nch: %1", audio_nch);
    logger()->debug("Audio tracks:");
	audios.list();

    logger()->debug("Subtitles:");
	subs.list();
    logger()->debug("Titles:");
	titles.list();
    logger()->debug("Chapters:");
	chapters.list();

#if PROGRAM_SWITCH
    logger()->debug("Programs:");
	programs.list();
#endif

    logger()->debug("Title: '%1'", title);
    logger()->debug("Meta data:");
	TMetaData::const_iterator i = meta_data.constBegin();
	while (i != meta_data.constEnd()) {
        logger()->debug(i.key() + " = " + i.value());
		i++;
	}

    logger()->debug("dvd_id: '%1'", dvd_id);
    logger()->debug("Angle: %1/%2", angle, angles);

    logger()->debug("initialized: %1", initialized);
}


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

#include "mediadata.h"
#include <QApplication>
#include <QFileInfo>
#include <cmath>
#include <QDebug>


TMediaData::TMediaData()
	: selected_type(TYPE_UNKNOWN)
	, video_hwdec(false) {
	init();
}

TMediaData::TMediaData(const QString& fname, Type sel_type, bool hwdec)
	: filename(fname)
	, selected_type(sel_type)
	, video_hwdec(hwdec) {
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
	video_aspect = 0;
	video_aspect_set = false;
	video_fps = 0;
	video_out_width = 0;
	video_out_height = 0;
	video_bitrate = 0;
	//video_hwdec = false;

	audio_bitrate = 0;
	audio_rate = 0;
	audio_nch = 0;

	title_is_menu = false;

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

	if (detectedDisc()) {
		bool ok;
		TDiscData disc = TDiscName::split(filename, &ok);
		if (ok) {
			if (isCD(detected_type)) {
				static const char* format = QT_TRANSLATE_NOOP("TMediaData", "%1 track %2");
				return qApp->translate("TMediaData", format).arg(title, QString::number(disc.title));
			}
			static const char* format = QT_TRANSLATE_NOOP("TMediaData", "%1 title %2");
			return qApp->translate("TMediaData", format).arg(title, QString::number(disc.title));
		}
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

		title = meta_data.value("TITLE");
		if (!title.isEmpty())
			return displayNameAddTitleOrTrack(title);

		title = meta_data.value("NAME");
		if (!title.isEmpty())
			return displayNameAddTitleOrTrack(title);

		if (!stream_title.isEmpty())
			return stream_title;

		if (detectedDisc()) {
			bool ok;
			TDiscData disc = TDiscName::split(filename, &ok);
			if (ok) {
				// See also Gui::TPlaylist::addFile() and TTitleData::getDisplayName()
				if (isCD(detected_type)) {
					return qApp->translate("Gui::TPlaylist", "Track %1").arg(QString::number(disc.title));
				}
				return qApp->translate("Gui::TPlaylist", "Title %1").arg(QString::number(disc.title));
			}
			return filename;
		}
	}

	// Don't parse disc
	if (detectedDisc()) {
		return filename;
	}

	// Remove path
	QFileInfo fi(filename);
	if (fi.exists()) {
		// Return filename without path
		return fi.fileName();
	}

	return filename;
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
	qDebug("TMediaData::list");

	qDebug("  filename: '%s'", filename.toUtf8().data());
	qDebug("  selected type: %s", typeToString(selected_type).toUtf8().data());
	qDebug("  detected type: %s", typeToString(detected_type).toUtf8().data());

	qDebug("  start: %f", start_sec);
	qDebug("  start sec set: %d", start_sec_set);
	qDebug("  time_sec: %f", time_sec);
	qDebug("  duration: %f", duration);

	qDebug("  demuxer: '%s'", demuxer.toUtf8().data());
	qDebug("  mpegts: %d", mpegts);

	qDebug("  video_width: %d", video_width);
	qDebug("  video_height: %d", video_height); 
	qDebug("  video_aspect: %f", video_aspect);
	qDebug("  video_aspect_set: %d", video_aspect_set);
	qDebug("  video_fps: '%f'", video_fps);

	qDebug("  video_out_width: %d", video_out_width);
	qDebug("  video_out_height: %d", video_out_height);

	qDebug("  video_format: '%s'", video_format.toUtf8().data());
	qDebug("  video_codec: '%s'", video_codec.toUtf8().data());
	qDebug("  video_bitrate: %d", video_bitrate);
	qDebug("  video_hwdec: %d", video_hwdec);
	qDebug("  Video tracks:");
	videos.list();

	qDebug("  audio_format: '%s'", audio_format.toUtf8().data());
	qDebug("  audio_codec: '%s'", audio_codec.toUtf8().data());
	qDebug("  audio_bitrate: %d", audio_bitrate);
	qDebug("  audio_rate: %d", audio_rate);
	qDebug("  audio_nch: %d", audio_nch);
	qDebug("  Audio tracks:");
	audios.list();

	qDebug("  Subtitles:");
	subs.list();
	qDebug("  Titles:");
	titles.list();
	qDebug("  Chapters:");
	chapters.list();

#if PROGRAM_SWITCH
	qDebug("  Programs:");
	programs.list();
#endif

	qDebug("  Title: '%s'", title.toUtf8().constData());
	qDebug("  Meta data:");
	MetaData::const_iterator i = meta_data.constBegin();
	while (i != meta_data.constEnd()) {
		qDebug() << i.key() << "=" << i.value();
		i++;
	}

	qDebug("  stream_title: '%s'", stream_title.toUtf8().constData());
	qDebug("  stream_url: '%s'", stream_title.toUtf8().constData());

	qDebug("  dvd_id: '%s'", dvd_id.toUtf8().data());
	qDebug("  title_is_menu: %d", title_is_menu);

	qDebug("  initialized: %d", initialized);
}


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
#include <QFileInfo>
#include <cmath>
#include <QDebug>


MediaData::MediaData() {
	reset();
}

MediaData::~MediaData() {
}

void MediaData::reset(bool clear_filename_and_selected_type) {

	start_sec = 0;
	start_sec_prop = 0;
	time_sec = 0;
	duration = 0;
	start_sec_set = false;
	start_sec_prop_set = false;

	video_width = 0;
	video_height = 0;
	video_aspect = 0;
	video_out_width = 0;
	video_out_height = 0;
	video_fps = 0;

	if (clear_filename_and_selected_type) {
		filename = "";
		selected_type = TYPE_UNKNOWN;
	}
	detected_type = TYPE_UNKNOWN;

	dvd_id = "";


#if PROGRAM_SWITCH
	programs.clear();
#endif
	videos.clear();
	audios.clear();
	subs.clear();

	n_chapters = 0;
	chapters.clear();

	titles.clear();

	// Clip info;
	meta_data.clear();

	stream_title = "";
	stream_url = "";

	demuxer = "";

	// Other data
	video_format = "";
	audio_format = "";
	video_codec = "";
	audio_codec = "";
	video_bitrate = 0;
	audio_bitrate = 0;
	audio_rate = 0;
	audio_nch = 0;

	initialized = false;

}

bool MediaData::isCD(Type type) {
	return type == TYPE_CDDA || type == TYPE_VCD;
}

bool MediaData::isDVD(Type type) {
	return type == TYPE_DVD || type == TYPE_DVDNAV;
}

QString MediaData::displayName(bool show_tag) {
	if (show_tag) {
		QString name = meta_data.value("NAME");
		if (!name.isEmpty())
			return name;
		name = meta_data.value("TITLE");
		if (!name.isEmpty())
			return name;
		if (!stream_title.isEmpty())
			return stream_title;
	}

	QFileInfo fi(filename);
	if (fi.exists()) 
		return fi.fileName(); // filename without path
	else
		return filename;
}

QString MediaData::typeToString(Type type) {

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

MediaData::Type MediaData::stringToType(QString type) {

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


void MediaData::list() {
	qDebug("MediaData::list");

	qDebug("  filename: '%s'", filename.toUtf8().data());
	qDebug("  selected type: %s", typeToString(selected_type).toUtf8().data());
	qDebug("  detected type: %s", typeToString(detected_type).toUtf8().data());

	qDebug("  start: %f", start_sec);
	qDebug("  start prop: %f", start_sec_prop);
	qDebug("  start prop set: %d", start_sec_prop_set);
	qDebug("  duration: %f", duration);

	qDebug("  video_width: %d", video_width); 
	qDebug("  video_height: %d", video_height); 
	qDebug("  video_aspect: %f", video_aspect);
	qDebug("  video_fps: '%f'", video_fps );

	qDebug("  video_out_width: %d", video_out_width);
	qDebug("  video_out_height: %d", video_out_height);

	qDebug("  dvd_id: '%s'", dvd_id.toUtf8().data());

	qDebug("  Videos:");
	videos.list();

	qDebug("  Audios:");
	audios.list();

	qDebug("  Subs:");
	subs.list();

	qDebug("  Chapters: %d", n_chapters);
	chapters.list();

	qDebug("  Titles:");
	titles.list();

#if PROGRAM_SWITCH
	qDebug("  Programs:");
	programs.list();
#endif

	qDebug("  demuxer: '%s'", demuxer.toUtf8().data() );
	qDebug("  video_format: '%s'", video_format.toUtf8().data() );
	qDebug("  audio_format: '%s'", audio_format.toUtf8().data() );
	qDebug("  video_codec: '%s'", video_codec.toUtf8().data() );
	qDebug("  audio_codec: '%s'", audio_codec.toUtf8().data() );
	qDebug("  video_bitrate: %d", video_bitrate );
	qDebug("  audio_bitrate: %d", audio_bitrate );
	qDebug("  audio_rate: %d", audio_rate );
	qDebug("  audio_nch: %d", audio_nch );

	qDebug("  Meta data:");
	MetaData::const_iterator i = meta_data.constBegin();
	while (i != meta_data.constEnd()) {
		qDebug() << i.key() << "=" << i.value();
		i++;
	}

	qDebug("  initialized: %d", initialized);
}


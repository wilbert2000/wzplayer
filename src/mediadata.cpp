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

void MediaData::reset() {

	start_sec = 0;
	time_sec = 0;
	duration = 0;
	start_sec_set = false;

	video_width = 0;
	video_height = 0;
	video_out_width = 0;
	video_out_height = 0;

	video_aspect = 0;

	filename = "";
	dvd_id = "";
	type = TYPE_UNKNOWN;


#if PROGRAM_SWITCH
	programs.clear();
#endif
	videos.clear();
	audios.clear();
	titles.clear();

	subs.clear();

	chapters.clear();

	n_chapters = 0;

	initialized=false;

	// Clip info;
	meta_data.clear();

	stream_title = "";
	stream_url = "";

	// Other data
	demuxer="";
	video_format="";
	audio_format="";
	video_bitrate=0;
	video_fps="";
	audio_bitrate=0;
	audio_rate=0;
	audio_nch=0;
	video_codec="";
	audio_codec="";
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


void MediaData::list() {
	qDebug("MediaData::list");

	qDebug("  filename: '%s'", filename.toUtf8().data());
	qDebug("  duration: %f", duration);

	qDebug("  video_width: %d", video_width); 
	qDebug("  video_height: %d", video_height); 
	qDebug("  video_aspect: %f", video_aspect);

	qDebug("  video_out_width: %d", video_out_width);
	qDebug("  video_out_height: %d", video_out_height);

	qDebug("  novideo(): %d", noVideo());

	qDebug("  type: %d", type);
	qDebug("  dvd_id: '%s'", dvd_id.toUtf8().data());

	qDebug("  initialized: %d", initialized);

	qDebug("  chapters: %d", n_chapters);
	chapters.list();

	qDebug("  Subs:");
	subs.list();

#if PROGRAM_SWITCH
	qDebug("  Programs:");
	programs.list();
#endif
	qDebug("  Videos:");
	videos.list();

	qDebug("  Audios:");
	audios.list();

	qDebug("  Titles:");
	titles.list();

	//qDebug("  chapters: %d", chapters);
	//qDebug("  angles: %d", angles);

	qDebug("  demuxer: '%s'", demuxer.toUtf8().data() );
	qDebug("  video_format: '%s'", video_format.toUtf8().data() );
	qDebug("  audio_format: '%s'", audio_format.toUtf8().data() );
	qDebug("  video_bitrate: %d", video_bitrate );
	qDebug("  video_fps: '%s'", video_fps.toUtf8().data() );
	qDebug("  audio_bitrate: %d", audio_bitrate );
	qDebug("  audio_rate: %d", audio_rate );
	qDebug("  audio_nch: %d", audio_nch );
	qDebug("  video_codec: '%s'", video_codec.toUtf8().data() );
	qDebug("  audio_codec: '%s'", audio_codec.toUtf8().data() );

	qDebug("  Meta data:");
	MetaData::const_iterator i = meta_data.constBegin();
	while (i != meta_data.constEnd()) {
		qDebug() << i.key() << "=" << i.value();
		i++;
	}
}


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

#ifndef _MEDIADATA_H_
#define _MEDIADATA_H_

#include <QString>
#include <QSettings>

#include "config.h"
#include "discname.h"
#include "maps/tracks.h"
#include "subtracks.h"
#include "maps/titletracks.h"
#include "maps/chapters.h"


class MediaData {
public:
	// Types of media
	enum Type {
		TYPE_UNKNOWN = -1,
		TYPE_FILE = 0,
		TYPE_DVD = DiscName::DVD,
		TYPE_DVDNAV = DiscName::DVDNAV,
		TYPE_VCD = DiscName::VCD,
		TYPE_CDDA = DiscName::CDDA,
		TYPE_BLURAY = DiscName::BLURAY,
		TYPE_STREAM,
		TYPE_TV
	};

	MediaData();
	MediaData(const QString &fame, Type sel_type);
	virtual ~MediaData() {}

	QString filename;

	Type selected_type;
	Type detected_type;

	// Start time from first status line or reported by player
	double start_sec;
	// Current time video, without start time substracted
	// See MediaSettings for time with start time substracted
	double time_sec;
	double duration;
	bool start_sec_set;
	bool start_sec_prop_set;

	//Resolution of the video
	int video_width;
	int video_height;
	double video_aspect;
	double video_fps;

	// Resolution with aspect and filters applied
	int video_out_width;
	int video_out_height;

	bool noVideo() { return video_out_width <= 0; } // Can be audio

	QString dvd_id;

	Maps::TTracks videos;
	Maps::TTracks audios;
	SubTracks subs;

	int n_chapters;
	Maps::TChapters chapters;

	Maps::TTitleTracks titles;

#if PROGRAM_SWITCH
	Tracks programs;
#endif

	QString stream_title;
	QString stream_url;

	QString demuxer;

	// Other data not really useful for us,
	// just to show info to the user.
	QString video_format;
	QString audio_format;
	QString video_codec;
	QString audio_codec;
	int video_bitrate;
	int audio_bitrate;
	int audio_rate;
	int audio_nch;

	// Meta data names and values
	typedef QMap<QString, QString> MetaData;
	MetaData meta_data;

	bool initialized;

	static bool isCD(Type type);
	static bool isDVD(Type type);
	static bool isDisc(Type type);
	bool detectedDisc();

	static QString typeToString(Type type);
	static Type stringToType(QString type);

	QString displayName(bool show_tag = true);
	void list();

private:
	void init();
};

#endif

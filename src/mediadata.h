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


class TMediaData {
public:
	// Types of media
	enum Type {
		TYPE_UNKNOWN = -1,
		TYPE_FILE = 0,

		// Need to be the same as TDiscName
		TYPE_DVD = TDiscName::DVD,
		TYPE_DVDNAV = TDiscName::DVDNAV,
		TYPE_VCD = TDiscName::VCD,
		TYPE_CDDA = TDiscName::CDDA,
		TYPE_BLURAY = TDiscName::BLURAY,

		TYPE_STREAM,
		TYPE_TV
	};

	TMediaData();
	TMediaData(const QString& fname, Type sel_type);
	virtual ~TMediaData() {}

	QString filename;

	Type selected_type;
	Type detected_type;

	// Start time reported by player
	double start_sec;
	// Current time video, without start time substracted
	// See TMediaSettings for time with start time substracted
	double time_sec;
	double duration;
	bool start_sec_set;

	//Resolution of the video
	int video_width;
	int video_height;
	double video_aspect;
	double video_fps;

	// Resolution with aspect and filters applied
	int video_out_width;
	int video_out_height;

	// Can be audio
	bool noVideo() const {
		return video_out_width <= 0 || video_out_height <= 0;
	}

	Maps::TTracks videos;
	Maps::TTracks audios;
	SubTracks subs;
	Maps::TTitleTracks titles;
	Maps::TChapters chapters;

#if PROGRAM_SWITCH
	Tracks programs;
#endif

	QString dvd_id;

	// DVDNAV
	bool title_is_menu;

	QString stream_title;
	QString stream_url;

	QString demuxer;
	bool mpegts;

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
	bool detectedDisc() const;
	bool selectedDisc() const;

	static QString typeToString(Type type);
	static Type stringToType(QString type);

	QString displayName(bool show_tag = true) const;
	void list() const;

private:
	void init();
};

#endif

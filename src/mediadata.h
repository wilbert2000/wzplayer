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

#ifndef MEDIADATA_H
#define MEDIADATA_H

#include <QString>
#include <QSettings>

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

    QString filename;

    // Type selected by TPlayer.open()
    Type selected_type;
    // Detected type only set for disc types by player proc
    Type detected_type;
    // Parsed disc data set by TPlayer::openDisc()
    TDiscName disc;
    // Is filename an image file
    bool image;
    // URL stream
    QString stream_url;

    // Start time reported by player
    int start_ms_player;
    double getStartSecPlayer() const { return double(start_ms_player) / 1000; }

    // Start time currently in use. Only set for MPV.
    // Did set start_ms
    bool start_ms_used;
    int start_ms;
    double getStartSec() const { return double(start_ms) / 1000; }
    // Kludge for low resolution time stamps MPlayer
    QString fuzzy_time;

    // Current time video, without start time substracted
    int pos_ms;
    double getPosSec() const { return double(pos_ms) / 1000; }
    void setPosSec(const double secs) { pos_ms = qRound(secs * 1000); }
    // Current time video, with start time substracted and MPEG-TS PTS
    // timestamp rollover corrected. Also stored in TMediaSettings.
    int pos_gui_ms;

    // Duration in ms
    int duration_ms;
    double getDurationSec() const { return double(duration_ms) / 1000; }

    // Demuxer
    QString demuxer;
    QString demuxer_description;
    bool mpegts;

    // Video
    QString vo;
    int video_width;
    int video_height;
    QString video_aspect;
    double video_aspect_original;
    double video_fps;

    // Resolution with aspect and filters applied
    int video_out_width;
    int video_out_height;

    bool hasVideo() const {
        return video_out_width > 0 && video_out_height > 0;
    }
    bool noVideo() const {
        return video_out_width <= 0 || video_out_height <= 0;
    }

    QString video_format;
    QString video_codec;
    QString video_codec_description;
    QString video_colorspace;
    QString video_out_colorspace;
    int video_bitrate;
    bool video_hwdec;
    Maps::TTracks videos;

    // Audio
    QString ao;
    QString audio_format;
    QString audio_codec;
    QString audio_codec_description;
    int audio_bitrate;
    int audio_rate;
    int audio_nch;
    Maps::TTracks audios;

    bool hasAudio() const {
        return audios.count() > 0;
    }
    bool noAudio() const {
        return audios.count() <= 0;
    }

    // Subtitles, titles and chapters
    TSubTracks subs;
    Maps::TTitleTracks titles;
    Maps::TChapters chapters;

    // Clip info
    // Beware, title used by TPlaylist::onNewMediaStartedPlaying() to ID discs
    QString title;
    // Meta data names and values
    typedef QMap<QString, QString> TMetaData;
    TMetaData meta_data;

    // DVD ID
    QString dvd_disc_id;
    QString dvd_disc_serial;
    // DVD angles
    int angle;
    int angles;

    static bool isCD(Type type);
    static bool isDVD(Type type);
    static bool isDisc(Type type);
    bool detectedDisc() const;
    bool selectedDisc() const;

    static QString typeToString(Type type);
    static Type stringToType(QString type);

    QString name() const;
    void list() const;

private:
    void init();
    QString getTitle() const;
};

#endif // MEDIADATA_H

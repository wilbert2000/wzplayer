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

#include "wzdebug.h"
#include "config.h"
#include "name.h"


LOG4QT_DECLARE_STATIC_LOGGER(logger, TMediaData)


TMediaData::TMediaData() :
    selected_type(TYPE_UNKNOWN),
    detected_type(TYPE_UNKNOWN),
    image(false),

    start_ms_player(0),
    start_ms_used(false),
    start_ms(0),

    pos_ms(0),
    pos_gui_ms(0),
    duration_ms(0),

    mpegts(false),

    video_width(0),
    video_height(0),
    video_aspect_original(-1),
    video_fps(0),

    video_out_width(0),
    video_out_height(0),
    video_bitrate(-1),
    video_hwdec(false),

    audio_bitrate(-1),
    audio_rate(0),
    audio_nch(0),

    angle(0),
    angles(0) {
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

QString TMediaData::getTitle() const {

    if (filename.isEmpty()) {
        return "";
    }

    QString title = TName::cleanTitle(this->title);
    if (!title.isEmpty()) {
        return title;
    }
    title = TName::cleanTitle(meta_data.value("title"));
    if (!title.isEmpty()) {
        return title;
    }
    title = TName::cleanTitle(meta_data.value("name"));
    if (!title.isEmpty()) {
        return title;
    }

    return "";
}

// Return name field for playlist
QString TMediaData::name() const {

    if (filename.isEmpty()) {
        return "";
    }

    if (disc.valid) {
        if (disc.title == 0) {
            QString title = getTitle();
            if (title.isEmpty()) {
                return disc.displayName();
            }
            return QString("%1 - %2").arg(title).arg(disc.displayName());
        }
        return disc.displayName(false);
    }

    if (selected_type != TYPE_FILE) {
        QString title = getTitle();
        if (!title.isEmpty()) {
            return title;
        }
    }

    return TName::nameForURL(filename);
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

    WZDEBUG("filename: '" + filename + "'");
    WZDEBUG("selected type: " + typeToString(selected_type));
    WZDEBUG("detected type: " + typeToString(detected_type));
    WZDEBUG("disc URL valid: " + QString::number(disc.valid));
    if (disc.valid) {
        WZDEBUG("disc URL: " + disc.toString(true));
    }
    WZDEBUG("stream_url: '" + stream_url + "'");

    WZDEBUG("start_ms_player: " + QString::number(start_ms_player));
    WZDEBUG("start_ms_used: " + QString::number(start_ms_used));
    WZDEBUG("start_ms: " + QString::number(start_ms));
    WZDEBUG("fuzzy time: '" + fuzzy_time + "'");
    WZDEBUG("pos_ms: " + QString::number(pos_ms));
    WZDEBUG("pos_gui_ms: " + QString::number(pos_gui_ms));
    WZDEBUG("duration_ms: " + QString::number(duration_ms));

    WZDEBUG("demuxer: '" + demuxer + "'");
    WZDEBUG("mpegts: " + QString::number(mpegts));

    WZDEBUG("video driver: '" + vo + "'");
    WZDEBUG("video_width: " + QString::number(video_width));
    WZDEBUG("video_height: " + QString::number(video_height));
    WZDEBUG("video_aspect: '" + video_aspect + "'");
    WZDEBUG("video_aspect_original: " + QString::number(video_aspect_original));
    WZDEBUG("video_fps: " + QString::number(video_fps));
    WZDEBUG("video_colorspace: '" + video_colorspace + "'");
    WZDEBUG("video_out_colorspace: '" + video_out_colorspace + "'");

    WZDEBUG("video_out_width: " + QString::number(video_out_width));
    WZDEBUG("video_out_height: " + QString::number(video_out_height));

    WZDEBUG("video_format: '" + video_format + "'");
    WZDEBUG("video_codec: '" + video_codec + "'");
    WZDEBUG("video_codec_description: '" + video_codec_description + "'");
    WZDEBUG("video_bitrate: " + QString::number(video_bitrate));
    WZDEBUG("video_hwdec: " + QString::number(video_hwdec));
    WZDEBUG("Video tracks:");
    videos.list();

    WZDEBUG("audio driver: '" + ao + "'");
    WZDEBUG("audio_format: '" + audio_format + "'");
    WZDEBUG("audio_codec: '" + audio_codec + "'");
    WZDEBUG("audio_bitrate: " + QString::number(audio_bitrate));
    WZDEBUG("audio_rate: " + QString::number(audio_rate));
    WZDEBUG("audio_nch: " + QString::number(audio_nch));
    WZDEBUG("Audio tracks:");
    audios.list();

    WZDEBUG("subtitles:");
    subs.list();
    WZDEBUG("titles:");
    titles.list();
    WZDEBUG("chapters:");
    chapters.list();

    WZDEBUG("title: '" + title + "'");
    WZDEBUG("meta data:");
    TMetaData::const_iterator i = meta_data.constBegin();
    while (i != meta_data.constEnd()) {
        WZDEBUG("'" + i.key() + "' = '" + i.value() + "'");
        i++;
    }

    WZDEBUG("dvd_disc_id: '" + dvd_disc_id + "'");
    WZDEBUG("dvd_disc_serial: '" + dvd_disc_serial + "'");
    WZDEBUG("angle: " + QString::number(angle) + "/" + QString::number(angles));
}


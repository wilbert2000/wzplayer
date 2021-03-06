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

#include "settings/mediasettings.h"
#include "settings/preferences.h"
#include "settings/aspectratio.h"
#include "maps/tracks.h"
#include "mediadata.h"
#include "subtracks.h"
#include "wztime.h"
#include "config.h"
#include "wzdebug.h"

#include <QSettings>

namespace Settings {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TMediaSettings)


TMediaSettings::TMediaSettings(TMediaData* mdat)
    : volume(pref->initial_volume)
    , mute(false)
    , md(mdat) {

    reset();
}

TMediaSettings::~TMediaSettings() {
}

void TMediaSettings::reset() {
    WZTRACE("");

    player_id = pref->player_id;

    current_ms = 0;
    current_video_id = NoneSelected;
    current_audio_id = NoneSelected;
    external_audio = "";
    current_sub_idx = NoneSelected;
    current_sub_set_by_user = false;

    // Only used for loading settings for local files
    // and external subs during restart
    sub = SubData();
    sub.setID(NoneSelected);

    external_subtitles_fps = SFPS_None;

    current_secondary_sub_idx = NoneSelected;

    playing_single_track = false;
    current_angle = 0;

    aspect_ratio.setID(TAspectRatio::AspectAuto);

    restore_volume = true;
    old_volume = volume;
    volume = pref->initial_volume;
    old_mute = mute;
    mute = false;

    sub_delay = 0;
    audio_delay = 0;
    sub_pos = pref->initial_sub_pos; // 100% by default
    sub_scale = pref->initial_sub_scale;
    sub_scale_mpv = pref->initial_sub_scale_mpv;
    sub_scale_ass = pref->initial_sub_scale_ass;

    closed_caption_channel = 0; // disabled

    brightness = pref->initial_brightness;
    contrast = pref->initial_contrast;
    gamma = pref->initial_gamma;
    hue = pref->initial_hue;
    saturation = pref->initial_saturation;

    color_space = TColorSpace::COLORSPACE_AUTO;

    audio_equalizer = pref->initial_audio_equalizer;

    speed = 1.0;

    phase_filter = false;
    deblock_filter = false;
    dering_filter = false;
    gradfun_filter = false;
    noise_filter = false;
    postprocessing_filter = pref->initial_postprocessing;
    upscaling_filter = false;
    current_denoiser = NoDenoise;
    current_unsharp = 0;

    stereo3d_in = "none";
    stereo3d_out = QString::null;

    //current_deinterlacer = NoDeinterlace;
    current_deinterlacer = pref->initial_deinterlace;

    add_letterbox = false;

    karaoke_filter = false;
    extrastereo_filter = false;
    volnorm_filter = pref->initial_volnorm;

    audio_use_channels = pref->initial_audio_channels; //ChDefault; // (0)
    stereo_mode = pref->initial_stereo_mode; //Stereo; // (0)

    zoom_factor = pref->initial_zoom_factor; // 1.0;
    zoom_factor_fullscreen = pref->initial_zoom_factor; // 1.0;
    pan_offset = QPoint();
    pan_offset_fullscreen = QPoint();

    rotate = 0;
    flip = false;
    mirror = false;

    in_point_ms = 0;
    out_point_ms = -1;
    loop = false;

    current_demuxer = "unknown";

    forced_demuxer = "";
    if (pref->use_lavf_demuxer) forced_demuxer = "lavf";

    forced_video_codec = "";
    forced_audio_codec = "";

    original_demuxer = "";
    original_video_codec = "";
    original_audio_codec = "";

    player_additional_options = "";
    player_additional_video_filters = "";
    player_additional_audio_filters = "";
}

double TMediaSettings::aspectToDouble() {

    double aspect = aspect_ratio.toDouble();
    if (aspect == 0) {
        return (double) md->video_width / md->video_height;
    }
    if (aspect == -1) {
        return (double) md->video_aspect_original;
    }
    return aspect;
}

QString TMediaSettings::getColorSpaceOptionString() {

    switch (color_space) {
        case TColorSpace::COLORSPACE_BT_601: return "bt.601";
        case TColorSpace::COLORSPACE_BT_709: return "bt.709";
        case TColorSpace::COLORSPACE_SMPTE_240M: return "smpte-240m";
        case TColorSpace::COLORSPACE_BT_2020_NCL: return "bt.2020-ncl";
        case TColorSpace::COLORSPACE_BT_2020_CL: return "bt.2020-cl";
        case TColorSpace::COLORSPACE_RGB: return "rgb";
        case TColorSpace::COLORSPACE_XYZ: return "xyz";
        case TColorSpace::COLORSPACE_YCGCO: return "ycgco";
        default: break;
    }

    return "auto";
}

// static
QString TMediaSettings::getColorSpaceDescriptionString(TColorSpace colorSpace) {

    // TODO: translate (tr) needs descent from QObject
    switch (colorSpace) {
        case TColorSpace::COLORSPACE_BT_601: return "ITU-R BT.601 (SD)";
        case TColorSpace::COLORSPACE_BT_709: return "ITU-R BT.709 (HD)";
        case TColorSpace::COLORSPACE_SMPTE_240M: return "SMPTE-240M";
        case TColorSpace::COLORSPACE_BT_2020_NCL: return "BT.2020-NCL";
        case TColorSpace::COLORSPACE_BT_2020_CL: return "BT.2020-CL";
        case TColorSpace::COLORSPACE_RGB: return "RGB";
        case TColorSpace::COLORSPACE_XYZ: return "XYZ";
        case TColorSpace::COLORSPACE_YCGCO: return "YCgCo";
        default: break;
    }

    return "automatic";
}

QString TMediaSettings::getColorSpaceDescriptionString() {
    return getColorSpaceDescriptionString(color_space);
}

void TMediaSettings::list() {

    WZDEBUG("current_ms: " + QString::number(current_ms));
    WZDEBUG("current_video_id: " + QString::number(current_video_id));
    WZDEBUG("current_audio_id: " + QString::number(current_audio_id));

    WZDEBUG("current_sub_idx: " + QString::number(current_sub_idx));
    WZDEBUG("current_sub_set_by_user: "
            + QString::number(current_sub_set_by_user));
    WZDEBUG("external_subtitles: '" + sub.filename() + "'");
    WZDEBUG("external_subtitles_fps: "
            + QString::number(external_subtitles_fps));
    WZDEBUG("current_secondary_sub_idx: "
            + QString::number(current_secondary_sub_idx));

    WZDEBUG("current_angle: " + QString::number(current_angle));
    WZDEBUG("aspect_ratio: " + aspect_ratio.toString());
    WZDEBUG("color space: " + getColorSpaceDescriptionString());

    WZDEBUG("volume: " + QString::number(volume));
    WZDEBUG("mute: " + QString::number(mute));
    WZDEBUG("external_audio: '" + external_audio + "'");
    WZDEBUG("sub_delay: " + QString::number(sub_delay));
    WZDEBUG("audio_delay: " + QString::number(sub_delay));
    WZDEBUG("sub_pos: " + QString::number(sub_pos));
    WZDEBUG("sub_scale: " + QString::number(sub_scale));
    WZDEBUG("sub_scale_mpv: " + QString::number(sub_scale_mpv));
    WZDEBUG("sub_scale_ass: " + QString::number(sub_scale_ass));

    WZDEBUG("closed_caption_channel: "
            + QString::number(closed_caption_channel));

    WZDEBUG("brightness: " + QString::number(brightness));
    WZDEBUG("contrast: " + QString::number(contrast));
    WZDEBUG("gamma: " + QString::number(gamma));
    WZDEBUG("hue: " + QString::number(hue));
    WZDEBUG("saturation: " + QString::number(saturation));

    WZDEBUG("speed: " + QString::number(speed));

    WZDEBUG("phase_filter: " + QString::number(phase_filter));
    WZDEBUG("deblock_filter: " + QString::number(deblock_filter));
    WZDEBUG("dering_filter: " + QString::number(dering_filter));
    WZDEBUG("gradfun_filter: " + QString::number(gradfun_filter));
    WZDEBUG("noise_filter: " + QString::number(noise_filter));
    WZDEBUG("postprocessing_filter: " + QString::number(postprocessing_filter));
    WZDEBUG("upscaling_filter: " + QString::number(upscaling_filter));
    WZDEBUG("current_denoiser: " + QString::number(current_denoiser));
    WZDEBUG("current_unsharp: " + QString::number(current_unsharp));

    WZDEBUG("stereo3d_in: '" + stereo3d_in + "'");
    WZDEBUG("stereo3d_out: '" + stereo3d_out + "'");

    WZDEBUG("current_deinterlacer: " + QString::number(current_deinterlacer));

    WZDEBUG("add_letterbox: " + QString::number(add_letterbox));

    WZDEBUG("karaoke_filter: " + QString::number(karaoke_filter));
    WZDEBUG("extrastereo_filter: " + QString::number(extrastereo_filter));
    WZDEBUG("volnorm_filter: " + QString::number(volnorm_filter));

    WZDEBUG("audio_use_channels: " + QString::number(audio_use_channels));
    WZDEBUG("stereo_mode: " + QString::number(stereo_mode));

    WZDEBUG("zoom_factor: " + QString::number(zoom_factor));
    WZDEBUG("zoom_factor_fullscreen: "
            + QString::number(zoom_factor_fullscreen));
    WZDEBUG("pan_offset: (" + QString::number(pan_offset.x()) + ", "
             + QString::number(pan_offset.y()) + ")");
    WZDEBUG("pan_offset_fullscreen: ("
            + QString::number(pan_offset_fullscreen.x()) + ", "
            + QString::number(pan_offset_fullscreen.y()) + ")");

    WZDEBUG("flip: " + QString::number(flip));
    WZDEBUG("mirror: " + QString::number(mirror));
    WZDEBUG("rotate: " + QString::number(rotate));

    WZDEBUG("loop: " + QString::number(loop));
    WZDEBUG("in_point: " + TWZTime::formatMS(in_point_ms));
    WZDEBUG("out_point: " + TWZTime::formatMS(out_point_ms));

    WZDEBUG("current_demuxer: '" + current_demuxer + "'");

    WZDEBUG("forced_demuxer: '" + forced_demuxer + "'");
    WZDEBUG("forced_video_codec: '" + forced_video_codec + "'");
    WZDEBUG("forced_audio_codec: '" + forced_video_codec + "'");

    WZDEBUG("original_demuxer: '" + original_demuxer + "'");
    WZDEBUG("original_video_codec: '" + original_video_codec + "'");
    WZDEBUG("original_audio_codec: '" + original_video_codec + "'");

    WZDEBUG("player_additional_options: '" + player_additional_options + "'");
    WZDEBUG("player_additional_video_filters: '"
            + player_additional_video_filters + "'");
    WZDEBUG("player_additional_audio_filters: '"
            + player_additional_audio_filters + "'");
}

void TMediaSettings::save(QSettings* set) {
    WZDEBUG("");

    set->beginGroup("player_" + QString::number(player_id));

    set->setValue("current_demuxer", current_demuxer);
    set->setValue("forced_demuxer", forced_demuxer);
    set->setValue("forced_video_codec", forced_video_codec);
    set->setValue("forced_audio_codec", forced_audio_codec);
    set->setValue("original_demuxer", original_demuxer);
    set->setValue("original_video_codec", original_video_codec);
    set->setValue("original_audio_codec", original_audio_codec);

    // Save the tracks ID in a demuxer section
    QString demuxer_section = QString("demuxer_%1").arg(current_demuxer);
    if (!forced_demuxer.isEmpty()) {
        demuxer_section = QString("demuxer_%1").arg(forced_demuxer);
    }

    set->beginGroup(demuxer_section);

    set->setValue("current_video_id", current_video_id);
    set->setValue("current_audio_id", current_audio_id);

    set->setValue("current_secondary_sub_idx", current_secondary_sub_idx);

    set->endGroup();

    // Subtitles
    // Used to be in demux group as index "current_sub_id"
    // Player group is compromise between the needs of
    // internal and external subs
    if (current_sub_set_by_user) {
        set->setValue("current_sub_set_by_user", true);
        SubData sub = md->subs.itemAt(current_sub_idx);
        set->setValue("sub_type", sub.type());
        set->setValue("sub_id", sub.ID());
    }

    // Used to be outside player group as "external_subtitles"
    set->setValue("sub_filename", sub.filename());

    set->endGroup(); // player

    set->setValue("external_subtitles_fps", external_subtitles_fps);

    if (md->duration_ms > 0 && current_ms < md->duration_ms - 10000) {
        set->setValue("current_ms", current_ms);
    } else {
        set->setValue("current_ms", 0);
    }

    set->setValue("current_angle", current_angle);

    set->setValue("aspect_ratio", aspect_ratio.toInt());
    set->setValue("volume", volume);
    set->setValue("mute", mute);
    set->setValue("external_audio", external_audio);
    set->setValue("sub_delay", sub_delay);
    set->setValue("audio_delay", audio_delay);
    set->setValue("sub_pos", sub_pos);
    set->setValue("sub_scale", sub_scale);
    set->setValue("sub_scale_mpv", sub_scale_mpv);
    set->setValue("sub_scale_ass", sub_scale_ass);

    set->setValue("closed_caption_channel", closed_caption_channel);

    set->setValue("brightness", brightness);
    set->setValue("contrast", contrast);
    set->setValue("gamma", gamma);
    set->setValue("hue", hue);
    set->setValue("saturation", saturation);

    set->setValue("audio_equalizer", audio_equalizer);

    set->setValue("speed", speed);

    set->setValue("phase_filter", phase_filter);
    set->setValue("deblock_filter", deblock_filter);
    set->setValue("dering_filter", dering_filter);
    set->setValue("gradfun_filter", gradfun_filter);
    set->setValue("noise_filter", noise_filter);
    set->setValue("postprocessing_filter", postprocessing_filter);
    set->setValue("upscaling_filter", upscaling_filter);
    set->setValue("current_denoiser", current_denoiser);
    set->setValue("current_unsharp", current_unsharp);

    set->setValue("stereo3d_in", stereo3d_in);
    set->setValue("stereo3d_out", stereo3d_out);

    set->setValue("current_deinterlacer", current_deinterlacer);

    set->setValue("add_letterbox", add_letterbox);

    set->setValue("karaoke_filter", karaoke_filter);
    set->setValue("extrastereo_filter", extrastereo_filter);
    set->setValue("volnorm_filter", volnorm_filter);

    set->setValue("audio_use_channels", audio_use_channels);
    set->setValue("stereo_mode", stereo_mode);

    set->setValue("zoom_factor", zoom_factor);
    set->setValue("zoom_factor_fullscreen", zoom_factor_fullscreen);
    set->setValue("pan_offset", pan_offset);
    set->setValue("pan_offset_fullscreen", pan_offset_fullscreen);

    set->setValue("rotate", rotate);
    set->setValue("flip", flip);
    set->setValue("mirror", mirror);

    set->setValue("loop", loop);
    set->setValue("in_point", in_point_ms);
    set->setValue("out_point", out_point_ms);

    set->setValue("player_additional_options", player_additional_options);
    set->setValue("player_additional_video_filters", player_additional_video_filters);
    set->setValue("player_additional_audio_filters", player_additional_audio_filters);
}

void TMediaSettings::convertOldSelectedTrack(int &id) {

    // const int oldNoneSelected = -1000;

    if (id < NoneSelected) {
        id = NoneSelected;
    }
}

void TMediaSettings::load(QSettings* set) {
    WZDEBUG("");

    // Remember player id, at save time in can be changed
    player_id = pref->player_id;

    set->beginGroup("player_" + QString::number(player_id));

    current_demuxer = set->value("current_demuxer", current_demuxer).toString();
    forced_demuxer = set->value("forced_demuxer", forced_demuxer).toString();
    if (pref->use_lavf_demuxer) forced_demuxer = "lavf";
    forced_video_codec = set->value("forced_video_codec", forced_video_codec).toString();
    forced_audio_codec = set->value("forced_audio_codec", forced_audio_codec).toString();
    original_demuxer = set->value("original_demuxer", original_demuxer).toString();
    original_video_codec = set->value("original_video_codec", original_video_codec).toString();
    original_audio_codec = set->value("original_audio_codec", original_audio_codec).toString();

    // Load the tracks ID from a demuxer section
    QString demuxer_section = QString("demuxer_%1").arg(current_demuxer);
    if (!forced_demuxer.isEmpty()) {
        demuxer_section = QString("demuxer_%1").arg(forced_demuxer);
    }
    WZDEBUG("demuxer_section: '" + demuxer_section + "'");

    set->beginGroup(demuxer_section);

    current_video_id = set->value("current_video_id", NoneSelected).toInt();
    convertOldSelectedTrack(current_video_id);
    current_audio_id = set->value("current_audio_id", NoneSelected).toInt();
    convertOldSelectedTrack(current_audio_id);

    current_secondary_sub_idx = set->value("current_secondary_sub_id", NoneSelected).toInt();

    set->endGroup(); // demuxer

    current_sub_idx = NoneSelected;
    current_sub_set_by_user = set->value("current_sub_set_by_user", false).toBool();
    sub.setType((SubData::Type) set->value("sub_type", sub.type()).toInt());
    sub.setID(set->value("sub_id", sub.ID()).toInt());
    sub.setFilename(set->value("sub_filename", sub.filename()).toString());

    set->endGroup(); // player

    // Old config
    if (sub.filename().isEmpty()) {
        sub.setFilename(set->value("external_subtitles", "").toString());
    }

    current_ms = set->value("current_ms", current_ms).toInt();

    current_angle = set->value("current_angle", current_angle).toInt();
    if (current_angle < 0) {
        current_angle = 0;
    }

    aspect_ratio.setID(TAspectRatio::variantToTMenuID(set->value("aspect_ratio", aspect_ratio.toInt())));
    restore_volume = false;
    volume = set->value("volume", volume).toInt();
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    mute = set->value("mute", mute).toBool();
    external_subtitles_fps = set->value("external_subtitles_fps", external_subtitles_fps).toInt();
    external_audio = set->value("external_audio", external_audio).toString();
    sub_delay = set->value("sub_delay", sub_delay).toInt();
    audio_delay = set->value("audio_delay", audio_delay).toInt();
    sub_pos = set->value("sub_pos", sub_pos).toInt();
    sub_scale = set->value("sub_scale", sub_scale).toDouble();
    sub_scale_mpv = set->value("sub_scale_mpv", sub_scale_mpv).toDouble();
    sub_scale_ass = set->value("sub_scale_ass", sub_scale_ass).toDouble();

    closed_caption_channel = set->value("closed_caption_channel", closed_caption_channel).toInt();

    brightness = set->value("brightness", brightness).toInt();
    contrast = set->value("contrast", contrast).toInt();
    gamma = set->value("gamma", gamma).toInt();
    hue = set->value("hue", hue).toInt();
    saturation = set->value("saturation", saturation).toInt();

    audio_equalizer = set->value("audio_equalizer", audio_equalizer).toList();

    speed = set->value("speed", speed).toDouble();

    phase_filter = set->value("phase_filter", phase_filter).toBool();
    deblock_filter = set->value("deblock_filter", deblock_filter).toBool();
    dering_filter = set->value("dering_filter", dering_filter).toBool();
    gradfun_filter = set->value("gradfun_filter", gradfun_filter).toBool();
    noise_filter = set->value("noise_filter", noise_filter).toBool();
    postprocessing_filter = set->value("postprocessing_filter", postprocessing_filter).toBool();
    upscaling_filter = set->value("upscaling_filter", upscaling_filter).toBool();
    current_denoiser = set->value("current_denoiser", current_denoiser).toInt();
    current_unsharp = set->value("current_unsharp", current_unsharp).toInt();

    stereo3d_in = set->value("stereo3d_in", stereo3d_in).toString();
    stereo3d_out = set->value("stereo3d_out", stereo3d_out).toString();

    current_deinterlacer = set->value("current_deinterlacer", current_deinterlacer).toInt();

    add_letterbox = set->value("add_letterbox", add_letterbox).toBool();

    karaoke_filter = set->value("karaoke_filter", karaoke_filter).toBool();
    extrastereo_filter = set->value("extrastereo_filter", extrastereo_filter).toBool();
    volnorm_filter = set->value("volnorm_filter", volnorm_filter).toBool();

    audio_use_channels = set->value("audio_use_channels", audio_use_channels).toInt();
    stereo_mode = set->value("stereo_mode", stereo_mode).toInt();

    zoom_factor = set->value("zoom_factor", zoom_factor).toDouble();
    if (zoom_factor < TConfig::ZOOM_MIN || zoom_factor > TConfig::ZOOM_MAX)
        zoom_factor = 1;
    zoom_factor_fullscreen = set->value("zoom_factor_fullscreen", zoom_factor_fullscreen).toDouble();
    if (zoom_factor_fullscreen < TConfig::ZOOM_MIN || zoom_factor_fullscreen > TConfig::ZOOM_MAX)
        zoom_factor_fullscreen = 1;
    pan_offset = set->value("pan_offset", pan_offset).toPoint();
    pan_offset_fullscreen = set->value("pan_offset_fullscreen",
                                       pan_offset_fullscreen).toPoint();

    rotate = set->value("rotate", rotate).toInt();
    flip = set->value("flip", flip).toBool();
    mirror = set->value("mirror", mirror).toBool();

    loop = set->value("loop", loop).toBool();
    in_point_ms = set->value("in_point", in_point_ms).toInt();
    if (in_point_ms < 0) in_point_ms = 0;
    out_point_ms = set->value("out_point", out_point_ms).toInt();

    player_additional_options = set->value("player_additional_options",
        player_additional_options).toString();
    player_additional_video_filters = set->value(
        "player_additional_video_filters",
        player_additional_video_filters).toString();
    player_additional_audio_filters = set->value(
        "player_additional_audio_filters",
        player_additional_audio_filters).toString();

    // ChDefault not used anymore
    if (audio_use_channels == ChDefault) audio_use_channels = ChStereo;
}

} // namespace Settings

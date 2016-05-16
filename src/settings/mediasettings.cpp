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

#include <QSettings>

#include "log4qt/logger.h"
#include "config.h"
#include "settings/aspectratio.h"
#include "settings/mediasettings.h"
#include "settings/preferences.h"
#include "maps/tracks.h"
#include "subtracks.h"
#include "mediadata.h"


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
    logger()->debug("reset");

    player_id = pref->player_id;

	current_sec = 0;

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

#if PROGRAM_SWITCH
	current_program_id = NoneSelected;
#endif

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

    in_point = 0;
	out_point = -1;
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

void TMediaSettings::list() {
    logger()->debug("list");

    logger()->debug("  current_sec: " + QString::number(current_sec));
    logger()->debug("  current_video_id: %1", current_video_id);
    logger()->debug("  current_audio_id: %1", current_audio_id);
    logger()->debug("  current_sub_idx: %1", current_sub_idx);
    logger()->debug("  current_sub_set_by_user: %1", current_sub_set_by_user);
    logger()->debug("  external_subtitles: '%1'", sub.filename());
    logger()->debug("  external_subtitles_fps: '%1'", external_subtitles_fps);
    logger()->debug("  current_secondary_sub_idx: %1", current_secondary_sub_idx);

#if PROGRAM_SWITCH
    logger()->debug("  current_program_id: %1", current_program_id);
#endif
    logger()->debug("  current_angle: %1", current_angle);

    logger()->debug("  aspect_ratio: %1", aspect_ratio.toString());

    logger()->debug("  volume: %1", volume);
    logger()->debug("  mute: %1", mute);
    logger()->debug("  external_audio: '%1'", external_audio);
    logger()->debug("  sub_delay: %1", sub_delay);
    logger()->debug("  audio_delay: %1", sub_delay);
    logger()->debug("  sub_pos: %1", sub_pos);
    logger()->debug("  sub_scale: " + QString::number(sub_scale));
    logger()->debug("  sub_scale_mpv: " + QString::number(sub_scale_mpv));
    logger()->debug("  sub_scale_ass: " + QString::number(sub_scale_ass));

    logger()->debug("  closed_caption_channel: %1", closed_caption_channel);

    logger()->debug("  brightness: %1", brightness);
    logger()->debug("  contrast: %1", contrast);
    logger()->debug("  gamma: %1", gamma);
    logger()->debug("  hue: %1", hue);
    logger()->debug("  saturation: %1", saturation);

    logger()->debug("  speed: " + QString::number(speed));

    logger()->debug("  phase_filter: %1", phase_filter);
    logger()->debug("  deblock_filter: %1", deblock_filter);
    logger()->debug("  dering_filter: %1", dering_filter);
    logger()->debug("  gradfun_filter: %1", gradfun_filter);
    logger()->debug("  noise_filter: %1", noise_filter);
    logger()->debug("  postprocessing_filter: %1", postprocessing_filter);
    logger()->debug("  upscaling_filter: %1", upscaling_filter);
    logger()->debug("  current_denoiser: %1", current_denoiser);
    logger()->debug("  current_unsharp: %1", current_unsharp);

    logger()->debug("  stereo3d_in: %1", stereo3d_in);
    logger()->debug("  stereo3d_out: %1", stereo3d_out);

    logger()->debug("  current_deinterlacer: %1", current_deinterlacer);

    logger()->debug("  add_letterbox: %1", add_letterbox);

    logger()->debug("  karaoke_filter: %1", karaoke_filter);
    logger()->debug("  extrastereo_filter: %1", extrastereo_filter);
    logger()->debug("  volnorm_filter: %1", volnorm_filter);

    logger()->debug("  audio_use_channels: %1", audio_use_channels);
    logger()->debug("  stereo_mode: %1", stereo_mode);

    logger()->debug("  zoom_factor: " + QString::number(zoom_factor));
    logger()->debug("  zoom_factor_fullscreen: "
                    + QString::number(zoom_factor_fullscreen));
    logger()->debug("  pan_offset: (%1, %2)", pan_offset.x(), pan_offset.y());
    logger()->debug("  pan_offset_fullscreen: (%1, %2)",
                    pan_offset_fullscreen.x(), pan_offset_fullscreen.y());

    logger()->debug("  flip: %1", flip);
    logger()->debug("  mirror: %1", mirror);
    logger()->debug("  rotate: %1", rotate);

    logger()->debug("  loop: %1", loop);
    logger()->debug("  in_point: " + QString::number(in_point));
    logger()->debug("  out_point: " + QString::number(out_point));

    logger()->debug("  current_demuxer: '%1'", current_demuxer);

    logger()->debug("  forced_demuxer: '%1'", forced_demuxer);
    logger()->debug("  forced_video_codec: '%1'", forced_video_codec);
    logger()->debug("  forced_audio_codec: '%1'", forced_video_codec);

    logger()->debug("  original_demuxer: '%1'", original_demuxer);
    logger()->debug("  original_video_codec: '%1'", original_video_codec);
    logger()->debug("  original_audio_codec: '%1'", original_video_codec);

    logger()->debug("  player_additional_options: '%1'",
                    player_additional_options);
    logger()->debug("  player_additional_video_filters: '%1'",
                    player_additional_video_filters);
    logger()->debug("  player_additional_audio_filters: '%1'",
                    player_additional_audio_filters);
}

void TMediaSettings::save(QSettings* set) {
    logger()->debug("save");

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

#if PROGRAM_SWITCH
	set->setValue("current_program_id", current_program_id);
#endif

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

    if (current_sec < md->duration - 10) {
        set->setValue("current_sec", current_sec);
    } else {
        set->setValue("current_sec", 0);
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
	set->setValue("in_point", in_point);
	set->setValue("out_point", out_point);

	set->setValue("mplayer_additional_options", player_additional_options);
	set->setValue("mplayer_additional_video_filters", player_additional_video_filters);
	set->setValue("mplayer_additional_audio_filters", player_additional_audio_filters);
}

void TMediaSettings::convertOldSelectedTrack(int &id) {

	// const int oldNoneSelected = -1000;

	if (id < NoneSelected) {
		id = NoneSelected;
	}
}

void TMediaSettings::load(QSettings* set) {
    logger()->debug("load");

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
    logger()->debug("load: demuxer_section: " + demuxer_section);

	set->beginGroup(demuxer_section);

	current_video_id = set->value("current_video_id", NoneSelected).toInt();
	convertOldSelectedTrack(current_video_id);
	current_audio_id = set->value("current_audio_id", NoneSelected).toInt();
	convertOldSelectedTrack(current_audio_id);

	current_secondary_sub_idx = set->value("current_secondary_sub_id", NoneSelected).toInt();

#if PROGRAM_SWITCH
	current_program_id = set->value("current_program_id", NoneSelected).toInt();
#endif

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

	current_sec = set->value("current_sec", current_sec).toDouble();

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
	pan_offset_fullscreen = set->value("pan_offset_fullscreen", pan_offset_fullscreen).toPoint();

	rotate = set->value("rotate", rotate).toInt();
	flip = set->value("flip", flip).toBool();
	mirror = set->value("mirror", mirror).toBool();

	loop = set->value("loop", loop).toBool();
	in_point = set->value("in_point", in_point).toInt();
    if (in_point < 0) in_point = 0;
	out_point = set->value("out_point", out_point).toInt();

	player_additional_options = set->value("mplayer_additional_options", player_additional_options).toString();
	player_additional_video_filters = set->value("mplayer_additional_video_filters", player_additional_video_filters).toString();
	player_additional_audio_filters = set->value("mplayer_additional_audio_filters", player_additional_audio_filters).toString();

	// ChDefault not used anymore
	if (audio_use_channels == ChDefault) audio_use_channels = ChStereo;
}

} // namespace Settings

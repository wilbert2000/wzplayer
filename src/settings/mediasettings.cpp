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

#include <QDebug>
#include <QSettings>

#include "settings/aspectratio.h"
#include "settings/mediasettings.h"
#include "settings/preferences.h"
#include "maps/tracks.h"
#include "subtracks.h"
#include "mediadata.h"


namespace Settings {

TMediaSettings::TMediaSettings(TMediaData* mdat)
	: volume(pref->initial_volume)
	, mute(false)
	, md(mdat) {

	reset();
}

TMediaSettings::~TMediaSettings() {
}

void TMediaSettings::reset() {
	qDebug("Settings::TMediaSettings::reset");

	current_sec = 0;

	current_video_id = NoneSelected;
	current_audio_id = NoneSelected;
	external_audio = "";
	preferred_language_audio_set = false;
	current_sub_idx = NoneSelected;
	current_sub_set_by_user = false;

	// Only used for loading settings for local files
	// and external subs during restart
	sub = SubData();
	sub.setID(NoneSelected);

	external_subtitles_fps = SFPS_None;

#ifdef MPV_SUPPORT
	current_secondary_sub_idx = NoneSelected;
#endif

	playing_single_track = false;
	current_angle_id = 0;

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

#ifdef MPLAYER_SUPPORT
	karaoke_filter = false;
	extrastereo_filter = false;
#endif
	volnorm_filter = pref->initial_volnorm;

	audio_use_channels = pref->initial_audio_channels; //ChDefault; // (0)
	stereo_mode = pref->initial_stereo_mode; //Stereo; // (0)

	zoom_factor = pref->initial_zoom_factor; // 1.0;
	zoom_factor_fullscreen = pref->initial_zoom_factor; // 1.0;
	pan_offset = QPoint();
	pan_offset_fullscreen = QPoint();

	rotate = NoRotate;
	flip = false;
	mirror = false;

	loop = false;
	A_marker = -1;
	B_marker = -1;

	is264andHD = false;

	current_demuxer = "unknown";

	forced_demuxer = "";
	if (pref->use_lavf_demuxer) forced_demuxer = "lavf";

	forced_video_codec = "";
	forced_audio_codec = "";

	original_demuxer = "";
	original_video_codec = "";
	original_audio_codec = "";

	mplayer_additional_options = "";
	mplayer_additional_video_filters = "";
	mplayer_additional_audio_filters = "";
}

double TMediaSettings::aspectToDouble() {
	return aspect_ratio.toDouble(md->video_out_width, md->video_out_height);
}

void TMediaSettings::list() {
	qDebug("Settings::TMediaSettings::list");

	qDebug("  current_sec: %f", current_sec);
	qDebug("  current_video_id: %d", current_video_id);
	qDebug("  current_audio_id: %d", current_audio_id);
	qDebug("  current_sub_idx: %d", current_sub_idx);
	qDebug("  current_sub_set_by_user: %d", current_sub_set_by_user);
	qDebug("  external_subtitles: '%s'", sub.filename().toUtf8().data());
	qDebug("  external_subtitles_fps: '%d'", external_subtitles_fps);

#ifdef MPV_SUPPORT
	qDebug("  current_secondary_sub_idx: %d", current_secondary_sub_idx);
#endif

#if PROGRAM_SWITCH
	qDebug("  current_program_id: %d", current_program_id);
#endif
	qDebug("  current_angle_id: %d", current_angle_id);

	qDebug("  aspect_ratio: %s", aspect_ratio.toString().toUtf8().constData());

	qDebug("  volume: %d", volume);
	qDebug("  mute: %d", mute);
	qDebug("  external_audio: '%s'", external_audio.toUtf8().constData());
	qDebug("  sub_delay: %d", sub_delay);
	qDebug("  audio_delay: %d", sub_delay);
	qDebug("  sub_pos: %d", sub_pos);
	qDebug("  sub_scale: %f", sub_scale);
	qDebug("  sub_scale_mpv: %f", sub_scale_mpv);
	qDebug("  sub_scale_ass: %f", sub_scale_ass);

	qDebug("  closed_caption_channel: %d", closed_caption_channel);

	qDebug("  brightness: %d", brightness);
	qDebug("  contrast: %d", contrast);
	qDebug("  gamma: %d", gamma);
	qDebug("  hue: %d", hue);
	qDebug("  saturation: %d", saturation);

	qDebug("  speed: %f", speed);

	qDebug("  phase_filter: %d", phase_filter);
	qDebug("  deblock_filter: %d", deblock_filter);
	qDebug("  dering_filter: %d", dering_filter);
	qDebug("  gradfun_filter: %d", gradfun_filter);
	qDebug("  noise_filter: %d", noise_filter);
	qDebug("  postprocessing_filter: %d", postprocessing_filter);
	qDebug("  upscaling_filter: %d", upscaling_filter);
	qDebug("  current_denoiser: %d", current_denoiser);
	qDebug("  current_unsharp: %d", current_unsharp);

	qDebug("  stereo3d_in: %s", stereo3d_in.toUtf8().constData());
	qDebug("  stereo3d_out: %s", stereo3d_out.toUtf8().constData());

	qDebug("  current_deinterlacer: %d", current_deinterlacer);

	qDebug("  add_letterbox: %d", add_letterbox);

#ifdef MPLAYER_SUPPORT
	qDebug("  karaoke_filter: %d", karaoke_filter);
	qDebug("  extrastereo_filter: %d", extrastereo_filter);
#endif
	qDebug("  volnorm_filter: %d", volnorm_filter);

	qDebug("  audio_use_channels: %d", audio_use_channels);
	qDebug("  stereo_mode: %d", stereo_mode);

	qDebug("  zoom_factor: %f", zoom_factor);
	qDebug("  zoom_factor_fullscreen: %f", zoom_factor_fullscreen);
	qDebug() << "  pan_offset:" << pan_offset;
	qDebug() << "  pan_offset_fullscreen:" << pan_offset_fullscreen;

	qDebug("  rotate: %d", rotate);
	qDebug("  flip: %d", flip);
	qDebug("  mirror: %d", mirror);

	qDebug("  loop: %d", loop);
	qDebug("  A_marker: %d", A_marker);
	qDebug("  B_marker: %d", B_marker);

	qDebug("  current_demuxer: '%s'", current_demuxer.toUtf8().data());

	qDebug("  forced_demuxer: '%s'", forced_demuxer.toUtf8().data());
	qDebug("  forced_video_codec: '%s'", forced_video_codec.toUtf8().data());
	qDebug("  forced_audio_codec: '%s'", forced_video_codec.toUtf8().data());

	qDebug("  original_demuxer: '%s'", original_demuxer.toUtf8().data());
	qDebug("  original_video_codec: '%s'", original_video_codec.toUtf8().data());
	qDebug("  original_audio_codec: '%s'", original_video_codec.toUtf8().data());

	qDebug("  mplayer_additional_options: '%s'", mplayer_additional_options.toUtf8().data());
	qDebug("  mplayer_additional_video_filters: '%s'", mplayer_additional_video_filters.toUtf8().data());
	qDebug("  mplayer_additional_audio_filters: '%s'", mplayer_additional_audio_filters.toUtf8().data());

	qDebug("  is264andHD: %d", is264andHD);
}

void TMediaSettings::save(QSettings* set, int player_id) {
	qDebug("Settings::TMediaSettings::save");

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

	// Is this too agressive writing towards NoneSelected?
	if (current_video_id > md->videos.firstID())
		set->setValue("current_video_id", current_video_id);
	else set->setValue("current_video_id", NoneSelected);
	if (current_audio_id > md->audios.firstID())
		set->setValue("current_audio_id", current_video_id);
	else set->setValue("current_audio_id", NoneSelected);

	// Old config
	set->remove("current_sub_id");

#ifdef MPV_SUPPORT
	set->setValue("current_secondary_sub_idx", current_secondary_sub_idx);
#endif

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

	// Old config
	set->remove("external_subtitles");

	set->setValue("external_subtitles_fps", external_subtitles_fps);

	set->setValue("current_sec", current_sec);

	// Old config
	set->remove("current_title_id");
	set->remove("current_chapter_id");

	set->setValue("current_angle_id", current_angle_id);

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

#ifdef MPLAYER_SUPPORT
	set->setValue("karaoke_filter", karaoke_filter);
	set->setValue("extrastereo_filter", extrastereo_filter);
#endif
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
	set->setValue("A_marker", A_marker);
	set->setValue("B_marker", B_marker);

	set->setValue("mplayer_additional_options", mplayer_additional_options);
	set->setValue("mplayer_additional_video_filters", mplayer_additional_video_filters);
	set->setValue("mplayer_additional_audio_filters", mplayer_additional_audio_filters);

	set->setValue("is264andHD", is264andHD);
}

void TMediaSettings::convertOldSelectedTrack(int &id) {

	// const int oldNoneSelected = -1000;

	if (id < NoneSelected) {
		id = NoneSelected;
	}
}

void TMediaSettings::load(QSettings* set, int player_id) {
	qDebug("Settings::TMediaSettings::load");

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
	qDebug("Settings::TMediaSettings::load: demuxer_section: %s", demuxer_section.toUtf8().constData());

	set->beginGroup(demuxer_section);

	current_video_id = set->value("current_video_id", NoneSelected).toInt();
	convertOldSelectedTrack(current_video_id);
	current_audio_id = set->value("current_audio_id", NoneSelected).toInt();
	convertOldSelectedTrack(current_audio_id);

#ifdef MPV_SUPPORT
	current_secondary_sub_idx = set->value("current_secondary_sub_id", NoneSelected).toInt();
#endif

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

	current_angle_id = set->value("current_angle_id", current_angle_id).toInt();
	if (current_angle_id < 0) {
		current_angle_id = 0;
	}

	aspect_ratio.setID(TAspectRatio::toTMenuID(set->value("aspect_ratio", aspect_ratio.toInt())));
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

#ifdef MPLAYER_SUPPORT
	karaoke_filter = set->value("karaoke_filter", karaoke_filter).toBool();
	extrastereo_filter = set->value("extrastereo_filter", extrastereo_filter).toBool();
#endif
	volnorm_filter = set->value("volnorm_filter", volnorm_filter).toBool();

	audio_use_channels = set->value("audio_use_channels", audio_use_channels).toInt();
	stereo_mode = set->value("stereo_mode", stereo_mode).toInt();

	zoom_factor = set->value("zoom_factor", zoom_factor).toDouble();
	zoom_factor_fullscreen = set->value("zoom_factor_fullscreen", zoom_factor_fullscreen).toDouble();
	pan_offset = set->value("pan_offset", pan_offset).toPoint();
	pan_offset_fullscreen = set->value("pan_offset_fullscreen", pan_offset_fullscreen).toPoint();

	rotate = set->value("rotate", rotate).toInt();
	flip = set->value("flip", flip).toBool();
	mirror = set->value("mirror", mirror).toBool();

	loop = set->value("loop", loop).toBool();
	A_marker = set->value("A_marker", A_marker).toInt();
	B_marker = set->value("B_marker", B_marker).toInt();


	mplayer_additional_options = set->value("mplayer_additional_options", mplayer_additional_options).toString();
	mplayer_additional_video_filters = set->value("mplayer_additional_video_filters", mplayer_additional_video_filters).toString();
	mplayer_additional_audio_filters = set->value("mplayer_additional_audio_filters", mplayer_additional_audio_filters).toString();

	is264andHD = set->value("is264andHD", is264andHD).toBool();

	// ChDefault not used anymore
	if (audio_use_channels == ChDefault) audio_use_channels = ChStereo;
}

} // namespace Settings

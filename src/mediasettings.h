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

#ifndef _MEDIASETTINGS_H_
#define _MEDIASETTINGS_H_


/* Settings the user has set for this file, and that we need to */
/* restore the video after a restart */

#include <QString>
#include <QSize>
#include <QPoint>
#include "config.h"
#include "mediadata.h"
#include "subtracks.h"
#include "audioequalizerlist.h"

class QSettings;

class TMediaSettings {

public:
	enum Denoise { NoDenoise = 0, DenoiseNormal = 1, DenoiseSoft = 2 };
	enum Aspect { AspectAuto = 1, Aspect43 = 2, Aspect54 = 3, Aspect149 = 4,
                  Aspect169 = 5, Aspect1610 = 6, Aspect235 = 7, Aspect11 = 8, 
                  Aspect32 = 9, Aspect1410 = 10, Aspect118 = 11,
                  AspectNone = 0 };
	enum Deinterlace { NoDeinterlace = 0, L5 = 1, Yadif = 2, LB = 3, 
                       Yadif_1 = 4, Kerndeint = 5 };
	enum AudioChannels { ChDefault = 0, ChStereo = 2, ChSurround = 4, 
                         ChFull51 = 6, ChFull61 = 7, ChFull71 = 8 };
	enum StereoMode { Stereo = 0, Left = 1, Right = 2, Mono = 3, Reverse = 4 };

	enum Rotate { NoRotate = -1, Clockwise_flip = 0, Clockwise = 1, 
                  Counterclockwise = 2, Counterclockwise_flip = 3 };

	// Must be < 0, any ID >= 0 can be valid
	// SubNone must be -1, because that is used by the player process for SubNone
	enum IDs { NoneSelected = -2, SubNone = -1 };

	enum SubFPS { SFPS_None, SFPS_23, SFPS_24, SFPS_25, SFPS_30, SFPS_23976, SFPS_29970 };

	TMediaSettings(TMediaData* mdat);
	virtual ~TMediaSettings();

	virtual void reset();

	double current_sec;

	int current_video_id;
	int current_audio_id;
	QString external_audio; // external audio file
	bool preferred_language_audio_set;

	int current_sub_idx;
	bool current_sub_set_by_user;
	// Only used for loading settings for local files
	// and external subs during restart
	SubData sub;
	int external_subtitles_fps;

#ifdef MPV_SUPPORT
	int current_secondary_sub_idx;
#endif

	int current_title_id;
	bool playing_single_track;
	int current_angle_id;

#if PROGRAM_SWITCH
	int current_program_id;
#endif

	int aspect_ratio_id;

	//bool fullscreen;

	int volume;
	bool mute;

	int brightness, contrast, gamma, hue, saturation;

	TAudioEqualizerList audio_equalizer;

	int sub_delay;
	int audio_delay;

	// Subtitles position (0-100)
	int sub_pos;
	double sub_scale;
	double sub_scale_mpv;
	double sub_scale_ass;

	int closed_caption_channel; // 0 = disabled

	double speed; // Speed of playback: 1.0 = normal speed

	int current_deinterlacer;

	bool add_letterbox;

	// Filters in menu
	bool phase_filter;
	bool deblock_filter;
	bool dering_filter;
	bool gradfun_filter;
	bool noise_filter;
	bool postprocessing_filter;
	bool upscaling_filter; //!< Software scaling

	int current_denoiser;
	int current_unsharp;

	QString stereo3d_in;
	QString stereo3d_out;

	bool karaoke_filter;
	bool extrastereo_filter;
	bool volnorm_filter;

	int audio_use_channels;
	int stereo_mode;

	// playerwindow
	double zoom_factor;
	double zoom_factor_fullscreen;
	QPoint pan_offset;
	QPoint pan_offset_fullscreen;

	int rotate;
	bool flip; //!< Flip image
	bool mirror; //!< Mirrors the image on the Y axis.

	bool loop; //!< Loop. If true repeat the file
	int A_marker;
	int B_marker;

	//! The codec of the video is ffh264 and it's high definition
	bool is264andHD;

	QString current_demuxer;

	// Advanced settings
	QString forced_demuxer;
	QString forced_video_codec;
	QString forced_audio_codec;

	// A copy of the original values, so we can restore them.
	QString original_demuxer;
	QString original_video_codec;
	QString original_audio_codec;

	// Options to mplayer (for this file only)
	QString mplayer_additional_options;
	QString mplayer_additional_video_filters;
	QString mplayer_additional_audio_filters;

	// Some things that were before in mediadata
	// They can vary, because of filters, so better here

	//Resolution used by player
    //Can be bigger that video resolution
    //because of the aspect ratio or expand filter
    int win_width;
    int win_height;
    double win_aspect();

	//! Returns the aspect as a double. Returns 0 if aspect == AspectNone.
	double aspectToNum(Aspect aspect);
	static QString aspectToString(Aspect aspect);

	void list();

	void save(QSettings * set, int player_id);
	void load(QSettings * set, int player_id);

private:
	TMediaData* md;
	void convertOldSelectedTrack(int &id);
};

#endif

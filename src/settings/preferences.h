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


#ifndef _SETTINGS_PREFERENCES_H_
#define _SETTINGS_PREFERENCES_H_

#include <QSize>
#include <QString>
#include <QStringList>

#include "config.h"
#include "settings/smplayersettings.h"
#include "settings/assstyles.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "filters.h"

#ifdef UPDATE_CHECKER
#include "updatecheckerdata.h"
#endif


namespace Settings {

typedef QList<QVariant> TAudioEqualizerList;

class TPreferences : public TSMPlayerSettings {

public:
	enum OSDLevel { None = 0, Seek = 1, SeekTimer = 2, SeekTimerTotal = 3 };
	enum OnTop { NeverOnTop = 0, AlwaysOnTop = 1, WhilePlayingOnTop = 2 };
	enum Resize { Never = 0, Always = 1, Afterload = 2 };
	enum Priority { Realtime = 0, High = 1, AboveNormal = 2, Normal = 3,
                    BelowNormal = 4, Idle = 5 };
	enum WheelFunction { DoNothing = 1, Seeking = 2, Volume = 4, Zoom = 8,
                         ChangeSpeed = 16 };
	enum OptionState { Detect = -1, Disabled = 0, Enabled = 1 };
	enum H264LoopFilter { LoopDisabled = 0, LoopEnabled = 1, LoopDisabledOnHD = 2 };
	enum AutoAddToPlaylistFilter { NoFiles = 0, VideoFiles = 1, AudioFiles = 2, MultimediaFiles = 3, ConsecutiveFiles = 4 };
	enum ToolbarActivation { Anywhere = 1, NearToolbar = 2 };

	Q_DECLARE_FLAGS(WheelFunctions, WheelFunction)

	TPreferences();
	virtual ~TPreferences();

	virtual void reset();

	void save();
	void load();

	double monitor_aspect_double();
	void setupScreenshotFolder();


    /* *******
       General
       ******* */

	int config_version;

	QString mplayer_bin;
	QString vo; // video output
	QString ao; // audio output

	bool use_screenshot;
#ifdef MPV_SUPPORT
	QString screenshot_template;
	QString screenshot_format;
#endif
	QString screenshot_directory;
#ifdef CAPTURE_STREAM
	QString capture_directory;
#endif

	// SMPlayer will remember all media settings for all videos.
	// This options allow to disable it:
	bool dont_remember_media_settings; 	// Will not remember anything
	bool dont_remember_time_pos;		// Will not remember time pos

	QString audio_lang; 		// Preferred audio language
	QString subtitle_lang;		// Preferred subtitle language

	// Video
	bool use_direct_rendering;
	bool use_double_buffer;
	bool use_soft_video_eq;
	bool use_slices;
	int autoq; 	//!< Postprocessing quality
	bool add_blackborders_on_fullscreen;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	#ifdef SCREENSAVER_OFF
	bool turn_screensaver_off;
	#endif
	#ifdef AVOID_SCREENSAVER
	bool avoid_screensaver;
	#endif
#else
	bool disable_screensaver;
#endif

#ifndef Q_OS_WIN
	struct VDPAU_settings {
		bool ffh264vdpau;
		bool ffmpeg12vdpau;
		bool ffwmv3vdpau;
		bool ffvc1vdpau;
		bool ffodivxvdpau;
		bool disable_video_filters;
	} vdpau;
#endif

	// Audio
	bool use_soft_vol;
	int softvol_max;
	OptionState use_scaletempo;
	bool use_hwac3; // -afm hwac3
	bool use_audio_equalizer;

	// Global volume options
	bool global_volume;
	int volume;
	bool mute;

	// Global equalizer
	bool global_audio_equalizer;
	TAudioEqualizerList audio_equalizer;

	bool autosync;
	int autosync_factor;

	// For the -mc option
	bool use_mc;
	double mc_value;

	// When playing a mp4 file, it will use a m4a file for audio if a there's a file with same name but extension m4a
	bool autoload_m4a;
	int min_step; //<! Step to increase of decrease the controls for color, contrast, brightness and so on

	// Misc
	OSDLevel osd_level;
	double osd_scale; // mpv
	double subfont_osd_scale; // mplayer

	QString file_settings_method; //!< Method to be used for saving file settings


    /* ***************
       Drives (CD/DVD)
       *************** */

	QString dvd_device;
	QString cdrom_device;
	QString bluray_device;

#ifdef Q_OS_WIN
	bool enable_audiocd_on_windows;
#endif

	int vcd_initial_title;

	bool use_dvdnav; //!< Opens DVDs using dvdnav: instead of dvd:


    /* ***********
       Performance
       *********** */

	int priority;
	bool frame_drop;
	bool hard_frame_drop;
	bool coreavc;
	H264LoopFilter h264_skip_loop_filter;
	int HD_height; //!< An HD is a video which height is equal or greater than this.

	int threads; //!< number of threads to use for decoding (-lavdopts threads <1-8>)
	QString hwdec; //!< hardware video decoding (mpv only)

	int cache_for_files;
	int cache_for_streams;
	int cache_for_dvds;
	int cache_for_vcds;
	int cache_for_audiocds;
	int cache_for_tv;

#ifdef YOUTUBE_SUPPORT
	bool enable_yt_support;
	int yt_quality;
	QString yt_user_agent;
	bool yt_use_https_main;
	bool yt_use_https_vi;
#endif
#ifdef MPV_SUPPORT
	bool enable_streaming_sites;
#endif


	/* *********
	   Subtitles
	   ********* */

	QString subcp; // -subcp
	bool use_enca;
	QString enca_lang;
	int subfuzziness;
	bool autoload_sub;

	bool use_ass_subtitles;
	bool enable_ass_styles;
	int ass_line_spacing;

	bool use_forced_subs_only;

	bool subtitles_on_screenshots;

	OptionState change_sub_scale_should_restart;

	//! If true, loading an external subtitle will be done
	//! by using the sub_load slave command. Otherwise
	//! mplayer will be restarted.
	bool fast_load_sub;

	// ASS styles
	TAssStyles ass_styles;
	bool force_ass_styles; // Use ass styles even for ass files
	QString user_forced_ass_style; //!< Specifies a style defined by the user to be used with -ass-force-style

	//! If false, options requiring freetype won't be used
	bool freetype_support;
#ifdef Q_OS_WIN
	bool use_windowsfontdir;
#endif


    /* ********
       Advanced
       ******** */

#if USE_ADAPTER
	int adapter; //Screen for overlay. If -1 it won't be used.
#endif

	unsigned int color_key;

	bool use_mplayer_window;

	QString monitor_aspect;

	bool use_idx; //!< Use -idx
	bool use_lavf_demuxer;

	// Let the user pass options to mplayer
	QString mplayer_additional_options;
	QString mplayer_additional_video_filters;
	QString mplayer_additional_audio_filters;

	// Logging
	bool log_enabled;
	bool log_verbose;
	QString log_filter;
	bool log_file;

	//! If true, playerlayer erases its background
	bool repaint_video_background; 

	//! If true it will autoload edl files with the same name of the file
    //! to play
	bool use_edl_files;

	//! If true it will pass to mplayer the -playlist option
	bool use_playlist_option;

	//! Preferred connection method: ipv4 or ipv6
	bool prefer_ipv4;

	//! If false, -brightness, -contrast and so on, won't be passed to
	//! mplayer. It seems that some graphic cards don't support those options.
	bool change_video_equalizer_on_startup;

	OptionState use_correct_pts; //!< Pass -correct-pts to mplayer

	QString actions_to_run; //!< List of actions to run every time a video loads.

	//! Show file tag in window title
	bool show_tag_in_window_title;

	int time_to_kill_mplayer;

#ifdef MPRIS2
	bool use_mpris2;
#endif


	/* *********
	   GUI stuff
	   ********* */

	// TODO: fullscreen is not a preference...
	bool fullscreen;
	bool start_in_fullscreen;
	OnTop stay_on_top;
	double size_factor;

	int resize_method; 	//!< Mainwindow resize method

	QString style; 	//!< SMPlayer look

	// Function of mouse buttons:
	QString mouse_left_click_function;
	QString mouse_right_click_function;
	QString mouse_double_click_function;
	QString mouse_middle_click_function;
	QString mouse_xbutton1_click_function;
	QString mouse_xbutton2_click_function;
	int wheel_function;

	WheelFunctions wheel_function_cycle;
	bool wheel_function_seeking_reverse;

	// Configurable seeking
	int seeking1; // By default 10s
	int seeking2; // By default 1m
	int seeking3; // By default 10m
	int seeking4; // For mouse wheel, by default 30s

	bool update_while_seeking;
	int time_slider_drag_delay;

	//! If true, seeking will be done using a
	//! percentage (with fractions) instead of time.
	bool relative_seeking;  
	bool precise_seeking; //! Enable precise_seeking (only available with mplayer2)

	bool reset_stop; //! Pressing the stop button resets the position

	//! If true, the left click in the video is delayed some ms
	//! to check if the user double clicked
	bool delay_left_click;

	QString language;
	QString iconset;

	//! Number of times to show the balloon remembering that the program
	//! is still running in the system tray.
	int balloon_count;

	//! If true, the position of the main window will be saved before
	//! entering in fullscreen and will restore when going back to
	//! window mode.
	bool restore_pos_after_fullscreen;

	bool save_window_size_on_exit;

	//! Close the main window when a file or playlist finish
	bool close_on_finish;

#ifdef AUTO_SHUTDOWN_PC
	bool auto_shutdown_pc;
#endif

	QString default_font;

	//!< Pause the current file when the main window is not visible
	bool pause_when_hidden; 

	QString gui; //!< The name of the GUI to use

	QSize default_size; // Default size of the main window

	bool hide_video_window_on_audio_files;

	bool report_mplayer_crashes;

	bool auto_add_to_playlist; //!< Add files to open to playlist
	AutoAddToPlaylistFilter media_to_add_to_playlist;


    /* ********
       TV (dvb)
       ******** */

	bool check_channels_conf_on_startup;
	int initial_tv_deinterlace;
	QString last_dvb_channel;
	QString last_tv_channel;


    /* ********
       Network
       ******** */

	// Proxy
	bool use_proxy;
	int proxy_type;
	QString proxy_host;
	int proxy_port;
	QString proxy_username;
	QString proxy_password;


    /* ***********
       Directories
       *********** */

	QString latest_dir; //!< Directory of the latest file loaded
	QString last_dvd_directory;
	bool save_dirs; // Save or not the latest dirs

    /* **************
       Initial values
       ************** */

	double initial_sub_scale;
	double initial_sub_scale_mpv;
	double initial_sub_scale_ass;
	int initial_volume;
	int initial_contrast;
	int initial_brightness;
	int initial_hue;
	int initial_saturation;
	int initial_gamma;

	TAudioEqualizerList initial_audio_equalizer;

	//! Default value for zoom (1.0 = no zoom)
	double initial_zoom_factor;

	//! Default value for position of subtitles on screen
	//! 100 = 100% at the bottom
	int initial_sub_pos;

	bool initial_postprocessing; //!< global postprocessing filter
	bool initial_volnorm;

	int initial_deinterlace;

	int initial_audio_channels;
	int initial_stereo_mode;

	int initial_audio_track;
	int initial_subtitle_track;


    /* *********
       Instances
       ********* */
#ifdef SINGLE_INSTANCE
	bool use_single_instance;
#endif


    /* ****************
       Floating control
       **************** */

	ToolbarActivation floating_activation_area;
	int floating_hide_delay;


    /* *******
       History
       ******* */

	TRecents history_recents;
	TURLHistory history_urls;


    /* *******
       Filters
       ******* */
	TFilters filters;


    /* *********
       SMPlayer info
       ********* */

#ifdef CHECK_UPGRADED
	QString smplayer_stable_version;
	bool check_if_upgraded;
#endif


    /* *********
       Update
       ********* */

#ifdef UPDATE_CHECKER
	UpdateCheckerData update_checker_data;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Settings::TPreferences::WheelFunctions)

extern TPreferences* pref;

} // namespace Settings

#endif // _SETTINGS_PREFERENCES_H_

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


#ifndef SETTINGS_PREFERENCES_H
#define SETTINGS_PREFERENCES_H

#include <QSize>
#include <QString>

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
	enum TPlayerID {
		MPLAYER = 0, MPV = 1
	};
	enum TOSDLevel {
		None = 0, Seek = 1, SeekTimer = 2, SeekTimerTotal = 3
	};
	enum TOnTop {
		NeverOnTop = 0, AlwaysOnTop = 1, WhilePlayingOnTop = 2
	};
	enum TResize {
		Never = 0, Always = 1, Afterload = 2
	};
	enum TPriority {
		Realtime = 0, High = 1, AboveNormal = 2, Normal = 3, BelowNormal = 4,
		Idle = 5
	};
	enum TWheelFunction {
		DoNothing = 1, Seeking = 2, Volume = 4, Zoom = 8, ChangeSpeed = 16
	};
	enum TOptionState {
		Detect = -1, Disabled = 0, Enabled = 1
	};
	enum TAutoAddToPlaylistFilter {
		NoFiles = 0, VideoFiles = 1, AudioFiles = 2, MultimediaFiles = 3,
		ConsecutiveFiles = 4
	};
	enum TToolbarActivation {
		Anywhere = 1, NearToolbar = 2
	};

	Q_DECLARE_FLAGS(TWheelFunctions, TWheelFunction)

	TPreferences();
	virtual ~TPreferences();

	void reset();

	void load();
	void save();


	// General tab
	// Version config file
	int config_version;

	// Media player
	QString player_bin;
	TPlayerID player_id;
	QString player_abs_path;
	bool isMPlayer() const { return player_id == MPLAYER; }
	bool isMPV() const { return player_id == MPV; }
	QString playerName() const;
	QString playerAbsolutePath() const;
	void setPlayerBin();

	// Media settings per file
	bool remember_media_settings;
	bool remember_time_pos;
	QString file_settings_method; //!< Method to be used for saving file settings

	// Screenshot
	bool use_screenshot;
#ifdef MPV_SUPPORT
	QString screenshot_template;
	QString screenshot_format;
#endif
	QString screenshot_directory;
#ifdef CAPTURE_STREAM
	QString capture_directory;
#endif
	void setupScreenshotFolder();


	// Video tab
	QString vo; // video output
	QString hwdec; //!< hardware video decoding (mpv only)
	bool frame_drop;
	bool hard_frame_drop;
	bool use_soft_video_eq;
	int postprocessing_quality; 	//!< Postprocessing quality

	bool add_blackborders_on_fullscreen;

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


	// Audio tab
	QString ao;
	bool use_soft_vol;
	int softvol_max;
	TOptionState use_scaletempo;
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


	// Preferred tab
	QString audio_lang; 		// Preferred audio language
	QString subtitle_lang;		// Preferred subtitle language

	// OSD
	TOSDLevel osd_level;
	double osd_scale; // mpv
	double subfont_osd_scale; // mplayer


	// Drives
	QString dvd_device;
	QString cdrom_device;
	QString bluray_device;

#ifdef Q_OS_WIN
	bool enable_audiocd_on_windows;
#endif

	int vcd_initial_title;

	bool use_dvdnav; //!< Opens DVDs using dvdnav: instead of dvd:


	// Performance tab
	int priority;

	int cache_for_files;
	int cache_for_streams;
	int cache_for_dvds;
	int cache_for_vcds;
	int cache_for_audiocds;
	int cache_for_tv;


	// Subtitles
	QString sub_code_page; // Code page subtitles
	bool use_enca; // Pass subtitle encoding to player
	QString enca_lang; // Subtitle language
	int subfuzziness;
	bool autoload_sub;

	bool use_ass_subtitles;
	bool enable_ass_styles;
	int ass_line_spacing;

	bool use_forced_subs_only;

	bool subtitles_on_screenshots;

	TOptionState change_sub_scale_should_restart;

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


	// Advanced tab
	QString monitor_aspect;
	double monitor_aspect_double();

#if USE_ADAPTER
	int adapter; //Screen for overlay. If -1 it won't be used.
#endif

	unsigned int color_key;

	bool use_idx; //!< Use -idx
	bool use_lavf_demuxer;

	// Let the user pass options to mplayer
	QString mplayer_additional_options;
	QString mplayer_additional_video_filters;
	QString mplayer_additional_audio_filters;


	// Logging tab
	bool log_debug_enabled;
	bool log_verbose;
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

	TOptionState use_correct_pts; //!< Pass -correct-pts to mplayer

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
	TOnTop stay_on_top;
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

	TWheelFunctions wheel_function_cycle;
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

	QString default_font;

	//!< Pause the current file when the main window is not visible
	bool pause_when_hidden; 

	QSize default_size; // Default size of the main window

	bool hide_video_window_on_audio_files;

	bool report_mplayer_crashes;

	bool auto_add_to_playlist; //!< Add files to open to playlist
	TAutoAddToPlaylistFilter media_to_add_to_playlist;


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

	TToolbarActivation floating_activation_area;
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
       Update
       ********* */

#ifdef UPDATE_CHECKER
	UpdateCheckerData update_checker_data;
#endif

private:
	void setPlayerBin0();
	void setPlayerID();
	void setAbsolutePath();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Settings::TPreferences::TWheelFunctions)

extern TPreferences* pref;

} // namespace Settings

#endif // SETTINGS_PREFERENCES_H

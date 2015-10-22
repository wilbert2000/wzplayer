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

#include "settings/preferences.h"

#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QRegExp>
#include <QDir>
#include <QLocale>
#include <QNetworkProxy>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#if QT_VERSION >= 0x040400
#include <QDesktopServices>
#endif

#include "paths.h"
#include "settings/mediasettings.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "settings/filters.h"
#include "gui/autohidetoolbar.h"
#include "helper.h"

#ifdef YOUTUBE_SUPPORT
#include "retrieveyoutubeurl.h"
#endif

#define CURRENT_CONFIG_VERSION 5


namespace Settings {

TPreferences* pref = 0;

TPreferences::TPreferences() :
	TSMPlayerSettings(Paths::configPath() + "/smplayer.ini") {

	reset();
	load();
	pref = this;
}

TPreferences::~TPreferences() {

	pref = 0;
}

void TPreferences::reset() {
    /* *******
       General
       ******* */

	config_version = CURRENT_CONFIG_VERSION;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	mplayer_bin= "mplayer/mplayer.exe";
#else
	mplayer_bin = "mplayer";
#endif

	vo = ""; 
	ao = "";

	use_screenshot = true;
#ifdef MPV_SUPPORT
	screenshot_template = "cap_%F_%p_%02n";
#endif
	screenshot_directory="";
#ifdef PORTABLE_APP
	screenshot_directory= "./screenshots";
#else
	#if QT_VERSION < 0x040400
	QString default_screenshot_path = Paths::configPath() + "/screenshots";
	if (QFile::exists(default_screenshot_path)) {
		screenshot_directory = default_screenshot_path;
	}
	#endif
#endif

	dont_remember_media_settings = false;
	dont_remember_time_pos = false;

	audio_lang = "";
	subtitle_lang = "";

	use_direct_rendering = false;
	use_double_buffer = true;

	use_soft_video_eq = false;
	use_slices = false;
	autoq = 6;
	add_blackborders_on_fullscreen = false;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	#ifdef SCREENSAVER_OFF
	turn_screensaver_off = false;
	#endif
	#ifdef AVOID_SCREENSAVER
	avoid_screensaver = true;
	#endif
#else
	disable_screensaver = true;
#endif

#ifndef Q_OS_WIN
	vdpau.ffh264vdpau = true;
	vdpau.ffmpeg12vdpau = true;
	vdpau.ffwmv3vdpau = true;
	vdpau.ffvc1vdpau = true;
	vdpau.ffodivxvdpau = false;
	vdpau.disable_video_filters = true;
#endif

#ifdef Q_OS_WIN
	use_soft_vol = false;
#else
	use_soft_vol = true;
#endif
	softvol_max = 110; // 110 = default value in mplayer
	use_scaletempo = Detect;
	use_hwac3 = false;
	use_audio_equalizer = true;

	global_volume = true;
	volume = 50;
	mute = false;

	global_audio_equalizer = true;
	audio_equalizer << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0; // FIXME: use initial_audio_equalizer (but it's set later)

	autosync = false;
	autosync_factor = 100;

	use_mc = false;
	mc_value = 0;

	autoload_m4a = true;
	min_step = 4;

	osd_level = None;
	osd_scale = 1;
	subfont_osd_scale = 3;

	file_settings_method = "hash"; // Possible values: normal & hash


    /* ***************
       Drives (CD/DVD)
       *************** */

	dvd_device = "";
	cdrom_device = "";
	bluray_device = "";

#ifndef Q_OS_WIN
	// Try to set default values
	if (QFile::exists("/dev/dvd")) dvd_device = "/dev/dvd";
	if (QFile::exists("/dev/cdrom")) cdrom_device = "/dev/cdrom";
#endif

#ifdef Q_OS_WIN
	enable_audiocd_on_windows = false;
#endif

	vcd_initial_title = 2; // Most VCD's start at title #2

	use_dvdnav = true;

    /* ***********
       Performance
       *********** */

	priority = AboveNormal; // Option only for windows
	frame_drop = false;
	hard_frame_drop = false;
	coreavc = false;
	h264_skip_loop_filter = LoopEnabled;
	HD_height = 720;

	threads = 1;
	hwdec = "no";

	cache_for_files = 2048;
	cache_for_streams = 2048;
	cache_for_dvds = 0; // not recommended to use cache for dvds
	cache_for_vcds = 1024;
	cache_for_audiocds = 1024;
	cache_for_tv = 3000;

#ifdef YOUTUBE_SUPPORT
	enable_yt_support = true;
	yt_quality = RetrieveYoutubeUrl::MP4_720p;
	//yt_user_agent = "Mozilla/5.0 (X11; Linux x86_64; rv:5.0.1) Gecko/20100101 Firefox/5.0.1";
	yt_user_agent = "";
	yt_use_https_main = false;
	yt_use_https_vi = false;
#endif
#ifdef MPV_SUPPORT
	enable_streaming_sites = false;
#endif


    /* *********
       Subtitles
       ********* */

	subcp = "ISO-8859-1";
	use_enca = false;
	enca_lang = QString(QLocale::system().name()).section("_",0,0);
	subfuzziness = 1;
	autoload_sub = false;

	use_ass_subtitles = true;
	enable_ass_styles = true;
	ass_line_spacing = 0;

	use_forced_subs_only = false;

	subtitles_on_screenshots = false;

	change_sub_scale_should_restart = Detect;

	fast_load_sub = true;

	// ASS styles
	// Nothing to do, default values are given in
	// TAssStyles constructor

	force_ass_styles = false;
	user_forced_ass_style.clear();

	freetype_support = true;
#ifdef Q_OS_WIN
	use_windowsfontdir = false;
#endif


    /* ********
       Advanced
       ******** */

#if USE_ADAPTER
	adapter = -1;
#endif

	color_key = 0x020202;

	use_mplayer_window = false;

	monitor_aspect=""; // Autodetect

	use_idx = false;
	use_lavf_demuxer = false;

	mplayer_additional_options="";
	#ifdef PORTABLE_APP
	mplayer_additional_options="-nofontconfig";
	#endif
    mplayer_additional_video_filters="";
    mplayer_additional_audio_filters="";

	log_enabled = true;
	log_verbose = false;
	log_filter = ".*";
	log_file = false;

	// "Repaint video background" in the preferences dialog
#ifdef Q_OS_WIN
	repaint_video_background = true;
#else
	repaint_video_background = false;
#endif

	use_edl_files = true;

	prefer_ipv4 = true;

	use_short_pathnames = false;

	change_video_equalizer_on_startup = true;

	use_correct_pts = Detect;

	actions_to_run = "";

	show_tag_in_window_title = true;

	time_to_kill_mplayer = 5000;

#ifdef MPRIS2
	use_mpris2 = true;
#endif


    /* *********
       GUI stuff
       ********* */

	fullscreen = false;
	start_in_fullscreen = false;
	stay_on_top = NeverOnTop;
	size_factor = 1.0; // 100%

	resize_method = Never;

	style = "";

	mouse_left_click_function = "dvdnav_mouse";
	mouse_right_click_function = "show_context_menu";
	mouse_double_click_function = "fullscreen";
	mouse_middle_click_function = "mute";
	mouse_xbutton1_click_function = "";
	mouse_xbutton2_click_function = "";
	wheel_function = Seeking;
	wheel_function_cycle = Seeking | Volume | Zoom | ChangeSpeed;
	wheel_function_seeking_reverse = false;

	seeking1 = 10;
	seeking2 = 60;
	seeking3 = 10*60;
	seeking4 = 30;

	update_while_seeking = false;
	time_slider_drag_delay = 200;
	relative_seeking = false;
	precise_seeking = true;

	reset_stop = false;
	delay_left_click = false;

	language = "";

	balloon_count = 5;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	restore_pos_after_fullscreen = true;
#else
	restore_pos_after_fullscreen = false;
#endif

	save_window_size_on_exit = true;

	close_on_finish = false;

#ifdef AUTO_SHUTDOWN_PC
	auto_shutdown_pc = false;
#endif

	default_font = "";

	pause_when_hidden = false;

	gui = "DefaultGUI";
	iconset = "H2O";

	// Used to be default_size = QSize(683, 509);
	// Now 360p 16:9 is 640 x 360 (360 + 99 = 459)
	default_size = QSize(640, 459);

	hide_video_window_on_audio_files = true;

	report_mplayer_crashes = true;

#if REPORT_OLD_MPLAYER
	reported_mplayer_is_old = false;
#endif

	auto_add_to_playlist = true;
	media_to_add_to_playlist = NoFiles;

#if LOGO_ANIMATION
	animated_logo = true;
#endif


    /* ********
       TV (dvb)
       ******** */

	check_channels_conf_on_startup = true;
	initial_tv_deinterlace = TMediaSettings::Yadif_1;
	last_dvb_channel = "";
	last_tv_channel = "";


    /* ********
       Network
       ******** */

	// Proxy
	use_proxy = false;
	proxy_type = QNetworkProxy::HttpProxy;
	proxy_host = "";
	proxy_port = 0;
	proxy_username = "";
	proxy_password = "";


    /* ***********
       Directories
       *********** */

	latest_dir = QDir::homePath();
	last_dvd_directory="";
	save_dirs = true;

    /* **************
       Initial values
       ************** */

	initial_sub_scale = 5;
	initial_sub_scale_mpv = 1;
	initial_sub_scale_ass = 1;
	initial_volume = 40;
	initial_contrast = 0;
	initial_brightness = 0;
	initial_hue = 0;
	initial_saturation = 0;
	initial_gamma = 0;

	initial_audio_equalizer << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;

	initial_zoom_factor = 1.0;
	initial_sub_pos = 100; // 100%

	initial_postprocessing = false;
	initial_volnorm = false;

	initial_deinterlace = TMediaSettings::NoDeinterlace;

	initial_audio_channels = TMediaSettings::ChDefault;
	initial_stereo_mode = TMediaSettings::Stereo;

	initial_audio_track = 1;
	initial_subtitle_track = 1;


    /* ************
       MPlayer info
       ************ */

	mplayer_detected_version = -1; //None version parsed yet
	mplayer_user_supplied_version = -1;
	mplayer_is_mplayer2 = false;
	mplayer2_detected_version = QString::null;


    /* *********
       Instances
       ********* */
#ifdef SINGLE_INSTANCE
	use_single_instance = true;
#endif


    /* ****************
       Floating control
       **************** */

	floating_control_width = 70; //70 %
	floating_activation_area = Gui::TAutohideToolbar::Anywhere;
	floating_hide_delay = 3000;


    /* *******
       History
       ******* */

	history_recents.clear();
	history_urls.clear();


    /* *******
       Filters
       ******* */

	filters.init();


    /* *********
       SMPlayer info
       ********* */

#ifdef CHECK_UPGRADED
	smplayer_stable_version = "";
	check_if_upgraded = true;
#endif
}

void TPreferences::save() {
	qDebug("Settings::TPreferences::save");

    /* *******
       General
       ******* */

	beginGroup("General");

	setValue("config_version", config_version);

	setValue("mplayer_bin", mplayer_bin);
	setValue("driver/vo", vo);
	setValue("driver/audio_output", ao);

	setValue("use_screenshot", use_screenshot);
	#ifdef MPV_SUPPORT
	setValue("screenshot_template", screenshot_template);
	#endif
	#if QT_VERSION >= 0x040400
	setValue("screenshot_folder", screenshot_directory);
	#else
	setValue("screenshot_directory", screenshot_directory);
	#endif

	setValue("dont_remember_media_settings", dont_remember_media_settings);
	setValue("dont_remember_time_pos", dont_remember_time_pos);

	setValue("audio_lang", audio_lang);
	setValue("subtitle_lang", subtitle_lang);

	setValue("use_direct_rendering", use_direct_rendering);
	setValue("use_double_buffer", use_double_buffer);
	setValue("use_soft_video_eq", use_soft_video_eq);
	setValue("use_slices", use_slices);
	setValue("autoq", autoq);
	setValue("add_blackborders_on_fullscreen", add_blackborders_on_fullscreen);

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	#ifdef SCREENSAVER_OFF
	setValue("turn_screensaver_off", turn_screensaver_off);
	#endif
	#ifdef AVOID_SCREENSAVER
	setValue("avoid_screensaver", avoid_screensaver);
	#endif
#else
	setValue("disable_screensaver", disable_screensaver);
#endif

#ifndef Q_OS_WIN
	setValue("vdpau_ffh264vdpau", vdpau.ffh264vdpau);
	setValue("vdpau_ffmpeg12vdpau", vdpau.ffmpeg12vdpau);
	setValue("vdpau_ffwmv3vdpau", vdpau.ffwmv3vdpau);
	setValue("vdpau_ffvc1vdpau", vdpau.ffvc1vdpau);
	setValue("vdpau_ffodivxvdpau", vdpau.ffodivxvdpau);
	setValue("vdpau_disable_video_filters", vdpau.disable_video_filters);
#endif

	setValue("use_soft_vol", use_soft_vol);
	setValue("softvol_max", softvol_max);
	setValue("use_scaletempo", use_scaletempo);
	setValue("use_hwac3", use_hwac3);
	setValue("use_audio_equalizer", use_audio_equalizer);

	setValue("global_volume", global_volume);
	setValue("volume", volume);
	setValue("mute", mute);

	setValue("global_audio_equalizer", global_audio_equalizer);
	setValue("audio_equalizer", audio_equalizer);

	setValue("autosync", autosync);
	setValue("autosync_factor", autosync_factor);

	setValue("use_mc", use_mc);
	setValue("mc_value", mc_value);

	setValue("autoload_m4a", autoload_m4a);
	setValue("min_step", min_step);

	setValue("osd_level", osd_level);
	setValue("osd_scale", osd_scale);
	setValue("subfont_osd_scale", subfont_osd_scale);

	setValue("file_settings_method", file_settings_method);

	endGroup(); // General


    /* ***************
       Drives (CD/DVD)
       *************** */

	beginGroup("drives");

	setValue("dvd_device", dvd_device);
	setValue("cdrom_device", cdrom_device);
	setValue("bluray_device", bluray_device);

#ifdef Q_OS_WIN
	setValue("enable_audiocd_on_windows", enable_audiocd_on_windows);
#endif

	setValue("vcd_initial_title", vcd_initial_title);

	setValue("use_dvdnav", use_dvdnav);

	endGroup(); // drives


    /* ***********
       Performance
       *********** */

	beginGroup("performance");

	setValue("priority", priority);
	setValue("frame_drop", frame_drop);
	setValue("hard_frame_drop", hard_frame_drop);
	setValue("coreavc", coreavc);
	setValue("h264_skip_loop_filter", h264_skip_loop_filter);
	setValue("HD_height", HD_height);

	setValue("threads", threads);
	setValue("hwdec", hwdec);

	setValue("cache_for_files", cache_for_files);
	setValue("cache_for_streams", cache_for_streams);
	setValue("cache_for_dvds", cache_for_dvds);
	setValue("cache_for_vcds", cache_for_vcds);
	setValue("cache_for_audiocds", cache_for_audiocds);
	setValue("cache_for_tv", cache_for_tv);

	endGroup(); // performance

#ifdef YOUTUBE_SUPPORT
	beginGroup("youtube");
	setValue("enable_yt_support", enable_yt_support);
	setValue("quality", yt_quality);
	setValue("user_agent", yt_user_agent);
	setValue("yt_use_https_main", yt_use_https_main);
	setValue("yt_use_https_vi", yt_use_https_vi);
	endGroup();
#endif

#ifdef MPV_SUPPORT
	beginGroup("streaming");
	setValue("enable_streaming_sites", enable_streaming_sites);
	endGroup();
#endif



    /* *********
       Subtitles
       ********* */

	beginGroup("subtitles");

	setValue("subcp", subcp);
	setValue("use_enca", use_enca);
	setValue("enca_lang", enca_lang);
	setValue("subfuzziness", subfuzziness);
	setValue("autoload_sub", autoload_sub);

	setValue("use_ass_subtitles", use_ass_subtitles);
	setValue("enable_ass_styles", enable_ass_styles);
	setValue("ass_line_spacing", ass_line_spacing);
	setValue("use_forced_subs_only", use_forced_subs_only);

	setValue("subtitles_on_screenshots", subtitles_on_screenshots);

	setValue("change_sub_scale_should_restart", change_sub_scale_should_restart);

	setValue("fast_load_sub", fast_load_sub);

	// ASS styles
	ass_styles.save(this);
	setValue("force_ass_styles", force_ass_styles);
	setValue("user_forced_ass_style", user_forced_ass_style);

	setValue("freetype_support", freetype_support);
#ifdef Q_OS_WIN
	setValue("use_windowsfontdir", use_windowsfontdir);
#endif

	endGroup(); // subtitles


    /* ********
       Advanced
       ******** */

	beginGroup("advanced");

#if USE_ADAPTER
	setValue("adapter", adapter);
#endif

	setValue("color_key", QString::number(color_key, 16));

	setValue("use_mplayer_window", use_mplayer_window);

	setValue("monitor_aspect", monitor_aspect);

	setValue("use_idx", use_idx);
	setValue("use_lavf_demuxer", use_lavf_demuxer);

	setValue("mplayer_additional_options", mplayer_additional_options);
	setValue("mplayer_additional_video_filters", mplayer_additional_video_filters);
	setValue("mplayer_additional_audio_filters", mplayer_additional_audio_filters);

	setValue("log_enabled", log_enabled);
	setValue("log_verbose", log_verbose);
	setValue("log_file", log_file);
	setValue("log_filter", log_filter);

	setValue("repaint_video_background", repaint_video_background);

	setValue("use_edl_files", use_edl_files);

	setValue("prefer_ipv4", prefer_ipv4);

	setValue("use_short_pathnames", use_short_pathnames);

	setValue("change_video_equalizer_on_startup", change_video_equalizer_on_startup);

	setValue("correct_pts", use_correct_pts);

	setValue("actions_to_run", actions_to_run);

	setValue("show_tag_in_window_title", show_tag_in_window_title);

	setValue("time_to_kill_mplayer", time_to_kill_mplayer);

#ifdef MPRIS2
	setValue("use_mpris2", use_mpris2);
#endif

	endGroup(); // advanced


    /* *********
       GUI stuff
       ********* */

	beginGroup("gui");

	setValue("start_in_fullscreen", start_in_fullscreen);

	setValue("stay_on_top", (int) stay_on_top);
	setValue("size_factor", size_factor);
	setValue("resize_method", resize_method);

	setValue("style", style);

	setValue("mouse_left_click_function", mouse_left_click_function);
	setValue("mouse_right_click_function", mouse_right_click_function);
	setValue("mouse_double_click_function", mouse_double_click_function);
	setValue("mouse_middle_click_function", mouse_middle_click_function);
	setValue("mouse_xbutton1_click_function", mouse_xbutton1_click_function);
	setValue("mouse_xbutton2_click_function", mouse_xbutton2_click_function);
	setValue("mouse_wheel_function", wheel_function);
	setValue("wheel_function_cycle", (int) wheel_function_cycle);
	setValue("wheel_function_seeking_reverse", wheel_function_seeking_reverse);

	setValue("seeking1", seeking1);
	setValue("seeking2", seeking2);
	setValue("seeking3", seeking3);
	setValue("seeking4", seeking4);

	setValue("update_while_seeking", update_while_seeking);
	setValue("time_slider_drag_delay", time_slider_drag_delay);
	setValue("relative_seeking", relative_seeking);
	setValue("precise_seeking", precise_seeking);

	setValue("reset_stop", reset_stop);
	setValue("delay_left_click", delay_left_click);

	setValue("language", language);
	setValue("iconset", iconset);

	setValue("balloon_count", balloon_count);

	setValue("restore_pos_after_fullscreen", restore_pos_after_fullscreen);
	setValue("save_window_size_on_exit", save_window_size_on_exit);

	setValue("close_on_finish", close_on_finish);

#ifdef AUTO_SHUTDOWN_PC
	setValue("auto_shutdown_pc", auto_shutdown_pc);
#endif

	setValue("default_font", default_font);

	setValue("pause_when_hidden", pause_when_hidden);

	setValue("gui", gui);

	setValue("default_size", default_size);

	setValue("hide_video_window_on_audio_files", hide_video_window_on_audio_files);

	setValue("report_mplayer_crashes", report_mplayer_crashes);

#if REPORT_OLD_MPLAYER
	setValue("reported_mplayer_is_old", reported_mplayer_is_old);
#endif

	setValue("auto_add_to_playlist", auto_add_to_playlist);
	setValue("media_to_add_to_playlist", media_to_add_to_playlist);

#if LOGO_ANIMATION
	setValue("animated_logo", animated_logo);
#endif

	endGroup(); // gui


    /* ********
       TV (dvb)
       ******** */

	beginGroup("tv");
	setValue("check_channels_conf_on_startup", check_channels_conf_on_startup);
	setValue("initial_tv_deinterlace", initial_tv_deinterlace);
	setValue("last_dvb_channel", last_dvb_channel);
	setValue("last_tv_channel", last_tv_channel);
	endGroup(); // tv


    /* ********
       Network
       ******** */

	beginGroup("proxy");
	setValue("use_proxy", use_proxy);
	setValue("type", proxy_type);
	setValue("host", proxy_host);
	setValue("port", proxy_port);
	setValue("username", proxy_username);
	setValue("password", proxy_password);
	endGroup(); // proxy


    /* ***********
       Directories
       *********** */

	beginGroup("directories");
	if (save_dirs) {
		setValue("latest_dir", latest_dir);
		setValue("last_dvd_directory", last_dvd_directory);
	} else {
		setValue("latest_dir", "");
		setValue("last_dvd_directory", "");
	}
	setValue("save_dirs", save_dirs);
	endGroup(); // directories


    /* **************
       Initial values
       ************** */

	beginGroup("defaults");

	setValue("initial_sub_scale", initial_sub_scale);
	setValue("initial_sub_scale_mpv", initial_sub_scale_mpv);
	setValue("initial_sub_scale_ass", initial_sub_scale_ass);
	setValue("initial_volume", initial_volume);
	setValue("initial_contrast", initial_contrast);
	setValue("initial_brightness", initial_brightness);
	setValue("initial_hue", initial_hue);
	setValue("initial_saturation", initial_saturation);
	setValue("initial_gamma", initial_gamma);

	setValue("initial_audio_equalizer", initial_audio_equalizer);

	setValue("initial_zoom_factor", initial_zoom_factor);
	setValue("initial_sub_pos", initial_sub_pos);

	setValue("initial_volnorm", initial_volnorm);
	setValue("initial_postprocessing", initial_postprocessing);

	setValue("initial_deinterlace", initial_deinterlace);

	setValue("initial_audio_channels", initial_audio_channels);
	setValue("initial_stereo_mode", initial_stereo_mode);

	setValue("initial_audio_track", initial_audio_track);
	setValue("initial_subtitle_track", initial_subtitle_track);

	endGroup(); // defaults


    /* ************
       MPlayer info
       ************ */

	beginGroup("mplayer_info");
	setValue("mplayer_detected_version", mplayer_detected_version);
	setValue("mplayer_user_supplied_version", mplayer_user_supplied_version);
	setValue("is_mplayer2", mplayer_is_mplayer2);
	setValue("mplayer2_detected_version", mplayer2_detected_version);
	endGroup(); // mplayer_info


    /* *********
       Instances
       ********* */
#ifdef SINGLE_INSTANCE
	beginGroup("instances");
	setValue("single_instance_enabled", use_single_instance);
	endGroup(); // instances
#endif


    /* ****************
       Floating control
       **************** */

	beginGroup("floating_control");
	setValue("width", floating_control_width);
	setValue("activation_area", floating_activation_area);
	setValue("hide_delay", floating_hide_delay);
	endGroup(); // floating_control


    /* *******
       History
       ******* */

	beginGroup("history");
	setValue("recents", history_recents);
	setValue("recents/max_items", history_recents.maxItems());
	setValue("urls", history_urls);
	setValue("urls/max_items", history_urls.maxItems());
	endGroup(); // history


    /* *******
       Filters
       ******* */

	filters.save(this);


    /* *********
       SMPlayer info
       ********* */

	beginGroup("smplayer");
#ifdef CHECK_UPGRADED
	setValue("stable_version", smplayer_stable_version);
	setValue("check_if_upgraded", check_if_upgraded);
#endif
	endGroup();


    /* *********
       Update
       ********* */

#ifdef UPDATE_CHECKER
	update_checker_data.save(this);
#endif

	sync();
}

void TPreferences::load() {
	qDebug("Settings::TPreferences::load");

    /* *******
       General
       ******* */

	beginGroup("General");

	config_version = value("config_version", 0).toInt();

	mplayer_bin = value("mplayer_bin", mplayer_bin).toString();
	vo = value("driver/vo", vo).toString();
	ao = value("driver/audio_output", ao).toString();

	use_screenshot = value("use_screenshot", use_screenshot).toBool();
	#ifdef MPV_SUPPORT
	screenshot_template = value("screenshot_template", screenshot_template).toString();
	#endif
	#if QT_VERSION >= 0x040400
	screenshot_directory = value("screenshot_folder", screenshot_directory).toString();
	setupScreenshotFolder();
	#else
	screenshot_directory = value("screenshot_directory", screenshot_directory).toString();
	#endif

	dont_remember_media_settings = value("dont_remember_media_settings", dont_remember_media_settings).toBool();
	dont_remember_time_pos = value("dont_remember_time_pos", dont_remember_time_pos).toBool();

	audio_lang = value("audio_lang", audio_lang).toString();
	subtitle_lang = value("subtitle_lang", subtitle_lang).toString();

	use_direct_rendering = value("use_direct_rendering", use_direct_rendering).toBool();
	use_double_buffer = value("use_double_buffer", use_double_buffer).toBool();
	
	use_soft_video_eq = value("use_soft_video_eq", use_soft_video_eq).toBool();
	use_slices = value("use_slices", use_slices).toBool();
	autoq = value("autoq", autoq).toInt();
	add_blackborders_on_fullscreen = value("add_blackborders_on_fullscreen", add_blackborders_on_fullscreen).toBool();

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	#ifdef SCREENSAVER_OFF
	turn_screensaver_off = value("turn_screensaver_off", turn_screensaver_off).toBool();
	#endif
	#ifdef AVOID_SCREENSAVER
	avoid_screensaver = value("avoid_screensaver", avoid_screensaver).toBool();
	#endif
#else
	disable_screensaver = value("disable_screensaver", disable_screensaver).toBool();
#endif

#ifndef Q_OS_WIN
	vdpau.ffh264vdpau = value("vdpau_ffh264vdpau", vdpau.ffh264vdpau).toBool();
	vdpau.ffmpeg12vdpau = value("vdpau_ffmpeg12vdpau", vdpau.ffmpeg12vdpau).toBool();
	vdpau.ffwmv3vdpau = value("vdpau_ffwmv3vdpau", vdpau.ffwmv3vdpau).toBool();
	vdpau.ffvc1vdpau = value("vdpau_ffvc1vdpau", vdpau.ffvc1vdpau).toBool();
	vdpau.ffodivxvdpau = value("vdpau_ffodivxvdpau", vdpau.ffodivxvdpau).toBool();
	vdpau.disable_video_filters = value("vdpau_disable_video_filters", vdpau.disable_video_filters).toBool();
#endif

	use_soft_vol = value("use_soft_vol", use_soft_vol).toBool();
	softvol_max = value("softvol_max", softvol_max).toInt();
	use_scaletempo = (OptionState) value("use_scaletempo", use_scaletempo).toInt();
	use_hwac3 = value("use_hwac3", use_hwac3).toBool();
	use_audio_equalizer = value("use_audio_equalizer", use_audio_equalizer).toBool();

	global_volume = value("global_volume", global_volume).toBool();
	volume = value("volume", volume).toInt();
	mute = value("mute", mute).toBool();

	global_audio_equalizer = value("global_audio_equalizer", global_audio_equalizer).toBool();
	audio_equalizer = value("audio_equalizer", audio_equalizer).toList();

	autosync = value("autosync", autosync).toBool();
	autosync_factor = value("autosync_factor", autosync_factor).toInt();

	use_mc = value("use_mc", use_mc).toBool();
	mc_value = value("mc_value", mc_value).toDouble();

	autoload_m4a = value("autoload_m4a", autoload_m4a).toBool();
	min_step = value("min_step", min_step).toInt();

	osd_level = (OSDLevel) value("osd_level", (int) osd_level).toInt();
	osd_scale = value("osd_scale", osd_scale).toDouble();
	subfont_osd_scale = value("subfont_osd_scale", subfont_osd_scale).toDouble();

	file_settings_method = value("file_settings_method", file_settings_method).toString();

	endGroup(); // General


    /* ***************
       Drives (CD/DVD)
       *************** */

	beginGroup("drives");

	dvd_device = value("dvd_device", dvd_device).toString();
	cdrom_device = value("cdrom_device", cdrom_device).toString();
	bluray_device = value("bluray_device", bluray_device).toString();

#ifdef Q_OS_WIN
	enable_audiocd_on_windows = value("enable_audiocd_on_windows", enable_audiocd_on_windows).toBool();
#endif

	vcd_initial_title = value("vcd_initial_title", vcd_initial_title).toInt();

	use_dvdnav = value("use_dvdnav", use_dvdnav).toBool();

	endGroup(); // drives


    /* ***********
       Performance
       *********** */

	beginGroup("performance");

	priority = value("priority", priority).toInt();
	frame_drop = value("frame_drop", frame_drop).toBool();
	hard_frame_drop = value("hard_frame_drop", hard_frame_drop).toBool();
	coreavc = value("coreavc", coreavc).toBool();
	h264_skip_loop_filter = (H264LoopFilter) value("h264_skip_loop_filter", h264_skip_loop_filter).toInt();
	HD_height = value("HD_height", HD_height).toInt();

	threads = value("threads", threads).toInt();
	hwdec = value("hwdec", hwdec).toString();

	cache_for_files = value("cache_for_files", cache_for_files).toInt();
	cache_for_streams = value("cache_for_streams", cache_for_streams).toInt();
	cache_for_dvds = value("cache_for_dvds", cache_for_dvds).toInt();
	cache_for_vcds = value("cache_for_vcds", cache_for_vcds).toInt();
	cache_for_audiocds = value("cache_for_audiocds", cache_for_audiocds).toInt();
	cache_for_tv = value("cache_for_tv", cache_for_tv).toInt();

	endGroup(); // performance

#ifdef YOUTUBE_SUPPORT
	beginGroup("youtube");
	enable_yt_support = value("enable_yt_support", enable_yt_support).toBool();
	yt_quality = value("quality", yt_quality).toInt();
	yt_user_agent = value("user_agent", yt_user_agent).toString();
	yt_use_https_main = value("yt_use_https_main", yt_use_https_main).toBool();
	yt_use_https_vi = value("yt_use_https_vi", yt_use_https_vi).toBool();
	endGroup();
#endif

#ifdef MPV_SUPPORT
	beginGroup("streaming");
	enable_streaming_sites = value("enable_streaming_sites", enable_streaming_sites).toBool();
	endGroup();
#endif


    /* *********
       Subtitles
       ********* */

	beginGroup("subtitles");

	subcp = value("subcp", subcp).toString();
	use_enca = value("use_enca", use_enca).toBool();
	enca_lang = value("enca_lang", enca_lang).toString();
	subfuzziness = value("subfuzziness", subfuzziness).toInt();
	autoload_sub = value("autoload_sub", autoload_sub).toBool();

	use_ass_subtitles = value("use_ass_subtitles", use_ass_subtitles).toBool();
	enable_ass_styles = value("enable_ass_styles", enable_ass_styles).toBool();
	ass_line_spacing = value("ass_line_spacing", ass_line_spacing).toInt();

	use_forced_subs_only = value("use_forced_subs_only", use_forced_subs_only).toBool();

	subtitles_on_screenshots = value("subtitles_on_screenshots", subtitles_on_screenshots).toBool();

	change_sub_scale_should_restart = (OptionState) value("change_sub_scale_should_restart", change_sub_scale_should_restart).toInt();

	fast_load_sub = value("fast_load_sub", fast_load_sub).toBool();

	// ASS styles
	ass_styles.load(this);
	force_ass_styles = value("force_ass_styles", force_ass_styles).toBool();
	user_forced_ass_style = value("user_forced_ass_style", user_forced_ass_style).toString();

	freetype_support = value("freetype_support", freetype_support).toBool();
#ifdef Q_OS_WIN
	use_windowsfontdir = value("use_windowsfontdir", use_windowsfontdir).toBool();
#endif

	endGroup(); // subtitles


    /* ********
       Advanced
       ******** */

	beginGroup("advanced");

#if USE_ADAPTER
	adapter = value("adapter", adapter).toInt();
#endif

	bool ok;
	QString color = value("color_key", QString::number(color_key, 16)).toString();
	unsigned int temp_color_key = color.toUInt(&ok, 16);
	if (ok)
		color_key = temp_color_key;

	use_mplayer_window = value("use_mplayer_window", use_mplayer_window).toBool();

	monitor_aspect = value("monitor_aspect", monitor_aspect).toString();

	use_idx = value("use_idx", use_idx).toBool();
	use_lavf_demuxer = value("use_lavf_demuxer", use_lavf_demuxer).toBool();

	mplayer_additional_options = value("mplayer_additional_options", mplayer_additional_options).toString();
	mplayer_additional_video_filters = value("mplayer_additional_video_filters", mplayer_additional_video_filters).toString();
	mplayer_additional_audio_filters = value("mplayer_additional_audio_filters", mplayer_additional_audio_filters).toString();

	log_enabled = value("log_enabled", log_enabled).toBool();
	if (log_enabled) {
		log_verbose = value("log_verbose", log_verbose).toBool();
		log_file = value("log_file", log_file).toBool();
	} else {
		log_verbose = false;
		log_file = false;
	}
	log_filter = value("log_filter", log_filter).toString();

	repaint_video_background = value("repaint_video_background", repaint_video_background).toBool();

	use_edl_files = value("use_edl_files", use_edl_files).toBool();

	prefer_ipv4 = value("prefer_ipv4", prefer_ipv4).toBool();

	use_short_pathnames = value("use_short_pathnames", use_short_pathnames).toBool();

	use_correct_pts = (OptionState) value("correct_pts", use_correct_pts).toInt();

	actions_to_run = value("actions_to_run", actions_to_run).toString();

	show_tag_in_window_title = value("show_tag_in_window_title", show_tag_in_window_title).toBool();

	time_to_kill_mplayer = value("time_to_kill_mplayer", time_to_kill_mplayer).toInt();

#ifdef MPRIS2
	use_mpris2 = value("use_mpris2", use_mpris2).toBool();
#endif

	endGroup(); // advanced


    /* *********
       GUI stuff
       ********* */

	beginGroup("gui");

	start_in_fullscreen = value("start_in_fullscreen", start_in_fullscreen).toBool();

	stay_on_top = (TPreferences::OnTop) value("stay_on_top", (int) stay_on_top).toInt();
	size_factor = value("size_factor", size_factor).toDouble();
	// Backward compatibility. Size used to be stored as percentage.
	if (size_factor > 24.0) size_factor = size_factor / 100;
	resize_method = value("resize_method", resize_method).toInt();

	style = value("style", style).toString();

	mouse_left_click_function = value("mouse_left_click_function", mouse_left_click_function).toString();
	mouse_right_click_function = value("mouse_right_click_function", mouse_right_click_function).toString();
	mouse_double_click_function = value("mouse_double_click_function", mouse_double_click_function).toString();
	mouse_middle_click_function = value("mouse_middle_click_function", mouse_middle_click_function).toString();
	mouse_xbutton1_click_function = value("mouse_xbutton1_click_function", mouse_xbutton1_click_function).toString();
	mouse_xbutton2_click_function = value("mouse_xbutton2_click_function", mouse_xbutton2_click_function).toString();
	wheel_function = value("mouse_wheel_function", wheel_function).toInt();
	{
		int wheel_function_cycle_int = value("wheel_function_cycle", (int) wheel_function_cycle).toInt();
		wheel_function_cycle = (WheelFunctions) wheel_function_cycle_int;
	}
	wheel_function_seeking_reverse = value("wheel_function_seeking_reverse", wheel_function_seeking_reverse).toBool();

	seeking1 = value("seeking1", seeking1).toInt();
	seeking2 = value("seeking2", seeking2).toInt();
	seeking3 = value("seeking3", seeking3).toInt();
	seeking4 = value("seeking4", seeking4).toInt();

	update_while_seeking = value("update_while_seeking", update_while_seeking).toBool();
	time_slider_drag_delay = value("time_slider_drag_delay", time_slider_drag_delay).toInt();
	relative_seeking = value("relative_seeking", relative_seeking).toBool();
	precise_seeking = value("precise_seeking", precise_seeking).toBool();

	reset_stop = value("reset_stop", reset_stop).toBool();
	delay_left_click = value("delay_left_click", delay_left_click).toBool();

	language = value("language", language).toString();
	iconset= value("iconset", iconset).toString();

	balloon_count = value("balloon_count", balloon_count).toInt();

	restore_pos_after_fullscreen = value("restore_pos_after_fullscreen", restore_pos_after_fullscreen).toBool();
	save_window_size_on_exit = 	value("save_window_size_on_exit", save_window_size_on_exit).toBool();

	close_on_finish = value("close_on_finish", close_on_finish).toBool();

#ifdef AUTO_SHUTDOWN_PC
	auto_shutdown_pc = value("auto_shutdown_pc", auto_shutdown_pc).toBool();
#endif

	default_font = value("default_font", default_font).toString();

	pause_when_hidden = value("pause_when_hidden", pause_when_hidden).toBool();

	gui = value("gui", gui).toString();

	default_size = value("default_size", default_size).toSize();

	hide_video_window_on_audio_files = value("hide_video_window_on_audio_files", hide_video_window_on_audio_files).toBool();

	report_mplayer_crashes = value("report_mplayer_crashes", report_mplayer_crashes).toBool();

#if REPORT_OLD_MPLAYER
	reported_mplayer_is_old = value("reported_mplayer_is_old", reported_mplayer_is_old).toBool();
#endif

	auto_add_to_playlist = value("auto_add_to_playlist", auto_add_to_playlist).toBool();
	media_to_add_to_playlist = (AutoAddToPlaylistFilter) value("media_to_add_to_playlist", media_to_add_to_playlist).toInt();

#if LOGO_ANIMATION
	animated_logo = value("animated_logo", animated_logo).toBool();
#endif

	endGroup(); // gui


    /* ********
       TV (dvb)
       ******** */

	beginGroup("tv");
	check_channels_conf_on_startup = value("check_channels_conf_on_startup", check_channels_conf_on_startup).toBool();
	initial_tv_deinterlace = value("initial_tv_deinterlace", initial_tv_deinterlace).toInt();
	last_dvb_channel = value("last_dvb_channel", last_dvb_channel).toString();
	last_tv_channel = value("last_tv_channel", last_tv_channel).toString();
	endGroup(); // tv


    /* ********
       Network
       ******** */

	beginGroup("proxy");
	use_proxy = value("use_proxy", use_proxy).toBool();
	proxy_type = value("type", proxy_type).toInt();
	proxy_host = value("host", proxy_host).toString();
	proxy_port = value("port", proxy_port).toInt();
	proxy_username = value("username", proxy_username).toString();
	proxy_password = value("password", proxy_password).toString();
	endGroup(); // proxy


    /* ***********
       Directories
       *********** */

	beginGroup("directories");
	save_dirs = value("save_dirs", save_dirs).toBool();
	if (save_dirs) {
		latest_dir = value("latest_dir", latest_dir).toString();
		last_dvd_directory = value("last_dvd_directory", last_dvd_directory).toString();
	}
	endGroup(); // directories


    /* **************
       Initial values
       ************** */

	beginGroup("defaults");

	initial_sub_scale = value("initial_sub_scale", initial_sub_scale).toDouble();
	initial_sub_scale_mpv = value("initial_sub_scale_mpv", initial_sub_scale_mpv).toDouble();
	initial_sub_scale_ass = value("initial_sub_scale_ass", initial_sub_scale_ass).toDouble();
	initial_volume = value("initial_volume", initial_volume).toInt();
	initial_contrast = value("initial_contrast", initial_contrast).toInt();
	initial_brightness = value("initial_brightness", initial_brightness).toInt();
	initial_hue = value("initial_hue", initial_hue).toInt();
	initial_saturation = value("initial_saturation", initial_saturation).toInt();
	initial_gamma = value("initial_gamma", initial_gamma).toInt();

	initial_audio_equalizer = value("initial_audio_equalizer", initial_audio_equalizer).toList();

	initial_zoom_factor = value("initial_zoom_factor", initial_zoom_factor).toDouble();
	initial_sub_pos = value("initial_sub_pos", initial_sub_pos).toInt();

	initial_volnorm = value("initial_volnorm", initial_volnorm).toBool();
	initial_postprocessing = value("initial_postprocessing", initial_postprocessing).toBool();

	initial_deinterlace = value("initial_deinterlace", initial_deinterlace).toInt();

	initial_audio_channels = value("initial_audio_channels", initial_audio_channels).toInt();
	initial_stereo_mode = value("initial_stereo_mode", initial_stereo_mode).toInt();

	initial_audio_track = value("initial_audio_track", initial_audio_track).toInt();
	initial_subtitle_track = value("initial_subtitle_track", initial_subtitle_track).toInt();

	endGroup(); // defaults


    /* ************
       MPlayer info
       ************ */

	beginGroup("mplayer_info");
	mplayer_detected_version = value("mplayer_detected_version", mplayer_detected_version).toInt();
	mplayer_user_supplied_version = value("mplayer_user_supplied_version", mplayer_user_supplied_version).toInt();
	mplayer_is_mplayer2 = value("is_mplayer2", mplayer_is_mplayer2).toBool();
	mplayer2_detected_version = value("mplayer2_detected_version", mplayer2_detected_version).toString();

	endGroup(); // mplayer_info


    /* *********
       Instances
       ********* */
#ifdef SINGLE_INSTANCE
	beginGroup("instances");
	use_single_instance = value("single_instance_enabled", use_single_instance).toBool();
	endGroup(); // instances
#endif


    /* ****************
       Floating control
       **************** */

	beginGroup("floating_control");
	floating_control_width = value("width", floating_control_width).toInt();
	floating_activation_area = (Gui::TAutohideToolbar::Activation) value("activation_area", floating_activation_area).toInt();
	floating_hide_delay = value("hide_delay", floating_hide_delay).toInt();
	endGroup(); // floating_control


    /* *******
       History
       ******* */

	beginGroup("history");

	history_recents.setMaxItems(value("recents/max_items", history_recents.maxItems()).toInt());
	history_recents.fromStringList(value("recents", history_recents).toStringList());

	history_urls.setMaxItems(value("urls/max_items", history_urls.maxItems()).toInt());
	history_urls.fromStringList(value("urls", history_urls).toStringList());

	endGroup(); // history


    /* *******
       Filters
       ******* */

	filters.load(this);


    /* *********
       SMPlayer info
       ********* */

	beginGroup("smplayer");
#ifdef CHECK_UPGRADED
	smplayer_stable_version = value("stable_version", smplayer_stable_version).toString();
	check_if_upgraded = value("check_if_upgraded", check_if_upgraded).toBool();
#endif
	endGroup();


    /* *********
       Update
       ********* */

#ifdef UPDATE_CHECKER
	update_checker_data.load(this);
#endif


	qDebug("Settings::TPreferences::load: config_version: %d, CURRENT_CONFIG_VERSION: %d", config_version, CURRENT_CONFIG_VERSION);
	// Fix some values if config is old
	if (config_version < CURRENT_CONFIG_VERSION) {
		qDebug("TPreferences::load: config version is old, updating it");
		/*
		if (config_version <= 2) {
			use_slices = false;
		}
		if (config_version <= 3) {
			osd = None;
			frame_drop = false;
			cache_for_files = 2048;
			cache_for_streams = 2048;
			time_to_kill_mplayer = 1000;
		}
		*/
		if (config_version < 4) {
			use_slices = false;
			osd_level = None;
			frame_drop = false;
			cache_for_files = 2048;
			cache_for_streams = 2048;
			resize_method = Never;
		}
		if (config_version <= 4) {
			if (time_to_kill_mplayer < 5000)
				time_to_kill_mplayer = 5000;
			use_dvdnav = true;
			if (time_slider_drag_delay < 200)
				time_slider_drag_delay = 200;
		}

		config_version = CURRENT_CONFIG_VERSION;
	}

#ifdef Q_OS_WIN
	// Check if the mplayer binary exists and try to fix it
	if (!QFile::exists(mplayer_bin)) {
		qWarning("mplayer_bin '%s' doesn't exist", mplayer_bin.toLatin1().constData());
		bool fixed = false;
		if (QFile::exists("mplayer/mplayer.exe")) {
			mplayer_bin = "mplayer/mplayer.exe";
			fixed = true;
		}
		else
		if (QFile::exists("mplayer/mplayer2.exe")) {
			mplayer_bin = "mplayer/mplayer2.exe";
			fixed = true;
		}
		else
		if (QFile::exists("mplayer/mpv.exe")) {
			mplayer_bin = "mplayer/mpv.exe";
			fixed = true;
		}
		if (fixed) {
			qWarning("mplayer_bin changed to '%s'", mplayer_bin.toLatin1().constData());
		}
	}
#endif
#ifdef Q_OS_LINUX
	if (!QFile::exists(mplayer_bin)) {
		QString app_path = Helper::findExecutable(mplayer_bin);
		if (!app_path.isEmpty()) {
			mplayer_bin = app_path;
		} else {
			// Executable not found, try to find an alternative
			if (mplayer_bin.startsWith("mplayer")) {
				app_path = Helper::findExecutable("mpv");
				if (!app_path.isEmpty()) mplayer_bin = app_path;
			}
			else
			if (mplayer_bin.startsWith("mpv")) {
				app_path = Helper::findExecutable("mplayer");
				if (!app_path.isEmpty()) mplayer_bin = app_path;
			}
		}
	}
#endif
}

double TPreferences::monitor_aspect_double() {

	QRegExp exp("(\\d+)[:/](\\d+)");
	if (exp.indexIn(monitor_aspect) >= 0) {
		int w = exp.cap(1).toInt();
		int h = exp.cap(2).toInt();
		qDebug("Settings::TPreferences::monitor_aspect_double: monitor_aspect parsed successfully: %d:%d", w, h);
		return (double) w / h;
	}

	bool ok;
	double res = monitor_aspect.toDouble(&ok);
	if (ok) {
		qDebug("Settings::TPreferences::monitor_aspect_double: monitor_aspect parsed successfully: %f", res);
		return res;
	}

	qDebug("Settings::TPreferences::monitor_aspect_double: monitor_aspect set to 0");
	return 0;
}

void TPreferences::setupScreenshotFolder() {
#if QT_VERSION >= 0x040400
	if (screenshot_directory.isEmpty()) {
		#if QT_VERSION >= 0x050000
		QString pdir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
		if (pdir.isEmpty()) pdir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
		if (pdir.isEmpty()) pdir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
		#else
		QString pdir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
		if (pdir.isEmpty()) pdir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
		if (pdir.isEmpty()) pdir = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
		#endif
		if (pdir.isEmpty()) pdir = "/tmp";
		if (!QFile::exists(pdir)) {
			qWarning("Settings::TPreferences::setupScreenshotFolder: folder '%s' does not exist. Using /tmp as fallback", pdir.toUtf8().constData());
			pdir = "/tmp";
		}
		QString default_screenshot_path = QDir::toNativeSeparators(pdir + "/smplayer_screenshots");
		if (!QFile::exists(default_screenshot_path)) {
			qDebug("Settings::TPreferences::setupScreenshotFolder: creating '%s'", default_screenshot_path.toUtf8().constData());
			if (!QDir().mkdir(default_screenshot_path)) {
				qWarning("Settings::TPreferences::setupScreenshotFolder: failed to create '%s'", default_screenshot_path.toUtf8().constData());
			}
		}
		if (QFile::exists(default_screenshot_path)) {
			screenshot_directory = default_screenshot_path;
		}
	}
	else {
		screenshot_directory = QDir::toNativeSeparators(screenshot_directory);
	}
#endif
}

} // namespace Settings

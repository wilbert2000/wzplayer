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

#include "settings/preferences.h"

#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QRegExp>
#include <QDir>
#include <QLocale>
#include <QNetworkProxy>

#ifdef Q_OS_WIN
#include <QSysInfo> // To get Windows version
#endif

#include "settings/paths.h"
#include "settings/assstyles.h"
#include "settings/mediasettings.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "settings/filters.h"
#include "helper.h"


#define CURRENT_CONFIG_VERSION 12


namespace Settings {

TPreferences* pref = 0;

TPreferences::TPreferences() :
	TPlayerSettings(TPaths::iniPath()) {

	reset();
	load();
	pref = this;
}

TPreferences::~TPreferences() {

	pref = 0;
}

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
QString default_mplayer_bin = "mplayer.exe";
QString default_mpv_bin = "mpv.exe";
#else
QString default_mplayer_bin = "mplayer";
QString default_mpv_bin = "mpv";
#endif

void TPreferences::reset() {

	config_version = CURRENT_CONFIG_VERSION;

	// General tab
	player_id = ID_MPLAYER;
	player_bin = default_mplayer_bin;
	mplayer_bin = default_mplayer_bin;
	mpv_bin = default_mpv_bin;

	remember_media_settings = false;
	remember_time_pos = false;
	global_volume = true;

	use_screenshot = true;
	screenshot_template = "cap_%F_%p_%02n";
	screenshot_format = "jpg";

#ifdef PORTABLE_APP
	screenshot_directory= "./screenshots";
#else
	screenshot_directory = "";
#endif


	// Video tab
	// Video driver

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA) {
		mplayer_vo = "direct3d";
		mpv_vo = mplayer_vo;
	} else {
		mplayer_vo = "directx";
		mpv_vo = mplayer_vo;
	}
#else
#ifdef Q_OS_OS2
	mplayer_vo = "kva";
	mpv_vo = mplayer_vo;
#else
	mplayer_vo = "xv";
	mpv_vo = ""; // Players default
#endif
#endif

	vo = mplayer_vo;

	hwdec = "auto";

	frame_drop = false;
	hard_frame_drop = false;
	use_soft_video_eq = false;
	postprocessing_quality = 6;

#ifndef Q_OS_WIN
	vdpau.ffh264vdpau = true;
	vdpau.ffmpeg12vdpau = true;
	vdpau.ffwmv3vdpau = true;
	vdpau.ffvc1vdpau = true;
	vdpau.ffodivxvdpau = false;
	vdpau.disable_video_filters = true;
#endif


	// Audio tab

#ifdef Q_OS_OS2
	ao = "kai";
#else
#ifdef Q_OS_LINUX
	ao = "pulse";
#else
	ao = ""; // Players default
#endif
#endif
	mplayer_ao = ao;
	mpv_ao = ao;

	use_soft_vol = false;
	// 100 is no amplification. 110 is default in mplayer, 130 in MPV...
	softvol_max = 110;

	use_scaletempo = Detect;
	use_hwac3 = false;
	use_audio_equalizer = true;

	volume = 50;
	mute = false;

	global_audio_equalizer = true;
	audio_equalizer << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0; // FIXME: use initial_audio_equalizer (but it's set later)

	autosync = false;
	autosync_factor = 100;

	use_mc = false;
	mc_value = 0;

	autoload_m4a = true;
	min_step = 5;

	osd_level = None;
	osd_scale = 1;
	subfont_osd_scale = 3;

	file_settings_method = "hash"; // Possible values: normal & hash

	// Preferred
	audio_lang = "";

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
	// TODO:
	enable_audiocd_on_windows = false;
#endif

	vcd_initial_title = 2; // Most VCD's start at title #2

	use_dvdnav = true;

    /* ***********
       Performance
       *********** */
	cache_enabled = false;
	cache_for_files = 2048;
	cache_for_streams = 2048;
	cache_for_dvds = 0; // not recommended to use cache for dvds
	cache_for_vcds = 1024;
	cache_for_audiocds = 1024;
	cache_for_tv = 3000;


	// Subtitles
	subtitle_fuzziness = 1;
	subtitle_language = "";
	select_first_subtitle = false;

	subtitle_enca_language = ""; // Auto detect subtitle encoding language
	// To use lang from system:
	// subtitle_enca_language = QString(QLocale::system().name()).section("_" , 0, 0);
	subtitle_encoding_fallback = ""; // Auto detect subtitle encoding

	use_ass_subtitles = true;
	use_custom_ass_style = false;
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

	color_key = 0x020202;

	monitor_aspect=""; // Autodetect

	use_idx = false;
	use_lavf_demuxer = false;

	player_additional_options = "";
#ifdef PORTABLE_APP
	player_additional_options = "-nofontconfig";
#endif
	player_additional_video_filters = "";
	player_additional_audio_filters = "";

	log_debug_enabled = true;
	log_verbose = false;
	log_file = false;

	use_edl_files = true;

	use_playlist_option = false;

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

	resize_on_load = true;
	resize_on_docking = true;

	style = "";

	mouse_left_click_function = "play_or_pause";
	mouse_right_click_function = "show_context_menu";
	mouse_double_click_function = "fullscreen";
	mouse_middle_click_function = "next_wheel_function";
	mouse_xbutton1_click_function = "";
	mouse_xbutton2_click_function = "";
	wheel_function = Zoom;
	wheel_function_cycle = Volume | Zoom;
	wheel_function_seeking_reverse = false;

	seeking1 = 10;
	seeking2 = 60;
	seeking3 = 10*60;
	seeking4 = 30;

	update_while_seeking = true;
	time_slider_drag_delay = 200;
	relative_seeking = false;
	precise_seeking = true;

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

	default_font = "";

	pause_when_hidden = false;

	iconset = "H2O";

	// Used to be default_size = QSize(683, 509);
	// Now 360p 16:9 is 640 x 360 (360 + 99 = 459)
	default_size = QSize(640, 459);

	hide_video_window_on_audio_files = true;

	report_player_crashes = true;

	media_to_add_to_playlist = NoFiles;


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
	last_dvd_directory = "";
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



    /* *********
       Instances
       ********* */
#ifdef SINGLE_INSTANCE
	use_single_instance = true;
#endif


    /* ****************
       Floating control
       **************** */

	floating_activation_area = Anywhere;
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
}

void TPreferences::save() {
	qDebug("Settings::TPreferences::save");

    /* *******
       General
       ******* */
	beginGroup("General");

	setValue("config_version", config_version);

	beginGroup("mplayer");
	setValue("bin", mplayer_bin);
	setValue("vo", mplayer_vo);
	setValue("ao", mplayer_ao);
	endGroup();

	beginGroup("mpv");
	setValue("bin", mpv_bin);
	setValue("vo", mpv_vo);
	setValue("ao", mpv_ao);
	setValue("hwdec", hwdec);
	setValue("screenshot_template", screenshot_template);
	setValue("screenshot_format", screenshot_format);
	endGroup();

	setValue("player_bin", player_bin);
	setValue("vo", vo);
	setValue("ao", ao);

	setValue("dont_remember_media_settings", !remember_media_settings);
	setValue("dont_remember_time_pos", !remember_time_pos);
	setValue("file_settings_method", file_settings_method);

	setValue("use_screenshot", use_screenshot);
	// "screenshot_folder" used to be "screenshot_directory"
	// before QT_VERSION 0x040400
	setValue("screenshot_folder", screenshot_directory);


	// Video tab
	setValue("frame_drop", frame_drop);
	setValue("hard_frame_drop", hard_frame_drop);
	setValue("use_soft_video_eq", use_soft_video_eq);
	setValue("postprocessing_quality", postprocessing_quality);

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

	setValue("audio_lang", audio_lang);

	setValue("autoload_m4a", autoload_m4a);
	setValue("min_step", min_step);

	setValue("osd_level", osd_level);
	setValue("osd_scale", osd_scale);
	setValue("subfont_osd_scale", subfont_osd_scale);

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
	setValue("cache_enabled", cache_enabled);
	setValue("cache_for_files", cache_for_files);
	setValue("cache_for_streams", cache_for_streams);
	setValue("cache_for_dvds", cache_for_dvds);
	setValue("cache_for_vcds", cache_for_vcds);
	setValue("cache_for_audiocds", cache_for_audiocds);
	setValue("cache_for_tv", cache_for_tv);

	endGroup(); // performance


    /* *********
       Subtitles
       ********* */
	beginGroup("subtitles");

	setValue("subtitle_fuzziness", subtitle_fuzziness);
	setValue("subtitle_language", subtitle_language);
	setValue("select_first_subtitle", select_first_subtitle);

	setValue("subtitle_enca_language", subtitle_enca_language);
	setValue("subtitle_encoding_fallback", subtitle_encoding_fallback);

	setValue("use_ass_subtitles", use_ass_subtitles);
	setValue("use_custom_ass_style", use_custom_ass_style);
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

	setValue("color_key", QString::number(color_key, 16));

	setValue("monitor_aspect", monitor_aspect);

	setValue("use_idx", use_idx);
	setValue("use_lavf_demuxer", use_lavf_demuxer);

	setValue("mplayer_additional_options", player_additional_options);
	setValue("mplayer_additional_video_filters", player_additional_video_filters);
	setValue("mplayer_additional_audio_filters", player_additional_audio_filters);

	setValue("log_debug_enabled", log_debug_enabled);
	setValue("log_verbose", log_verbose);
	setValue("log_file", log_file);

	setValue("use_edl_files", use_edl_files);

	setValue("use_playlist_option", use_playlist_option);

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
	setValue("resize_on_load", resize_on_load);
	setValue("resize_on_docking", resize_on_docking);

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

	setValue("delay_left_click", delay_left_click);

	setValue("language", language);
	setValue("iconset", iconset);

	setValue("balloon_count", balloon_count);

	setValue("restore_pos_after_fullscreen", restore_pos_after_fullscreen);
	setValue("save_window_size_on_exit", save_window_size_on_exit);

	setValue("close_on_finish", close_on_finish);

	setValue("default_font", default_font);

	setValue("pause_when_hidden", pause_when_hidden);

	setValue("default_size", default_size);

	setValue("hide_video_window_on_audio_files", hide_video_window_on_audio_files);

	setValue("report_player_crashes", report_player_crashes);

	setValue("media_to_add_to_playlist", media_to_add_to_playlist);

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

	endGroup(); // defaults


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
       Update
       ********* */
	update_checker_data.save(this);

	sync();
}

TPreferences::TPlayerID TPreferences::getPlayerID(const QString& player) {

	QFileInfo fi(player);
	QString name = fi.fileName();
	if (name.isEmpty()) {
		name = player;
	}
	if (name.toLower().startsWith("mplayer")) {
		return ID_MPLAYER;
	}
	return ID_MPV;
}

void TPreferences::setPlayerID() {

	player_id = getPlayerID(player_bin);
}

QString TPreferences::playerName() const {

	if (player_id == ID_MPLAYER)
		return "MPlayer";
	return "MPV";
}

QString TPreferences::getAbsolutePathPlayer(const QString& player) {

	QString path = player;
	QString found_player = Helper::findExecutable(path);
	if (!found_player.isEmpty()) {
		path = found_player;
	}
	qDebug() << "Settings::TPreferences::getAbsolutePathPlayer:" << path;
	return path;
}

void TPreferences::setPlayerBin(QString bin,
								bool allow_other_player,
								TPlayerID wanted_player) {

	// Check binary and try to fix it
	if (bin.isEmpty()) {
		if (wanted_player == ID_MPLAYER) {
			bin = default_mplayer_bin;
		} else {
			bin = default_mpv_bin;
		}
	}

	QString found_bin = Helper::findExecutable(bin);

	// Try to find an alternative if not found
	if (found_bin.isEmpty()) {
		TPlayerID found_id = ID_MPLAYER;
		QFileInfo fi(bin);
		if (wanted_player == ID_MPV || fi.baseName().startsWith("mpv")) {
			// Assume wanted mpv, try default mpv first
			if (bin != default_mpv_bin) {
				found_bin = Helper::findExecutable(default_mpv_bin);
				if (!found_bin.isEmpty()) {
					found_id = ID_MPV;
				}
			}
			if (found_bin.isEmpty()) {
				if (bin != default_mplayer_bin
					&& (allow_other_player || wanted_player == ID_MPLAYER)) {
					// Try default mplayer
					found_bin = Helper::findExecutable(default_mplayer_bin);
				}
			} else {
				found_id = ID_MPV;
			}
		} else {
			// Try default mplayer
			if (bin != default_mplayer_bin
				&& (allow_other_player || wanted_player == ID_MPLAYER)) {
				found_bin = Helper::findExecutable(default_mplayer_bin);
			}
			if (found_bin.isEmpty()
				&& bin != default_mpv_bin
				&& (allow_other_player || wanted_player == ID_MPV)) {
				// Try default mpv
				found_bin = Helper::findExecutable(default_mpv_bin);
				if (!found_bin.isEmpty()) {
					found_id = ID_MPV;
				}
			}
		}

		if (found_bin.isEmpty()) {
			qWarning() << "Settings::TPreferences""setPlayerBin: failed to find player"
					   << bin;
		} else if (allow_other_player || found_id == wanted_player) {
			qWarning() << "Settings::TPreferences""setPlayerBin: failed to find player"
					   << bin << "Selecting" << found_bin << "instead.";
			bin = found_bin;
		} else {
			qWarning() << "Settings::TPreferences""setPlayerBin: failed to find player"
					   << bin << "Maybe you could use" << found_bin << "instead.";
		}
	} else {
		bin = found_bin;
	}

	qDebug() << "Settings::TPreferences::setPlayerBin: selected player"
			 << bin;
	player_bin = bin;

	setPlayerID();

	// Store player and set drivers for player
	if (!found_bin.isEmpty()) {
		if (player_id == ID_MPLAYER) {
			mplayer_bin = player_bin;
			vo = mplayer_vo;
			ao = mplayer_ao;
		} else {
			mpv_bin = player_bin;
			vo = mpv_vo;
			ao = mpv_ao;
		}
	}

	qDebug() << "Settings::TPreferences::setPlayerBin: selected vo" << vo
			 << "mplayer vo" << mplayer_vo
			 << "mplayer ao" << mplayer_ao
			 << "selected ao" << ao
			 << "mpv vo" << mpv_vo
			 << "mpv ao" << mpv_ao;
}

void TPreferences::load() {
	qDebug("Settings::TPreferences::load");

	// General tab
	beginGroup("General");

	config_version = value("config_version", 0).toInt();

	beginGroup("mplayer");
	mplayer_bin = value("bin", mplayer_bin).toString();
	mplayer_vo  = value("vo", mplayer_vo).toString();
	mplayer_ao = value("ao", mplayer_ao).toString();
	endGroup();

	beginGroup("mpv");
	mpv_bin = value("bin", mpv_bin).toString();
	mpv_vo = value("vo", mpv_vo).toString();
	mpv_ao = value("ao", mpv_ao).toString();
	hwdec = value("hwdec", hwdec).toString();
	screenshot_template = value("screenshot_template", screenshot_template).toString();
	screenshot_format = value("screenshot_format", screenshot_format).toString();
	endGroup();

	setPlayerBin(value("player_bin", player_bin).toString(), true, player_id);

	// Media settings per file
	remember_media_settings = !value("dont_remember_media_settings", !remember_media_settings).toBool();
	remember_time_pos = !value("dont_remember_time_pos", !remember_time_pos).toBool();
	file_settings_method = value("file_settings_method", file_settings_method).toString();

	// Screenshots
	use_screenshot = value("use_screenshot", use_screenshot).toBool();
	// Note: "screenshot_folder" used to be "screenshot_directory" before Qt 4.4
	screenshot_directory = value("screenshot_folder", screenshot_directory).toString();
	setupScreenshotFolder();


	// Video tab
	frame_drop = value("frame_drop", frame_drop).toBool();
	hard_frame_drop = value("hard_frame_drop", hard_frame_drop).toBool();
	use_soft_video_eq = value("use_soft_video_eq", use_soft_video_eq).toBool();
	postprocessing_quality = value("postprocessing_quality", postprocessing_quality).toInt();

#ifndef Q_OS_WIN
	vdpau.ffh264vdpau = value("vdpau_ffh264vdpau", vdpau.ffh264vdpau).toBool();
	vdpau.ffmpeg12vdpau = value("vdpau_ffmpeg12vdpau", vdpau.ffmpeg12vdpau).toBool();
	vdpau.ffwmv3vdpau = value("vdpau_ffwmv3vdpau", vdpau.ffwmv3vdpau).toBool();
	vdpau.ffvc1vdpau = value("vdpau_ffvc1vdpau", vdpau.ffvc1vdpau).toBool();
	vdpau.ffodivxvdpau = value("vdpau_ffodivxvdpau", vdpau.ffodivxvdpau).toBool();
	vdpau.disable_video_filters = value("vdpau_disable_video_filters", vdpau.disable_video_filters).toBool();
#endif


	// Audio tab
	use_soft_vol = value("use_soft_vol", use_soft_vol).toBool();
	softvol_max = value("softvol_max", softvol_max).toInt();
	if (softvol_max < 100)
		softvol_max = 100;
	else if (softvol_max > 1000)
		softvol_max = 1000;
	use_scaletempo = (TOptionState) value("use_scaletempo", use_scaletempo).toInt();
	use_hwac3 = value("use_hwac3", use_hwac3).toBool();
	use_audio_equalizer = value("use_audio_equalizer", use_audio_equalizer).toBool();

	global_volume = value("global_volume", global_volume).toBool();
	volume = value("volume", volume).toInt();
	if (volume < 0) volume = 0;
	if (volume > 100) volume = 100;

	mute = value("mute", mute).toBool();

	global_audio_equalizer = value("global_audio_equalizer", global_audio_equalizer).toBool();
	audio_equalizer = value("audio_equalizer", audio_equalizer).toList();

	autosync = value("autosync", autosync).toBool();
	autosync_factor = value("autosync_factor", autosync_factor).toInt();

	use_mc = value("use_mc", use_mc).toBool();
	mc_value = value("mc_value", mc_value).toDouble();

	audio_lang = value("audio_lang", audio_lang).toString();

	autoload_m4a = value("autoload_m4a", autoload_m4a).toBool();
	min_step = value("min_step", min_step).toInt();

	// OSD
	osd_level = (TOSDLevel) value("osd_level", (int) osd_level).toInt();
	osd_scale = value("osd_scale", osd_scale).toDouble();
	subfont_osd_scale = value("subfont_osd_scale", subfont_osd_scale).toDouble();

	endGroup(); // General


	// Drives (CD/DVD)
	beginGroup("drives");

	dvd_device = value("dvd_device", dvd_device).toString();
	cdrom_device = value("cdrom_device", cdrom_device).toString();
	bluray_device = value("bluray_device", bluray_device).toString();

#ifdef Q_OS_WIN
	enable_audiocd_on_windows = value("enable_audiocd_on_windows", enable_audiocd_on_windows).toBool();
#endif

	// TODO: move to Preferred??
	vcd_initial_title = value("vcd_initial_title", vcd_initial_title).toInt();

	use_dvdnav = value("use_dvdnav", use_dvdnav).toBool();

	endGroup(); // drives


    /* ***********
       Performance
       *********** */

	beginGroup("performance");

	cache_enabled = value("cache_enabled", cache_enabled).toBool();
	cache_for_files = value("cache_for_files", cache_for_files).toInt();
	cache_for_streams = value("cache_for_streams", cache_for_streams).toInt();
	cache_for_dvds = value("cache_for_dvds", cache_for_dvds).toInt();
	cache_for_vcds = value("cache_for_vcds", cache_for_vcds).toInt();
	cache_for_audiocds = value("cache_for_audiocds", cache_for_audiocds).toInt();
	cache_for_tv = value("cache_for_tv", cache_for_tv).toInt();

	endGroup(); // performance


    /* *********
       Subtitles
       ********* */

	beginGroup("subtitles");

	subtitle_fuzziness = value("subtitle_fuzziness", subtitle_fuzziness).toInt();
	subtitle_language = value("subtitle_language", subtitle_language).toString();
	select_first_subtitle = value("select_first_subtitle", select_first_subtitle).toBool();

	subtitle_enca_language = value("subtitle_enca_language", subtitle_enca_language).toString();
	subtitle_encoding_fallback = value("subtitle_encoding_fallback", subtitle_encoding_fallback).toString();

	use_ass_subtitles = value("use_ass_subtitles", use_ass_subtitles).toBool();
	use_custom_ass_style = value("use_custom_ass_style", use_custom_ass_style).toBool();
	ass_line_spacing = value("ass_line_spacing", ass_line_spacing).toInt();

	use_forced_subs_only = value("use_forced_subs_only", use_forced_subs_only).toBool();

	subtitles_on_screenshots = value("subtitles_on_screenshots", subtitles_on_screenshots).toBool();

	change_sub_scale_should_restart = (TOptionState) value("change_sub_scale_should_restart", change_sub_scale_should_restart).toInt();

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

	bool ok;
	QString color = value("color_key", QString::number(color_key, 16)).toString();
	unsigned int temp_color_key = color.toUInt(&ok, 16);
	if (ok)
		color_key = temp_color_key;
	else
		qWarning() << "Settings::TPreferences: failed to parse color key"
				   << color;

	monitor_aspect = value("monitor_aspect", monitor_aspect).toString();

	use_idx = value("use_idx", use_idx).toBool();
	use_lavf_demuxer = value("use_lavf_demuxer", use_lavf_demuxer).toBool();

	player_additional_options = value("mplayer_additional_options", player_additional_options).toString();
	player_additional_video_filters = value("mplayer_additional_video_filters", player_additional_video_filters).toString();
	player_additional_audio_filters = value("mplayer_additional_audio_filters", player_additional_audio_filters).toString();

	// Load log settings
	log_debug_enabled = value("log_debug_enabled", log_debug_enabled).toBool();
	log_verbose = value("log_verbose", log_verbose).toBool();
	log_file = value("log_file", log_file).toBool();

	use_edl_files = value("use_edl_files", use_edl_files).toBool();

	use_playlist_option = value("use_playlist_option", use_playlist_option).toBool();

	use_correct_pts = (TOptionState) value("correct_pts", use_correct_pts).toInt();

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

	stay_on_top = (TPreferences::TOnTop) value("stay_on_top", (int) stay_on_top).toInt();
	size_factor = value("size_factor", size_factor).toDouble();
	// Backward compatibility. Size used to be stored as percentage.
	if (size_factor > 24.0) size_factor = size_factor / 100;
	if (size_factor < 0.1) size_factor = 0.1;
	if (size_factor > 4.0) size_factor = 4.0;
	resize_on_load = value("resize_on_load", resize_on_load).toBool();
	resize_on_docking = value("resize_on_docking", resize_on_docking).toBool();

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
		wheel_function_cycle = (TWheelFunctions) wheel_function_cycle_int;
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

	delay_left_click = value("delay_left_click", delay_left_click).toBool();

	language = value("language", language).toString();
	iconset= value("iconset", iconset).toString();

	balloon_count = value("balloon_count", balloon_count).toInt();

	restore_pos_after_fullscreen = value("restore_pos_after_fullscreen", restore_pos_after_fullscreen).toBool();
	save_window_size_on_exit = 	value("save_window_size_on_exit", save_window_size_on_exit).toBool();

	close_on_finish = value("close_on_finish", close_on_finish).toBool();

	default_font = value("default_font", default_font).toString();

	pause_when_hidden = value("pause_when_hidden", pause_when_hidden).toBool();

	default_size = value("default_size", default_size).toSize();

	hide_video_window_on_audio_files = value("hide_video_window_on_audio_files", hide_video_window_on_audio_files).toBool();

	report_player_crashes = value("report_player_crashes", report_player_crashes).toBool();

	media_to_add_to_playlist = (TAutoAddToPlaylistFilter) value("media_to_add_to_playlist", media_to_add_to_playlist).toInt();

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
	if (initial_volume < 0) initial_volume = 0;
	if (initial_volume > 100) initial_volume = 100;
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

	endGroup(); // defaults



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
	floating_activation_area = (TToolbarActivation) value("activation_area", floating_activation_area).toInt();
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
       Update
       ********* */
	update_checker_data.load(this);


	qDebug("Settings::TPreferences::load: config_version: %d, CURRENT_CONFIG_VERSION: %d", config_version, CURRENT_CONFIG_VERSION);
	// Fix some values if config is old
	if (config_version < CURRENT_CONFIG_VERSION) {
		qDebug("TPreferences::load: config version is old, updating it");
		if (config_version < 4) {
			osd_level = None;
			frame_drop = false;
			cache_for_files = 2048;
			cache_for_streams = 2048;
		}
		if (config_version <= 5) {
			if (time_to_kill_mplayer < 5000)
				time_to_kill_mplayer = 5000;
			use_dvdnav = true;
			if (time_slider_drag_delay < 200)
				time_slider_drag_delay = 200;
			if (min_step < 5)
				min_step = 5;

			remove("floating_control/width");
			remove("advanced/use_mplayer_window");
			remove("mplayer_info");
		}

		if (config_version < 9) {
			remove("advanced/log_enabled");
			remove("advanced/log_filter");
		}

		if (config_version < 10) {
			remove("gui/gui");
			remove("skin_gui");
			remove("mini_gui");
			remove("mpc_gui");
			remove("gui/auto_shutdown_pc");
			remove("performance/h264_skip_loop_filter");
			remove("performance/HD_height");
			remove("performance/coreavc");
			remove("performance/threads");
			remove("youtube");
			remove("videopreview");
			remove("streaming");
		}

		if (config_version < 11) {
			remove("smplayer");
			remove("General/use_direct_rendering");
			remove("General/use_double_buffer");
			remove("General/use_slices");
			remove("General/autoq");

			subtitle_language = value("General/subtitle_lang", subtitle_language).toString();
			remove("General/subtitle_lang");

			remove("gui/reset_stop");

			subtitle_fuzziness = value("subtitles/subfuzziness", subtitle_fuzziness).toInt();
			remove("subtitles/subfuzziness");
			remove("subtitles/autoload_sub");

			remove("subtitles/use_enca");
			remove("subtitles/enca_lang");
			remove("subtitles/subcp");

			remove("performance/hwdec");
			remove("performance/frame_drop");
			remove("performance/hard_frame_drop");
			remove("performance/priority");

			remove("advanced/prefer_ipv4");

			remove("defaults/initial_audio_track");
			remove("defaults/initial_subtitle_track");
		}

		if (config_version < 12) {
			use_custom_ass_style = value("enable_ass_styles", use_custom_ass_style).toBool();
			remove("subtitles/enable_ass_styles");
			remove("advanced/repaint_video_background");
			remove("gui/report_player_crashes");

			if (vo == "player_default") {
				vo = "";
			}
			if (ao == "player_default") {
				ao = "";
			}
		}

		remove("General/capture_directory");
		remove("General/add_blackborders_on_fullscreen");
		remove("gui/auto_add_to_playlist");
		remove("gui/resize_method");

		config_version = CURRENT_CONFIG_VERSION;
		sync();
	}
} // load()

bool TPreferences::useColorKey() const {

#if defined(Q_OS_WIN)
	return vo.startsWith("directx");
#else
#if defined(Q_OS_OS2)
	return vo.startsWith("kva");
#else
	return false;
#endif
#endif

}

double TPreferences::monitorAspectDouble() {

	QRegExp exp("(\\d+)[:/](\\d+)");

	if (monitor_aspect.isEmpty()) {
		return 0;
	}
	if (exp.indexIn(monitor_aspect) >= 0) {
		int w = exp.cap(1).toInt();
		int h = exp.cap(2).toInt();
		qDebug("Settings::TPreferences::monitorAspectDouble: monitor aspect set to %d:%d", w, h);
		return h <= 0.01 ? 0 : (double) w / h;
	}

	bool ok;
	double res = monitor_aspect.toDouble(&ok);
	if (ok) {
		qDebug("Settings::TPreferences::monitorAspectDouble: monitor aspect set to %f", res);
		return res;
	}

	qWarning("Settings::TPreferences::monitorAspectDouble: failed to parse monitor aspect, set to auto detect");
	return 0;
}

void TPreferences::setupScreenshotFolder() {

	if (screenshot_directory.isEmpty()) {
		QString pdir = TPaths::location(TPaths::TLocation::PicturesLocation);
		if (pdir.isEmpty()) pdir = TPaths::location(TPaths::TLocation::DocumentsLocation);
		if (pdir.isEmpty()) pdir = TPaths::location(TPaths::TLocation::HomeLocation);

		if (pdir.isEmpty())
			pdir = "/tmp";

		if (QFile::exists(pdir)) {
			QString default_screenshot_path = QDir::toNativeSeparators(pdir + "/screenshots");
			if (QFile::exists(default_screenshot_path)) {
				screenshot_directory = default_screenshot_path;
			} else if (QDir().mkdir(default_screenshot_path)) {
				qDebug() << "Settings::TPreferences::setupScreenshotFolder: created"
						 << default_screenshot_path;
				screenshot_directory = default_screenshot_path;
			} else {
				qWarning() << "Settings::TPreferences::setupScreenshotFolder: failed to create"
						   << default_screenshot_path;
			}
		} else {
			qWarning() << "Settings::TPreferences::setupScreenshotFolder: folder"
					   << pdir << "does not exist";
		}
	} else {
		screenshot_directory = QDir::toNativeSeparators(screenshot_directory);
		QFileInfo fi(screenshot_directory);
		if (fi.isDir() && fi.isWritable()) {
			qDebug() << "Settings::TPreferences::setupScreenshotFolder: screenshot directory set to"
					 << screenshot_directory;
		} else {
			qWarning() << "Settings::TPreferences::setupScreenshotFolder: disabling screenshots, screenshot directory not writable"
					   << screenshot_directory;
			// Disable screenshots and capture
			screenshot_directory = "";
		}
	}

	if (screenshot_directory.isEmpty()) {
		use_screenshot = false;
	}
}

} // namespace Settings

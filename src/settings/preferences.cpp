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

#include <QSettings>
#include <QFileInfo>
#include <QRegExp>
#include <QDir>
#include <QLocale>
#include <QNetworkProxy>

#ifdef Q_OS_WIN
#include <QSysInfo> // To get Windows version
#endif

#include "wzdebug.h"
#include "log4qt/logmanager.h"
#include "settings/paths.h"
#include "settings/assstyles.h"
#include "settings/mediasettings.h"
#include "settings/recents.h"
#include "settings/filters.h"
#include "wzfiles.h"


namespace Settings {

static const int CURRENT_CONFIG_VERSION = 27;
const Log4Qt::Level TPreferences::log_default_level = Log4Qt::Level::DEBUG_INT;

TPreferences* pref = 0;
bool TPreferences::log_override = false;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TPreferences)


TPreferences::TPreferences() :
    QSettings(TPaths::iniFileName(), QSettings::IniFormat) {

    reset();
}


// Default names player executables
#if defined(Q_OS_WIN)
QString default_mplayer_bin = "mplayer.exe";
QString default_mpv_bin = "mpv.exe";
#else
QString default_mplayer_bin = "mplayer";
QString default_mpv_bin = "mpv";
#endif


void TPreferences::reset() {

    config_version = CURRENT_CONFIG_VERSION;

    // Player section
    player_id = ID_MPV;
    player_bin = default_mpv_bin;
    mpv_bin = default_mpv_bin;
    mplayer_bin = default_mplayer_bin;
    report_player_crashes = true;

    remember_media_settings = false;
    remember_time_pos = false;
    global_volume = true;
    file_settings_method = "hash"; // Possible values: normal & hash

    // Log
    log_verbose = false;
    log_level = log_default_level;
    log_window_max_events = 1000;


    // Advanced tab
    actions_to_run = "";
    player_additional_options = "";
    player_additional_video_filters = "";
    player_additional_audio_filters = "";


    // Demuxer section
    use_lavf_demuxer = false;
    use_idx = true;


    // Video section
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
    mplayer_vo = ""; // Players default
    mpv_vo = "";
#endif

    vo = mpv_vo;

#ifndef Q_OS_WIN
    vdpau.ffh264vdpau = true;
    vdpau.ffmpeg12vdpau = true;
    vdpau.ffwmv3vdpau = true;
    vdpau.ffvc1vdpau = true;
    vdpau.ffodivxvdpau = false;
    vdpau.disable_video_filters = true;
#endif

    hwdec = "auto";
    use_soft_video_eq = false;

    // Synchronization
    frame_drop = false;
    hard_frame_drop = false;
    use_correct_pts = Detect;

    initial_postprocessing = false;
    postprocessing_quality = 6;
    initial_deinterlace = TMediaSettings::NoDeinterlace;
    initial_tv_deinterlace = TMediaSettings::Yadif_1;
    initial_zoom_factor = 1.0;

    monitor_aspect = ""; // Autodetect

    color_key = 0x020202;

    initial_contrast = 0;
    initial_brightness = 0;
    initial_hue = 0;
    initial_saturation = 0;
    initial_gamma = 0;

    osd_level = None;
    osd_scale = 0.5;
    subfont_osd_scale = 3;


    // Audio section
#ifdef Q_OS_LINUX
    ao = "pulse";
#else
    ao = ""; // Players default
#endif

    mplayer_ao = ao;
    mpv_ao = ao;

    initial_audio_channels = TMediaSettings::ChDefault;
    initial_stereo_mode = TMediaSettings::Stereo;
    use_hwac3 = false;
    use_audio_equalizer = false;
    use_scaletempo = Detect;

    // Volume
    initial_volume = 100;
    volume = initial_volume;
    mute = false;
    initial_volnorm = false;

    initial_audio_equalizer << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
    global_audio_equalizer = true;
    audio_equalizer = initial_audio_equalizer;

    autosync = false;
    autosync_factor = 100;

    use_mc = false;
    mc_value = 0;

    audio_lang = "";

    autoload_m4a = true;
    min_step = 5;


    // Subtitles section
    subtitle_fuzziness = 1;
    subtitle_language = "";
    select_first_subtitle = false;

    subtitle_enca_language = ""; // Auto detect subtitle encoding language
    // To use lang from system:
    // subtitle_enca_language = QString(QLocale::system().name()).section("_" , 0, 0);
    subtitle_encoding_fallback = ""; // Auto detect subtitle encoding

    // Libraries tab
    freetype_support = true;
    use_ass_subtitles = true;
    initial_sub_scale_ass = 1;
    ass_line_spacing = 0;

    use_custom_ass_style = false;
    force_ass_styles = false;
    user_forced_ass_style.clear();

    initial_sub_pos = 100; // 100%
    initial_sub_scale = 5;
    initial_sub_scale_mpv = 1;

    use_forced_subs_only = false;


    // Interface section
    language = "";
    iconset = "";
    style = "";

    // Main window
    stay_on_top = NeverOnTop;
    size_factor = 1.0; // 100%

    // 360p 16:9 is 640 x 360 (360 + 99 = 459)
    // 480p 4:3 is 640 x 480 (480 + 99 = 579)
    default_size = QSize(640, 579);

    use_single_window = false;
    save_window_size_on_exit = false;
    resize_on_load = true;
    pause_when_hidden = false;
    hide_video_window_on_audio_files = true;
    close_on_finish = false;

    // Fullscreen
    fullscreen = false;
    floating_hide_delay = 3000;
    start_in_fullscreen = false;


    // Playlist
    addDirectories = true;
    addVideo = true;
    addAudio = true;
    addPlaylists = false;
    addImages = true;

    imageDuration = 10;

    useDirectoriePlaylists = false;

    nameBlacklist.clear();
    titleBlacklist = QStringList() << "RARBG" << "\\.com";
    rxTitleBlacklist.clear();

    // History
    history_recents.clear();
    history_urls.clear();

    save_dirs = true;
    last_dir = QDir::homePath();
    last_dvd_directory = last_dir;
    last_iso = "";

    // TV (dvb)
    last_dvb_channel = "";
    last_tv_channel = "";

    last_clipboard = "";


    // Actions section
    // Mouse tab
    mouse_left_click_function = "play_pause";
    delay_left_click = true;
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
    seeking_current_action = 1; // seek short jump

    seek_rate = 200;
    seek_relative = false;
    seek_keyframes = false;
    seek_preview = true;


    // Drives section
    cdrom_device = "";
    vcd_initial_title = 2; // Most VCD's start at title #2
    dvd_device = "";
    use_dvdnav = true; // MPlayer only
    bluray_device = "";

#ifndef Q_OS_WIN
    // Try to set default values
    if (QFile::exists("/dev/cdrom")) cdrom_device = "/dev/cdrom";
    if (QFile::exists("/dev/dvd")) dvd_device = "/dev/dvd";
#endif


    // Capture section
    screenshot_directory = "";

    use_screenshot = false;
    screenshot_template = "cap_%F_%p_%02n";
    screenshot_format = "jpg";
    subtitles_on_screenshots = false;


    // Performance section
    cache_enabled = false;
    cache_for_files = 2048;
    cache_for_streams = 2048;
    cache_for_tv = 3000;
    cache_for_brs = 0;
    cache_for_dvds = 0; // not recommended to use cache for dvds
    cache_for_vcds = 1024;
    cache_for_audiocds = 1024;

    // Network
    ipPrefer = IP_PREFER_AUTO;

    // proxy
    use_proxy = false;
    proxy_type = QNetworkProxy::HttpProxy;
    proxy_host = "";
    proxy_port = 0;
    proxy_username = "";
    proxy_password = "";


    // Misc
    use_edl_files = true;
    time_to_kill_player = 5000;
    balloon_count = 5;
    change_video_equalizer_on_startup = true;
    clean_config = false;

    filters.init();
}

void TPreferences::save() {
    WZT;

    setValue("config_version", config_version);

    // General
    beginGroup("General");
    setValue("modified", QTime::currentTime());
    endGroup();


    // Players
    beginGroup("players");

    if (isMPlayer()) {
        mplayer_additional_options = player_additional_options;
    } else {
        mpv_additional_options = player_additional_options;
    }

    beginGroup("mplayer");
    setValue("bin", mplayer_bin);
    setValue("vo", mplayer_vo);
    setValue("ao", mplayer_ao);
    setValue("options", mplayer_additional_options);
    endGroup();

    beginGroup("mpv");
    setValue("bin", mpv_bin);
    setValue("vo", mpv_vo);
    setValue("ao", mpv_ao);

    setValue("hwdec", hwdec);
    setValue("screenshot_template", screenshot_template);
    setValue("screenshot_format", screenshot_format);

    setValue("options", mpv_additional_options);
    endGroup();

    setValue("player_bin", player_bin);

    setValue("report_player_crashes", report_player_crashes);

    setValue("remember_media_settings", remember_media_settings);
    setValue("remember_time_pos", remember_time_pos);
    setValue("file_settings_method", file_settings_method);

    endGroup();


    // Demuxer
    beginGroup("demuxer");
    setValue("use_lavf_demuxer", use_lavf_demuxer);
    setValue("use_idx", use_idx);
    endGroup();


    // Video
    beginGroup("video");

#ifndef Q_OS_WIN
    setValue("vdpau_ffh264vdpau", vdpau.ffh264vdpau);
    setValue("vdpau_ffmpeg12vdpau", vdpau.ffmpeg12vdpau);
    setValue("vdpau_ffwmv3vdpau", vdpau.ffwmv3vdpau);
    setValue("vdpau_ffvc1vdpau", vdpau.ffvc1vdpau);
    setValue("vdpau_ffodivxvdpau", vdpau.ffodivxvdpau);
    setValue("vdpau_disable_video_filters", vdpau.disable_video_filters);
#endif

    setValue("use_soft_video_eq", use_soft_video_eq);

    setValue("frame_drop", frame_drop);
    setValue("hard_frame_drop", hard_frame_drop);
    setValue("correct_pts", use_correct_pts);

    setValue("initial_postprocessing", initial_postprocessing);
    setValue("postprocessing_quality", postprocessing_quality);
    setValue("initial_deinterlace", initial_deinterlace);
    setValue("initial_tv_deinterlace", initial_tv_deinterlace);
    setValue("initial_zoom_factor", initial_zoom_factor);

    setValue("monitor_aspect", monitor_aspect);

    setValue("initial_contrast", initial_contrast);
    setValue("initial_brightness", initial_brightness);
    setValue("initial_hue", initial_hue);
    setValue("initial_saturation", initial_saturation);
    setValue("initial_gamma", initial_gamma);

    setValue("color_key", QString::number(color_key, 16));

    setValue("osd_level", osd_level);
    setValue("osd_scale", osd_scale);
    setValue("subfont_osd_scale", subfont_osd_scale);
    endGroup();


    beginGroup("audio");

    setValue("initial_audio_channels", initial_audio_channels);
    setValue("use_hwac3", use_hwac3);
    setValue("use_audio_equalizer", use_audio_equalizer);
    setValue("use_scaletempo", use_scaletempo);

    setValue("initial_stereo_mode", initial_stereo_mode);

    setValue("global_volume", global_volume);
    setValue("initial_volume", initial_volume);

    setValue("volume", volume);
    setValue("mute", mute);

    setValue("global_audio_equalizer", global_audio_equalizer);
    setValue("initial_audio_equalizer", initial_audio_equalizer);
    setValue("audio_equalizer", audio_equalizer);

    setValue("initial_volnorm", initial_volnorm);

    setValue("autosync", autosync);
    setValue("autosync_factor", autosync_factor);

    setValue("use_mc", use_mc);
    setValue("mc_value", mc_value);

    setValue("audio_lang", audio_lang);

    setValue("autoload_m4a", autoload_m4a);
    setValue("min_step", min_step);
    endGroup();


    // Subtitles
    beginGroup("subtitles");

    setValue("subtitle_fuzziness", subtitle_fuzziness);
    setValue("subtitle_language", subtitle_language);
    setValue("select_first_subtitle", select_first_subtitle);

    setValue("subtitle_enca_language", subtitle_enca_language);
    setValue("subtitle_encoding_fallback", subtitle_encoding_fallback);

    setValue("freetype_support", freetype_support);
    setValue("use_ass_subtitles", use_ass_subtitles);
    setValue("initial_sub_scale_ass", initial_sub_scale_ass);
    setValue("ass_line_spacing", ass_line_spacing);

    setValue("initial_sub_pos", initial_sub_pos);
    setValue("initial_sub_scale", initial_sub_scale);
    setValue("initial_sub_scale_mpv", initial_sub_scale_mpv);

    // ASS styles
    setValue("use_custom_ass_style", use_custom_ass_style);
    ass_styles.save(this);
    setValue("force_ass_styles", force_ass_styles);

    setValue("user_forced_ass_style", user_forced_ass_style);
    setValue("use_forced_subs_only", use_forced_subs_only);

    endGroup(); // subtitles


    // Interface
    beginGroup("interface");
    setValue("language", language);
    setValue("iconset", iconset);
    setValue("style", style);

    setValue("use_single_window", use_single_window);
    setValue("default_size", default_size);
    setValue("save_window_size_on_exit", save_window_size_on_exit);
    setValue("resize_on_load", resize_on_load);
    setValue("hide_video_window_on_audio_files",
             hide_video_window_on_audio_files);
    setValue("pause_when_hidden", pause_when_hidden);
    setValue("close_on_finish", close_on_finish);

    setValue("stay_on_top", (int) stay_on_top);
    setValue("size_factor", size_factor);

    setValue("hide_delay", floating_hide_delay);
    setValue("start_in_fullscreen", start_in_fullscreen);
    endGroup();


    beginGroup("log");
    setValue("log_verbose", log_verbose);
    setValue("log_level", log_level.toString());
    setValue("log_window_max_events", log_window_max_events);
    endGroup();


    beginGroup("history");
    setValue("recents", history_recents);
    setValue("recents/max_items", history_recents.getMaxItems());
    setValue("urls", history_urls);
    setValue("urls/max_items", history_urls.getMaxItems());

    setValue("save_dirs", save_dirs);

    if (save_dirs) {
        setValue("last_dir", last_dir);
        setValue("last_iso", last_iso);
        setValue("last_dvd_directory", last_dvd_directory);
    } else {
        setValue("last_dir", "");
        setValue("last_iso", "");
        setValue("last_dvd_directory", "");
    }

    setValue("last_dvb_channel", last_dvb_channel);
    setValue("last_tv_channel", last_tv_channel);

    setValue("last_clipboard", last_clipboard);
    endGroup(); // history


    beginGroup("playlist");
    setValue("add_directories", addDirectories);
    setValue("add_video", addVideo);
    setValue("add_audio", addAudio);
    setValue("add_playlists", addPlaylists);
    setValue("add_images", addImages);
    setValue("image_duration", imageDuration);
    setValue("use_directorie_playlists", useDirectoriePlaylists);
    setValue("name_blacklist", nameBlacklist);
    setValue("title_blacklist", titleBlacklist);
    endGroup();


    beginGroup("mouse");

    setValue("mouse_left_click_function", mouse_left_click_function);
    setValue("delay_left_click", delay_left_click);
    setValue("mouse_right_click_function", mouse_right_click_function);
    setValue("mouse_double_click_function", mouse_double_click_function);
    setValue("mouse_middle_click_function", mouse_middle_click_function);
    setValue("mouse_xbutton1_click_function", mouse_xbutton1_click_function);
    setValue("mouse_xbutton2_click_function", mouse_xbutton2_click_function);
    setValue("mouse_wheel_function", wheel_function);

    setValue("wheel_function_cycle", (int) wheel_function_cycle);
    setValue("wheel_function_seeking_reverse", wheel_function_seeking_reverse);
    endGroup();

    beginGroup("seeking");
    setValue("seeking1", seeking1);
    setValue("seeking2", seeking2);
    setValue("seeking3", seeking3);
    setValue("seeking4", seeking4);
    setValue("seeking_current_action", seeking_current_action);

    setValue("seek_rate", seek_rate);
    setValue("seek_relative", seek_relative);
    setValue("seek_keyframes", seek_keyframes);
    setValue("seek_preview", seek_preview);
    endGroup();


    // Drives (CD/DVD)
    beginGroup("drives");
    setValue("cdrom_device", cdrom_device);
    setValue("vcd_initial_title", vcd_initial_title);
    setValue("dvd_device", dvd_device);
    setValue("use_dvdnav", use_dvdnav);
    setValue("bluray_device", bluray_device);
    endGroup(); // drives


    // Capture
    beginGroup("capture");
    setValue("screenshot_directory", screenshot_directory);
    setValue("use_screenshot", use_screenshot);
    setValue("subtitles_on_screenshots", subtitles_on_screenshots);
    endGroup();


    // Performance
    beginGroup("performance");
    setValue("cache_enabled", cache_enabled);
    setValue("cache_for_files", cache_for_files);
    setValue("cache_for_streams", cache_for_streams);
    setValue("cache_for_tv", cache_for_tv);
    setValue("cache_for_brs", cache_for_brs);
    setValue("cache_for_dvds", cache_for_dvds);
    setValue("cache_for_vcds", cache_for_vcds);
    setValue("cache_for_audiocds", cache_for_audiocds);
    endGroup(); // performance


    // Network
    beginGroup("network");
    setValue("ip_prefer", ipPrefer);

    beginGroup("proxy");
    setValue("use_proxy", use_proxy);
    setValue("type", proxy_type);
    setValue("host", proxy_host);
    setValue("port", proxy_port);
    setValue("username", proxy_username);
    setValue("password", proxy_password);
    endGroup(); // proxy
    endGroup();


    update_checker_data.save(this);


    // Advanced
    beginGroup("advanced");
    setValue("actions_to_run", actions_to_run);

    setValue("player_additional_video_filters",
             player_additional_video_filters);
    setValue("player_additional_audio_filters",
             player_additional_audio_filters);

    setValue("use_edl_files", use_edl_files);
    setValue("time_to_kill_player", time_to_kill_player);
    setValue("balloon_count", balloon_count);
    setValue("change_video_equalizer_on_startup",
             change_video_equalizer_on_startup);

    endGroup(); // advanced

    filters.save(this);

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

    if (player_id == ID_MPLAYER) {
        return "MPlayer";
    }
    return "MPV";
}

QString TPreferences::playerIDToString(TPlayerID pid) {

    if (pid == ID_MPLAYER) {
        return "mplayer";
    }
    return "mpv";
}

QString TPreferences::getAbsolutePathPlayer(const QString& player) {

    QString path = player;
    QString found_player = TWZFiles::findExecutable(path);
    if (!found_player.isEmpty()) {
        path = found_player;
    }
    WZD << path;
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

    QString found_bin = TWZFiles::findExecutable(bin);

    // Try to find an alternative if not found
    if (found_bin.isEmpty()) {
        TPlayerID found_id = ID_MPLAYER;
        QFileInfo fi(bin);
        if (wanted_player == ID_MPV || fi.baseName().startsWith("mpv")) {
            // Try default mpv first
            if (bin != default_mpv_bin) {
                found_bin = TWZFiles::findExecutable(default_mpv_bin);
                if (!found_bin.isEmpty()) {
                    found_id = ID_MPV;
                }
            }
            if (found_bin.isEmpty()) {
                if (bin != default_mplayer_bin
                    && (allow_other_player || wanted_player == ID_MPLAYER)) {
                    // Try default mplayer
                    found_bin = TWZFiles::findExecutable(default_mplayer_bin);
                }
            } else {
                found_id = ID_MPV;
            }
        } else {
            // Try default mplayer
            if (bin != default_mplayer_bin
                && (allow_other_player || wanted_player == ID_MPLAYER)) {
                found_bin = TWZFiles::findExecutable(default_mplayer_bin);
            }
            if (found_bin.isEmpty()
                && bin != default_mpv_bin
                && (allow_other_player || wanted_player == ID_MPV)) {
                // Try default mpv
                found_bin = TWZFiles::findExecutable(default_mpv_bin);
                if (!found_bin.isEmpty()) {
                    found_id = ID_MPV;
                }
            }
        }

        if (found_bin.isEmpty()) {
            WZW << "Failed to find player" << bin;
        } else if (allow_other_player || found_id == wanted_player) {
            WZW << "Failed to find player" << bin << "selecting" << found_bin;
            bin = found_bin;
        } else {
            WZWARN("Failed to find player \"" + bin
                   + "\". Maybe try \"" + found_bin + "\" instead.");
        }
    } else {
        bin = found_bin;
    }

    player_bin = bin;
    setPlayerID();

    // Store player and set drivers for player
    if (!found_bin.isEmpty()) {
        if (player_id == ID_MPLAYER) {
            mplayer_bin = player_bin;
            vo = mplayer_vo;
            ao = mplayer_ao;
            player_additional_options = mplayer_additional_options;
        } else {
            mpv_bin = player_bin;
            vo = mpv_vo;
            ao = mpv_ao;
            player_additional_options = mpv_additional_options;
        }
    }

    WZI << "Selected player" << bin;
    WZT << "mplayer video driver vo" << mplayer_vo
        << "audio driver ao" << mplayer_ao
        << "additional options" << mplayer_additional_options;
    WZT << "mpv video driver vo" << mpv_vo
        << "audio driver ao" << mpv_ao
        << "additional options" << mpv_additional_options;
}

void TPreferences::setAction(QString& action, const QString& name) {
    action = value(name, action).toString();
}

void TPreferences::load() {

    config_version = value("config_version", 0).toInt();

    // Log
    beginGroup("log");
    log_level = Log4Qt::Level::fromString(
        value("log_level", log_level.toString()).toString());
    if (log_level < Log4Qt::Level::TRACE_INT) {
        log_level = log_default_level;
    }
    log_verbose = value("log_verbose", log_verbose).toBool();
    log_window_max_events = value("log_window_max_events",
                                  log_window_max_events).toInt();
    if (log_window_max_events < 10) {
        log_window_max_events = 10;
    } else if (log_window_max_events > 100000) {
        log_window_max_events = 100000;
    }
    endGroup(); // Log

    // Update Log4Qt. Command line options --loglevel override log level.
    if (log_override) {
        WZINFO(QString("Log level %1 overriden by command line level %2")
               .arg(log_level.toString())
               .arg(Log4Qt::LogManager::rootLogger()->level().toString()));
    } else {
        Log4Qt::LogManager::rootLogger()->setLevel(log_level);
        Log4Qt::LogManager::qtLogger()->setLevel(log_level);
        WZINFO("Log level set to " + log_level.toString());
    }

    // Players
    beginGroup("players");

    beginGroup("mplayer");
    mplayer_bin = value("bin", mplayer_bin).toString();
    mplayer_vo  = value("vo", mplayer_vo).toString();
    mplayer_ao = value("ao", mplayer_ao).toString();
    mplayer_additional_options = value("options",
                                       mplayer_additional_options).toString();
    endGroup();

    beginGroup("mpv");
    mpv_bin = value("bin", mpv_bin).toString();
    mpv_vo = value("vo", mpv_vo).toString();
    mpv_ao = value("ao", mpv_ao).toString();
    hwdec = value("hwdec", hwdec).toString();
    screenshot_template = value("screenshot_template", screenshot_template)
                          .toString();
    screenshot_format = value("screenshot_format", screenshot_format)
                        .toString();
    mpv_additional_options = value("options", mpv_additional_options)
                             .toString();
    endGroup();

    setPlayerBin(value("player_bin", player_bin).toString(), true, player_id);

    report_player_crashes = value("report_player_crashes",
                                  report_player_crashes).toBool();

    // Media settings per file
    remember_media_settings = value("remember_media_settings",
                                    remember_media_settings).toBool();
    remember_time_pos = value("remember_time_pos", remember_time_pos).toBool();
    file_settings_method = value("file_settings_method",
                                 file_settings_method).toString();

    endGroup(); // players


    // Demuxer
    beginGroup("demuxer");
    use_lavf_demuxer = value("use_lavf_demuxer", use_lavf_demuxer).toBool();
    use_idx = value("use_idx", use_idx).toBool();
    endGroup();


    // Video
    beginGroup("video");

#ifndef Q_OS_WIN
    vdpau.ffh264vdpau = value("vdpau_ffh264vdpau", vdpau.ffh264vdpau).toBool();
    vdpau.ffmpeg12vdpau = value("vdpau_ffmpeg12vdpau", vdpau.ffmpeg12vdpau)
                          .toBool();
    vdpau.ffwmv3vdpau = value("vdpau_ffwmv3vdpau", vdpau.ffwmv3vdpau).toBool();
    vdpau.ffvc1vdpau = value("vdpau_ffvc1vdpau", vdpau.ffvc1vdpau).toBool();
    vdpau.ffodivxvdpau = value("vdpau_ffodivxvdpau", vdpau.ffodivxvdpau)
                         .toBool();
    vdpau.disable_video_filters = value("vdpau_disable_video_filters",
                                        vdpau.disable_video_filters).toBool();
#endif

    use_soft_video_eq = value("use_soft_video_eq", use_soft_video_eq).toBool();

    frame_drop = value("frame_drop", frame_drop).toBool();
    hard_frame_drop = value("hard_frame_drop", hard_frame_drop).toBool();
    use_correct_pts = (TOptionState) value("correct_pts", use_correct_pts)
                      .toInt();

    initial_postprocessing = value("initial_postprocessing",
                                   initial_postprocessing).toBool();
    postprocessing_quality = value("postprocessing_quality",
                                   postprocessing_quality).toInt();
    initial_deinterlace = value("initial_deinterlace",
                                initial_deinterlace).toInt();
    initial_tv_deinterlace = value("initial_tv_deinterlace",
                                   initial_tv_deinterlace).toInt();
    initial_zoom_factor = value("initial_zoom_factor", initial_zoom_factor)
                          .toDouble();

    monitor_aspect = value("monitor_aspect", monitor_aspect).toString();

    initial_contrast = value("initial_contrast", initial_contrast).toInt();
    initial_brightness = value("initial_brightness", initial_brightness)
                         .toInt();
    initial_hue = value("initial_hue", initial_hue).toInt();
    initial_saturation = value("initial_saturation", initial_saturation)
                         .toInt();
    initial_gamma = value("initial_gamma", initial_gamma).toInt();

    bool ok;
    QString color = value("color_key", QString::number(color_key, 16))
                    .toString();
    unsigned int temp_color_key = color.toUInt(&ok, 16);
    if (ok) {
        color_key = temp_color_key;
    } else {
        logger()->warn("load: failed to parse color key '" + color + "'");
    }

    // OSD
    osd_level = (TOSDLevel) value("osd_level", (int) osd_level).toInt();
    osd_scale = value("osd_scale", osd_scale).toDouble();
    subfont_osd_scale = value("subfont_osd_scale", subfont_osd_scale)
                        .toDouble();
    endGroup();

    // Audio tab
    beginGroup("audio");
    initial_audio_channels = value("initial_audio_channels",
                                   initial_audio_channels).toInt();
    use_hwac3 = value("use_hwac3", use_hwac3).toBool();
    use_audio_equalizer = value("use_audio_equalizer", use_audio_equalizer)
                          .toBool();
    use_scaletempo = (TOptionState) value("use_scaletempo", use_scaletempo)
                     .toInt();

    initial_stereo_mode = value("initial_stereo_mode", initial_stereo_mode)
                          .toInt();

    global_volume = value("global_volume", global_volume).toBool();
    initial_volume = value("initial_volume", initial_volume).toInt();
    if (initial_volume < 0) initial_volume = 0;
    if (initial_volume > 100) initial_volume = 100;

    volume = value("volume", volume).toInt();
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    mute = value("mute", mute).toBool();

    global_audio_equalizer = value("global_audio_equalizer",
                                   global_audio_equalizer).toBool();
    initial_audio_equalizer = value("initial_audio_equalizer",
                                    initial_audio_equalizer).toList();
    audio_equalizer = value("audio_equalizer", audio_equalizer).toList();

    initial_volnorm = value("initial_volnorm", initial_volnorm).toBool();

    autosync = value("autosync", autosync).toBool();
    autosync_factor = value("autosync_factor", autosync_factor).toInt();

    use_mc = value("use_mc", use_mc).toBool();
    mc_value = value("mc_value", mc_value).toDouble();

    audio_lang = value("audio_lang", audio_lang).toString();

    autoload_m4a = value("autoload_m4a", autoload_m4a).toBool();
    min_step = value("min_step", min_step).toInt();
    endGroup();


    // Subtitles
    beginGroup("subtitles");

    subtitle_fuzziness = value("subtitle_fuzziness", subtitle_fuzziness)
                         .toInt();
    subtitle_language = value("subtitle_language", subtitle_language)
                        .toString();
    select_first_subtitle = value("select_first_subtitle",
                                  select_first_subtitle).toBool();

    subtitle_enca_language = value("subtitle_enca_language",
                                   subtitle_enca_language).toString();
    subtitle_encoding_fallback = value("subtitle_encoding_fallback",
                                       subtitle_encoding_fallback).toString();

    freetype_support = value("freetype_support", freetype_support).toBool();
    use_ass_subtitles = value("use_ass_subtitles", use_ass_subtitles).toBool();
    initial_sub_scale_ass = value("initial_sub_scale_ass",
                                  initial_sub_scale_ass).toDouble();
    ass_line_spacing = value("ass_line_spacing", ass_line_spacing).toInt();

    initial_sub_pos = value("initial_sub_pos", initial_sub_pos).toInt();
    initial_sub_scale = value("initial_sub_scale", initial_sub_scale)
                        .toDouble();
    initial_sub_scale_mpv = value("initial_sub_scale_mpv",
                                  initial_sub_scale_mpv).toDouble();

    use_custom_ass_style = value("use_custom_ass_style", use_custom_ass_style)
                           .toBool();
    // ASS styles
    ass_styles.load(this);

    force_ass_styles = value("force_ass_styles", force_ass_styles).toBool();
    user_forced_ass_style = value("user_forced_ass_style",
                                  user_forced_ass_style).toString();

    use_forced_subs_only = value("use_forced_subs_only", use_forced_subs_only)
                           .toBool();

    endGroup(); // subtitles


    beginGroup("interface");
    language = value("language", language).toString();
    iconset= value("iconset", iconset).toString();
    style = value("style", style).toString();

    use_single_window = value("use_single_window", use_single_window).toBool();
    default_size = value("default_size", default_size).toSize();
    save_window_size_on_exit = value("save_window_size_on_exit",
                                     save_window_size_on_exit).toBool();
    resize_on_load = value("resize_on_load", resize_on_load).toBool();
    hide_video_window_on_audio_files = value("hide_video_window_on_audio_files",
                                             hide_video_window_on_audio_files)
                                       .toBool();
    pause_when_hidden = value("pause_when_hidden", pause_when_hidden).toBool();
    close_on_finish = value("close_on_finish", close_on_finish).toBool();

    stay_on_top = (TPreferences::TOnTop) value("stay_on_top",
                                               (int) stay_on_top).toInt();
    size_factor = value("size_factor", size_factor).toDouble();

    floating_hide_delay = value("hide_delay", floating_hide_delay).toInt();
    start_in_fullscreen = value("start_in_fullscreen", start_in_fullscreen)
                          .toBool();

    endGroup();


    beginGroup("history");
    history_recents.setMaxItems(value("recents/max_items",
                                      history_recents.getMaxItems()).toInt());
    history_recents.fromStringList(value("recents", history_recents)
                                   .toStringList());

    history_urls.setMaxItems(value("urls/max_items", history_urls.getMaxItems())
                             .toInt());
    history_urls.fromStringList(value("urls", history_urls).toStringList());

    save_dirs = value("save_dirs", save_dirs).toBool();
    if (save_dirs) {
        last_dir = value("last_dir", last_dir).toString();
        last_iso = value("last_iso", last_iso).toString();
        last_dvd_directory = value("last_dvd_directory", last_dvd_directory)
                             .toString();
    }

    last_dvb_channel = value("last_dvb_channel", last_dvb_channel).toString();
    last_tv_channel = value("last_tv_channel", last_tv_channel).toString();

    last_clipboard = value("last_clipboard", last_clipboard).toString();
    endGroup(); // history


    beginGroup("playlist");
    addDirectories = value("add_directories", addDirectories).toBool();
    addVideo = value("add_video", addVideo).toBool();
    addAudio = value("add_audio", addAudio).toBool();
    addPlaylists = value("add_playlists", addPlaylists).toBool();
    addImages = value("add_images", addImages).toBool();

    imageDuration = value("image_duration", imageDuration).toInt();

    useDirectoriePlaylists = value("use_directorie_playlists",
                                   useDirectoriePlaylists).toBool();

    nameBlacklist = value("name_blacklist", nameBlacklist).toStringList();
    titleBlacklist = value("title_blacklist", titleBlacklist).toStringList();
    endGroup();
    compileTitleBlackList();


    beginGroup("mouse");
    setAction(mouse_left_click_function, "mouse_left_click_function");
    delay_left_click = value("delay_left_click", delay_left_click).toBool();
    setAction(mouse_right_click_function, "mouse_right_click_function");
    setAction(mouse_double_click_function, "mouse_double_click_function");
    setAction(mouse_middle_click_function, "mouse_middle_click_function");
    setAction(mouse_xbutton1_click_function, "mouse_xbutton1_click_function");
    setAction(mouse_xbutton2_click_function, "mouse_xbutton2_click_function");
    wheel_function = value("mouse_wheel_function", wheel_function).toInt();
    {
        int wheel_function_cycle_int = value("wheel_function_cycle",
                                             (int) wheel_function_cycle)
                                       .toInt();
        wheel_function_cycle = (TWheelFunctions) wheel_function_cycle_int;
    }
    wheel_function_seeking_reverse = value("wheel_function_seeking_reverse",
                                           wheel_function_seeking_reverse)
                                     .toBool();
    endGroup();

    beginGroup("seeking");
    seeking1 = value("seeking1", seeking1).toInt();
    seeking2 = value("seeking2", seeking2).toInt();
    seeking3 = value("seeking3", seeking3).toInt();
    seeking4 = value("seeking4", seeking4).toInt();
    seeking_current_action = value("seeking_current_action",
                                   seeking_current_action).toInt();

    seek_rate = value("seek_rate", seek_rate).toInt();
    seek_relative = value("seek_relative", seek_relative).toBool();
    seek_keyframes = value("seek_keyframes", seek_keyframes).toBool();
    seek_preview = value("seek_preview", seek_preview).toBool();
    endGroup();


    // Drives (CD/DVD)
    beginGroup("drives");
    cdrom_device = value("cdrom_device", cdrom_device).toString();
    vcd_initial_title = value("vcd_initial_title", vcd_initial_title).toInt();
    dvd_device = value("dvd_device", dvd_device).toString();
    use_dvdnav = value("use_dvdnav", use_dvdnav).toBool();
    bluray_device = value("bluray_device", bluray_device).toString();
    endGroup(); // drives


    // Capture
    beginGroup("capture");
    screenshot_directory = value("screenshot_directory", screenshot_directory)
                           .toString();

    use_screenshot = value("use_screenshot", use_screenshot).toBool();
    subtitles_on_screenshots = value("subtitles_on_screenshots",
                                     subtitles_on_screenshots).toBool();
    endGroup();
    setupScreenshotFolder();


    // Performance
    beginGroup("performance");
    cache_enabled = value("cache_enabled", cache_enabled).toBool();
    cache_for_files = value("cache_for_files", cache_for_files).toInt();
    cache_for_streams = value("cache_for_streams", cache_for_streams).toInt();
    cache_for_tv = value("cache_for_tv", cache_for_tv).toInt();
    cache_for_brs = value("cache_for_brs", cache_for_brs).toInt();
    cache_for_dvds = value("cache_for_dvds", cache_for_dvds).toInt();
    cache_for_vcds = value("cache_for_vcds", cache_for_vcds).toInt();
    cache_for_audiocds = value("cache_for_audiocds", cache_for_audiocds)
                         .toInt();
    endGroup(); // performance


    // Network
    beginGroup("network");

    int i = value("ip_prefer", ipPrefer).toInt();
    if (i == IP_PREFER_4 || i == IP_PREFER_6) {
        ipPrefer = static_cast<TIPPrefer>(i);
    } else {
        ipPrefer = IP_PREFER_AUTO;
    }

    beginGroup("proxy");
    use_proxy = value("use_proxy", use_proxy).toBool();
    proxy_type = value("type", proxy_type).toInt();
    proxy_host = value("host", proxy_host).toString();
    proxy_port = value("port", proxy_port).toInt();
    proxy_username = value("username", proxy_username).toString();
    proxy_password = value("password", proxy_password).toString();
    endGroup(); // proxy
    endGroup();


    // Update
    update_checker_data.load(this);


    // Advanced
    beginGroup("advanced");
    actions_to_run = value("actions_to_run", actions_to_run).toString();
    // player_additional_options already done
    player_additional_video_filters = value("player_additional_video_filters",
        player_additional_video_filters).toString();
    player_additional_audio_filters = value("player_additional_audio_filters",
        player_additional_audio_filters).toString();

    use_edl_files = value("use_edl_files", use_edl_files).toBool();
    time_to_kill_player = value("time_to_kill_player", time_to_kill_player)
                          .toInt();
    if (time_to_kill_player < 3000) {
        time_to_kill_player = 3000;
    }
    balloon_count = value("balloon_count", balloon_count).toInt();
    change_video_equalizer_on_startup = value(
        "change_video_equalizer_on_startup", change_video_equalizer_on_startup)
        .toBool();

    endGroup(); // advanced

    filters.load(this);


    WZI << "Loaded configuration file version" << config_version
        << "executable CURRENT_CONFIG_VERSION is" << CURRENT_CONFIG_VERSION;

    // Check config version
    if (config_version < CURRENT_CONFIG_VERSION) {
        if (config_version > 0) {
            WZINFO("Config is old, updating it");
        }
        clean_config = true;
        config_version = CURRENT_CONFIG_VERSION;
    }
} // load()

void TPreferences::clearRecents() {

    history_recents.clear();
    emit recentsChanged();
}

void TPreferences::addRecent(const QString& url, const QString& title) {

    history_recents.addRecent(url, title);
    emit recentsChanged();
}

bool TPreferences::useColorKey() const {

#if defined(Q_OS_WIN)
    return vo.startsWith("directx");
#else
    return false;
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
        WZINFO(QString("Monitor aspect set to %1:%2").arg(w).arg(h));
        return h <= 0 ? 0 : (double) w / h;
    }

    bool ok;
    double res = monitor_aspect.toDouble(&ok);
    if (ok) {
        WZINFO("Monitor aspect ratio set to " + QString::number(res));
        return res;
    }

    WZWARN("Failed to parse monitor aspect ratio, reset to auto detect");
    return 0;
}

void TPreferences::setupScreenshotFolder() {

    if (screenshot_directory.isEmpty()) {
        QString pdir = TPaths::location(QStandardPaths::PicturesLocation);
        if (pdir.isEmpty()) {
            WZTRACE("No pictures location");
            pdir = TPaths::location(QStandardPaths::DocumentsLocation);
        }
        if (pdir.isEmpty()) {
            WZTRACE("No documents location");
            pdir = TPaths::location(QStandardPaths::HomeLocation);
        }
        if (pdir.isEmpty()) {
            WZTRACE("No home location");
            pdir = "/tmp";
        }
        screenshot_directory = QDir::toNativeSeparators(pdir + "/screenshots");
    } else {
        screenshot_directory = QDir::toNativeSeparators(screenshot_directory);
    }

    if (QDir(screenshot_directory).exists()) {
        WZT << "Using folder" << screenshot_directory;
    } else {
        WZTRACE(QString("Folder '%1' not found, disabling screenshots")
               .arg(screenshot_directory));
        use_screenshot = false;
        screenshot_directory = "";
    }
}

void TPreferences::compileTitleBlackList() {

    rxTitleBlacklist.clear();
    QRegExp rx("", Qt::CaseInsensitive);
    for(int i = titleBlacklist.count() - 1; i >= 0; i--) {
        const QString& s = titleBlacklist.at(i);
        if (!s.isEmpty()) {
            rx.setPattern(s);
            if (rx.isValid()) {
                WZT << "Adding" << rx.pattern();
                rxTitleBlacklist.append(rx);
            } else {
                WZW << "Failed to parse regular expression" << s;
            }
        }
    }
}


} // namespace Settings

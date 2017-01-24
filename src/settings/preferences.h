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


#ifndef SETTINGS_PREFERENCES_H
#define SETTINGS_PREFERENCES_H

#include <QSize>
#include <QString>
#include <QSettings>

#include "log4qt/level.h"
#include "filters.h"
#include "settings/updatecheckerdata.h"
#include "settings/assstyles.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"


namespace Settings {

typedef QList<QVariant> TAudioEqualizerList;


class TPreferences : public QSettings {
public:
    enum TPlayerID { ID_MPLAYER = 0, ID_MPV = 1 };
    enum TOSDLevel { None = 0, Seek = 1, SeekTimer = 2, SeekTimerTotal = 3 };
    enum TOnTop { NeverOnTop = 0, AlwaysOnTop = 1, WhilePlayingOnTop = 2 };

    enum TWheelFunction {
        DoNothing = 1,
        Seeking = 2,
        Volume = 4,
        Zoom = 8,
        ChangeSpeed = 16
    };
    Q_DECLARE_FLAGS(TWheelFunctions, TWheelFunction)

    enum TOptionState { Detect = -1, Disabled = 0, Enabled = 1 };
    enum TAddToPlaylist {
        NoFiles = 0,
        VideoFiles = 1,
        AudioFiles = 2,
        MultimediaFiles = 3,
        ConsecutiveFiles = 4
    };

    enum TIPPrefer {
        IP_PREFER_AUTO,
        IP_PREFER_4,
        IP_PREFER_6
    };


    TPreferences();
    virtual ~TPreferences();

    void reset();

    void load();
    void save();

    // Version config file
    int config_version;

    // Player tab
    TPlayerID player_id;
    QString player_bin;

    // Mplayer
    QString mplayer_bin;
    QString mplayer_vo;
    QString mplayer_ao;

    // MPV
    QString mpv_bin;
    QString mpv_vo;
    QString mpv_ao;

    bool report_player_crashes;

    bool isMPlayer() const { return player_id == ID_MPLAYER; }
    bool isMPV() const { return player_id == ID_MPV; }

    QString playerName() const;

    static QString getAbsolutePathPlayer(const QString& player);
    static TPlayerID getPlayerID(const QString& player);
    static QString playerIDToString(TPlayerID pid);

    void setPlayerBin(QString bin, bool allow_other_player, TPlayerID wanted_player);

    // Media settings per file
    bool remember_media_settings;
    bool remember_time_pos;
    QString file_settings_method; //!< Method to be used for saving file settings

    // Check radio and TV channels on startup
    bool check_channels_conf_on_startup;


    // Demuxer tab
    bool use_lavf_demuxer;
    bool use_idx;


    // Video tab
    QString vo; // video output

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

    QString hwdec; //!< hardware video decoding (mpv only)
    bool use_soft_video_eq;

    // Sync
    bool frame_drop;
    bool hard_frame_drop;
    TOptionState use_correct_pts; //!< Pass -correct-pts to mplayer

    // Defaults
    bool initial_postprocessing;
    int postprocessing_quality;
    int initial_deinterlace;
    int initial_tv_deinterlace;
    double initial_zoom_factor; // Default value for zoom (1.0 = no zoom)

    // Monitor
    QString monitor_aspect;
    double monitorAspectDouble();

    int initial_contrast;
    int initial_brightness;
    int initial_hue;
    int initial_saturation;
    int initial_gamma;

    unsigned int color_key;
    bool useColorKey() const;

    // OSD
    TOSDLevel osd_level;
    double osd_scale; // mpv
    double subfont_osd_scale; // mplayer


    // Audio section
    QString ao;
    int initial_audio_channels;
    bool use_hwac3; // -afm hwac3
    bool use_audio_equalizer;
    TOptionState use_scaletempo;

    int initial_stereo_mode;

    // Global volume options
    bool global_volume;
    int initial_volume;

    int volume;
    bool mute;

    // Global equalizer
    bool global_audio_equalizer;
    TAudioEqualizerList initial_audio_equalizer;
    TAudioEqualizerList audio_equalizer;


    // Volume group
    bool initial_volnorm;

    // Synchronization group
    bool autosync;
    int autosync_factor;

    // For the -mc option
    bool use_mc;
    double mc_value;

    QString audio_lang; // Preferred audio language

    // When playing a mp4 file, it will use a m4a file for audio if a there's a file with same name but extension m4a
    bool autoload_m4a;

    // Step to increase or decrease the controls for volume, color, contrast,
    // brightness and so on
    int min_step;

    // Subtitles section
    int subtitle_fuzziness;
    QString subtitle_language; // Preferred subtitle language
    bool select_first_subtitle;

    QString subtitle_enca_language;
    QString subtitle_encoding_fallback;

    //! If false, options requiring freetype won't be used
#ifdef Q_OS_WIN
    bool use_windowsfontdir;
#endif

    // Libraries tab
    bool freetype_support;
    bool use_ass_subtitles;
    double initial_sub_scale_ass;
    int ass_line_spacing;

    //! Default value for position of subtitles on screen
    //! 100 = 100% at the bottom
    int initial_sub_pos;
    double initial_sub_scale;
    double initial_sub_scale_mpv;

    // ASS styles
    bool use_custom_ass_style;
    TAssStyles ass_styles;

    bool force_ass_styles; // Use ass styles even for ass files
    QString user_forced_ass_style; //!< Specifies a style defined by the user to be used with -ass-force-style

    bool use_forced_subs_only;


    // Interface section
    QString language;
    QString iconset;
    QString style;

    // Mainwindow
    bool use_single_window;
    QSize default_size;
    bool save_window_size_on_exit;
    bool resize_on_load;
    bool hide_video_window_on_audio_files;
    //!< Pause the current file when the main window is not visible
    bool pause_when_hidden;
    //! Close the main window when a file or playlist finish
    bool close_on_finish;

    TOnTop stay_on_top;
    double size_factor;


    // Fullscreen
    bool fullscreen;
    int floating_hide_delay;
    bool start_in_fullscreen;

    // Logging
    bool log_verbose;
    Log4Qt::Level log_level;
    int log_window_max_events;

    // History
    TRecents history_recents;
    TURLHistory history_urls;
    bool save_dirs; // Save or not the latest dirs

    QString latest_dir; //!< Directory of the latest file loaded
    QString last_dvd_directory;

    // TV (dvb)
    QString last_dvb_channel;
    QString last_tv_channel;


    // Playlist section
    TAddToPlaylist mediaToAddToPlaylist;

    bool addDirectories;
    bool addVideo;
    bool addAudio;
    bool addPlaylists;
    bool addImages;

    int imageDuration;

    bool useDirectoriePlaylists;

    QStringList nameBlacklist;
    QStringList titleBlacklist;
    QList<QRegExp*> rxTitleBlacklist;
    void setTitleBlackList();


    // Actions section
    // Function of mouse buttons:
    QString mouse_left_click_function;
    //! If true, the left click in the video is delayed some ms
    //! to check if the user double clicked
    bool delay_left_click;
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
    int seeking_current_action;

    bool update_while_seeking;
    int time_slider_drag_delay;

    //! If true, seeking will be done using a
    //! percentage (with fractions) instead of time.
    bool relative_seeking;
    bool precise_seeking; //! Enable precise_seeking (only available with mplayer2)


    // Drives section
    QString cdrom_device;
    int vcd_initial_title;

    QString dvd_device;
    bool use_dvdnav; //!< Opens DVDs using dvdnav: instead of dvd:
    bool useDVDNAV() const { return isMPlayer() && use_dvdnav; }

    QString bluray_device;


    // Capture section
    QString screenshot_directory;
    bool use_screenshot;
    QString screenshot_template;
    QString screenshot_format;
    bool subtitles_on_screenshots;

    void setupScreenshotFolder();


    // Performance section
    bool cache_enabled;
    int cache_for_files;
    int cache_for_streams;
    int cache_for_tv;
    int cache_for_brs;
    int cache_for_dvds;
    int cache_for_vcds;
    int cache_for_audiocds;


    // Network section
    TIPPrefer ipPrefer;
    bool use_proxy;
    int proxy_type;
    QString proxy_host;
    int proxy_port;
    QString proxy_username;
    QString proxy_password;


    // Update section
    TUpdateCheckerData update_checker_data;


    // Advanced section
    QString actions_to_run; //!< List of actions to run every time a video loads.
    QString player_additional_options;
    QString mplayer_additional_options;
    QString mpv_additional_options;
    QString player_additional_video_filters;
    QString player_additional_audio_filters;

    //! If true it will autoload edl files with the same name of the file
    //! to play
    bool use_edl_files;
    int time_to_kill_player;
    bool show_frames;

    //! Number of times to show the balloon remembering that the program
    //! is still running in the system tray.
    int balloon_count;

    //! If false, -brightness, -contrast and so on, won't be passed to
    //! mplayer. It seems that some graphic cards don't support those options.
    bool change_video_equalizer_on_startup;

    // Filters
    TFilters filters;

    bool clean_config;

private:

    void setPlayerBin0(QString bin);
    void setPlayerID();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Settings::TPreferences::TWheelFunctions)

extern TPreferences* pref;

} // namespace Settings

#endif // SETTINGS_PREFERENCES_H

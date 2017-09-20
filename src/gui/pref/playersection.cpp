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


#include "gui/pref/playersection.h"
#include "log4qt/logmanager.h"
#include "gui/filedialog.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"
#include "images.h"
#include "wzdebug.h"


using namespace Settings;

namespace Gui {
namespace Pref {

TPlayerSection::TPlayerSection(QWidget* parent)
    : TSection(parent, 0) {

    setupUi(this);

    mplayer_edit->setDialogType(TFileChooser::GetFileName);
    mpv_edit->setDialogType(TFileChooser::GetFileName);

    connect(mplayer_edit, SIGNAL(fileChanged(QString)),
            this, SLOT(onMPlayerFileChanged(QString)));
    connect(mpv_edit, SIGNAL(fileChanged(QString)),
            this, SLOT(onMPVFileChanged(QString)));
    connect(mplayer_radio, SIGNAL(clicked(bool)),
            this, SLOT(onPlayerRadioClicked(bool)));
    connect(mpv_radio, SIGNAL(clicked(bool)),
            this, SLOT(onPlayerRadioClicked(bool)));

#ifdef Q_OS_WIN
    radio_tv_group->hide();
#endif

    retranslateStrings();
}

TPlayerSection::~TPlayerSection() {
}

QString TPlayerSection::sectionName() {
    return tr("Player");
}

QPixmap TPlayerSection::sectionIcon() {
    return Images::icon("pref_player", iconSize);
}

void TPlayerSection::retranslateStrings() {

    retranslateUi(this);

    // Player
    mplayer_icon_label->setPixmap(Images::icon("mplayer"));
    mpv_icon_label->setPixmap(Images::icon("mpv"));

    mplayer_edit->setCaption(tr("Select the MPlayer executable"));
    mpv_edit->setCaption(tr("Select the MPV executable"));

#if defined(Q_OS_WIN)
    mplayer_edit->setFilter(tr("Executables") +" (*.exe)");
    mpv_edit->setFilter(tr("Executables") +" (*.exe)");
#else
    mplayer_edit->setFilter(tr("All files") +" (*)");
    mpv_edit->setFilter(tr("All files") +" (*)");
#endif

    // Media settings
    int filesettings_method_item = filesettings_method_combo->currentIndex();
    filesettings_method_combo->clear();
    filesettings_method_combo->addItem(tr("one ini file"), "normal");
    filesettings_method_combo->addItem(tr("multiple ini files"), "hash");
    filesettings_method_combo->setCurrentIndex(filesettings_method_item);

    // Radio and TV
    radio_tv_icon_label->setPixmap(Images::icon("pref_radio_tv"));

    // Tab advanced
    advanced_icon_label->setPixmap(Images::icon("pref_advanced"));

    createHelp();
}

void TPlayerSection::setData(TPreferences* pref) {

    // Player
    setPlayerID(pref->player_id);
    setPlayerPath(pref->mplayer_bin, pref->mpv_bin);
    report_player_crashes_check->setChecked(pref->report_player_crashes);

    // Media settings group
    settings_group->setChecked(pref->remember_media_settings);
    remember_time_check->setChecked(pref->remember_time_pos);
    remember_volume_check->setChecked(!pref->global_volume);
    remember_audio_eq_check->setChecked(!pref->global_audio_equalizer);
    setFileSettingsMethod(pref->file_settings_method);

    // Radio and TV
    radio_tv_rescan_check->setChecked(pref->check_channels_conf_on_startup);

    // Advanced tab
    setPlayerAdditionalArguments(pref->player_additional_options);
    setPlayerAdditionalVideoFilters(pref->player_additional_video_filters);
    setPlayerAdditionalAudioFilters(pref->player_additional_audio_filters);
    setActionsToRun(pref->actions_to_run);

    // Log
    setLogLevel(Log4Qt::LogManager::rootLogger()->level());
    setLogVerbose(pref->log_verbose);
    log_window_max_events_spinbox->setValue(pref->log_window_max_events);
}

void TPlayerSection::getData(TPreferences* pref) {

    TSection::getData(pref);

    // Update player
    pref->mplayer_bin = mplayer_edit->text();
    pref->mpv_bin = mpv_edit->text();
    QString bin = mplayer_radio->isChecked()
                  ? pref->mplayer_bin
                  : pref->mpv_bin;
    if (bin != pref->player_bin) {
        WZDEBUG("player bin changed, restarting app");
        _requiresRestartApp = true;
        pref->setPlayerBin(bin, false,
            mplayer_radio->isChecked() ? TPreferences::ID_MPLAYER
                                       : TPreferences::ID_MPV);
    }

    pref->report_player_crashes = report_player_crashes_check->isChecked();

    // Media settings
    pref->remember_media_settings = settings_group->isChecked();
    pref->remember_time_pos = remember_time_check->isChecked();
    pref->global_volume = !pref->remember_media_settings
                          || !remember_volume_check->isChecked();
    pref->global_audio_equalizer = !pref->remember_media_settings
                                   || !remember_audio_eq_check->isChecked();
    pref->file_settings_method = fileSettingsMethod();

    // Radio and TV
    pref->check_channels_conf_on_startup = radio_tv_rescan_check->isChecked();

    // Advanced tab
    restartIfStringChanged(pref->player_additional_options,
                           playerAdditionalArguments(),
                           "player_additional_options");
    if (pref->isMPlayer()) {
        pref->mplayer_additional_options = pref->player_additional_options;
    } else {
        pref->mpv_additional_options = pref->player_additional_options;
    }
    restartIfStringChanged(pref->player_additional_video_filters,
                           playerAdditionalVideoFilters(),
                           "player_additional_video_filters");
    restartIfStringChanged(pref->player_additional_audio_filters,
                           playerAdditionalAudioFilters(),
                           "player_additional_audio_filters");
    pref->actions_to_run = actionsToRun();

    // Log tab
    pref->log_level = logLevel();
    Log4Qt::LogManager::rootLogger()->setLevel(pref->log_level);
    Log4Qt::LogManager::qtLogger()->setLevel(pref->log_level);
    restartIfBoolChanged(pref->log_verbose,
        pref->log_level <= Log4Qt::Level::DEBUG_INT && logVerbose(),
        "log_verbose");
    pref->log_window_max_events = log_window_max_events_spinbox->value();
}

void TPlayerSection::setPlayerID(Settings::TPreferences::TPlayerID id) {

    if (id == Settings::TPreferences::ID_MPLAYER) {
        mplayer_radio->setChecked(true);
        mpv_radio->setChecked(false);
    } else {
        mplayer_radio->setChecked(false);
        mpv_radio->setChecked(true);
    }
}

void TPlayerSection::setPlayerPath(const QString& mplayer, const QString& mpv) {

    mplayer_edit->setText(mplayer);
    mpv_edit->setText(mpv);
}

void TPlayerSection::onMPlayerFileChanged(QString file) {

    if (mplayer_radio->isChecked()) {
        emit binChanged(TPreferences::ID_MPLAYER, true, pref->getAbsolutePathPlayer(file));
    }
}

void TPlayerSection::onMPVFileChanged(QString file) {

    if (mpv_radio->isChecked()) {
        emit binChanged(TPreferences::ID_MPV, true, pref->getAbsolutePathPlayer(file));
    }
}

void TPlayerSection::onPlayerRadioClicked(bool) {

    TPreferences::TPlayerID player_id;
    QString file;
    if (mplayer_radio->isChecked()) {
        player_id = TPreferences::ID_MPLAYER;
        file = mplayer_edit->text();
    } else {
        player_id = TPreferences::ID_MPV;
        file = mpv_edit->text();
    }
    emit binChanged(player_id, false, pref->getAbsolutePathPlayer(file));
}

void TPlayerSection::setFileSettingsMethod(const QString& method) {

    int index = filesettings_method_combo->findData(method);
    if (index < 0)
        index = 0;
    filesettings_method_combo->setCurrentIndex(index);
}

QString TPlayerSection::fileSettingsMethod() {
    return filesettings_method_combo->itemData(
                filesettings_method_combo->currentIndex()).toString();
}

void TPlayerSection::setLogLevel(Log4Qt::Level level) {

    int idx;
    switch (level.toInt()) {
        case Log4Qt::Level::NULL_INT:
        case Log4Qt::Level::ALL_INT:
        case Log4Qt::Level::TRACE_INT: idx = 0; break;
        case Log4Qt::Level::DEBUG_INT: idx = 1; break;
        case Log4Qt::Level::INFO_INT: idx = 2; break;
        case Log4Qt::Level::WARN_INT: idx = 3; break;
        case Log4Qt::Level::ERROR_INT: idx = 4; break;
        case Log4Qt::Level::FATAL_INT: idx = 5; break;
        case Log4Qt::Level::OFF_INT: idx = 6; break;
        default: idx = 1;
    }

    log_level_combo->setCurrentIndex(idx);
}

Log4Qt::Level TPlayerSection::logLevel() {

    Log4Qt::Level level;
    switch (log_level_combo->currentIndex()) {
        case 0: level = Log4Qt::Level::TRACE_INT; break;
        case 1: level = Log4Qt::Level::DEBUG_INT; break;
        case 2: level = Log4Qt::Level::INFO_INT; break;
        case 3: level = Log4Qt::Level::WARN_INT;break;
        case 4: level = Log4Qt::Level::ERROR_INT; break;
        case 5: level = Log4Qt::Level::FATAL_INT; break;
        case 6: level = Log4Qt::Level::OFF_INT; break;
        default: level =Log4Qt::Level:: DEBUG_INT;
    }

    return level;
}

void TPlayerSection::setLogVerbose(bool b) {
    log_verbose_check->setChecked(b);
}

bool TPlayerSection::logVerbose() {
    return log_verbose_check->isChecked();
}

void TPlayerSection::setPlayerAdditionalArguments(QString args) {
    player_args_edit->setText(args);
}

QString TPlayerSection::playerAdditionalArguments() {
    return player_args_edit->text();
}

void TPlayerSection::setPlayerAdditionalVideoFilters(QString s) {
    player_vfilters_edit->setText(s);
}

QString TPlayerSection::playerAdditionalVideoFilters() {
    return player_vfilters_edit->text();
}

void TPlayerSection::setPlayerAdditionalAudioFilters(QString s) {
    player_afilters_edit->setText(s);
}

QString TPlayerSection::playerAdditionalAudioFilters() {
    return player_afilters_edit->text();
}

void TPlayerSection::setActionsToRun(QString actions) {
    actions_to_run_edit->setText(actions);
}

QString TPlayerSection::actionsToRun() {
    return actions_to_run_edit->text();
}

void TPlayerSection::createHelp() {

    clearHelp();

    addSectionTitle(tr("Media player"));

    setWhatsThis(mplayer_radio, tr("MPlayer"),
        tr("Select MPlayer as the media player to use by WZPlayer."));

    setWhatsThis(mplayer_edit, tr("MPlayer executable"),
        tr("The path to the MPlayer executable file.")
        + "<br><b>"
        + tr("If this setting is wrong, WZPlayer won't be able to play"
             " anything!")
        + "</b>");

    setWhatsThis(mpv_radio, tr("MPV"),
        tr("Select MPV as the media player to use by WZPlayer."));

    setWhatsThis(mpv_edit, tr("MPV executable"),
        tr("The path to the MPV executable file.")
        + "<br><b>"
        + tr("If this setting is wrong, WZPlayer won't be able to play"
             " anything!")
        + "</b>");

    setWhatsThis(report_player_crashes_check,
        tr("Report player errors in a messagebox"),
        tr("Shows a messagebox when the player reports errors or crashes."
           " Errors will always be shown in the statusbar."));


    addSectionTitle(tr("Remember player settings for every file"));

    setWhatsThis(settings_group, tr("Remember settings for every file"),
        tr("If checked WZPlayer will remember the settings you make for every"
           " file and reload them when you play the file again."));

    setWhatsThis(remember_time_check, tr("Remember time position"),
        tr("If checked, WZPlayer will remember the last position "
           "of the file when you open it again. This option works only with "
           "regular files (not with DVDs, CDs, URLs...)."));

    setWhatsThis(remember_volume_check, tr("Remember volume"),
        tr("If checked, each file uses and remembers its own volume. If not"
           " checked, the volume is left unchanged when loading a new file.")
        + "<br>"
        + tr("This option also applies to the mute state."));

    setWhatsThis(remember_audio_eq_check, tr("Remember audio equalizer"),
        tr("If this option is not checked, all media files share the same audio"
           " equalizer.") + " " +
        tr("If it is checked, the audio equalizer values are saved along each"
           " file and restored when the file is played later."));

    setWhatsThis(filesettings_method_combo,
                 tr("Method to store the file settings"),
        tr("This option allows to change the way the file settings will be "
           "stored. The following options are available:")
        + "<ul><li>"
        + tr("<b>one ini file</b>: the settings for all played files will be "
             "saved in a single ini file (%1)")
            .arg(QString("<i>" + TPaths::configPath() + "/file_settings.ini</i>"))
        + "</li><li>"
        + tr("<b>multiple ini files</b>: one ini file will be used for each"
             " played file. Those ini files will be saved in the folder %1")
            .arg(QString("<i>"+TPaths::configPath()+"/file_settings</i>"))
                 + "</li></ul>" +
        tr("The latter will be faster when handling a lot of files."));

#ifndef Q_OS_WIN
    addSectionTitle(tr("Radio and TV"));
    setWhatsThis(radio_tv_rescan_check,
                 tr("Check for new radio and TV channels on startup"),
        tr("If this option is checked, WZPlayer will look for new radio and TV"
           " channels in ~/.mplayer/channels.conf.ter "
           " and ~/.mplayer/channels.conf."));
#endif


    addSectionTitle(tr("Extra player options"));
    setWhatsThis(player_args_edit, tr("Options"),
        tr("Here you can pass extra command line options to the player."
           " Write them separated by spaces. They are stored per player."));


    addSectionTitle(tr("Extra filters"));
    setWhatsThis(player_vfilters_edit, tr("Video filters"),
        tr("Here you can add extra video filters. Write them separated by"
           " commas. Don't use spaces!"));
    setWhatsThis(player_afilters_edit, tr("Audio filters"),
        tr("Here you can add extra audio filters. Write them separated by"
           " commas. Don't use spaces!"));


    addSectionTitle(tr("Actions to run"));
    setWhatsThis(actions_to_run_edit, tr("Actions list"),
        tr("Here you can specify a list of <i>actions</i> which will be"
           " run every time a file is opened. You'll find all available"
           " actions in the <b>Actions</b> section. The actions must be"
           " separated by spaces. Checkable actions can be followed by"
           " <i>true</i> or <i>false</i> to enable or disable the action.")
        + "<br>" + tr("Example:") + " <i>auto_zoom fullscreen true</i><br>"
        + tr("Limitation: the actions are run only when a file is opened and"
             " not when the player process is restarted (e.g. you select an"
             " audio or video filter needing a  player restart)."));


    addSectionTitle(tr("Logs"));
    setWhatsThis(log_level_combo, tr("Log level"),
        tr("Select the level a message must have to be written to the log."
           " You can view the log with menu <b><i>Window - View log</i></b>."));
    setWhatsThis(log_verbose_check, tr("Verbose"),
        tr("Request verbose messages from the player for troubleshooting."));
    setWhatsThis(log_window_max_events_spinbox, tr("Log window events"),
        tr("Specify the number of log events to remember for the log window."));
}

} // namespace Pref
} // namespace Gui

#include "moc_playersection.cpp"

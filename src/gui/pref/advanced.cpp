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


#include "gui/pref/advanced.h"
#include "log4qt/logmanager.h"
#include "images.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui { namespace Pref {

TAdvanced::TAdvanced(QWidget* parent, Qt::WindowFlags f) :
    TSection(parent, f) {

    setupUi(this);
    retranslateStrings();
}

TAdvanced::~TAdvanced() {
}

QString TAdvanced::sectionName() {
    return tr("Advanced");
}

QPixmap TAdvanced::sectionIcon() {
    return Images::icon("pref_advanced", iconSize);
}

void TAdvanced::retranslateStrings() {

    retranslateUi(this);
    icon_label->setPixmap(Images::icon("pref_advanced"));
    createHelp();
}

void TAdvanced::setData(TPreferences* pref) {


    setActionsToRun(pref->actions_to_run);

    setPlayerAdditionalArguments(pref->player_additional_options);
    setPlayerAdditionalVideoFilters(pref->player_additional_video_filters);
    setPlayerAdditionalAudioFilters(pref->player_additional_audio_filters);

    // Log
    setLogLevel(Log4Qt::LogManager::rootLogger()->level());
    setLogVerbose(pref->log_verbose);
    log_window_max_events_spinbox->setValue(pref->log_window_max_events);
}

void TAdvanced::getData(TPreferences* pref) {

    TSection::getData(pref);

    pref->actions_to_run = actionsToRun();

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

    // Log
    pref->log_level = logLevel();
    Log4Qt::LogManager::rootLogger()->setLevel(pref->log_level);
    Log4Qt::LogManager::qtLogger()->setLevel(pref->log_level);
    restartIfBoolChanged(pref->log_verbose,
        pref->log_level <= Log4Qt::Level::DEBUG_INT && logVerbose(),
        "log_verbose");
    pref->log_window_max_events = log_window_max_events_spinbox->value();
}

void TAdvanced::setLogLevel(Log4Qt::Level level) {

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

Log4Qt::Level TAdvanced::logLevel() {

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

void TAdvanced::setLogVerbose(bool b) {
    log_verbose_check->setChecked(b);
}

bool TAdvanced::logVerbose() {
    return log_verbose_check->isChecked();
}

void TAdvanced::setPlayerAdditionalArguments(QString args) {
    player_args_edit->setText(args);
}

QString TAdvanced::playerAdditionalArguments() {
    return player_args_edit->text();
}

void TAdvanced::setPlayerAdditionalVideoFilters(QString s) {
    player_vfilters_edit->setText(s);
}

QString TAdvanced::playerAdditionalVideoFilters() {
    return player_vfilters_edit->text();
}

void TAdvanced::setPlayerAdditionalAudioFilters(QString s) {
    player_afilters_edit->setText(s);
}

QString TAdvanced::playerAdditionalAudioFilters() {
    return player_afilters_edit->text();
}

void TAdvanced::setActionsToRun(QString actions) {
    actions_to_run_edit->setText(actions);
}

QString TAdvanced::actionsToRun() {
    return actions_to_run_edit->text();
}

void TAdvanced::createHelp() {

    clearHelp();

    addSectionTitle(tr("Logs"));

    setWhatsThis(log_level_combo, tr("Log level"),
        tr("Select the level a message must have to be written to the log."
           " You can view the log with menu <b><i>Window - View log</i></b>."));

    setWhatsThis(log_verbose_check, tr("Verbose"),
        tr("Request verbose messages from the player for troubleshooting."));

    setWhatsThis(log_window_max_events_spinbox, tr("Log window events"),
        tr("Specify the number of log events to remember for the log window."));

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
}

}} // namespace Gui::Pref

#include "moc_advanced.cpp"

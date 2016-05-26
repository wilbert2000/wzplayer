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
#include "images.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui { namespace Pref {

TAdvanced::TAdvanced(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);

	retranslateStrings();
}

TAdvanced::~TAdvanced() {
}

QString TAdvanced::sectionName() {
	return tr("Advanced");
}

QPixmap TAdvanced::sectionIcon() {
	return Images::icon("pref_advanced", icon_size);
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
}

void TAdvanced::getData(TPreferences* pref) {

	requires_restart = false;
	pref->actions_to_run = actionsToRun();

    restartIfStringChanged(pref->player_additional_options,
                           playerAdditionalArguments(),
                           "player_additional_options");
    restartIfStringChanged(pref->player_additional_video_filters,
                           playerAdditionalVideoFilters(),
                           "player_additional_video_filters");
    restartIfStringChanged(pref->player_additional_audio_filters,
                           playerAdditionalAudioFilters(),
                           "player_additional_audio_filters");
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

	addSectionTitle(tr("Advanced"));

	setWhatsThis(actions_to_run_edit, tr("Actions list"),
		tr("Here you can specify a list of <i>actions</i> which will be "
           "run every time a file is opened. You'll find all available "
		   "actions in the shortcut editor in the <b>Actions</b> "
           "section. The actions must be separated by spaces. Checkable "
           "actions can be followed by <i>true</i> or <i>false</i> to "
           "enable or disable the action.") +"<br>"+
		tr("Example:") +" <i>auto_zoom fullscreen true</i><br>" +
		tr("Limitation: the actions are run only when a file is opened and "
           "not when the mplayer process is restarted (e.g. you select an "
           "audio or video filter)."));

	addSectionTitle(tr("Options for player"));

	setWhatsThis(player_args_edit, tr("Options"),
		tr("Here you can pass extra options to the player. Write them separated by spaces."));

	setWhatsThis(player_vfilters_edit, tr("Video filters"),
		tr("Here you can add extra video filters. Write them separated by commas. Don't use spaces!"));

	setWhatsThis(player_afilters_edit, tr("Audio filters"),
		tr("Here you can add extra audio filters. Write them separated by commas. Don't use spaces!"));
}

}} // namespace Gui::Pref

#include "moc_advanced.cpp"

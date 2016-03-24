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


#include "gui/pref/general.h"
#include <QDebug>
#include "filedialog.h"
#include "images.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"


using namespace Settings;

namespace Gui {
namespace Pref {

TGeneral::TGeneral(QWidget* parent)
	: TWidget(parent, 0) {

	setupUi(this);

	mplayer_edit->setDialogType(FileChooser::GetFileName);
	mpv_edit->setDialogType(FileChooser::GetFileName);

	connect(mplayer_edit, SIGNAL(fileChanged(QString)),
			this, SLOT(onMPlayerFileChanged(QString)));
	connect(mpv_edit, SIGNAL(fileChanged(QString)),
			this, SLOT(onMPVFileChanged(QString)));
	connect(mplayer_radio, SIGNAL(clicked(bool)),
			this, SLOT(onRadioClicked(bool)));
	connect(mpv_radio, SIGNAL(clicked(bool)),
			this, SLOT(onRadioClicked(bool)));

#ifdef Q_OS_WIN
	radio_tv_group->hide();
#endif

	retranslateStrings();
}

TGeneral::~TGeneral() {
}

QString TGeneral::sectionName() {
	return tr("General");
}

QPixmap TGeneral::sectionIcon() {
	return Images::icon("pref_general", icon_size);
}

void TGeneral::retranslateStrings() {

	retranslateUi(this);

	// Player
	mplayer_icon_label->setPixmap(Images::icon("mplayer"));
	mpv_icon_label->setPixmap(Images::icon("mpv"));

	mplayer_edit->setCaption(tr("Select the MPlayer executable"));
	mpv_edit->setCaption(tr("Select the MPV executable"));

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
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

	createHelp();
}

void TGeneral::setData(TPreferences* pref) {

	// Player
	setPlayerID(pref->player_id);
	setPlayerPath(pref->mplayer_bin, pref->mpv_bin);
	report_player_crashes_check->setChecked(pref->report_player_crashes);

	// Media settings group
	setRememberSettings(pref->remember_media_settings);
	setRememberTimePos(!pref->remember_time_pos);
	setGlobalVolume(pref->global_volume);
	remember_audio_eq_check->setChecked(!pref->global_audio_equalizer);
	setFileSettingsMethod(pref->file_settings_method);

	// Radio and TV
	radio_tv_rescan_check->setChecked(pref->check_channels_conf_on_startup);

	requires_restart = false;
}

void TGeneral::getData(TPreferences* pref) {

	// Update player
	pref->mplayer_bin = mplayer_edit->text();
	pref->mpv_bin = mpv_edit->text();
	QString bin = mplayer_radio->isChecked() ? pref->mplayer_bin : pref->mpv_bin;
	if (pref->player_bin != bin) {
		requires_restart = true;
		pref->setPlayerBin(bin);
	}
	pref->report_player_crashes = report_player_crashes_check->isChecked();

	// Media settings
	pref->remember_media_settings = rememberSettings();
	pref->remember_time_pos = rememberTimePos();
	pref->global_volume = !pref->remember_media_settings || globalVolume();
	pref->global_audio_equalizer = !pref->remember_media_settings
								   || !remember_audio_eq_check->isChecked();
	pref->file_settings_method = fileSettingsMethod();

	// Radio and TV
	pref->check_channels_conf_on_startup = radio_tv_rescan_check->isChecked();
}

void TGeneral::setPlayerID(Settings::TPreferences::TPlayerID id) {

	if (id == Settings::TPreferences::ID_MPLAYER) {
		mplayer_radio->setChecked(true);
		mpv_radio->setChecked(false);
	} else {
		mplayer_radio->setChecked(false);
		mpv_radio->setChecked(true);
	}
}

Settings::TPreferences::TPlayerID TGeneral::playerID() {

	if (mplayer_radio->isChecked()) {
		return Settings::TPreferences::ID_MPLAYER;
	}
	return Settings::TPreferences::ID_MPV;
}

void TGeneral::setPlayerPath(const QString& mplayer, const QString& mpv) {

	mplayer_edit->setText(mplayer);
	mpv_edit->setText(mpv);
}

void TGeneral::onMPlayerFileChanged(QString file) {
	qDebug() << "Gui::Pref::TGeneral::onMPlayerFileChanged:" << file;

	if (mplayer_radio->isChecked()) {
		emit binChanged(pref->getAbsolutePathPlayer(file));
	}
}

void TGeneral::onMPVFileChanged(QString file) {
	qDebug() << "Gui::Pref::TGeneral::onMPVFileChanged:" << file;

	if (mpv_radio->isChecked()) {
		emit binChanged(pref->getAbsolutePathPlayer(file));
	}
}

void TGeneral::onRadioClicked(bool) {
	qDebug() << "Gui::Pref::TGeneral::onRadioClicked";

	QString file;
	if (mplayer_radio->isChecked()) {
		file = mplayer_edit->text();
	} else {
		file = mpv_edit->text();
	}
	emit binChanged(pref->getAbsolutePathPlayer(file));
}

void TGeneral::setRememberSettings(bool b) {
	settings_group->setChecked(b);
}

bool TGeneral::rememberSettings() {
	return settings_group->isChecked();
}

void TGeneral::setRememberTimePos(bool b) {
	remember_time_check->setChecked(b);
}

bool TGeneral::rememberTimePos() {
	return remember_time_check->isChecked();
}

void TGeneral::setGlobalVolume(bool b) {
	remember_volume_check->setChecked(!b);
}

bool TGeneral::globalVolume() {
	return !remember_volume_check->isChecked();
}

void TGeneral::setFileSettingsMethod(const QString& method) {

	int index = filesettings_method_combo->findData(method);
	if (index < 0)
		index = 0;
	filesettings_method_combo->setCurrentIndex(index);
}

QString TGeneral::fileSettingsMethod() {
	return filesettings_method_combo->itemData(filesettings_method_combo->currentIndex()).toString();
}

void TGeneral::createHelp() {

	clearHelp();

	setWhatsThis(mplayer_radio, tr("MPlayer"),
		tr("Select MPlayer as the media player to use by SMPlayer."));

	setWhatsThis(mplayer_edit, tr("MPlayer executable"),
		tr("The path to the MPlayer executable file.")
		+ "<br><b>"
		+ tr("If this setting is wrong, SMPlayer won't be able to play anything!")
		+ "</b>");

	setWhatsThis(mpv_radio, tr("MPV"),
		tr("Select MPV as the media player to use by SMPlayer."));

	setWhatsThis(mpv_edit, tr("MPV executable"),
		tr("The path to the MPV executable file.")
		+ "<br><b>"
		+ tr("If this setting is wrong, SMPlayer won't be able to play anything!")
		+ "</b>");

	setWhatsThis(report_player_crashes_check,
		tr("Report player errors in a messagebox"),
		tr("Shows a messagebox when the player reports errors or crashes."
		   " Errors will always be shown in the statusbar."));

	setWhatsThis(settings_group, tr("Remember settings for every file"),
		tr("When checked SMPlayer will remember the settings you make for each file"
		   " and reload them when you play the file again."));

	setWhatsThis(remember_time_check, tr("Remember time position"),
		tr("If you check this option, SMPlayer will remember the last position "
           "of the file when you open it again. This option works only with "
           "regular files (not with DVDs, CDs, URLs...)."));

	setWhatsThis(remember_volume_check, tr("Remember volume"),
		tr("If not checked, the same volume will be used for all files you play."
		   " If checked each file uses its own volume.")
		+ "<br>"
		+ tr("This option also applies to the mute state."));

	setWhatsThis(remember_audio_eq_check, tr("Remember audio equalizer"),
		tr("If this option is not checked, all media files share the same audio equalizer.") +" "+
		tr("If it is checked, the audio equalizer values are saved along each file "
		   "and loaded back when the file is played later."));

	setWhatsThis(filesettings_method_combo, tr("Method to store the file settings"),
		tr("This option allows to change the way the file settings would be "
           "stored. The following options are available:") +"<ul><li>" + 
		tr("<b>one ini file</b>: the settings for all played files will be "
           "saved in a single ini file (%1)").arg(QString("<i>"+TPaths::iniPath()+"/smplayer.ini</i>")) + "</li><li>" +
		tr("<b>multiple ini files</b>: one ini file will be used for each played file. "
           "Those ini files will be saved in the folder %1").arg(QString("<i>"+TPaths::iniPath()+"/file_settings</i>")) + "</li></ul>" +
		tr("The latter method could be faster if there is info for a lot of files."));

#ifndef Q_OS_WIN
	setWhatsThis(radio_tv_rescan_check, tr("Check for new radio and TV channels on startup"),
		tr("If this option is checked, SMPlayer will look for new TV and radio"
		   " channels in ~/.mplayer/channels.conf.ter"
		   " or ~/.mplayer/channels.conf."));
#endif

}

} // namespace Pref
} // namespace Gui

#include "moc_general.cpp"

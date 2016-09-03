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


#include "gui/pref/player.h"

#include <QDebug>

#include "gui/filedialog.h"
#include "images.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"


using namespace Settings;

namespace Gui {
namespace Pref {

TPlayer::TPlayer(QWidget* parent)
	: TWidget(parent, 0) {

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

TPlayer::~TPlayer() {
}

QString TPlayer::sectionName() {
    return tr("Player");
}

QPixmap TPlayer::sectionIcon() {
    return Images::icon("pref_player", icon_size);
}

void TPlayer::retranslateStrings() {

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

void TPlayer::setData(TPreferences* pref) {

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

void TPlayer::getData(TPreferences* pref) {

    // Update player
    pref->mplayer_bin = mplayer_edit->text();
    pref->mpv_bin = mpv_edit->text();
    QString bin = mplayer_radio->isChecked() ? pref->mplayer_bin : pref->mpv_bin;
    if (bin != pref->player_bin) {
        requires_restart = true;
        pref->setPlayerBin(bin, false,
            mplayer_radio->isChecked() ? TPreferences::ID_MPLAYER
                                       : TPreferences::ID_MPV);
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

void TPlayer::setPlayerID(Settings::TPreferences::TPlayerID id) {

	if (id == Settings::TPreferences::ID_MPLAYER) {
		mplayer_radio->setChecked(true);
		mpv_radio->setChecked(false);
	} else {
		mplayer_radio->setChecked(false);
		mpv_radio->setChecked(true);
	}
}

Settings::TPreferences::TPlayerID TPlayer::playerID() {

	if (mplayer_radio->isChecked()) {
		return Settings::TPreferences::ID_MPLAYER;
	}
	return Settings::TPreferences::ID_MPV;
}

void TPlayer::setPlayerPath(const QString& mplayer, const QString& mpv) {

	mplayer_edit->setText(mplayer);
	mpv_edit->setText(mpv);
}

void TPlayer::onMPlayerFileChanged(QString file) {

	if (mplayer_radio->isChecked()) {
		emit binChanged(TPreferences::ID_MPLAYER, true, pref->getAbsolutePathPlayer(file));
	}
}

void TPlayer::onMPVFileChanged(QString file) {

	if (mpv_radio->isChecked()) {
		emit binChanged(TPreferences::ID_MPV, true, pref->getAbsolutePathPlayer(file));
	}
}

void TPlayer::onPlayerRadioClicked(bool) {

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

void TPlayer::setRememberSettings(bool b) {
	settings_group->setChecked(b);
}

bool TPlayer::rememberSettings() {
	return settings_group->isChecked();
}

void TPlayer::setRememberTimePos(bool b) {
	remember_time_check->setChecked(b);
}

bool TPlayer::rememberTimePos() {
	return remember_time_check->isChecked();
}

void TPlayer::setGlobalVolume(bool b) {
	remember_volume_check->setChecked(!b);
}

bool TPlayer::globalVolume() {
	return !remember_volume_check->isChecked();
}

void TPlayer::setFileSettingsMethod(const QString& method) {

	int index = filesettings_method_combo->findData(method);
	if (index < 0)
		index = 0;
	filesettings_method_combo->setCurrentIndex(index);
}

QString TPlayer::fileSettingsMethod() {
	return filesettings_method_combo->itemData(filesettings_method_combo->currentIndex()).toString();
}

void TPlayer::createHelp() {

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

}

} // namespace Pref
} // namespace Gui

#include "moc_player.cpp"

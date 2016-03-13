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
#include "settings/preferences.h"
#include "filedialog.h"
#include "images.h"
#include "settings/mediasettings.h"
#include "settings/paths.h"


using namespace Settings;

namespace Gui { namespace Pref {

TGeneral::TGeneral(QWidget* parent)
	: TWidget(parent, 0) {

	setupUi(this);

	playerbin_edit->setDialogType(FileChooser::GetFileName);
	connect(playerbin_edit, SIGNAL(fileChanged(QString)),
			this, SLOT(fileChanged(QString)));

	screenshot_edit->setDialogType(FileChooser::GetDirectory);

	// Screenshots
#ifdef MPV_SUPPORT
	screenshot_format_combo->addItems(QStringList() << "png" << "ppm" << "pgm" << "pgmyuv" << "tga" << "jpg" << "jpeg");
#else
	screenshot_template_label->hide();
	screenshot_template_edit->hide();
	screenshot_format_label->hide();
	screenshot_format_combo->hide();
#endif

	retranslateStrings();
}

TGeneral::~TGeneral() {
}

QString TGeneral::sectionName() {
	return tr("General");
}

QPixmap TGeneral::sectionIcon() {
	return Images::icon("pref_general", 22);
}

void TGeneral::retranslateStrings() {

	retranslateUi(this);

	playerbin_edit->setCaption(tr("Select the player executable"));

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	playerbin_edit->setFilter(tr("Executables") +" (*.exe)");
#else
	playerbin_edit->setFilter(tr("All files") +" (*)");
#endif

	int filesettings_method_item = filesettings_method_combo->currentIndex();
	filesettings_method_combo->clear();
	filesettings_method_combo->addItem(tr("one ini file"), "normal");
	filesettings_method_combo->addItem(tr("multiple ini files"), "hash");
	filesettings_method_combo->setCurrentIndex(filesettings_method_item);

	screenshot_edit->setCaption(tr("Select a directory"));

	createHelp();
}

void TGeneral::setData(TPreferences* pref) {

	// General tab
	setPlayerPath(pref->player_bin);

	// Media settings group
	setRememberSettings(pref->remember_media_settings);
	setRememberTimePos(!pref->remember_time_pos);
	setGlobalVolume(pref->global_volume);
	setFileSettingsMethod(pref->file_settings_method);

	// Screenshots group
	setUseScreenshots(pref->use_screenshot);
	setScreenshotDir(pref->screenshot_directory);

#ifdef MPV_SUPPORT
	screenshot_template_edit->setText(pref->screenshot_template);
	setScreenshotFormat(pref->screenshot_format);
#endif

	requires_restart = false;
}

void TGeneral::getData(TPreferences* pref) {

	if (pref->player_bin != playerPath()) {
		requires_restart = true;
		pref->setPlayerBin(playerPath());
		// TODO: check fileChanged triggered if changed without exit of edit
		// emit binChanged(pref->playerAbsolutePath());
	}

	pref->remember_media_settings = rememberSettings();
	pref->remember_time_pos = rememberTimePos();
	pref->global_volume = globalVolume();
	pref->file_settings_method = fileSettingsMethod();

	// Screenshots
	restartIfBoolChanged(pref->use_screenshot, useScreenshots());
	restartIfStringChanged(pref->screenshot_directory, screenshotDir());

#ifdef MPV_SUPPORT
	restartIfStringChanged(pref->screenshot_template, screenshot_template_edit->text());
	restartIfStringChanged(pref->screenshot_format, screenshotFormat());
#endif

}

void TGeneral::setPlayerPath(const QString& path) {
	playerbin_edit->setText(path);
}

QString TGeneral::playerPath() {
	return playerbin_edit->text();
}

void TGeneral::fileChanged(QString file) {
	qDebug() << "Gui::Pref::TGeneral::fileChanged:" << file;

	emit binChanged(pref->getAbsolutePathPlayer(file));
}

void TGeneral::setUseScreenshots(bool b) {
	use_screenshots_check->setChecked(b);
}

bool TGeneral::useScreenshots() {
	return use_screenshots_check->isChecked();
}

void TGeneral::setScreenshotDir(const QString& path) {
	screenshot_edit->setText(path);
}

QString TGeneral::screenshotDir() {
	return screenshot_edit->text();
}

#ifdef MPV_SUPPORT
void TGeneral::setScreenshotFormat(const QString& format) {

	int i = screenshot_format_combo->findText(format);
	if (i < 0)
		i = 0;
	screenshot_format_combo->setCurrentIndex(i);
}

QString TGeneral::screenshotFormat() {
	return screenshot_format_combo->currentText();
}
#endif

void TGeneral::setRememberSettings(bool b) {
	remember_all_check->setChecked(b);
}

bool TGeneral::rememberSettings() {
	return remember_all_check->isChecked();
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

	setWhatsThis(playerbin_edit, tr("%1 executable").arg(pref->playerName()),
		tr("Here you must specify the %1 "
           "executable that SMPlayer will use.").arg(pref->playerName()) + "<br><b>" +
        tr("If this setting is wrong, SMPlayer won't be able to play "
           "anything!") + "</b>");

	setWhatsThis(remember_all_check, tr("Remember settings"),
		tr("Usually SMPlayer will remember the settings for each file you "
           "play (audio track selected, volume, filters...). Disable this "
           "option if you don't like this feature."));

	setWhatsThis(remember_time_check, tr("Remember time position for every file"),
		tr("If you check this option, SMPlayer will remember the last position "
           "of the file when you open it again. This option works only with "
           "regular files (not with DVDs, CDs, URLs...)."));

	setWhatsThis(remember_volume_check, tr("Remember volume for every file"),
		tr("If this option is not checked, the same volume will be used for "
		   "all files you play. If the option is checked each "
		   "file uses its own volume.") + "<br>" +
		tr("This option also applies to the mute state."));

	setWhatsThis(filesettings_method_combo, tr("Method to store the file settings"),
		tr("This option allows to change the way the file settings would be "
           "stored. The following options are available:") +"<ul><li>" + 
		tr("<b>one ini file</b>: the settings for all played files will be "
           "saved in a single ini file (%1)").arg(QString("<i>"+TPaths::iniPath()+"/smplayer.ini</i>")) + "</li><li>" +
		tr("<b>multiple ini files</b>: one ini file will be used for each played file. "
           "Those ini files will be saved in the folder %1").arg(QString("<i>"+TPaths::iniPath()+"/file_settings</i>")) + "</li></ul>" +
		tr("The latter method could be faster if there is info for a lot of files."));

	setWhatsThis(use_screenshots_check, tr("Enable screenshots"),
		tr("You can use this option to enable or disable the possibility to "
           "take screenshots."));

	setWhatsThis(screenshot_edit, tr("Screenshots folder"),
		tr("Here you can specify a folder where the screenshots taken by "
           "SMPlayer will be stored. If the folder is not valid the "
           "screenshot feature will be disabled."));

#ifdef MPV_SUPPORT
	setWhatsThis(screenshot_template_edit, tr("Template for screenshots"),
		tr("This option specifies the filename template used to save screenshots.") + " " +
		tr("For example %1 would save the screenshot as 'moviename_0001.png'.").arg("%F_%04n") + "<br>" +
		tr("%1 specifies the filename of the video without the extension, "
		   "%2 adds a 4 digit number padded with zeros.").arg("%F").arg("%04n") + " " +
		tr("For a full list of the template specifiers visit this link:") + 
		" <a href=\"http://mpv.io/manual/stable/#options-screenshot-template\">"
		"http://mpv.io/manual/stable/#options-screenshot-template</a>" + "<br>" +
		tr("This option only works with mpv."));

	setWhatsThis(screenshot_format_combo, tr("Format for screenshots"),
		tr("This option allows to choose the image file type used for saving screenshots.") + " " +
		tr("This option only works with mpv.") );
#endif

}

}} // namespace Gui::Pref

#include "moc_general.cpp"

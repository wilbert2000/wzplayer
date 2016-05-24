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


#include "gui/pref/interface.h"
#include <QDir>
#include <QStyleFactory>

#include "log4qt/logmanager.h"
#include "images.h"
#include "settings/preferences.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "settings/paths.h"
#include "languages.h"


namespace Gui {
namespace Pref {

TInterface::TInterface(QWidget* parent, Qt::WindowFlags f)
    : TWidget(parent, f),
    debug(logger()) {

	setupUi(this);

	// Style combo
	style_combo->addItem("<default>");
	style_combo->addItems(QStyleFactory::keys());

	// Icon set combo
	iconset_combo->addItem("Default");

	// User
	QDir icon_dir = Settings::TPaths::configPath() + "/themes";
    logger()->debug("user icon dir: " + icon_dir.absolutePath());
	QStringList iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	// Global
	icon_dir = Settings::TPaths::themesPath();
    logger()->debug("global icon dir: " + icon_dir.absolutePath());
	iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n = 0; n < iconsets.count(); n++) {
		if (iconset_combo->findText(iconsets[n]) == -1) {
			iconset_combo->addItem(iconsets[n]);
		}
	}

	connect(single_instance_check, SIGNAL(toggled(bool)), 
			this, SLOT(changeInstanceImages()));

	retranslateStrings();
}

TInterface::~TInterface() {
}

QString TInterface::sectionName() {
	return tr("Interface");
}

QPixmap TInterface::sectionIcon() {
	return Images::icon("instance1", icon_size);
}

void TInterface::createLanguageCombo() {

	QMap <QString,QString> m = Languages::translations();

	// Language combo
	QDir translation_dir = Settings::TPaths::translationPath();
	QStringList languages = translation_dir.entryList(QStringList() << "*.qm");
	QRegExp rx_lang("(.*)\\.qm");
	language_combo->clear();
	language_combo->addItem(tr("<Autodetect>"));
	for (int n=0; n < languages.count(); n++) {
		if (rx_lang.indexIn(languages[n]) > -1) {
			QString l = rx_lang.cap(1);
			QString text = l;
			if (m.contains(l)) text = m[l] + " ("+l+")";
			language_combo->addItem(text, l);
		}
	}
}

void TInterface::retranslateStrings() {

	retranslateUi(this);

	changeInstanceImages();

	// Language combo
	int language_item = language_combo->currentIndex();
	createLanguageCombo();
	language_combo->setCurrentIndex(language_item);

	// Iconset combo
	iconset_combo->setItemText(0, tr("Default"));

	style_combo->setItemText(0, tr("Default"));

	// Playlist
	int index = media_to_add_combo->currentIndex();
	media_to_add_combo->clear();
	media_to_add_combo->addItem(tr("None"), Settings::TPreferences::NoFiles);
	media_to_add_combo->addItem(tr("Video files"), Settings::TPreferences::VideoFiles);
	media_to_add_combo->addItem(tr("Audio files"), Settings::TPreferences::AudioFiles);
	media_to_add_combo->addItem(tr("Video and audio files"), Settings::TPreferences::MultimediaFiles);
	media_to_add_combo->addItem(tr("Consecutive files"), Settings::TPreferences::ConsecutiveFiles);
	media_to_add_combo->setCurrentIndex(index);

	createHelp();
}

void TInterface::setData(Settings::TPreferences* pref) {

	setLanguage(pref->language);
	setIconSet(pref->iconset);
	setStyle(pref->style);

	// Main window
	setUseSingleInstance(pref->use_single_window);
	setSaveSize(pref->save_window_size_on_exit);
	resize_on_load_check->setChecked(pref->resize_on_load);
	resize_on_docking_check->setChecked(pref->resize_on_docking);

	setPauseWhenHidden(pref->pause_when_hidden);
	setCloseOnFinish(pref->close_on_finish);
	setHideVideoOnAudioFiles(pref->hide_video_window_on_audio_files);

	// Fullscreen
	hide_toolbars_spin->setValue(pref->floating_hide_delay);
    show_toolbars_bottom_only_check->setChecked(
        pref->floating_activation_area == Settings::TPreferences::NearToolbar);
	setStartInFullscreen(pref->start_in_fullscreen);

	// Playlist
    setMediaToAddToPlaylist(pref->mediaToAddToPlaylist);
    setDirectoryRecursion(pref->addDirectories);
    video_check->setChecked(pref->addVideo);
    audio_check->setChecked(pref->addAudio);
    playlists_check->setChecked(pref->addPlaylists);
    images_check->setChecked(pref->addImages);
    image_duration_spinbox->setValue(pref->imageDuration);

    // Log
    setLogLevel(Log4Qt::LogManager::rootLogger()->level());
	setLogVerbose(pref->log_verbose);

	// History
	setRecentsMaxItems(pref->history_recents.maxItems());
	setURLMaxItems(pref->history_urls.maxItems());
	setRememberDirs(pref->save_dirs);
}

void TInterface::getData(Settings::TPreferences* pref) {

	requires_restart = false;
	language_changed = false;
	iconset_changed = false;
	style_changed = false;
	recents_changed = false;

	if (pref->language != language()) {
		pref->language = language();
		language_changed = true;
        logger()->debug("getData: chosen language " + pref->language);
	}
	if (pref->iconset != iconSet()) {
		pref->iconset = iconSet();
		iconset_changed = true;
	}
	if (pref->style != style()) {
		pref->style = style();
		style_changed = true;
	}

	// Main window
	pref->use_single_window = useSingleInstance();
	pref->save_window_size_on_exit = saveSize();
	pref->resize_on_load = resize_on_load_check->isChecked();
	pref->resize_on_docking = resize_on_docking_check->isChecked();
	pref->pause_when_hidden = pauseWhenHidden();
	pref->close_on_finish = closeOnFinish();
	pref->hide_video_window_on_audio_files = hideVideoOnAudioFiles();

	// Fullscreen
	pref->floating_hide_delay = hide_toolbars_spin->value();
    pref->floating_activation_area = show_toolbars_bottom_only_check->isChecked()
        ? Settings::TPreferences::NearToolbar : Settings::TPreferences::Anywhere;
	pref->start_in_fullscreen = startInFullscreen();

    // Playlist
    pref->mediaToAddToPlaylist = mediaToAddToPlaylist();
    pref->addDirectories = directoryRecursion();
    pref->addVideo = video_check->isChecked();
    pref->addAudio = audio_check->isChecked();
    pref->addPlaylists = playlists_check->isChecked();
    pref->addImages = images_check->isChecked();
    pref->imageDuration = image_duration_spinbox->value();

    // Log
    pref->log_level = logLevel();
    Log4Qt::LogManager::rootLogger()->setLevel(pref->log_level);
    Log4Qt::LogManager::qtLogger()->setLevel(pref->log_level);
    restartIfBoolChanged(pref->log_verbose,
        pref->log_level <= Log4Qt::Level::DEBUG_INT && logVerbose());

	// History
	if (pref->history_recents.maxItems() != recentsMaxItems()) {
		pref->history_recents.setMaxItems(recentsMaxItems());
		recents_changed = true;
	}

	if (pref->history_urls.maxItems() != urlMaxItems()) {
		pref->history_urls.setMaxItems(urlMaxItems());
		url_max_changed = true;
	}

	pref->save_dirs = rememberDirs();
}

void TInterface::setLanguage(const QString& lang) {

	if (lang.isEmpty()) {
		language_combo->setCurrentIndex(0);
	} else {
		int pos = language_combo->findData(lang);
		if (pos != -1) 
			language_combo->setCurrentIndex(pos);
		else
			language_combo->setCurrentText(lang);
	}
}

QString TInterface::language() {

	if (language_combo->currentIndex() <= 0)
		return "";
	else 
		return language_combo->itemData(language_combo->currentIndex()).toString();
}

void TInterface::setIconSet(const QString& set) {

	iconset_combo->setCurrentIndex(0);
	for (int n = 0; n < iconset_combo->count(); n++) {
		if (iconset_combo->itemText(n) == set) {
			iconset_combo->setCurrentIndex(n);
			break;
		}
	}
}

QString TInterface::iconSet() {

	if (iconset_combo->currentIndex() <= 0)
		return "";
	else
		return iconset_combo->currentText();
}

void TInterface::setSaveSize(bool b) {
	save_size_check->setChecked(b);
}

bool TInterface::saveSize() {
	return save_size_check->isChecked();
}

void TInterface::setStyle(const QString& style) {
	if (style.isEmpty()) 
		style_combo->setCurrentIndex(0);
	else
		style_combo->setCurrentText(style);
}

QString TInterface::style() {
	if (style_combo->currentIndex()==0)
		return "";
	else 
		return style_combo->currentText();
}

void TInterface::setStartInFullscreen(bool b) {
	start_fullscreen_check->setChecked(b);
}

bool TInterface::startInFullscreen() {
	return start_fullscreen_check->isChecked();
}

void TInterface::setUseSingleInstance(bool b) {
	single_instance_check->setChecked(b);
}

bool TInterface::useSingleInstance() {
	return single_instance_check->isChecked();
}

void TInterface::changeInstanceImages() {

	if (single_instance_check->isChecked())
		instances_icon->setPixmap(Images::icon("instance1"));
	else
		instances_icon->setPixmap(Images::icon("instance2"));
}

void TInterface::setCloseOnFinish(bool b) {
	close_on_finish_check->setChecked(b);
}

bool TInterface::closeOnFinish() {
	return close_on_finish_check->isChecked();
}

void TInterface::setPauseWhenHidden(bool b) {
	pause_on_minimize_check->setChecked(b);
}

bool TInterface::pauseWhenHidden() {
	return pause_on_minimize_check->isChecked();
}

void TInterface::setHideVideoOnAudioFiles(bool b) {
	hide_video_window_on_audio_check->setChecked(b);
}

bool TInterface::hideVideoOnAudioFiles() {
	return hide_video_window_on_audio_check->isChecked();
}
void TInterface::setMediaToAddToPlaylist(Settings::TPreferences::TAddToPlaylist type) {

    int i = media_to_add_combo->findData(type);
    if (i < 0)
        i = 0;
	media_to_add_combo->setCurrentIndex(i);
}

Settings::TPreferences::TAddToPlaylist TInterface::mediaToAddToPlaylist() {
    return (Settings::TPreferences::TAddToPlaylist)
        media_to_add_combo->itemData(media_to_add_combo->currentIndex()).toInt();
}

void TInterface::setDirectoryRecursion(bool b) {
	recursive_check->setChecked(b);
}

bool TInterface::directoryRecursion() {
	return recursive_check->isChecked();
}

void TInterface::setLogLevel(Log4Qt::Level level) {

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

Log4Qt::Level TInterface::logLevel() {

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

void TInterface::setLogVerbose(bool b) {
	log_verbose_check->setChecked(b);
}

bool TInterface::logVerbose() {
	return log_verbose_check->isChecked();
}

void TInterface::setRecentsMaxItems(int n) {
	recents_max_items_spin->setValue(n);
}

int TInterface::recentsMaxItems() {
	return recents_max_items_spin->value();
}

void TInterface::setURLMaxItems(int n) {
	url_max_items_spin->setValue(n);
}

int TInterface::urlMaxItems() {
	return url_max_items_spin->value();
}

void TInterface::setRememberDirs(bool b) {
	save_dirs_check->setChecked(b);
}

bool TInterface::rememberDirs() {
	return save_dirs_check->isChecked();
}

void TInterface::createHelp() {
	clearHelp();

	addSectionTitle(tr("Interface"));

	setWhatsThis(language_combo, tr("Language"),
		tr("Here you can change the language of the application."));

	setWhatsThis(iconset_combo, tr("Icon set"),
		tr("Select the icon set you prefer for the application."));

	setWhatsThis(style_combo, tr("Style"),
		tr("Select the style you prefer for the application."));


	addSectionTitle(tr("Main window"));

	setWhatsThis(single_instance_check,
		tr("Use only one window running WZPlayer"),
		tr("Check this option if you want to use an already running instance "
		   "of WZPlayer when opening other files."));

	setWhatsThis(save_size_check, tr("Remember position and size"),
        tr("If you check this option, the position and size of the main "
		   "window will be saved and restored when you run WZPlayer again."));

	setWhatsThis(resize_on_load_check, tr("Resize window when loading a new video"),
		tr("Adjust the dimension of the main window to a newly loaded video."));

	setWhatsThis(resize_on_docking_check, tr("Resize window when docking the playlist"),
		tr("Grows or shrinks the main window when you dock or undock the playlist."));

	setWhatsThis(pause_on_minimize_check, tr("Pause when minimized"),
		tr("If this option is enabled, the file will be paused when the "
		   "main window is hidden. When the window is restored, playback "
		   "will be resumed."));

	setWhatsThis(close_on_finish_check, tr("Close when finished"),
		tr("If this option is checked, the main window will be automatically "
		   "closed when the current file/playlist finishes."));

	setWhatsThis(hide_video_window_on_audio_check, tr("Hide video window when playing audio files"),
        tr("If this option is enabled the video window will be hidden when playing audio files."));

	addSectionTitle(tr("Fullscreen"));

	setWhatsThis(hide_toolbars_spin, tr("Hide toolbars after"),
		tr("Sets the time in milliseconds to hide the toolbars after the mouse left a toolbar."));

	setWhatsThis(show_toolbars_bottom_only_check,
		tr("Show toolbars only when moving the mouse to the bottom of the screen"),
		tr("If this option is checked, the toolbars will only be displayed when"
		   " the mouse is moved to the bottom of the screen or over a hidden"
		   " toolbar. Otherwise the control will appear whenever the mouse is"
		   " moved, no matter its position."));

	setWhatsThis(start_fullscreen_check, tr("Start videos in fullscreen"),
		tr("If this option is checked, all videos will start to play in "
		   "fullscreen mode."));

	addSectionTitle(tr("Playlist"));

	setWhatsThis(media_to_add_combo, tr("Add files from folder"),
		tr("This option allows to add files automatically to the playlist:") +"<br>"+
		tr("<b>None</b>: no files will be added") +"<br>"+
		tr("<b>Video files</b>: all video files found in the folder will be added") +"<br>"+
		tr("<b>Audio files</b>: all audio files found in the folder will be added") +"<br>"+
		tr("<b>Video and audio files</b>: all video and audio files found in the folder will be added") +"<br>"+
		tr("<b>Consecutive files</b>: consecutive files (like video_1.avi, video_2.avi) will be added"));

	setWhatsThis(recursive_check, tr("Add files in directories recursively"),
		tr("Check this option if you want that adding a directory will also "
		"add the files in subdirectories recursively. Otherwise only the "
		"files in the selected directory will be added."));

	addSectionTitle(tr("Logs"));

    setWhatsThis(log_level_combo, tr("Log level"),
        tr("Select which messages will be written to the log. You can view the"
           " log with menu <b><i>Window - View log</i></b>."));

	setWhatsThis(log_verbose_check, tr("Verbose"),
		tr("Request verbose messages from the player for troubleshooting."));

	addSectionTitle(tr("History"));

	setWhatsThis(recents_max_items_spin, tr("Recent files"),
        tr("Select the maximum number of items that will be shown in the "
           "<b>Open->Recent files</b> submenu. If you set it to 0 that "
           "menu won't be shown at all."));

	setWhatsThis(url_max_items_spin, tr("Max. URLs"),
        tr("Select the maximum number of items that the <b>Open->URL</b> "
           "dialog will remember. Set it to 0 if you don't want any URL "
           "to be stored."));

	setWhatsThis(save_dirs_check, tr("Remember last directory"),
		tr("If this option is checked, WZPlayer will remember the last folder you use to open a file."));
}

}} // namespace Gui::Pref

#include "moc_interface.cpp"

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


#include "gui/pref/interface.h"
#include <QDir>
#include <QStyleFactory>
#include <QFontDialog>

#include "images.h"
#include "settings/preferences.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "settings/paths.h"
#include "languages.h"


namespace Gui {
namespace Pref {

TInterface::TInterface(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);

	// Style combo
	style_combo->addItem("<default>");
	style_combo->addItems(QStyleFactory::keys());

	// Icon set combo
	iconset_combo->addItem("Default");

	// User
	QDir icon_dir = Settings::TPaths::configPath() + "/themes";
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	QStringList iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

	// Global
	icon_dir = Settings::TPaths::themesPath();
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n = 0; n < iconsets.count(); n++) {
		if (iconset_combo->findText(iconsets[n]) == -1) {
			iconset_combo->addItem(iconsets[n]);
		}
	}

#ifdef SINGLE_INSTANCE
	connect(single_instance_check, SIGNAL(toggled(bool)), 
			this, SLOT(changeInstanceImages()));
#else
	single_instance_check->hide();
#endif

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
	QRegExp rx_lang("smplayer_(.*)\\.qm");
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

	int mainwindow_resize = resize_window_combo->currentIndex();

	retranslateUi(this);

	changeInstanceImages();

	resize_window_combo->setCurrentIndex(mainwindow_resize);

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
	setDefaultFont(pref->default_font);

	// Main window
#ifdef SINGLE_INSTANCE
	setUseSingleInstance(pref->use_single_instance);
#endif

	setSaveSize(pref->save_window_size_on_exit);
	setResizeMethod(pref->resize_method);
	setPauseWhenHidden(pref->pause_when_hidden);
	setCloseOnFinish(pref->close_on_finish);
	setHideVideoOnAudioFiles(pref->hide_video_window_on_audio_files);
	setShowTagInTitle(pref->show_tag_in_window_title);

	// Fullscreen
	hide_toolbars_spin->setValue(pref->floating_hide_delay);
	show_toolbars_bottom_only_check->setChecked(pref->floating_activation_area == Settings::TPreferences::NearToolbar);
	setStartInFullscreen(pref->start_in_fullscreen);
	setBlackbordersOnFullscreen(pref->add_blackborders_on_fullscreen);

	// Playlist
	setMediaToAdd(pref->media_to_add_to_playlist);

	setLogDebugEnabled(pref->log_debug_enabled);
	setLogVerbose(pref->log_verbose);
	setLogFile(pref->log_file);

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
		qDebug("Gui::Pref::TInterface::getData: chosen language: '%s'", pref->language.toUtf8().data());
	}
	if (pref->iconset != iconSet()) {
		pref->iconset = iconSet();
		iconset_changed = true;
	}
	if (pref->style != style()) {
		pref->style = style();
		style_changed = true;
	}
	pref->default_font = defaultFont();

	// Main window
#ifdef SINGLE_INSTANCE
	pref->use_single_instance = useSingleInstance();
#endif

	pref->resize_method = resizeMethod();
	pref->save_window_size_on_exit = saveSize();
	pref->close_on_finish = closeOnFinish();
	pref->pause_when_hidden = pauseWhenHidden();
	pref->hide_video_window_on_audio_files = hideVideoOnAudioFiles();
	pref->show_tag_in_window_title = showTagInTitle();

	// Fullscreen
	pref->floating_hide_delay = hide_toolbars_spin->value();
	pref->floating_activation_area = show_toolbars_bottom_only_check->isChecked() ? Settings::TPreferences::NearToolbar : Settings::TPreferences::Anywhere;
	pref->start_in_fullscreen = startInFullscreen();
	if (pref->add_blackborders_on_fullscreen != blackbordersOnFullscreen()) {
		pref->add_blackborders_on_fullscreen = blackbordersOnFullscreen();
		if (pref->fullscreen)
			requires_restart = true;
	}

	// Playlist
	pref->media_to_add_to_playlist = (Settings::TPreferences::TAutoAddToPlaylistFilter) mediaToAdd();

	pref->log_debug_enabled = logDebugEnabled();
	restartIfBoolChanged(pref->log_verbose, logVerbose());
	pref->log_file = logFile();

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

void TInterface::setResizeMethod(int v) {
	resize_window_combo->setCurrentIndex(v);
}

int TInterface::resizeMethod() {
	return resize_window_combo->currentIndex();
}

void TInterface::setSaveSize(bool b) {
	save_size_check->setChecked(b);
}

bool TInterface::saveSize() {
	return save_size_check->isChecked();
}

void TInterface::setShowTagInTitle(bool b) {
	use_filename_in_title_check->setChecked(!b);
}

bool TInterface::showTagInTitle() {
	return !use_filename_in_title_check->isChecked();
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


void TInterface::setBlackbordersOnFullscreen(bool b) {
	blackborders_on_fs_check->setChecked(b);
}

bool TInterface::blackbordersOnFullscreen() {
	return blackborders_on_fs_check->isChecked();
}

#ifdef SINGLE_INSTANCE
void TInterface::setUseSingleInstance(bool b) {
	single_instance_check->setChecked(b);
	//singleInstanceButtonToggled(b);
}

bool TInterface::useSingleInstance() {
	return single_instance_check->isChecked();
}
#endif

void TInterface::setDefaultFont(const QString& font_desc) {
	default_font_edit->setText(font_desc);
}

QString TInterface::defaultFont() {
	return default_font_edit->text();
}

void TInterface::on_changeFontButton_clicked() {
	QFont f = qApp->font();

	if (!default_font_edit->text().isEmpty()) {
		f.fromString(default_font_edit->text());
	}

	bool ok;
	f = QFontDialog::getFont(&ok, f, this);

	if (ok) {
		default_font_edit->setText(f.toString());
	}
}

void TInterface::changeInstanceImages() {

#ifdef SINGLE_INSTANCE
	if (single_instance_check->isChecked())
		instances_icon->setPixmap(Images::icon("instance1"));
	else
		instances_icon->setPixmap(Images::icon("instance2"));
#else
	instances_icon->setPixmap(Images::icon("instance1"));
#endif
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
void TInterface::setMediaToAdd(int type) {
	int i = media_to_add_combo->findData(type);
	if (i < 0) i = 0;
	media_to_add_combo->setCurrentIndex(i);
}

int TInterface::mediaToAdd() {
	return media_to_add_combo->itemData(media_to_add_combo->currentIndex()).toInt();
}

void TInterface::setDirectoryRecursion(bool b) {
	recursive_check->setChecked(b);
}

bool TInterface::directoryRecursion() {
	return recursive_check->isChecked();
}

void TInterface::setSavePlaylistOnExit(bool b) {
	autosave_on_exit_check->setChecked(b);
}

bool TInterface::savePlaylistOnExit() {
	return autosave_on_exit_check->isChecked();
}

void TInterface::setLogDebugEnabled(bool b) {
	log_debug_check->setChecked(b);
}

bool TInterface::logDebugEnabled() {
	return log_debug_check->isChecked();
}

void TInterface::setLogVerbose(bool b) {
	log_verbose_check->setChecked(b);
}

bool TInterface::logVerbose() {
	return log_verbose_check->isChecked();
}

void TInterface::setLogFile(bool b) {
	log_file_check->setChecked(b);
}

bool TInterface::logFile() {
	return log_file_check->isChecked();
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


	setWhatsThis(changeFontButton, tr("Default font"),
		tr("You can change the application's font."));


	addSectionTitle(tr("Main window"));

#ifdef SINGLE_INSTANCE
	setWhatsThis(single_instance_check,
		tr("Use only one running instance of SMPlayer"),
		tr("Check this option if you want to use an already running instance "
		   "of SMPlayer when opening other files."));
#endif

	setWhatsThis(resize_window_combo, tr("Autoresize"),
        tr("The main window can be resized automatically. Select the option "
           "you prefer."));

	setWhatsThis(save_size_check, tr("Remember position and size"),
        tr("If you check this option, the position and size of the main "
           "window will be saved and restored when you run SMPlayer again."));

	setWhatsThis(pause_on_minimize_check, tr("Pause when minimized"),
		tr("If this option is enabled, the file will be paused when the "
		   "main window is hidden. When the window is restored, playback "
		   "will be resumed."));

	setWhatsThis(close_on_finish_check, tr("Close when finished"),
		tr("If this option is checked, the main window will be automatically "
		   "closed when the current file/playlist finishes."));

	setWhatsThis(hide_video_window_on_audio_check, tr("Hide video window when playing audio files"),
        tr("If this option is enabled the video window will be hidden when playing audio files."));

	setWhatsThis(use_filename_in_title_check, tr("Always use file name in window title"),
		tr("If media provides a title it will be used for the window title, "
		   "unless this option is checked, then the file name will always be used."));


	addSectionTitle(tr("Fullscreen"));

	setWhatsThis(hide_toolbars_spin, tr("Hide toolbars after"),
		tr("Sets the time (in milliseconds) to hide the toolbars after the mouse went away from the control."));

	setWhatsThis(show_toolbars_bottom_only_check, tr("Show toolbars only when moving the mouse to the bottom of the screen"),
		tr("If this option is checked, the toolbars will only be displayed when the mouse is moved "
		   "to the bottom of the screen. Otherwise the control will appear whenever the mouse is moved, no matter "
		   "its position."));

	setWhatsThis(start_fullscreen_check, tr("Start videos in fullscreen"),
		tr("If this option is checked, all videos will start to play in "
		   "fullscreen mode."));

	setWhatsThis(blackborders_on_fs_check, tr("Add black borders on fullscreen"),
		tr("If this option is enabled, black borders will be added to the "
		   "image in fullscreen mode. This allows subtitles to be displayed "
		   "on the black borders."));

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

	setWhatsThis(autosave_on_exit_check, tr("Save copy of playlist on exit"),
		tr("If this option is checked, a copy of the playlist will be saved "
		   "in the smplayer configuration when smplayer is closed, and it will "
		   "reloaded automatically when smplayer is run again."));

	addSectionTitle(tr("Logs"));

	setWhatsThis(log_debug_check, tr("Log debug messages"),
		tr("If checked, SMPlayer will log debug messages, "
		   "which might give additional information in case of trouble. "
		   "Non-debug messages are always logged. "
		   "You can view the log with menu <b>Options - View log</b>."));

	setWhatsThis(log_verbose_check, tr("Verbose"),
		tr("Request verbose messages from player for troubleshooting."));

	setWhatsThis(log_file_check, tr("Save SMPlayer log to file"),
		tr("If this option is checked, the SMPlayer log wil be recorded to %1")
		  .arg("<i>"+ Settings::TPaths::configPath() + "/smplayer_log.txt</i>"));

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
		tr("If this option is checked, SMPlayer will remember the last folder you use to open a file."));
}

}} // namespace Gui::Pref

#include "moc_interface.cpp"

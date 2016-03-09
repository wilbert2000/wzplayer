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


#define SINGLE_INSTANCE_TAB 2

namespace Gui { namespace Pref {

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
	tabWidget->setTabEnabled(SINGLE_INSTANCE_TAB, false);
#endif

	retranslateStrings();
}

TInterface::~TInterface()
{
}

QString TInterface::sectionName() {
	return tr("Interface");
}

QPixmap TInterface::sectionIcon() {
    return Images::icon("pref_gui", 22);
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
	int mainwindow_resize = mainwindow_resize_combo->currentIndex();
	int timeslider_pos = timeslider_behaviour_combo->currentIndex();

	retranslateUi(this);

	mainwindow_resize_combo->setCurrentIndex(mainwindow_resize);
	timeslider_behaviour_combo->setCurrentIndex(timeslider_pos);

	// Icons
	resize_window_icon->setPixmap(Images::icon("resize_window"));
	/* volume_icon->setPixmap(Images::icon("speaker")); */

#ifdef SINGLE_INSTANCE
	changeInstanceImages();
#endif

	// Seek widgets
	seek1->setLabel(tr("&Short jump"));
	seek2->setLabel(tr("&Medium jump"));
	seek3->setLabel(tr("&Long jump"));
	seek4->setLabel(tr("Mouse &wheel jump"));

	if (qApp->isLeftToRight()) {
		seek1->setIcon(Images::icon("forward10s", 32));
		seek2->setIcon(Images::icon("forward1m", 32));
		seek3->setIcon(Images::icon("forward10m", 32));
	} else {
		seek1->setIcon(Images::flippedIcon("forward10s", 32));
		seek2->setIcon(Images::flippedIcon("forward1m", 32));
		seek3->setIcon(Images::flippedIcon("forward10m", 32));
	}
	seek4->setIcon(Images::icon("mouse",32));

	// Language combo
	int language_item = language_combo->currentIndex();
	createLanguageCombo();
	language_combo->setCurrentIndex(language_item);

	// Iconset combo
	iconset_combo->setItemText(0, tr("Default"));

	style_combo->setItemText(0, tr("Default"));

	createHelp();
}

void TInterface::setData(Settings::TPreferences* pref) {
	setLanguage(pref->language);
	setIconSet(pref->iconset);

	setResizeMethod(pref->resize_method);
	setSaveSize(pref->save_window_size_on_exit);
	setShowTagInTitle(pref->show_tag_in_window_title);

#ifdef SINGLE_INSTANCE
	setUseSingleInstance(pref->use_single_instance);
#endif
	setSeeking1(pref->seeking1);
	setSeeking2(pref->seeking2);
	setSeeking3(pref->seeking3);
	setSeeking4(pref->seeking4);

	setUpdateWhileDragging(pref->update_while_seeking);
	setRelativeSeeking(pref->relative_seeking);
	setPreciseSeeking(pref->precise_seeking);

	setDefaultFont(pref->default_font);

	setHideVideoOnAudioFiles(pref->hide_video_window_on_audio_files);

	setStyle(pref->style);

	floating_activation_area_check->setChecked(pref->floating_activation_area == Settings::TPreferences::NearToolbar);
	floating_hide_delay_spin->setValue(pref->floating_hide_delay);

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

	pref->resize_method = resizeMethod();
	pref->save_window_size_on_exit = saveSize();
	pref->show_tag_in_window_title = showTagInTitle();

#ifdef SINGLE_INSTANCE
	pref->use_single_instance = useSingleInstance();
#endif

	pref->seeking1 = seeking1();
	pref->seeking2 = seeking2();
	pref->seeking3 = seeking3();
	pref->seeking4 = seeking4();

	pref->update_while_seeking = updateWhileDragging();
	pref->relative_seeking= relativeSeeking();
	pref->precise_seeking = preciseSeeking();

	pref->default_font = defaultFont();

	pref->hide_video_window_on_audio_files = hideVideoOnAudioFiles();

	if (pref->style != style()) {
		pref->style = style();
		style_changed = true;
	}

	pref->floating_activation_area = floating_activation_area_check->isChecked() ? Settings::TPreferences::NearToolbar : Settings::TPreferences::Anywhere;
	pref->floating_hide_delay = floating_hide_delay_spin->value();

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
	}
	else {
		int pos = language_combo->findData(lang);
		if (pos != -1) 
			language_combo->setCurrentIndex(pos);
		else
			language_combo->setCurrentText(lang);
	}
}

QString TInterface::language() {
	if (language_combo->currentIndex()==0) 
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

	if (iconset_combo->currentIndex() == 0)
		return "";
	else
		return iconset_combo->currentText();
}

void TInterface::setResizeMethod(int v) {
	mainwindow_resize_combo->setCurrentIndex(v);
}

int TInterface::resizeMethod() {
	return mainwindow_resize_combo->currentIndex();
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

#ifdef SINGLE_INSTANCE
void TInterface::setUseSingleInstance(bool b) {
	single_instance_check->setChecked(b);
	//singleInstanceButtonToggled(b);
}

bool TInterface::useSingleInstance() {
	return single_instance_check->isChecked();
}
#endif

void TInterface::setSeeking1(int n) {
	seek1->setTime(n);
}

int TInterface::seeking1() {
	return seek1->time();
}

void TInterface::setSeeking2(int n) {
	seek2->setTime(n);
}

int TInterface::seeking2() {
	return seek2->time();
}

void TInterface::setSeeking3(int n) {
	seek3->setTime(n);
}

int TInterface::seeking3() {
	return seek3->time();
}

void TInterface::setSeeking4(int n) {
	seek4->setTime(n);
}

int TInterface::seeking4() {
	return seek4->time();
}

void TInterface::setUpdateWhileDragging(bool b) {
	if (b) 
		timeslider_behaviour_combo->setCurrentIndex(0);
	else
		timeslider_behaviour_combo->setCurrentIndex(1);
}

bool TInterface::updateWhileDragging() {
	return (timeslider_behaviour_combo->currentIndex() == 0);
}

void TInterface::setRelativeSeeking(bool b) {
	relative_seeking_button->setChecked(b);
	absolute_seeking_button->setChecked(!b);
}

bool TInterface::relativeSeeking() {
	return relative_seeking_button->isChecked();
}

void TInterface::setPreciseSeeking(bool b) {
	precise_seeking_check->setChecked(b);
}

bool TInterface::preciseSeeking() {
	return precise_seeking_check->isChecked();
}

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

#ifdef SINGLE_INSTANCE
void TInterface::changeInstanceImages() {
	if (single_instance_check->isChecked())
		instances_icon->setPixmap(Images::icon("instance1"));
	else
		instances_icon->setPixmap(Images::icon("instance2"));
}
#endif

void TInterface::setHideVideoOnAudioFiles(bool b) {
	hide_video_window_on_audio_check->setChecked(b);
}

bool TInterface::hideVideoOnAudioFiles() {
	return hide_video_window_on_audio_check->isChecked();
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

	setWhatsThis(mainwindow_resize_combo, tr("Autoresize"),
        tr("The main window can be resized automatically. Select the option "
           "you prefer."));

	setWhatsThis(save_size_check, tr("Remember position and size"),
        tr("If you check this option, the position and size of the main "
           "window will be saved and restored when you run SMPlayer again."));

	setWhatsThis(hide_video_window_on_audio_check, tr("Hide video window when playing audio files"),
        tr("If this option is enabled the video window will be hidden when playing audio files."));

	setWhatsThis(use_filename_in_title_check, tr("Always use file name in window title"),
		tr("If media provides a title it will be used for the window title, "
		   "unless this option is checked, then the file name will always be used."));

	setWhatsThis(language_combo, tr("Language"),
		tr("Here you can change the language of the application."));

	setWhatsThis(iconset_combo, tr("Icon set"),
		tr("Select the icon set you prefer for the application."));

	setWhatsThis(style_combo, tr("Style"),
        tr("Select the style you prefer for the application."));


	setWhatsThis(changeFontButton, tr("Default font"),
        tr("You can change here the application's font."));

	addSectionTitle(tr("Seeking"));

	setWhatsThis(seek1, tr("Short jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("short jump")));

	setWhatsThis(seek2, tr("Medium jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("medium jump")));

	setWhatsThis(seek3, tr("Long jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("long jump")));

	setWhatsThis(seek4, tr("Mouse wheel jump"),
        tr("Select the time that should be go forward or backward when you "
           "move the mouse wheel."));

	setWhatsThis(timeslider_behaviour_combo, tr("Behaviour of time slider"),
        tr("Select what to do when dragging the time slider."));

	setWhatsThis(seeking_method_group, tr("Seeking method"),
		tr("Sets the method to be used when seeking with the slider. "
           "Absolute seeking may be a little bit more accurate, while "
           "relative seeking may work better with files with a wrong length."));

	setWhatsThis(precise_seeking_check, tr("Precise seeking"),
		tr("If this option is enabled, seeks are more accurate but they "
           "can be a little bit slower. May not work with some video formats.") +"<br>"+
		tr("Note: this option only works with MPlayer2"));

#ifdef SINGLE_INSTANCE
	addSectionTitle(tr("Instances"));

	setWhatsThis(single_instance_check, 
        tr("Use only one running instance of SMPlayer"),
        tr("Check this option if you want to use an already running instance "
           "of SMPlayer when opening other files."));
#endif

	addSectionTitle(tr("Floating control"));

	setWhatsThis(floating_activation_area_check, tr("Show only when moving the mouse to the bottom of the screen"),
		tr("If this option is checked, the floating control will only be displayed when the mouse is moved "
           "to the bottom of the screen. Otherwise the control will appear whenever the mouse is moved, no matter "
           "its position."));

	setWhatsThis(floating_hide_delay_spin, tr("Time to hide the control"),
		tr("Sets the time (in milliseconds) to hide the control after the mouse went away from the control."));

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

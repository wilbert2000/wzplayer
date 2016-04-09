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

#include "gui/pref/capture.h"
#include <QDebug>
#include "images.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"


using namespace Settings;

namespace Gui {
namespace Pref {

TCapture::TCapture(QWidget* parent)
	: TWidget(parent, 0) {

	setupUi(this);
	screenshot_edit->setDialogType(FileChooser::GetDirectory);
	screenshot_format_combo->addItems(QStringList() << "png" << "ppm" << "pgm" << "pgmyuv" << "tga" << "jpg" << "jpeg");
	retranslateStrings();
}

TCapture::~TCapture() {
}

QString TCapture::sectionName() {
	return tr("Capture");
}

QPixmap TCapture::sectionIcon() {
	return Images::icon("screenshot", icon_size);
}

void TCapture::retranslateStrings() {

	retranslateUi(this);

	icon_label->setPixmap(Images::icon("screenshot"));

	screenshot_edit->setCaption(tr("Select a directory"));

	createHelp();
}

void TCapture::setData(Settings::TPreferences* pref) {

	setUseScreenshots(pref->use_screenshot);
	setScreenshotDir(pref->screenshot_directory);
	screenshot_template_edit->setText(pref->screenshot_template);
	setScreenshotFormat(pref->screenshot_format);
	setSubtitlesOnScreenshots(pref->subtitles_on_screenshots);
}

void TCapture::getData(Settings::TPreferences* pref) {

	// Screenshots
	bool enable = useScreenshots();
	QString dir = screenshotDir();
	if (dir.isEmpty()) {
		enable = false;
	} else {
		QFileInfo fi(dir);
		if (!fi.isDir() || !fi.isWritable()) {
			qWarning() << "Gui::Pref::TCapture::getData: screenshot directory not writable"
					   << dir;
			enable = false;
			// Need to clear dir to disable capture
			dir = "";
		}
	}
	restartIfBoolChanged(pref->use_screenshot, enable);
	restartIfStringChanged(pref->screenshot_directory, dir);

	restartIfStringChanged(pref->screenshot_template, screenshot_template_edit->text());
	restartIfStringChanged(pref->screenshot_format, screenshotFormat());

	restartIfBoolChanged(pref->subtitles_on_screenshots, subtitlesOnScreenshots());
}

void TCapture::setUseScreenshots(bool b) {
	screenshots_group->setChecked(b);
}

bool TCapture::useScreenshots() {
	return screenshots_group->isChecked();
}

void TCapture::setScreenshotDir(const QString& path) {
	screenshot_edit->setText(path);
}

QString TCapture::screenshotDir() {
	return screenshot_edit->text();
}

void TCapture::setScreenshotFormat(const QString& format) {

	int i = screenshot_format_combo->findText(format);
	if (i < 0)
		i = 0;
	screenshot_format_combo->setCurrentIndex(i);
}

QString TCapture::screenshotFormat() {
	return screenshot_format_combo->currentText();
}

void TCapture::setSubtitlesOnScreenshots(bool b) {
	subtitles_on_screeshots_check->setChecked(b);
}

bool TCapture::subtitlesOnScreenshots() {
	return subtitles_on_screeshots_check->isChecked();
}

void TCapture::createHelp() {

	clearHelp();

	addSectionTitle(tr("Capture"));

	addSectionGroup(tr("Screenshots"));

	setWhatsThis(screenshots_group, tr("Enable screenshots"),
		tr("You can use this option to enable or disable the possibility to "
		   "take screenshots."));

	setWhatsThis(screenshot_edit, tr("Screenshots folder"),
		tr("Here you can specify a folder where the screenshots taken by "
		   "WZPlayer will be stored. If the folder is not valid the "
		   "screenshot feature will be disabled."));

	setWhatsThis(screenshot_template_edit, tr("Template for screenshots"),
		tr("MPV only. This option specifies the filename template used to save screenshots.") + " " +
		tr("For example %1 would save the screenshot as 'moviename_0001.png'.").arg("%F_%04n") + "<br>" +
		tr("%1 specifies the filename of the video without the extension, "
		   "%2 adds a 4 digit number padded with zeros.").arg("%F").arg("%04n") + " " +
		tr("For a full list of the template specifiers visit this link:") +
		" <a href=\"http://mpv.io/manual/stable/#options-screenshot-template\">"
		"http://mpv.io/manual/stable/#options-screenshot-template</a>");

	setWhatsThis(screenshot_format_combo, tr("Format for screenshots"),
		tr("MPV only. Choose the image file type used for saving screenshots."));

	setWhatsThis(subtitles_on_screeshots_check,
		tr("Include subtitles on screenshots"),
		tr("If this option is checked, the subtitles will appear in the "
		   "screenshots. <b>Note:</b> it may cause some troubles sometimes."));
}

}} // namespace Gui::Pref

#include "moc_capture.cpp"

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


#include "gui/pref/drives.h"
#include "images.h"
#include "settings/preferences.h"

#include <QFile>
#include <QFileInfoList>
#include <QDir>

namespace Gui { namespace Pref {

#ifdef Q_OS_WIN
#include <windows.h>

bool isCDDevice(QString drive) {
		unsigned int r =  GetDriveTypeW((LPCWSTR) drive.utf16());
		qDebug("isCDDevice: '%s' r: %d", drive.toUtf8().data(), r);
		return (r == DRIVE_CDROM);
	}

#endif

#ifdef Q_OS_OS2  // fixme SCS
bool isCDDevice(QString drive) {
	return true;
}
#endif

TDrives::TDrives(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f)
{
	setupUi(this);

#ifndef Q_OS_WIN
	check_drives_button->hide();
#endif

	updateDriveCombos();

	retranslateStrings();
}

TDrives::~TDrives()
{
}

QString TDrives::sectionName() {
	return tr("Drives");
}

QPixmap TDrives::sectionIcon() {
    return Images::icon("pref_devices", 22);
}


void TDrives::retranslateStrings() {
	retranslateUi(this);

	cdrom_drive_icon->setPixmap(Images::icon("cdrom_drive"));
	dvd_drive_icon->setPixmap(Images::icon("dvd_drive"));
	bluray_drive_icon->setPixmap(Images::icon("bluray_drive"));

	createHelp();
}

void TDrives::updateDriveCombos(bool detect_cd_devices) {
	qDebug("Gui::Pref::TDrives::updateDriveCombos: detect_cd_devices: %d", detect_cd_devices);

	// Save current values
	QString current_dvd_device = dvdDevice();
	QString current_cd_device = cdromDevice();
	QString current_bluray_device = blurayDevice();

	dvd_device_combo->clear();
	cdrom_device_combo->clear();
	bluray_device_combo->clear();

	// DVD device combo
	// In windows, insert the drives letters
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	QFileInfoList list = QDir::drives();
	for (int n = 0; n < list.size(); n++) {
		QString s = list[n].filePath();
		bool is_cd_device = true;
		if (detect_cd_devices) is_cd_device = isCDDevice(s);
		if (is_cd_device) {
			if (s.endsWith("/")) s = s.remove(s.length()-1,1);
			dvd_device_combo->addItem(s);
			cdrom_device_combo->addItem(s);
			bluray_device_combo->addItem(s);
		}
	}
#else
	QDir dev_dir("/dev");
	QStringList devices = dev_dir.entryList(QStringList() << "dvd*" << "cdrom*" << "cdrw*" << "sr*" << "cdrecorder*" << "acd[0-9]*" << "cd[0-9]*", 
                                             QDir::Files | QDir::System | QDir::Readable);
	for (int n=0; n < devices.count(); n++) {
		QString device_name = "/dev/" + devices[n];
		qDebug("Gui::Pref::TDrives::TDrives: device found: '%s'", device_name.toUtf8().constData());
		dvd_device_combo->addItem(device_name);
		cdrom_device_combo->addItem(device_name);
		bluray_device_combo->addItem(device_name);
	}
#endif

	// Restore previous values
	setDVDDevice(current_dvd_device);
	setCDRomDevice(current_cd_device);
	setBlurayDevice(current_bluray_device);
}

void TDrives::setData(Settings::TPreferences* pref) {
	setDVDDevice(pref->dvd_device);
	setCDRomDevice(pref->cdrom_device);
	setBlurayDevice(pref->bluray_device);
	setUseDVDNav(pref->use_dvdnav);
}

void TDrives::getData(Settings::TPreferences* pref) {
	requires_restart = false;

	pref->dvd_device = dvdDevice();
	pref->cdrom_device = cdromDevice();
	pref->bluray_device = blurayDevice();
	pref->use_dvdnav = useDVDNav();
}

void TDrives::setDVDDevice(const QString& dir) {
	dvd_device_combo->setCurrentText(dir);
}

QString TDrives::dvdDevice() {
	return dvd_device_combo->currentText();
}

void TDrives::setBlurayDevice(const QString& dir) {
	bluray_device_combo->setCurrentText(dir);
}

QString TDrives::blurayDevice() {
	return bluray_device_combo->currentText();
}

void TDrives::setCDRomDevice(const QString& dir) {
	cdrom_device_combo->setCurrentText(dir);
}

QString TDrives::cdromDevice() {
	return cdrom_device_combo->currentText();
}

void TDrives::setUseDVDNav(bool b) {
	use_dvdnav_check->setChecked(b);
}

bool TDrives::useDVDNav() {
	return use_dvdnav_check->isChecked();
}

void TDrives::on_check_drives_button_clicked() {
	qDebug("Gui::Pref::TDrives::on_check_drives_button_clicked");
	updateDriveCombos(true);
}

void TDrives::createHelp() {
	clearHelp();

	setWhatsThis(cdrom_device_combo, tr("CD device"),
		tr("Choose your CDROM device. It will be used to play "
		   "VCDs and Audio CDs."));

	setWhatsThis(dvd_device_combo, tr("DVD device"),
		tr("Choose your DVD device. It will be used to play DVDs."));

	setWhatsThis(use_dvdnav_check, tr("Enable DVD menus"),
		tr("If this option is checked, SMPlayer will play DVDs using "
           "dvdnav. Requires a recent version of MPlayer compiled with dvdnav "
		   "support."));

	setWhatsThis(bluray_device_combo, tr("Blu-ray device"),
		tr("Choose your Blu-ray device. It will be used to play Blu-ray discs."));
}

}} // namespace Gui::Pref

#include "moc_drives.cpp"

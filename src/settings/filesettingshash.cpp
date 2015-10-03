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

#include "settings/filesettingshash.h"

#include <QFile>
#include <QDir>

#include "settings/../paths.h"
#include "mediasettings.h"
#include "filehash.h" // hash function

namespace Settings {

QString TFileSettingsHash::iniFilenameFor(const QString& filename) {

	QString hash = FileHash::calculateHash(filename);
	if (hash.isEmpty()) {
		return QString();
	}

	QString dir_name = Paths::configPath() + "/file_settings/" + hash[0];
	QDir dir(Paths::configPath());
	if (!dir.exists(dir_name)) {
		if (!dir.mkpath(dir_name)) {
			qWarning("Settings::TFileSettingsHash::iniFilenameFor: failed to create directory '%s'",
					 dir_name.toUtf8().constData());
			return QString();
		}
	}

	return dir_name + "/" + hash + ".ini";
}

TFileSettingsHash::TFileSettingsHash(const QString& filename) :
	TFileSettingsBase(TFileSettingsHash::iniFilenameFor(filename), 0) {
}

TFileSettingsHash::~TFileSettingsHash() {
}

bool TFileSettingsHash::existSettingsFor(const QString& filename) {
	qDebug("TFileSettingsHash::existSettingsFor: '%s'", filename.toUtf8().constData());

	QString config_file = iniFilenameFor(filename);
	qDebug("TFileSettingsHash::existSettingsFor: config_file: '%s'", config_file.toUtf8().constData());
	return QFile::exists(config_file);
}

void TFileSettingsHash::loadSettingsFor(const QString& filename, MediaSettings& mset, int player) {
	qDebug("FileSettings::loadSettingsFor: '%s'", filename.toUtf8().constData());

	mset.reset();
	beginGroup("file_settings");
	mset.load(this, player);
	endGroup();
}

void TFileSettingsHash::saveSettingsFor(const QString& filename, MediaSettings & mset, int player) {
	qDebug("TFileSettingsHash::saveSettingsFor: '%s'", filename.toUtf8().constData());

	beginGroup("file_settings");
	mset.save(this, player);
	endGroup();
	sync();
}

} // namespace Settings

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

#include "settings/filesettingshash.h"

#include <QFile>
#include <QDir>

#include "log4qt/logger.h"
#include "settings/paths.h"
#include "settings/mediasettings.h"
#include "filehash.h" // hash function

namespace Settings {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TFileSettingsHash)


QString TFileSettingsHash::iniFilenameFor(const QString& filename) {

	QString hash = FileHash::calculateHash(filename);
	if (hash.isEmpty()) {
		return QString();
	}

	QString dir_name = TPaths::configPath() + "/file_settings/" + hash[0];
	QDir dir(TPaths::configPath());
	if (!dir.exists(dir_name)) {
		if (!dir.mkpath(dir_name)) {
            logger()->warn("iniFilenameFor: failed to create directory '"
                           + dir_name + "'");
			return QString();
		}
	}

	return dir_name + "/" + hash + ".ini";
}

TFileSettingsHash::TFileSettingsHash(const QString& filename) :
	TFileSettingsBase(TFileSettingsHash::iniFilenameFor(filename)) {
}

TFileSettingsHash::~TFileSettingsHash() {
}

bool TFileSettingsHash::existSettingsFor(const QString& filename) {
    logger()->debug("existSettingsFor: '" + filename + "'");

	QString config_file = iniFilenameFor(filename);
    logger()->debug("existSettingsFor: config_file: '" + config_file + "'");
	return QFile::exists(config_file);
}

void TFileSettingsHash::loadSettingsFor(const QString& filename, TMediaSettings& mset) {
    logger()->debug("loadSettingsFor: '" + filename + "'");

	beginGroup("file_settings");
    mset.load(this);
	endGroup();
}

void TFileSettingsHash::saveSettingsFor(const QString& filename, TMediaSettings& mset) {
    logger()->debug("saveSettingsFor: '" + filename + "'");

	beginGroup("file_settings");
    mset.save(this);
	endGroup();
	sync();
}

} // namespace Settings

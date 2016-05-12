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

#include "settings/filesettings.h"
#include <QFileInfo>

#include "log4qt/logger.h"
#include "settings/mediasettings.h"
#include "settings/paths.h"
#include "config.h"


namespace Settings {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TFileSettings)

TFileSettings::TFileSettings() :
    TFileSettingsBase(TPaths::configPath() + "/" + TConfig::PROGRAM_ID
                      + "_files.ini") {
}

TFileSettings::~TFileSettings() {
}

QString TFileSettings::filenameToGroupname(const QString& filename) {

	QString s = filename;
	s = s.replace('/', '_');
	s = s.replace('\\', '_');
	s = s.replace(':', '_');
	s = s.replace('.', '_');
	s = s.replace(' ', '_');

	QFileInfo fi(filename);
	if (fi.exists()) {
		s += "_" + QString::number(fi.size());
	}

	return s;	
}

bool TFileSettings::existSettingsFor(const QString& filename) {

    QString group_name = filenameToGroupname(filename);
    logger()->debug("existSettingsFor: group name: '" + group_name + "'");
    beginGroup(group_name);
    bool saved = value("saved", false).toBool();
    endGroup();

    logger()->info("existSettingsFor: '" + filename + "' " + QString::number(saved));
    return saved;
}

void TFileSettings::loadSettingsFor(const QString& filename, TMediaSettings& mset) {
    logger()->info("loadSettingsFor: '" + filename + "'");

	QString group_name = filenameToGroupname(filename);
    logger()->debug("loadSettingsFor: group name: '" + group_name +"'");
	beginGroup(group_name);
    mset.load(this);
	endGroup();
}

void TFileSettings::saveSettingsFor(const QString& filename, TMediaSettings& mset) {
    logger()->info("saveSettingsFor: '" + filename + "'");

	QString group_name = filenameToGroupname(filename);

    logger()->debug("saveSettingsFor: group name: '" + group_name + "'");

	beginGroup(group_name);
	setValue("saved", true);
    mset.save(this);
	endGroup();
}

} // namespace Settings

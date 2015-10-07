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

#include "settings/filesettings.h"
#include "settings/../paths.h"
#include "mediasettings.h"
#include <QFileInfo>

namespace Settings {

TFileSettings::TFileSettings() :
	TFileSettingsBase(Paths::configPath() + "/smplayer_files.ini") {
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
	qDebug("Settings::TFileSettings::existSettingsFor: '%s'", filename.toUtf8().constData());

	QString group_name = filenameToGroupname(filename);
	qDebug("Settings::TFileSettings::existSettingsFor: group_name: '%s'", group_name.toUtf8().constData());
	beginGroup(group_name);
	bool saved = value("saved", false).toBool();
	endGroup();

	return saved;
}

void TFileSettings::loadSettingsFor(const QString& filename, TMediaSettings& mset, int player) {
	qDebug("Settings::TFileSettings::loadSettingsFor: '%s'", filename.toUtf8().constData());

	QString group_name = filenameToGroupname(filename);
	qDebug("Settings::TFileSettings::loadSettingsFor: group_name: '%s'", group_name.toUtf8().constData());
	mset.reset();
	beginGroup(group_name);
	mset.load(this, player);
	endGroup();
}

void TFileSettings::saveSettingsFor(const QString& filename, TMediaSettings& mset, int player) {
	qDebug("Settings::TFileSettings::saveSettingsFor: '%s'", filename.toUtf8().constData());

	QString group_name = filenameToGroupname(filename);

	qDebug("Settings::TFileSettings::saveSettingsFor: group_name: '%s'", group_name.toUtf8().constData());

	beginGroup(group_name);
	setValue("saved", true);
	mset.save(this, player);
	endGroup();
}

} // namespace Settings

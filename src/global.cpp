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


#include "global.h"
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include "paths.h"
#include "preferences.h"
#include "log.h"
#include "translator.h"

namespace Global {

QSettings* settings = 0;
Preferences* pref = 0;
TLog* log = 0;
Translator* translator = 0;

void global_init(const QString& config_path) {

	// Settings
	if (config_path.isEmpty()) {
		Paths::createConfigDirectory();
	} else {
		Paths::setConfigPath(config_path);
	}
	settings = new QSettings(Paths::configPath() + "/smplayer.ini",
							 QSettings::IniFormat);

	// Preferences
	pref = new Preferences();

	// Log
	log = new TLog(pref->log_enabled, pref->log_file, pref->log_filter);
	qDebug() << "Global::global_init: started log at UTC " +
				QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

	// Translator
	translator = new Translator();
	translator->load(pref->language);

	// Fonts
#ifdef Q_OS_WIN
	Paths::createFontFile();
#endif
}

void global_end() {
	qDebug("global_end");

	delete translator;
	delete log;
	delete pref;
	delete settings;
}

} // namespace Global

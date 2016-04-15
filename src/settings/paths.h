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

#ifndef SETTINGS_PATHS_H
#define SETTINGS_PATHS_H

#include <QString>

#if QT_VERSION_MAJOR >= 5
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif


namespace Settings {

class TPaths {
public:

	// Fix Qt locations
#if QT_VERSION_MAJOR >= 5
	enum TLocation {
        DataLocation = QStandardPaths::DataLocation,
        AppDataLocation = QStandardPaths::AppDataLocation,
		PicturesLocation = QStandardPaths::PicturesLocation,
		DocumentsLocation = QStandardPaths::DocumentsLocation,
		HomeLocation = QStandardPaths::HomeLocation
    };
#else
	enum TLocation {
        //DataLocation = QDesktopServices::DataLocation,
        //AppDataLocation = QDesktopServices::AppDataLocation,
        PicturesLocation = QDesktopServices::PicturesLocation,
		DocumentsLocation = QDesktopServices::DocumentsLocation,
		HomeLocation = QDesktopServices::HomeLocation
	};
#endif

	static QString location(TLocation type);

	static void setAppPath(const QString& path) { app_path = path; }
	static QString appPath() { return app_path; }

	static void setConfigPath(const QString& path);
	//! Return the path where wzplayer should save its config files
	static QString configPath() { return config_path; }

	// Ini file name
	static QString iniPath();

	static QString dataPath();
	static QString translationPath();
	static QString docPath();
	static QString themesPath();
	static QString shortcutsPath();
	static QString qtTranslationPath();
	static QString doc(const QString& file, QString locale = QString::null, bool english_fallback = true);
	static QString subtitleStyleFile();

#ifdef Q_OS_WIN
	static QString fontPath();
    static QString fontConfigFilename();
    static void createFontFile();
#endif

private:
	static QString app_path;
	static QString config_path;
};

} // namespace Settings

#endif // SETTINGS_PATHS_H

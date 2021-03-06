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
#include <QStandardPaths>


namespace Settings {

class TPaths {
public:
    static QString location(QStandardPaths::StandardLocation type);

    static void setConfigPath(bool portable);
    static QString configPath() { return configDir; }
    static QString iniFileName();
    static QString dataPath();
    static QString favoritesPath();
    static QString favoritesFilename();
    static QString themesPath();
    static QString shortcutsPath();
    static QString translationPath();
    static QString qtTranslationPath();
    static QString subtitleStyleFileName();
    static QString playerInfoFileName();
    static QString fileSettingsFileName();
    static QString fileSettingsHashPath();
    static QString genericCachePath();

private:
    static QString configDir;
    static bool portable;

    static QString getDataSubDir(const QString& subdir);
};

} // namespace Settings

#endif // SETTINGS_PATHS_H

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

#include "settings/paths.h"
#include "wzdebug.h"
#include "config.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QFile>
#include <QDir>


namespace Settings {

QString TPaths::configDir;
bool TPaths::portable;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TPaths)


QString TPaths::location(QStandardPaths::StandardLocation type) {

    if (portable) {
        return qApp->applicationDirPath();
    }
    return QStandardPaths::writableLocation(type);
}

void TPaths::setConfigPath(bool portable) {

    TPaths::portable = portable;
    if (portable) {
        configDir = qApp->applicationDirPath();
    } else {
        configDir = QStandardPaths::writableLocation(
                    QStandardPaths::AppConfigLocation);
        // Create config directory
        QDir dir(configDir);
        if (!dir.mkpath(configDir)) {
            WZE << "Failed to create configuration directory" << configDir
                << strerror(errno);
        }
    }
    WZT << "Configuration directory set to" << configDir;
}

QString TPaths::iniFileName() {
    return configDir + QDir::separator() + TConfig::PROGRAM_ID + ".ini";
}

QString TPaths::dataPath() {

    if (portable) {
        return qApp->applicationDirPath();
    }
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString TPaths::favoritesPath() {
    return dataPath() + "/Favorites";
}

QString TPaths::favoritesFilename() {
    return favoritesPath() + "/" + TConfig::WZPLAYLIST;
}

QString TPaths::genericCachePath() {

    QString cache;
    const char* XDG_CACHE_HOME = getenv("XDG_CACHE_HOME");
    if (XDG_CACHE_HOME == NULL) {
        cache = QDir::homePath() + "/.cache";
    } else {
        cache = QString(XDG_CACHE_HOME);
    }
    return cache;
}

QString TPaths::getDataSubDir(const QString& subdir) {

    if (TPaths::portable) {
        return qApp->applicationDirPath() + "/" + subdir;
    }

#if defined(Q_OS_WIN)
    return qApp->applicationDirPath() + "/" + subdir;
#else
    QString share = "/" + TConfig::PROGRAM_ORG + "/" + TConfig::PROGRAM_ID
            + "/" + subdir;

    // Try ~/.local
    const char* XDG_DATA_HOME = getenv("XDG_DATA_HOME");
    QString path;
    if (XDG_DATA_HOME == NULL) {
        path = QDir::homePath() + "/.local/share" + share;
    } else {
        path = QString(XDG_DATA_HOME) + share;
    }
    QFileInfo fi(path);
    if (fi.isDir()) {
        return path;
    }

    // Try /usr/local
    path = "/usr/local/share" + share;
    fi.setFile(path);
    if (fi.isDir()) {
        return path;
    }

    // Return system wide dir
    return "/usr/share" + share;
#endif
}

QString TPaths::translationPath() {
    return getDataSubDir("translations");
}

QString TPaths::themesPath() {
    return getDataSubDir("themes");
}

QString TPaths::shortcutsPath() {
    return getDataSubDir("shortcuts");
}

QString TPaths::qtTranslationPath() {
    return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString TPaths::subtitleStyleFileName() {
    return configDir + QDir::separator() + "styles.ass";
}

QString TPaths::playerInfoFileName() {
    return dataPath() +  "/player_info_version_3.ini";
}

QString TPaths::fileSettingsFileName() {
    return dataPath() +  "/" + TConfig::PROGRAM_ID + "_files.ini";
}

QString TPaths::fileSettingsHashPath() {
    return dataPath() +  "/file_settings";
}

} // namespace Settings


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

QString TPaths::config_path;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TPaths)


QString TPaths::location(TLocation type) {

    QString path;

#ifdef PORTABLE_APP
    path = qApp->applicationDirPath();
#else

    // Switch to roaming on Windows
    if (type == DataLocation) {
        type = AppDataLocation;
    }

    path = QStandardPaths::writableLocation(
        static_cast<QStandardPaths::StandardLocation>(type));
#endif

    WZDEBUG(QString("Returning '%1' for %2").arg(path).arg(type));
    return path;
}

void TPaths::setConfigPath() {

#if defined(PORTABLE_APP)
    config_path = qApp->applicationDirPath();
#else
#if defined(Q_OS_WIN)
    config_path = location(TLocation::DataLocation);
#else
    const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
    if (XDG_CONFIG_HOME == NULL) {
        config_path = QDir::homePath() + "/.config";
    } else {
        config_path = QString(XDG_CONFIG_HOME);
    }
    config_path = config_path + "/" + TConfig::PROGRAM_ID;
#endif
#endif

    WZINFO(QString("Config path set to '%1'").arg(config_path));

    // Create config directory
#ifndef PORTABLE_APP
    QDir dir(config_path);
    if (!dir.mkpath(config_path)) {
        WZERROR(QString("Failed to create configuration directory '%1'. %2")
                .arg(config_path).arg(strerror(errno)));
    }
#endif

} // void TPaths::setConfigPath()

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

static QString getDataSubDir(const QString& subdir) {

#if defined(Q_OS_WIN) || defined(PORTABLE_APP)
    return qApp->applicationDirPath() + "/" + subdir;
#else

    QString share = "/" + TConfig::PROGRAM_ID + "/" + subdir;

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

QString TPaths::iniFileName() {
    return config_path + QDir::separator() + TConfig::PROGRAM_ID + ".ini";
}

QString TPaths::subtitleStyleFileName() {
    return config_path + QDir::separator() + "styles.ass";
}

} // namespace Settings


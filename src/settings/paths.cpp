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

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QFile>
#include <QDir>
#include <QRegExp>

#ifdef Q_OS_WIN
#include <QIODevice>
#include "settings/preferences.h"
#else
#include <stdlib.h>
#endif

#include "wzdebug.h"
#include "config.h"

namespace Settings {

QString TPaths::config_path;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Settings::TPaths)


QString TPaths::location(TLocation type) {

    QString path;

#ifdef PORTABLE_APP
    path = qApp->applicationDirPath();
#else

#if QT_VERSION >= 0x050400
    // Switch to roaming on Windows
    if (type == DataLocation) {
        type = AppDataLocation;
    }
#endif

    path = QStandardPaths::writableLocation(
        static_cast<QStandardPaths::StandardLocation>(type));
#endif

    WZDEBUG("returning '" +path + "' for " + QString::number(type));
    return path;
}

void TPaths::setConfigPath(const QString& path) {

    // Set config_path
    if (path.isEmpty()) {

#ifdef PORTABLE_APP
        config_path = qApp->applicationDirPath();
#else
#if defined(Q_OS_WIN)
        config_path = location(TLocation::DataLocation);
#else
        const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
        if (XDG_CONFIG_HOME != NULL) {
            config_path = QString(XDG_CONFIG_HOME) + "/" + TConfig::PROGRAM_ID;
        } else {
            config_path = QDir::homePath() + "/.config/" + TConfig::PROGRAM_ID;
        }
#endif
#endif

    } else {
        config_path = path;
    }
    WZINFO("config path set to '" + config_path + "'");

    // Create config directory
#ifndef PORTABLE_APP
    QDir dir(config_path);
    if (!dir.mkpath(config_path)) {
        WZERROR("failed to create '" + config_path + "'");
    }
#endif

} // void TPaths::setConfigPath()

QString TPaths::dataPath() {

#ifdef DATA_PATH
    QString path = QString(DATA_PATH);
    if (!path.isEmpty()) {
        return path;
    }
#endif

    return qApp->applicationDirPath();
}

QString TPaths::translationPath() {

#ifdef TRANSLATION_PATH
    QString path = QString(TRANSLATION_PATH);
    if (!path.isEmpty())
        return path;
#endif

    return qApp->applicationDirPath() + "/translations";
}

QString TPaths::docPath() {

#ifdef DOC_PATH
    QString path = QString(DOC_PATH);
    if (!path.isEmpty())
        return path;
#endif

    return qApp->applicationDirPath() + "/docs";
}

QString TPaths::themesPath() {

#ifdef THEMES_PATH
    QString path = QString(THEMES_PATH);
    if (!path.isEmpty())
        return path;
#endif

    return qApp->applicationDirPath() + "/themes";
}

QString TPaths::shortcutsPath() {

#ifdef SHORTCUTS_PATH
    QString path = QString(SHORTCUTS_PATH);
    if (!path.isEmpty())
        return path;
#endif

    return qApp->applicationDirPath() + "/shortcuts";
}

QString TPaths::qtTranslationPath() {
    return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString TPaths::doc(const QString& file, QString locale, bool english_fallback) {

    if (locale.isEmpty()) {
        locale = QLocale::system().name();
    }

    QString f = docPath() + "/" + locale + "/" + file;
    if (QFile::exists(f))
        return f;

    if (locale.indexOf(QRegExp("_[A-Z]+")) >= 0) {
        locale.replace(QRegExp("_[A-Z]+"), "");
        f = docPath() + "/" + locale + "/" + file;
        if (QFile::exists(f))
            return f;
    }

    if (english_fallback) {
        f = docPath() + "/en/" + file;
        return f;
    }

    return QString::null;
}

QString TPaths::iniPath() {
    return config_path + QDir::separator() + TConfig::PROGRAM_ID + ".ini";
}

QString TPaths::subtitleStyleFile() {
    return config_path + "/styles.ass";
}

#ifdef Q_OS_WIN
QString TPaths::fontPathPlayer(const QString& bin) {
    return QFileInfo(bin).absolutePath() + "/fonts";
}

QStringList TPaths::fonts(const QString& font_dir) {
    WZDEBUG("retrieving '" + font_dir + "'");

    QDir dir(font_dir);
    return dir.entryList(QStringList() << "*.ttf" << "*.otf", QDir::Files);
}

QString TPaths::fontPath() {

    QString path = fontPathPlayer(pref->player_bin);
    if (fonts(path).count() > 0) {
        return path;
    }

    if (pref->player_id == Settings::TPreferences::ID_MPLAYER) {
        path = fontPathPlayer(pref->mpv_bin);
    } else {
        path = fontPathPlayer(pref->mplayer_bin);
    }
    if (fonts(path).count() > 0) {
        return path;
    }
    return qApp->applicationDirPath() + "/open-fonts";
}

QString TPaths::fontConfigFilename() {
    return configPath() + "/fonts.conf";
}

void TPaths::createFontFile() {
    WZDEBUG("");

    QString output = fontConfigFilename();
    QString fontDir = fontPath();

    // Check if the WZPlayer font config file already exists
    // and uses the current font dir
    if (QFile::exists(output)) {
        QFile i(output);
        if (i.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString text = i.readAll();
            if (text.contains("<dir>" + fontDir + "</dir>")) {
                logger()->info("createFontFile: reusing existing font config"
                             " file '%1' which uses font directory '%2'",
                             output, fontDir);
                return;
            }
        }
    }

    // Use the font file from the selected font dir
    QString input = fontDir + "/fonts.conf";
    if (!QFile::exists(input)) {
        WZWARN("fonts.conf '" + input + "' not found");
        return;
    }

    QFile infile(input);
    if (infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString text = infile.readAll();
        text = text.replace("<!-- <dir>WINDOWSFONTDIR</dir> -->", "<dir>WINDOWSFONTDIR</dir>");
        text = text.replace("<dir>WINDOWSFONTDIR</dir>", "<dir>" + fontPath() + "</dir>");

        WZINFO("saving '" + output + "'");
        QFile outfile(output);
        if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            outfile.write(text.toUtf8());
            outfile.close();
        }
    } else {
       WZWARN("failed to open '" + input + "'");
    }
}
#endif // Q_OS_WIN

} // namespace Settings


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

#include "settings/paths.h"
#include <QLibraryInfo>
#include <QLocale>
#include <QFile>
#include <QDir>
#include <QRegExp>

#ifdef Q_OS_WIN
#include <QIODevice>
#else
#include <stdlib.h>
#endif


namespace Settings {

QString TPaths::app_path;
QString TPaths::config_path;


void TPaths::setConfigPath(const QString& path) {

	// Set config_path
	if (path.isEmpty()) {

#ifdef PORTABLE_APP
		config_path = app_path;
#else

		// Normal use case, use home
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
		// TODO: use APPDATA or QDesktopServices::DataLocation
		config_path = QDir::homePath() + "/.smplayer";
#else
		const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
		if (XDG_CONFIG_HOME != NULL) {
			config_path = QString(XDG_CONFIG_HOME) + "/smplayer";
		} else {
			config_path = QDir::homePath() + "/.config/smplayer";
		}
#endif

#endif

	} else {
		config_path = path;
	}
	qDebug("Settings::TPaths::setConfigPath: config directory set to \"%s\"", config_path.toUtf8().constData());

	// Create config directory
#ifndef PORTABLE_APP
	if (!QFile::exists(config_path)) {
		QDir d;
		if (d.mkdir(config_path)) {
			qDebug("Settings::TPaths::setConfigPath: created config dir \"%s\"", config_path.toUtf8().constData());
		} else {
			qWarning("Settings::TPaths::setConfigPath: failed to create \"%s\"", config_path.toUtf8().constData());
		}
	}
#endif

} // void TPaths::setConfigPath(const QString& path)

QString TPaths::dataPath() {

#ifdef DATA_PATH
	QString path = QString(DATA_PATH);
	if (!path.isEmpty()) {
		return path;
	}
#endif

	return app_path;
}

QString TPaths::translationPath() {

#ifdef TRANSLATION_PATH
	QString path = QString(TRANSLATION_PATH);
	if (!path.isEmpty())
		return path;
#endif

	return app_path + "/translations";
}

QString TPaths::docPath() {

#ifdef DOC_PATH
	QString path = QString(DOC_PATH);
	if (!path.isEmpty())
		return path;
#endif

	return app_path + "/docs";
}

QString TPaths::themesPath() {

#ifdef THEMES_PATH
	QString path = QString(THEMES_PATH);
	if (!path.isEmpty())
		return path;
#endif

	return app_path + "/themes";
}

QString TPaths::shortcutsPath() {

#ifdef SHORTCUTS_PATH
	QString path = QString(SHORTCUTS_PATH);
	if (!path.isEmpty())
		return path;
#endif

	return app_path + "/shortcuts";
}

QString TPaths::qtTranslationPath() {
	return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString TPaths::doc(const QString& file, QString locale, bool english_fallback) {

	if (locale.isEmpty()) {
		locale = QLocale::system().name();
	}

	QString f = docPath() + "/" + locale + "/" + file;
	qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
	if (QFile::exists(f))
		return f;

	if (locale.indexOf(QRegExp("_[A-Z]+")) >= 0) {
		locale.replace(QRegExp("_[A-Z]+"), "");
		f = docPath() + "/" + locale + "/" + file;
		qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
		if (QFile::exists(f))
			return f;
	}

	if (english_fallback) {
		f = docPath() + "/en/" + file;
		return f;
	}

	return QString::null;
}

QString TPaths::subtitleStyleFile() {
	return config_path + "/styles.ass";
}

#ifdef Q_OS_WIN
QString TPaths::fontPath() {
	QString path = app_path + "/mplayer/fonts";
	QDir font_dir(path);
	QStringList files = font_dir.entryList(QStringList() << "*.ttf" << "*.otf", QDir::Files);
	//qDebug("Paths:fontPath: files in %s: %d", path.toUtf8().constData(), files.count());
	if (files.count() > 0) {
		return path;
	} else {
		return app_path + "/open-fonts";
	}
}

void TPaths::createFontFile() {
	qDebug("Settings::TPaths::createFontFile");

	QString output = configPath() + "/fonts.conf";

	// Check if the file already exists with the modified path
	if (QFile::exists(output)) {
		QFile i(output);
		if (i.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QString text = i.readAll();
			if (text.contains("<dir>" + fontPath() + "</dir>")) {
				qDebug("Settings::TPaths::createFontFile: file %s already exists with font path. Doing nothing.", output.toUtf8().constData());
				return;
			}
		}
	}

	QString input = app_path + "/mplayer/fonts/fonts.conf";
	if (!QFile::exists(input)) {
		qDebug("Settings::TPaths::createFontFile: %s doesn't exist", input.toUtf8().constData());
		input = app_path + "/mplayer/mpv/fonts.conf";
		if (!QFile::exists(input)) {
			qDebug("Settings::TPaths::createFontFile: %s doesn't exist", input.toUtf8().constData());
			qWarning("Settings::TPaths::createFontFile: failed to create fonts.conf");
			return;
		}
	}

	qDebug("Settings::TPaths::createFontFile: input: %s", input.toUtf8().constData());
	QFile infile(input);
	if (infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QString text = infile.readAll();
		text = text.replace("<!-- <dir>WINDOWSFONTDIR</dir> -->", "<dir>WINDOWSFONTDIR</dir>");
		text = text.replace("<dir>WINDOWSFONTDIR</dir>", "<dir>" + fontPath() + "</dir>");

		qDebug("Settings::TPaths::createFontFile: saving %s", output.toUtf8().constData());
		QFile outfile(output);
		if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			outfile.write(text.toUtf8());
			outfile.close();
		}
	}
}
#endif // Q_OS_WIN

} // namespace Settings


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

#include "paths.h"
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

QString Paths::app_path;
QString Paths::config_path;

void Paths::setAppPath(QString path) {
	app_path = path;
}

QString Paths::appPath() {
	return app_path;
}

void Paths::setConfigPath(const QString& path) {

	// Set config path
	if (path.isEmpty()) {

#ifdef PORTABLE_APP
		config_path = Paths::appPath();
#else
		// If a smplayer.ini exists in the app path, use that path
		// TODO: This is the old behaviour, but should prefer ini in home dir.
		if (QFile::exists(app_path + "/smplayer.ini")) {
			config_path = app_path;
		} else {

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
			const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
			if (XDG_CONFIG_HOME != NULL) {
				config_path = QString(XDG_CONFIG_HOME) + "/smplayer";
			} else {
				config_path = QDir::homePath() + "/.config/smplayer";
			}
#else
			config_path = QDir::homePath() + "/.smplayer";
#endif
		}
#endif
	}

	// Create config directory
#ifndef PORTABLE_APP
	if (!QFile::exists(config_path)) {
		QDir d;
		if (!d.mkdir(config_path)) {
			qWarning("Paths::setConfigPath: failed to create %s", config_path.toUtf8().data());
		}

		// Screenshot folder already created in preferences.cpp if Qt >= 4.4
#if QT_VERSION < 0x040400
		QString s = config_path + "/screenshots";
		if (!d.mkdir(s)) {
			qWarning("Paths::setConfigPath: failed to create %s", s.toUtf8().data());
		}
#endif
	}
#endif

}

QString Paths::dataPath() {
#ifdef DATA_PATH
	QString path = QString(DATA_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath();
#else
	return appPath();
#endif
}

QString Paths::translationPath() {
#ifdef TRANSLATION_PATH
	QString path = QString(TRANSLATION_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/translations";
#else
	return appPath() + "/translations";
#endif
}

QString Paths::docPath() {
#ifdef DOC_PATH
	QString path = QString(DOC_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/docs";
#else
	return appPath() + "/docs";
#endif
}

QString Paths::themesPath() {
#ifdef THEMES_PATH
	QString path = QString(THEMES_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/themes";
#else
	return appPath() + "/themes";
#endif
}

QString Paths::shortcutsPath() {
#ifdef SHORTCUTS_PATH
	QString path = QString(SHORTCUTS_PATH);
	if (!path.isEmpty())
		return path;
	else
		return appPath() + "/shortcuts";
#else
	return appPath() + "/shortcuts";
#endif
}

QString Paths::qtTranslationPath() {
	return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
}

QString Paths::doc(QString file, QString locale, bool english_fallback) {
	if (locale.isEmpty()) {
		locale = QLocale::system().name();
	}

	QString f = docPath() + "/" + locale + "/" + file;
	qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
	if (QFile::exists(f)) return f;

	if (locale.indexOf(QRegExp("_[A-Z]+")) != -1) {
		locale.replace(QRegExp("_[A-Z]+"), "");
		f = docPath() + "/" + locale + "/" + file;
		qDebug("Helper:doc: checking '%s'", f.toUtf8().data());
		if (QFile::exists(f)) return f;
	}

	if (english_fallback) {
		f = docPath() + "/en/" + file;
		return f;
	}

	return QString::null;
}

QString Paths::subtitleStyleFile() {
	return configPath() + "/styles.ass";
}

#ifdef Q_OS_WIN
QString Paths::fontPath() {
	QString path = appPath() + "/mplayer/fonts";
	QDir font_dir(path);
	QStringList files = font_dir.entryList(QStringList() << "*.ttf" << "*.otf", QDir::Files);
	//qDebug("Paths:fontPath: files in %s: %d", path.toUtf8().constData(), files.count());
	if (files.count() > 0) {
		return path;
	} else {
		return appPath() + "/open-fonts";
	}
}

void Paths::createFontFile() {
	qDebug("Paths::createFontFile");

	QString output = configPath() + "/fonts.conf";

	// Check if the file already exists with the modified path
	if (QFile::exists(output)) {
		QFile i(output);
		if (i.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QString text = i.readAll();
			if (text.contains("<dir>" + fontPath() + "</dir>")) {
				qDebug("Paths::createFontFile: file %s already exists with font path. Doing nothing.", output.toUtf8().constData());
				return;
			}
		}
	}

	QString input = appPath() + "/mplayer/fonts/fonts.conf";
	if (!QFile::exists(input)) {
		qDebug("Paths::createFontFile: %s doesn't exist", input.toUtf8().constData());
		input = appPath() + "/mplayer/mpv/fonts.conf";
		if (!QFile::exists(input)) {
			qDebug("Paths::createFontFile: %s doesn't exist", input.toUtf8().constData());
			qWarning("Paths::createFontFile: failed to create fonts.conf");
			return;
		}
	}

	qDebug("Paths::createFontFile: input: %s", input.toUtf8().constData());
	QFile infile(input);
	if (infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QString text = infile.readAll();
		text = text.replace("<!-- <dir>WINDOWSFONTDIR</dir> -->", "<dir>WINDOWSFONTDIR</dir>");
		text = text.replace("<dir>WINDOWSFONTDIR</dir>", "<dir>" + fontPath() + "</dir>");

		qDebug("Paths::createFontFile: saving %s", output.toUtf8().constData());
		QFile outfile(output);
		if (outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			outfile.write(text.toUtf8());
			outfile.close();
		}
	}
}
#endif // Q_OS_WIN


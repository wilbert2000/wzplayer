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

#include "helper.h"

#include <QApplication>
#include <QFileInfo>
#include <QColor>
#include <QDir>
#include <QTextCodec>
#include <QWidget>
#include <QUrl>

#include "log4qt/logger.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "extensions.h"
#include "config.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Helper)


QString Helper::formatTime(int secs) {

    bool negative = secs < 0;
	secs = abs(secs);

	int t = secs;
	int hours = (int) t / 3600;
	t -= hours * 3600;
	int minutes = (int) t / 60;
	t -= minutes * 60;
	int seconds = t;

    if (hours == 0) {
        return QString("%1%2:%3").arg(negative ? "-" : "")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1%2:%3:%4").arg(negative ? "-" : "")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    }
}

bool Helper::directoryContainsDVD(QString directory) {

	QDir dir(directory);
	QStringList l = dir.entryList();
	for (int n = 0; n < l.count(); n++) {
		if (l[n].toLower() == "video_ts")
			return true;
	}

	return false;
}

int Helper::qtVersion() {

	QRegExp rx("(\\d+)\\.(\\d+)\\.(\\d+)");
	QString v(qVersion());

	int r = 0;

	if (rx.indexIn(v) >= 0) {
		int n1 = rx.cap(1).toInt();
		int n2 = rx.cap(2).toInt();
		int n3 = rx.cap(3).toInt();
		r = n1 * 1000 + n2 * 100 + n3;
	}

    logger()->debug("qtVersion: Qt runtime version %1 counting as %2",
                    v, QString::number(r));
	return r;
}

QString Helper::clean(const QString& name) {
    logger()->trace("clean: '%1'", name);

    if (name.isEmpty()) {
        return "";
    }
    if (name == TConfig::WZPLAYLIST) {
        return name;
    }

    QString s = QUrl(name).toString(QUrl::RemoveScheme
                                    | QUrl::RemoveAuthority
                                    | QUrl::RemoveQuery
                                    | QUrl::RemoveFragment
                                    | QUrl::StripTrailingSlash);
    s = QFileInfo(s).fileName();
    if (s.isEmpty()) {
        s = name;
    }

    // \w Matches a word character (QChar::isLetterOrNumber(),
    //    QChar::isMark(), or '_')
    // \W Matches a non-word character

    s.replace("_", " ");

    static QRegExp rx1("([^\\s])\\.([^\\s])");
    s.replace(rx1, "\\1 \\2");

    static QRegExp rx2("([^\\s])-([^\\s])");
    s.replace(rx2, "\\1 - \\2");

    static QRegExp rx3("(\\d)[xX](\\d)");
    s.replace(rx3, "\\1 x \\2");

    static QRegExp rx4("([^\\d\\s\\WmMpPcCuU])(\\d+)");
    s.replace(rx4, "\\1 \\2");

    s = s.simplified();
    if (s.length() > 255) {
        s = s.left(252) + "...";
    }

    logger()->trace("Clean: returning '%1'", s);
    return s;
}

QString Helper::cleanName(const QString& name) {
    return clean(name);
}

QString Helper::cleanTitle(const QString& title) {

    foreach(QRegExp rx, TConfig::TITLE_BLACKLIST) {
        if (rx.indexIn(title) >= 0) {
            logger()->info("cleanTitle: '%1' blacklisted on '%2'",
                           title, rx.pattern());
            return "";
        }
    }

    return clean(title);
}

QStringList Helper::searchForConsecutiveFiles(const QString& initial_file) {
    logger()->debug("searchForConsecutiveFiles: initial file: '%1'",
                    initial_file);

	QStringList files_to_add;
	QStringList matching_files;

	QFileInfo fi(initial_file);
	QString basename = fi.completeBaseName();
	QString extension = fi.suffix();
	QString path = fi.absolutePath();

	QDir dir(path);
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Name);

	QRegExp rx("(\\d+)");

	int digits;
	int current_number;
	int pos = 0;
	QString next_name;
	bool next_found = false;
    while  ((pos = rx.indexIn(basename, pos)) >= 0) {
        logger()->debug("searchForConsecutiveFiles: captured '%1'", rx.cap(1));
		digits = rx.cap(1).length();
		current_number = rx.cap(1).toInt() + 1;
        next_name = basename.left(pos) + QString("%1")
                    .arg(current_number, digits, 10, QLatin1Char('0'));
		next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
		next_name += "*." + extension;
        logger()->debug("searchForConsecutiveFiles: next name '%1'",
                        next_name);
		matching_files = dir.entryList((QStringList)next_name);

		if (!matching_files.isEmpty()) {
			next_found = true;
			break;
		}
		pos  += digits;
	}

	if (next_found) {
        while (!matching_files.isEmpty()) {
            logger()->debug("searchForConsecutiveFiles: added '"
                          + matching_files[0] + "'");
			files_to_add << path  + "/" + matching_files[0];
			current_number++;
            next_name = basename.left(pos) + QString("%1")
                        .arg(current_number, digits, 10, QLatin1Char('0'));
			next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
			next_name += "*." + extension;
			matching_files = dir.entryList((QStringList)next_name);
            logger()->debug("searchForConsecutiveFiles: looking for '"
                            + next_name + "'");
		}
	}

	return files_to_add;
}

QStringList Helper::filesInDirectory(const QString& initial_file,
                                     const QStringList& filter) {
    logger()->debug("filesInDirectory: initial_file: '" + initial_file + "'");

	QFileInfo fi(initial_file);
	QString current_file = fi.fileName();
	QString path = fi.absolutePath();

	QDir d(path);
	QStringList all_files = d.entryList(filter, QDir::Files);

	QStringList r;
	for (int n = 0; n < all_files.count(); n++) {
		if (all_files[n] != current_file) {
			QString s = path +"/" + all_files[n];
			r << s;
		}
	}

	return r;
}

QStringList Helper::filesForPlaylist(const QString & initial_file,
									 Settings::TPreferences::TAddToPlaylist filter) {
	QStringList res;

	if (filter == TPreferences::ConsecutiveFiles) {
		res = searchForConsecutiveFiles(initial_file);
	} else {
        QStringList exts;
		switch (filter) {
            case TPreferences::VideoFiles:
                exts = extensions.video().forDirFilter();
                break;
            case TPreferences::AudioFiles:
                exts = extensions.audio().forDirFilter();
                break;
            case TPreferences::MultimediaFiles:
                exts = extensions.videoAndAudio().forDirFilter();
                break;
			default: ;
		}
        if (!exts.isEmpty())
            res = Helper::filesInDirectory(initial_file, exts);
	}

	return res;
}

QString Helper::findExecutable(const QString& name) {

    // Name is executable?
    QFileInfo fi(name);
    if (fi.isFile() && fi.isExecutable()) {
        logger()->debug("findExecutable: found '" + name + "'");
        return fi.absoluteFilePath();
    }

    // Search PATH
    char sep =
#ifdef Q_OS_LINUX
            ':';
#else
            ';';
#endif

    QByteArray env = qgetenv("PATH");
    QStringList search_paths = QString::fromLocal8Bit(env.constData())
                               .split(sep, QString::SkipEmptyParts);

#ifdef Q_OS_WIN
    // Add mplayer subdir of app dir to end of PATH
    search_paths << qApp->applicationDirPath() + "/mplayer"
                 << qApp->applicationDirPath() + "/mpv";

    // Add program files
    QString program_files(qgetenv("PROGRAMFILES"));
    if (!program_files.isEmpty()) {
        search_paths << program_files + "/mplayer" << program_files + "/mpv";
    }
#endif

    // Add app dir to end of PATH
    search_paths << qApp->applicationDirPath();

    for (int n = 0; n < search_paths.count(); n++) {
        QString candidate = search_paths[n] + "/" + name;
        fi.setFile(candidate);
        if (fi.isFile() && fi.isExecutable()) {
            logger()->info("findExecutable: found '" + fi.absoluteFilePath()
                         + "'");
            return fi.absoluteFilePath();
        }
        logger()->debug("findExecutable: '" + candidate + "' not executable");
    }

    // Name not found
    logger()->info("findExecutable: name '" + name + "' not found");
    return QString();
}


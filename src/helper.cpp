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
#include <QDebug>
#include "settings/paths.h"
#include "settings/preferences.h"
#include "extensions.h"

#ifdef Q_OS_WIN
#include <windows.h> // For the screensaver stuff
#endif

using namespace Settings;

QString Helper::formatTime(int secs) {
	bool negative = (secs < 0);
	secs = abs(secs);

	int t = secs;
	int hours = (int) t / 3600;
	t -= hours * 3600;
	int minutes = (int) t / 60;
	t -= minutes * 60;
	int seconds = t;

	//qDebug() << "Helper::formatTime:" << hours << ":" << minutes << ":" << seconds;

	return QString("%1%2:%3:%4").arg(negative ? "-" : "").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

QString Helper::timeForJumps(int secs) {
    int minutes = (int) secs / 60;
	int seconds = secs % 60;

	if (minutes==0) {
		return QObject::tr("%n second(s)", "", seconds);
	} else {
		if (seconds==0) 
			return QObject::tr("%n minute(s)", "", minutes);
		else {
			QString m = QObject::tr("%n minute(s)", "", minutes);
			QString s = QObject::tr("%n second(s)", "", seconds);
			return QObject::tr("%1 and %2").arg(m).arg(s);
		}
	}
}

bool Helper::directoryContainsDVD(QString directory) {
	//qDebug("Helper::directoryContainsDVD: '%s'", directory.latin1());

	QDir dir(directory);
	QStringList l = dir.entryList();
	bool valid = false;
	for (int n=0; n < l.count(); n++) {
		//qDebug("  * entry %d: '%s'", n, l[n].toUtf8().data());
		if (l[n].toLower() == "video_ts") valid = true;
	}

	return valid;
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

	qDebug() << "Helper::qtVersion: Qt runtime version" <<  v << "counting as" << r;
	return r;
}

QStringList Helper::searchForConsecutiveFiles(const QString & initial_file) {
	qDebug("Helper::searchForConsecutiveFiles: initial_file: '%s'", initial_file.toUtf8().constData());

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
	qDebug("Helper::searchForConsecutiveFiles: trying to find consecutive files");
	while  ((pos = rx.indexIn(basename, pos)) != -1) {
		qDebug("Helper::searchForConsecutiveFiles: captured: %s",rx.cap(1).toUtf8().constData());
		digits = rx.cap(1).length();
		current_number = rx.cap(1).toInt() + 1;
		next_name = basename.left(pos) + QString("%1").arg(current_number, digits, 10, QLatin1Char('0'));
		next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
		next_name += "*." + extension;
		qDebug("Helper::searchForConsecutiveFiles: next name = %s",next_name.toUtf8().constData());
		matching_files = dir.entryList((QStringList)next_name);

		if (!matching_files.isEmpty()) {
			next_found = true;
			break;
		}
		qDebug("Helper::searchForConsecutiveFiles: pos = %d",pos);
		pos  += digits;
	}

	if (next_found) {
		qDebug("Helper::searchForConsecutiveFiles: adding consecutive files");
		while (!matching_files.isEmpty()) {
			qDebug("Helper::searchForConsecutiveFiles: '%s' exists, added to the list", matching_files[0].toUtf8().constData());
			files_to_add << path  + "/" + matching_files[0];
			current_number++;
			next_name = basename.left(pos) + QString("%1").arg(current_number, digits, 10, QLatin1Char('0'));
			next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
			next_name += "*." + extension;
			matching_files = dir.entryList((QStringList)next_name);
			qDebug("Helper::searchForConsecutiveFiles: looking for '%s'", next_name.toUtf8().constData());
		}
	}

	return files_to_add;
}

QStringList Helper::filesInDirectory(const QString & initial_file, const QStringList & filter) {
	qDebug("Helper::filesInDirectory: initial_file: %s", initial_file.toUtf8().constData());
	//qDebug() << "Helper::filesInDirectory: filter:" << filter;

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

	//qDebug() << "Helper::filesInDirectory: result:" << r;

	return r;
}

QStringList Helper::filesForPlaylist(const QString & initial_file,
									 Settings::TPreferences::TAutoAddToPlaylistFilter filter) {
	QStringList res;

	if (filter == TPreferences::ConsecutiveFiles) {
		res = searchForConsecutiveFiles(initial_file);
	} else {
		TExtensions e;
		QStringList exts;
		switch (filter) {
			case TPreferences::VideoFiles: exts = e.video().forDirFilter(); break;
			case TPreferences::AudioFiles: exts = e.audio().forDirFilter(); break;
			case TPreferences::MultimediaFiles: exts = e.multimedia().forDirFilter(); break;
			default: ;
		}
		if (!exts.isEmpty()) res = Helper::filesInDirectory(initial_file, exts);
	}

	return res;
}

#ifdef Q_OS_WIN
// Check for Windows shortcuts
QStringList Helper::resolveSymlinks(const QStringList & files) {
	QStringList list = files;
	for (int n=0; n < list.count(); n++) {
		QFileInfo fi(list[n]);
		if (fi.isSymLink()) {
			list[n] = fi.symLinkTarget();
		}
	}
	return list;
}
#endif

QString Helper::findExecutable(const QString& name) {

	// Name is executable?
	QFileInfo fi(name);
	if (fi.isFile() && fi.isExecutable()) {
		qDebug() << "Helper::findExecutable: found" << name;
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
	QStringList search_paths = QString::fromLocal8Bit(env.constData()).split(sep, QString::SkipEmptyParts);

#ifdef Q_OS_WIN
	// Add mplayer subdir of app dir to end of PATH
	search_paths << TPaths::appPath() + "/mplayer" << TPaths::appPath() + "/mpv";
	// TODO: add some more...
#endif

	// Add app dir to end of PATH
	search_paths << TPaths::appPath();

	for (int n = 0; n < search_paths.count(); n++) {
		QString candidate = search_paths[n] + "/" + name;
		fi.setFile(candidate);
		if (fi.isFile() && fi.isExecutable()) {
			qDebug() << "Helper::findExecutable: found" << fi.absoluteFilePath();
			return fi.absoluteFilePath();
		}
		qDebug() << "Helper::findExecutable:" << candidate << "not accepted";
	}

	// Name not found
	qDebug() << "Helper::findExecutable: name" << name << "not found";
	return QString();
}


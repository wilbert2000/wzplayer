#include "wzfiles.h"
#include "wzdebug.h"
#include "extensions.h"

#include <QApplication>
#include <QDir>


LOG4QT_DECLARE_STATIC_LOGGER(logger, TWZFiles)


bool TWZFiles::directoryContainsDVD(const QString& directory) {

    QDir dir(directory);
    QStringList entries = dir.entryList();
    for (int i = 0; i < entries.count(); i++) {
        if (entries[i].toLower() == "video_ts") {
            return true;
        }
    }

    return false;
}

QString TWZFiles::findExecutable(const QString& name) {

    QFileInfo fi(name);
    if (fi.isFile() && fi.isExecutable()) {
        WZDEBUG("found '" + name + "'");
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
            WZINFO("found '" + fi.absoluteFilePath() + "'");
            return fi.absoluteFilePath();
        }
        WZDEBUG("'" + candidate + "' not executable");
    }

    // Name not found
    WZINFO("name '" + name + "' not found");
    return QString();
}

QStringList TWZFiles::searchForConsecutiveFiles(const QString& initial_file) {
    WZDEBUG("initial file '" + initial_file + "'");

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
        WZDEBUG("captured '" + rx.cap(1) + "'");
        digits = rx.cap(1).length();
        current_number = rx.cap(1).toInt() + 1;
        next_name = basename.left(pos) + QString("%1")
                    .arg(current_number, digits, 10, QLatin1Char('0'));
        next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
        next_name += "*." + extension;
        WZDEBUG("next name '" + next_name + "'");
        matching_files = dir.entryList((QStringList)next_name);

        if (!matching_files.isEmpty()) {
            next_found = true;
            break;
        }
        pos  += digits;
    }

    if (next_found) {
        while (!matching_files.isEmpty()) {
            WZDEBUG("added '" + matching_files[0] + "'");
            files_to_add << path  + "/" + matching_files[0];
            current_number++;
            next_name = basename.left(pos) + QString("%1")
                        .arg(current_number, digits, 10, QLatin1Char('0'));
            next_name.replace(QRegExp("([\\[\\]?*])"), "[\\1]");
            next_name += "*." + extension;
            matching_files = dir.entryList((QStringList)next_name);
            WZDEBUG("looking for '" + next_name + "'");
        }
    }

    return files_to_add;
}

QStringList TWZFiles::filesInDirectory(const QString& initial_file,
                                     const QStringList& filter) {
    WZDEBUG("initial_file: '" + initial_file + "'");

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

QStringList TWZFiles::filesForPlaylist(const QString & initial_file,
                               Settings::TPreferences::TAddToPlaylist filter) {

    QStringList res;

    if (filter == Settings::TPreferences::ConsecutiveFiles) {
        res = searchForConsecutiveFiles(initial_file);
    } else {
        QStringList exts;
        switch (filter) {
            case Settings::TPreferences::VideoFiles:
                exts = extensions.video().forDirFilter();
                break;
            case Settings::TPreferences::AudioFiles:
                exts = extensions.audio().forDirFilter();
                break;
            case Settings::TPreferences::MultimediaFiles:
                exts = extensions.videoAndAudio().forDirFilter();
                break;
            default: ;
        }
        if (!exts.isEmpty()) {
            res = filesInDirectory(initial_file, exts);
        }
    }

    return res;
}


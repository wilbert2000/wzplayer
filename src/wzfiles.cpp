#include "wzfiles.h"
#include "wzdebug.h"

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

bool TWZFiles::directoryIsEmpty(const QString& directory) {

    int c = QDir(directory).entryList(QDir::AllDirs
                                     | QDir::Files
                                     | QDir::Drives
                                     | QDir::NoDotAndDotDot
                                     | QDir::Hidden
                                     | QDir::System).count();
    WZDEBUG(QString("Found %1 files in '%2'").arg(c).arg(directory));
    return c == 0;
}

// TODO: see QStandardPaths::findExecutable()
QString TWZFiles::findExecutable(const QString& name) {

    QFileInfo fi(name);
    if (fi.isFile() && fi.isExecutable()) {
        WZTRACE("Found '" + name + "'");
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
            WZTRACE("Found '" + fi.absoluteFilePath() + "'");
            return fi.absoluteFilePath();
        }
    }

    // Name not found
    WZINFO("Executable '" + name + "' not found");
    return QString();
}

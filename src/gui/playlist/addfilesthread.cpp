#include "gui/playlist/addfilesthread.h"

#include <QSettings>
#include <QTreeWidgetItem>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QRegExp>
#include <QTextStream>
#include <QTextCodec>
#include "gui/playlist/playlistwidgetitem.h"
#include "discname.h"
#include "extensions.h"
#include "config.h"


namespace Gui {
namespace Playlist {

TAddFilesThread::TAddFilesThread(QObject *parent,
                                 const QStringList& aFiles,
                                 const QString& aPlaylistPath,
                                 bool recurseSubDirs,
                                 bool aSearchForItems) :
    QThread(parent),
    files(aFiles),
    playlistPath(aPlaylistPath),
    root(0),
    currentItem(0),
    stopRequested(false),
    recurse(recurseSubDirs),
    searchForItems(aSearchForItems) {
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    root = new QTreeWidgetItem();
    addFiles();
}

QTreeWidgetItem* TAddFilesThread::cleanAndAddItem(QString filename,
                                                  QString name,
                                                  double duration,
                                                  QTreeWidgetItem* parent) {

    bool protect_name = !name.isEmpty();
    QString alt_name = filename;

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);

#ifdef Q_OS_WIN
    // Check for Windows shortcuts
    if (fi.isSymLink()) {
        filename = fi.symLinkTarget();
    }
#endif

    if (fi.exists()) {
        filename = QDir::toNativeSeparators(fi.absoluteFilePath());
        alt_name = fi.fileName();
    } else if (!playlistPath.isEmpty()) {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            filename = QDir::toNativeSeparators(fi.absoluteFilePath());
            alt_name = fi.fileName();
        }
    }

    if (name.isEmpty()) {
        name = alt_name;
    }

    if (parent == 0) {
        parent = root;
    }
    TPlaylistWidgetItem* i = new TPlaylistWidgetItem(parent,
                                                     0,
                                                     filename,
                                                     name,
                                                     duration,
                                                     false);

    // Protect name
    if (protect_name) {
        i->setEdited(true);
    }

    return i;
}

QTreeWidgetItem* TAddFilesThread::openM3u(const QString& aPlaylistFileName,
                                          QTreeWidgetItem* parent) {


    QRegExp info("^#EXTINF:(.*),(.*)");

    // When inserting directly into the root, the name of the playlist gets
    // lost, so save it
    playlistFileName = aPlaylistFileName;
    QFileInfo fi(playlistFileName);
    // Path to use for relative filenames in playlist
    playlistPath = fi.path();
    latestDir = fi.absolutePath();
    emit setWinTitle(fi.fileName());

    bool utf8 = fi.suffix().toLower() == "m3u8";

    QFile f(playlistFileName);
    if (!f.open(QIODevice::ReadOnly)) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName), 6000);
        return 0;
    }

    QTextStream stream(&f);
    if (utf8) {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    QTreeWidgetItem* result = 0;
    bool setResult = true;
    if (parent != root || files.count() > 1) {
        // Put playlist in a folder if child or more than one file
        result = new TPlaylistWidgetItem(parent,
                                         0,
                                         playlistFileName,
                                         fi.fileName(),
                                         0,
                                         true);
        parent = result;
        setResult = false;
    }

    QString name;
    double duration = 0;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        // Ignore empty lines
        if (line.isEmpty()) {
            continue;
        }

        if (info.indexIn(line) >= 0) {
            duration = info.cap(1).toDouble();
            name = info.cap(2);
        } else if (line.startsWith("#")) {
            // Ignore comments
        } else {
            QTreeWidgetItem* w = cleanAndAddItem(line, name, duration, parent);
            if (setResult) {
                // Pass last as result when inserting into root
                result = w;
            }
            name = "";
            duration = 0;
        }
    }

    f.close();

    return result;
}

QTreeWidgetItem* TAddFilesThread::openPls(const QString& aPlaylistFileName,
                                          QTreeWidgetItem* parent) {

    // When inserting directly into the root, the name of the playlist gets
    // lost, so save it
    playlistFileName = aPlaylistFileName;
    QFileInfo fi(playlistFileName);
    // Path to use for relative filenames in playlist
    playlistPath = fi.path();
    latestDir = fi.absolutePath();
    emit setWinTitle(fi.fileName());

    QSettings set(playlistFileName, QSettings::IniFormat);
    set.beginGroup("playlist");

    QTreeWidgetItem* result = 0;
    if (set.status() == QSettings::NoError) {
        bool setResult = true;
        if (parent != root || files.count() > 1) {
            // Put playlist in a folder if child or more than one file
            result = new TPlaylistWidgetItem(parent,
                                             0,
                                             playlistFileName,
                                             fi.fileName(),
                                             0,
                                             true);
            parent = result;
            setResult = false;
        }

        QString filename;
        QString name;
        double duration;

        int num_items = set.value("NumberOfEntries", 0).toInt();
        for (int n = 1; n <= num_items; n++) {
            filename = set.value("File" + QString::number(n), "").toString();
            name = set.value("Title" + QString::number(n), "").toString();
            duration = (double) set.value("Length"
                                          + QString::number(n), 0).toInt();
            QTreeWidgetItem* w =cleanAndAddItem(filename, name, duration,
                                                parent);
            if (setResult) {
                // Pass last as result when inserting into root
                result = w;
            }
        }
    } else {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName), 6000);
    }

    set.endGroup();
    return result;
}

TPlaylistWidgetItem* TAddFilesThread::findFilename(const QString&) {
    return 0;
}

QTreeWidgetItem* TAddFilesThread::addFile(QTreeWidgetItem* parent,
                                          const QString &filename) {
    // Note: currently addFile loads playlists and addDirectory skips them,
    // giving a nice balance. Load if the individual file is requested,
    // skip when adding a directory.

    TPlaylistWidgetItem* existing_item = 0;
    TPlaylistWidgetItem* item;
    QFileInfo fi(filename);
    if (fi.exists()) {
        QString ext = fi.suffix().toLower();
        if (ext == "m3u" || ext == "m3u8") {
            return openM3u(filename, parent);
        }
        if (ext == "pls") {
            return openPls(filename, parent);
        }
        if (searchForItems) {
            existing_item = findFilename(filename);
        }
        item = new TPlaylistWidgetItem(parent,
                                       0,
                                       QDir::toNativeSeparators(filename),
                                       fi.fileName(),
                                       0,
                                       false);
    } else {
        if (searchForItems) {
            existing_item = findFilename(filename);
        }
        QString name;
        TDiscName disc(filename);
        if (disc.valid) {
            // See also TTitleData::getDisplayName() from titletracks.cpp
            if (disc.protocol == "cdda" || disc.protocol == "vcd") {
                name = tr("Track %1").arg(QString::number(disc.title));
            } else {
                name = tr("Title %1").arg(QString::number(disc.title));
            }
        } else {
            name = filename;
        }
        item = new TPlaylistWidgetItem(parent, 0, filename, name, 0, false);
    }

    if (existing_item) {
        item->setName(existing_item->name());
        item->setDuration(existing_item->duration());
        item->setPlayed(existing_item->played());
    }

    return item;
}

QTreeWidgetItem* TAddFilesThread::addDirectory(QTreeWidgetItem* parent,
                                               const QString &dir) {

    static TExtensions ext;
    static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

    emit displayMessage(dir, 0);

    QString playlist = dir + "/" + TConfig::PROGRAM_ID + ".m3u8";
    if (QFileInfo(playlist).exists()) {
        return openM3u(playlist, parent);
    }

    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(0,
                                                     0,
                                                     dir,
                                                     QDir(dir).dirName(),
                                                     0,
                                                     true);

    QFileInfo fi;
    QDir directory(dir);
    foreach(const QString& filename, directory.entryList()) {
        if (stopRequested) {
            break;
        }
        if (filename != "." && filename != "..") {
            fi.setFile(dir, filename);
            if (fi.isDir()) {
                if (recurse) {
                    addDirectory(w, fi.absoluteFilePath());
                }
            } else if (rx_ext.indexIn(fi.suffix()) >= 0) {
                addFile(w, fi.absoluteFilePath());
            }
        }
    }

    if (w->childCount()) {
        latestDir = dir;
        parent->addChild(w);
        return w;
    }

    delete w;
    return 0;
}

void TAddFilesThread::addFiles() {

    bool first = true;
    foreach(QString filename, files) {
        if (stopRequested) {
            break;
        }

        if (filename.isEmpty()) {
            continue;
        }

        if (filename.startsWith("file:")) {
            filename = QUrl(filename).toLocalFile();
        }

        QFileInfo fi(filename);

#ifdef Q_OS_WIN
        // Check for Windows shortcuts
        if (fi.isSymLink()) {
            fi.setFile(fi.symLinkTarget());
        }
#endif

        QTreeWidgetItem* result;
        if (fi.isDir()) {
            result = addDirectory(root, fi.absoluteFilePath());
        } else {
            result = addFile(root, fi.absoluteFilePath());
        }

        if (result) {
            result->setSelected(true);
            if (first) {
                currentItem = result;
                first = false;
            }
        }
    }
}

} // namespace Playlist
} // namespace Gui

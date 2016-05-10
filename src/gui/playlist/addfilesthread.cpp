#include "gui/playlist/addfilesthread.h"

#include <QSettings>
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
#include "iconprovider.h"


namespace Gui {
namespace Playlist {


TAddFilesThread::TAddFilesThread(QObject *parent,
                                 const QStringList& aFiles,
                                 bool recurseSubDirs,
                                 bool aSearchForItems) :
    QThread(parent),
    files(aFiles),
    root(0),
    currentItem(0),
    abortRequested(false),
    stopRequested(false),
    recurse(recurseSubDirs),
    searchForItems(aSearchForItems) {
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    root = new TPlaylistWidgetItem(iconProvider.folderIcon);
    addFiles();
    if (abortRequested) {
        delete root;
        root = 0;
    }
}

TPlaylistWidgetItem* TAddFilesThread::cleanAndAddItem(
        QString filename,
        QString name,
        double duration,
        TPlaylistWidgetItem* parent) {

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

    if (parent == 0) {
        parent = root;
    }

    if (fi.isDir()) {
        return addDirectory(parent, fi.absoluteFilePath());
    }

    if (name.isEmpty()) {
        name = alt_name;
    }

    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(parent, 0, filename, name,
                                                     duration, false,
                                                     iconProvider.icon(fi));

    // Protect name
    if (protect_name) {
        w->setEdited(true);
    }

    return w;
}

TPlaylistWidgetItem* TAddFilesThread::openM3u(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {


    QRegExp info("^#EXTINF:(.*),(.*)");

    QFileInfo fi(playlistFileName);
    // Path to use for relative filenames in playlist
    playlistPath = fi.path();

    bool utf8 = fi.suffix().toLower() == "m3u8";

    QFile f(playlistFileName);
    if (!f.open(QIODevice::ReadOnly)) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName),
                            6000);
        return 0;
    }

    QTextStream stream(&f);
    if (utf8) {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    // Put playlist in a folder
    TPlaylistWidgetItem* result = new TPlaylistWidgetItem(0, 0,
        playlistFileName, fi.fileName(), 0, true, iconProvider.folderIcon);

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
            cleanAndAddItem(line, name, duration, result);
            name = "";
            duration = 0;
        }
    }

    f.close();

    if (result->childCount()) {
        parent->addChild(result);
        latestDir = fi.absolutePath();
        return result;
    }

    delete result;
    return 0;
}

TPlaylistWidgetItem* TAddFilesThread::openPls(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {

    QFileInfo fi(playlistFileName);
    // Path to use for relative filenames in playlist
    playlistPath = fi.path();
    QSettings set(playlistFileName, QSettings::IniFormat);
    set.beginGroup("playlist");
    if (set.status() != QSettings::NoError) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName),
                            6000);
        return 0;
    }

    // Put playlist in a folder
    TPlaylistWidgetItem* result = new TPlaylistWidgetItem(0, 0,
        playlistFileName, fi.fileName(), 0, true, iconProvider.folderIcon);

    QString filename;
    QString name;
    double duration;

    int num_items = set.value("NumberOfEntries", 0).toInt();
    for (int n = 1; n <= num_items; n++) {
        QString ns = QString::number(n);
        filename = set.value("File" + ns, "").toString();
        name = set.value("Title" + ns, "").toString();
        duration = (double) set.value("Length" + ns, 0).toInt();
        cleanAndAddItem(filename, name, duration, result);
    }

    set.endGroup();

    if (result->childCount()) {
        parent->addChild(result);
        latestDir = fi.absolutePath();
        return result;
    }

    delete result;
    return 0;
}

TPlaylistWidgetItem* TAddFilesThread::findFilename(const QString&) {
    return 0;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              QString filename) {
    // Note: currently addFile loads playlists and addDirectory skips them,
    // giving a nice balance. Load if the individual file is requested,
    // skip when adding a directory.

    TPlaylistWidgetItem* existing_item = 0;
    TPlaylistWidgetItem* item;
    QFileInfo fi(filename);
    if (fi.exists()) {
        filename = QDir::toNativeSeparators(filename);
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
        item = new TPlaylistWidgetItem(parent, 0, filename, fi.fileName(), 0,
                                       false, iconProvider.icon(fi));
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
        item = new TPlaylistWidgetItem(parent, 0, filename, name, 0, false,
                                       iconProvider.icon(fi));
    }

    if (existing_item) {
        item->setName(existing_item->name());
        item->setDuration(existing_item->duration());
        item->setPlayed(existing_item->played());
    }

    return item;
}

TPlaylistWidgetItem* TAddFilesThread::addDirectory(TPlaylistWidgetItem* parent,
                                                   const QString &dir) {

    static TExtensions ext;
    static QRegExp rx_ext(ext.multimedia().forRegExp(), Qt::CaseInsensitive);

    emit displayMessage(dir, 0);

    QString playlist = dir + "/" + TConfig::PROGRAM_ID + ".m3u8";
    if (QFileInfo(playlist).exists()) {
        return openM3u(QDir::toNativeSeparators(playlist), parent);
    }

    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(0, 0, dir,
        QDir(dir).dirName(), 0, true, iconProvider.folderIcon);

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

        TPlaylistWidgetItem* result;
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

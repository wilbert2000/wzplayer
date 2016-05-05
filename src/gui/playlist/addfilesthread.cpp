#include "gui/playlist/addfilesthread.h"

#include <QTreeWidgetItem>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QRegExp>

#include "gui/playlist/playlistwidgetitem.h"
#include "discname.h"
#include "extensions.h"
#include "config.h"


namespace Gui {
namespace Playlist {

TAddFilesThread::TAddFilesThread(QObject *parent,
                                 const QStringList& aFiles,
                                 bool recurseSubDirs,
                                 bool aSearchForItems) :
    QThread(parent),
    root(0),
    currentItem(0),
    stopRequested(false),
    files(aFiles),
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

QTreeWidgetItem* TAddFilesThread::openM3u(const QString&,
                                          QTreeWidgetItem*) {
    return 0;
}

QTreeWidgetItem* TAddFilesThread::openPls(const QString&,
                                          QTreeWidgetItem*) {
    return 0;
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
    foreach(const QString filename, QDir(dir).entryList()) {
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
    foreach(QString file, files) {
        if (stopRequested) {
            break;
        }

        if (file.startsWith("file:")) {
            file = QUrl(file).toLocalFile();
        }

        QFileInfo fi(file);

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

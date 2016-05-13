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

    if (fi.exists()) {
        alt_name = fi.fileName();
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            alt_name = fi.fileName();
            // TODO: better preserve relative names
        }
    }

    if (parent == 0) {
        parent = root;
    }

    bool link = fi.isSymLink();
    if (link) {
        fi.setFile(fi.symLinkTarget());
        alt_name = fi.fileName();
    }

    if (fi.isDir()) {
        return addDirectory(parent, fi.absoluteFilePath());
    }

    if (name.isEmpty()) {
        name = alt_name;
    }

    if (link) {
        QString s = filename;
        filename = fi.absoluteFilePath();
        // For icon
        fi.setFile(s);
    }

    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(parent,
                                                     0,
                                                     filename,
                                                     name,
                                                     duration,
                                                     false,
                                                     iconProvider.icon(fi));

    // Protect name
    if (protect_name) {
        w->setEdited(true);
    }

    return w;
}

TPlaylistWidgetItem* TAddFilesThread::openM3u(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {

    QFileInfo fi(playlistFileName);
    bool utf8 = fi.suffix().toLower() != "m3u";

    QFile f(playlistFileName);
    if (!f.open(QIODevice::ReadOnly)) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName),
                            TConfig::ERROR_MESSAGE_DURATION);
        return 0;
    }

    QTextStream stream(&f);
    if (utf8) {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    // Path to use for relative filenames in playlist
    playlistPath = fi.absolutePath();

    QString name;
    double duration = 0;

    QRegExp rx("^#EXTINF:(.*),(.*)");

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        // Ignore empty lines and comments
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        if (rx.indexIn(line) >= 0) {
            duration = rx.cap(1).toDouble();
            name = rx.cap(2);
        } else {
            cleanAndAddItem(line, name, duration, parent);
            name = "";
            duration = 0;
        }
    }

    f.close();

    // Place siblings in folder respecting the order of the playlist
    // TODO:
    TPlaylistWidgetItem* currentFolder = 0;
    TPlaylistWidgetItem* item = 0;
    TPlaylistWidgetItem* prev = parent->childCount()
                                ? static_cast<TPlaylistWidgetItem*>(
                                      parent->child(0))
                                : 0;

    QString prevPath = prev ? prev->path() : "";
    QString path;
    int i = 1;
    while (i < parent->childCount()) {
        item = static_cast<TPlaylistWidgetItem*>(parent->child(i));

        path = item->path();
        if (path == prevPath) {
            if (currentFolder == 0 || path != currentFolder->filename()) {
                bool addPrev = currentFolder == 0;
                fi.setFile(path);

                currentFolder = new TPlaylistWidgetItem(parent,
                                                        item,
                                                        path,
                                                        fi.fileName(),
                                                        0,
                                                        true,
                                                        iconProvider.icon(fi));
                if (addPrev) {
                    currentFolder->addChild(prev);
                    i--;
                    parent->takeChild(i);
                }
            }
            currentFolder->addChild(item);
            i--;
            parent->takeChild(i);
        }

        prev = item;
        prevPath = path;
        i++;
    }

    return parent;
}

TPlaylistWidgetItem* TAddFilesThread::openPls(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {

    QSettings set(playlistFileName, QSettings::IniFormat);
    set.beginGroup("playlist");
    if (set.status() != QSettings::NoError) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName),
                            TConfig::ERROR_MESSAGE_DURATION);
        return 0;
    }

    QFileInfo fi(playlistFileName);
    // Path to use for relative filenames in playlist
    playlistPath = fi.absolutePath();

    QString filename;
    QString name;
    double duration;

    int num_items = set.value("NumberOfEntries", 0).toInt();
    for (int n = 1; n <= num_items; n++) {
        QString ns = QString::number(n);
        filename = set.value("File" + ns, "").toString();
        name = set.value("Title" + ns, "").toString();
        duration = (double) set.value("Length" + ns, 0).toInt();
        cleanAndAddItem(filename, name, duration, parent);
    }

    set.endGroup();
    return parent;
}

TPlaylistWidgetItem* TAddFilesThread::findFilename(const QString&) {
    return 0;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              const QString& filename) {
    // Note: currently addFile loads playlists and addDirectory skips them,
    // giving a nice balance. Load if the individual file is requested,
    // skip when adding a directory.

    TPlaylistWidgetItem* existing_item = 0;
    if (searchForItems) {
        existing_item = findFilename(filename);
    }

    TPlaylistWidgetItem* item;
    QFileInfo fi(filename);

    if (fi.exists()) {
        // Put playlist in a folder
        QString ext = fi.suffix().toLower();
        bool isPlaylist = ext == "m3u8" || ext == "m3u" || ext == "pls";

        item = new TPlaylistWidgetItem(isPlaylist ? 0 : parent,
                                       0,
                                       filename,
                                       fi.fileName(),
                                       0,
                                       isPlaylist,
                                       iconProvider.icon(fi));

        if (isPlaylist) {
            TPlaylistWidgetItem* playlistItem;
            if (ext == "pls") {
                playlistItem = openPls(filename, item);
            } else {
                playlistItem = openM3u(filename, item);
            }

            if (playlistItem && playlistItem->childCount())  {
                item = playlistItem;
                parent->addChild(item);
                latestDir = fi.absoluteFilePath();
            } else {
              delete item;
              item = 0;
            }
        }
    } else {
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

    if (item && existing_item) {
        item->setName(existing_item->name());
        item->setDuration(existing_item->duration());
        item->setPlayed(existing_item->played());
    }

    return item;
}

TPlaylistWidgetItem* TAddFilesThread::addDirectory(TPlaylistWidgetItem* parent,
                                                   const QString &dir) {

    emit displayMessage(dir, 0);

    QFileInfo fi(dir, TConfig::PROGRAM_ID + ".m3u8");
    if (fi.exists()) {
        return openM3u(fi.absoluteFilePath(), parent);
    }

    QDir directory(dir);

    TPlaylistWidgetItem* item = new TPlaylistWidgetItem(0, 0, dir,
        directory.dirName(), 0, true, iconProvider.folderIcon);


    static QRegExp ext(extensions.multimedia().forRegExp(),
                       Qt::CaseInsensitive);

    foreach(const QString& filename, directory.entryList()) {
        if (stopRequested) {
            break;
        }
        if (filename != "." && filename != "..") {
            fi.setFile(dir, filename);
            if (fi.isDir()) {
                if (recurse) {
                    addDirectory(item, fi.absoluteFilePath());
                }
            } else if (ext.indexIn(fi.suffix()) >= 0) {
                addFile(item, fi.absoluteFilePath());
            }
        }
    }

    if (item->childCount()) {
        latestDir = dir;
        parent->addChild(item);
        return item;
    }

    delete item;
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

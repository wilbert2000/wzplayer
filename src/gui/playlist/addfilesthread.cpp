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
    logger()->debug("cleanAndAddItem: '" + filename
                    + "' to '" + (parent ? parent->filename() + "'": "0"));

    bool protect_name = !name.isEmpty();
    QString alt_name = filename;

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);
    if (fi.exists()) {
        logger()->debug("cleanAndAddItem: found '" + fi.fileName() + "'");
        alt_name = fi.fileName();
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            filename = fi.absoluteFilePath();
            alt_name = fi.fileName();
            logger()->debug("cleanAndAddItem: found relative path for '"
                            + fi.fileName() + "'");
        }
    }

    // TODO:
    if (fi.isSymLink()) {
        logger()->debug("cleanAndAddItem: following '" + fi.fileName()
                        + "' to '" + fi.symLinkTarget() + "'");
        fi.setFile(fi.symLinkTarget());
        filename = fi.absoluteFilePath();
        alt_name = fi.fileName();
    }

    if (fi.isDir()) {
        return addDirectory(parent, fi.absoluteFilePath());
    }

    if (name.isEmpty()) {
        logger()->debug("cleanAndAddItem: assigning name '" + alt_name + "'");
        name = alt_name;
    }

    logger()->debug("cleanAndAddItem: creating '" + fi.absoluteFilePath() + "'");
    TPlaylistWidgetItem* w = new TPlaylistWidgetItem(parent,
                                                     0,
                                                     fi.absoluteFilePath(),
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

TPlaylistWidgetItem* TAddFilesThread::createFolder(TPlaylistWidgetItem* parent,
                                                   TPlaylistWidgetItem* after,
                                                   const QFileInfo& fi) {
    logger()->debug("createFolder: '" + fi.filePath()
                    + "' in '" + parent->filename() + "'");

    // QDir::path removes redundant .. and //
    // QDir::filePath does not

    QDir parentDir;
    if (parent->filename().isEmpty() || parent->filename() == ".") {
        parentDir.setPath(playlistPath);
    } else {
        parentDir.setPath(parent->filename());
    }
    if (parentDir.path() == fi.path()) {
        return new TPlaylistWidgetItem(parent,
                                       after,
                                       fi.absoluteFilePath(),
                                       fi.fileName(),
                                       0,
                                       true,
                                       iconProvider.icon(fi));
    }

    logger()->debug(parentDir.path() + " does not match path " + fi.path());
    QDir subDir(fi.path().mid(parentDir.path().size()));
    while (!subDir.path().isEmpty()
           && subDir.path() != "."
           && !subDir.isRoot()) {
        subDir.setPath("..");
    }
    subDir.setPath(parentDir.path() + QDir::separator() + subDir.dirName());
    QFileInfo subInfo(subDir.path());
    TPlaylistWidgetItem* folder = new TPlaylistWidgetItem(
                                      parent,
                                      after,
                                      subInfo.absoluteFilePath(),
                                      subInfo.fileName(),
                                      0,
                                      true,
                                      iconProvider.icon(subInfo));
    createFolder(folder, 0, subInfo);
    return folder;
}

TPlaylistWidgetItem* TAddFilesThread::openM3u(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {
    logger()->info("openM3u: '" + playlistFileName + "'");

    QFileInfo fi(playlistFileName);


    QFile file(playlistFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        emit displayMessage(tr("Failed to open %1").arg(playlistFileName),
                            TConfig::ERROR_MESSAGE_DURATION);
        return 0;
    }

    QTextStream stream(&file);
    if (fi.suffix().toLower() != "m3u") {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }


    // Path to use for relative filenames in playlist
    playlistPath = fi.absolutePath();
    QString savedPlaylistPath = playlistPath;

    QString name;
    double duration = 0;

    QRegExp rx("^#EXTINF:(.*),(.*)");

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        // Ignore empty lines
        if (line.isEmpty()) {
            continue;
        }
        if (rx.indexIn(line) >= 0) {
            duration = rx.cap(1).toDouble();
            name = rx.cap(2);
        } else if (!line.startsWith("#")) {
            cleanAndAddItem(line, name, duration, parent);
            playlistPath = savedPlaylistPath;
            name = "";
            duration = 0;
        }
    }

    file.close();

    // Place siblings together in a folder, respecting the order of the playlist
    // Can have multiple folders with the same name...
    TPlaylistWidgetItem* currentFolder = 0;
    QString prevPath;
    if (parent->childCount()) {
        fi.setFile(static_cast<TPlaylistWidgetItem*>(parent->child(0))
                   ->filename());
        prevPath = QDir::toNativeSeparators(fi.path());
    }
    QString path;
    int i = 1;
    while (i < parent->childCount()) {
        TPlaylistWidgetItem* item = static_cast<TPlaylistWidgetItem*>(
                                        parent->child(i));
        fi.setFile(item->filename());
        path = QDir::toNativeSeparators(fi.path());
        if (!path.isEmpty()
            && path != "."
            && path != playlistPath
            && path == prevPath) {
            if (currentFolder == 0 || path != currentFolder->filename()) {
                fi.setFile(path);
                currentFolder = createFolder(parent, item, fi);
                i--;
                TPlaylistWidgetItem* first = static_cast<TPlaylistWidgetItem*>(
                                                 parent->takeChild(i));
                logger()->debug("openM3u: moving first '" + first->filename()
                                + "' to '" + path + "'");
                currentFolder->addChild(first);
            }
            logger()->debug("openM3u: moving '" + item->filename()
                            + "' to '" + path + "'");
            i--;
            currentFolder->addChild(parent->takeChild(i));
        }

        prevPath = path;
        i++;
    }

    if (parent->filename().isEmpty() || parent->filename() == ".") {
        logger()->debug("openM3u: updating parent file name from '"
                        + parent->filename() + "' to '"
                        + playlistFileName + "'");
        fi.setFile(playlistFileName);
        parent->setFileInfo(fi);
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
    logger()->debug("addFile: " + filename);
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

    logger()->debug("addDirectory: " + dir);

    emit displayMessage(dir, 0);

    QFileInfo fi(dir, TConfig::WZPLAYLIST);
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

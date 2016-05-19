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

bool TAddFilesThread::blackListed(QString filename) {

    filename = QDir::toNativeSeparators(filename);

    // Note: using case insensitive
    if (filename.isEmpty()
        || blackList.contains(filename, Qt::CaseInsensitive)) {
        logger()->warn("blackListed: ignoring '" + filename
                       + "', it would create an infinite playlist");
        return true;
    }

    blackList.append(filename);
    return false;
}

bool TAddFilesThread::blackListed(const QFileInfo& fi) {
    return blackListed(fi.canonicalFilePath());
}

bool TAddFilesThread::blackListed(const QDir& dir) {
    return blackListed(dir.canonicalPath());
}

void TAddFilesThread::whiteList() {
    blackList.removeLast();
}

void TAddFilesThread::createPath(TPlaylistWidgetItem* parent,
                                 const QFileInfo& fi,
                                 const QString& name,
                                 double duration,
                                 bool protectName) {

    QString parentPath = parent->path();
    QString parentPathPlus;
    if (parentPath.endsWith("/")) {
        parentPathPlus = parentPath;
    } else {
        parentPathPlus = parentPath + "/";
    }
    // Remove extra slashes and dots from path
    QString path = fi.dir().path();

    if (path == parentPath || !path.startsWith(parentPathPlus)) {
        // Use cleaned path
        QString filename = path + "/" + fi.fileName();
        logger()->trace("createPath: creating '" + filename
                        + "' in '" + parentPath + "'");
        TPlaylistWidgetItem* w = new TPlaylistWidgetItem(parent, filename,
            name, duration, false, iconProvider.icon(fi));
        // Protect name
        if (protectName) {
            w->setEdited(true);
        }
        return;
    }

    QString dir = path.mid(parentPathPlus.length());
    int i = dir.indexOf("/");
    if (i >= 0) {
        dir = dir.left(i);
    }
    if (dir.isEmpty()) {
        logger()->error("createPath: invalid path");
        return;
    }
    path = parentPathPlus + dir;

    // Only check last child to preserve order of playlist
    if (parent->childCount()) {
        TPlaylistWidgetItem* child = parent->plChild(parent->childCount() - 1);
        if (child->path() == path) {
            createPath(child, fi, name, duration, protectName);
            return;
        }
    }

    logger()->debug("createPath: creating folder '" + path + "'");
    emit displayMessage(path, 0);
    TPlaylistWidgetItem* folder = new TPlaylistWidgetItem(parent, path, dir,
        0, true, iconProvider.folderIcon);

    createPath(folder, fi, name, duration, protectName);
    return;
}

void TAddFilesThread::cleanAndAddItem(TPlaylistWidgetItem* parent,
                                      QString filename,
                                      QString name,
                                      double duration) {

    bool protectName = !name.isEmpty();

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);
    if (fi.exists()) {
        logger()->trace("cleanAndAddItem: found '" + fi.fileName() + "'");
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            logger()->trace("cleanAndAddItem: found relative path to '"
                            + fi.fileName() + "'");
        } else if (fi.fileName() == TConfig::WZPLAYLIST) {
            // Try directory
            fi.setFile(fi.dir().path());
            if (!fi.exists()) {
                fi.setFile(playlistPath, fi.dir().path());
            }
            if (fi.exists()) {
                logger()->info("cleanAndAddItem: '" + filename + "' no longer"
                               " exists. Linking to '" + fi.absoluteFilePath()
                               + "' instead");
            } else {
                logger()->info("cleanAndAddItem: ignoring no longer existing"
                               " directorie with '" + filename + "'");
                return;
            }
        } else {
            logger()->trace("cleanAndAddItem: adding '" + filename + "'");
            new TPlaylistWidgetItem(parent, filename, name, duration, false,
                                    iconProvider.icon(fi));
            return;
        }
    }

    if (fi.isSymLink()) {
        logger()->debug("cleanAndAddItem: following '" + fi.fileName()
                        + "' to '" + fi.symLinkTarget() + "'");
        if (name.isEmpty()) {
            name = fi.fileName();
        }
        fi.setFile(fi.symLinkTarget());
        if (!fi.exists()) {
            logger()->warn("cleanAndAddItem: ignoring link '" + filename
                           + "' pointing to non existing target '"
                           + fi.filePath() + "'");
            return;
        }
    }

    QString savedPlaylistPath = playlistPath;
    TPlaylistWidgetItem* folder;
    if (fi.isDir()) {
        folder = addDirectory(parent, fi);
    } else {
        QString ext = fi.suffix().toLower();
        if (ext == "m3u8" || ext == "m3u" || ext == "pls") {
            folder = openPlaylist(parent, fi);
        } else {
            if (name.isEmpty()) {
                name = fi.fileName();
            }
            createPath(parent, fi, name, duration, protectName);
            folder = 0;
        }
    }

    if (folder && !name.isEmpty() && name != folder->name()) {
        folder->setName(name);
        if (protectName) {
            folder->setEdited(true);
        }
    }
    playlistPath = savedPlaylistPath;
}

bool TAddFilesThread::openM3u(TPlaylistWidgetItem* playlistItem,
                              const QFileInfo& fi,
                              bool utf8) {

    if (fi.fileName() == TConfig::WZPLAYLIST) {
        playlistItem->setName(fi.dir().dirName());
    }

    QFile file(fi.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream stream(&file);
    if (utf8) {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    QString name;
    double duration = 0;
    QRegExp rx("^#EXTINF:\\s*(\\d+)\\s*,\\s*(.*)");

    while (!stream.atEnd()) {
        if (stopRequested) {
            break;
        }
        QString line = stream.readLine().trimmed();
        // Ignore empty lines
        if (line.isEmpty()) {
            continue;
        }
        if (rx.indexIn(line) >= 0) {
            duration = rx.cap(1).toDouble();
            name = rx.cap(2).simplified();
        } else if (!line.startsWith("#")) {
            cleanAndAddItem(playlistItem, line, name, duration);
            name = "";
            duration = 0;
        }
    }

    file.close();

    return true;
}

bool TAddFilesThread::openPls(TPlaylistWidgetItem* playlistItem,
                              const QString& playlistFileName) {

    QSettings set(playlistFileName, QSettings::IniFormat);
    set.beginGroup("playlist");
    if (set.status() != QSettings::NoError) {
        return false;
    }

    QString filename;
    QString name;
    double duration;

    int num_items = set.value("NumberOfEntries", 0).toInt();
    for (int n = 1; n <= num_items; n++) {
        if (stopRequested) {
            break;
        }
        QString ns = QString::number(n);
        filename = set.value("File" + ns, "").toString();
        name = set.value("Title" + ns, "").toString().simplified();
        duration = (double) set.value("Length" + ns, 0).toInt();
        cleanAndAddItem(playlistItem, filename, name, duration);
    }

    set.endGroup();

    return true;
}

TPlaylistWidgetItem* TAddFilesThread::openPlaylist(TPlaylistWidgetItem *parent,
                                                   QFileInfo& fi) {
    logger()->info("openPlaylist: '" + fi.filePath() + "'");

    emit displayMessage(fi.filePath(), 0);

    if (blackListed(fi)) {
        return 0;
    }

    // Path to use for relative filenames in playlist, use QDir to remove .. ///
    playlistPath = QDir::toNativeSeparators(fi.dir().path());

    // Put playlist in a folder
    TPlaylistWidgetItem* playlistItem = new TPlaylistWidgetItem(0,
        playlistPath + QDir::separator() + fi.fileName(), fi.fileName(), 0,
        true, iconProvider.folderIcon);

    bool result;
    QString ext = fi.suffix().toLower();
    if (ext == "pls") {
        result = openPls(playlistItem, fi.absoluteFilePath());
    } else {
        result = openM3u(playlistItem, fi, ext != "m3u");
    }

    // Handle result
    if (result) {
        if (playlistItem->childCount())  {
            parent->addChild(playlistItem);
            latestDir = playlistPath;
        } else {
            logger()->warn("openPlaylist: found no playable items in playlist '"
                           + fi.absoluteFilePath() + "'");
            delete playlistItem;
            playlistItem = 0;
        }
    } else {
        logger()->error("openPlaylist: failed to open '" + fi.absoluteFilePath()
                        + "'");
        emit displayMessage(tr("Failed to open %1").arg(fi.absoluteFilePath()),
                            TConfig::ERROR_MESSAGE_DURATION);
        delete playlistItem;
        playlistItem = 0;
    }

    whiteList();

    return playlistItem;
}

TPlaylistWidgetItem* TAddFilesThread::findFilename(const QFileInfo&) {
    return 0;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              QFileInfo& fi) {

    TPlaylistWidgetItem* existing_item = 0;
    if (searchForItems) {
        existing_item = findFilename(fi);
    }

    TPlaylistWidgetItem* item;
    if (fi.exists()) {
        if (fi.isSymLink()) {
            // TODO: preserve link, for now still need switch to target
            fi.setFile(fi.symLinkTarget());
            if (!extensions.isMultiMedia(fi) && !extensions.isPlayList(fi)) {
                return 0;
            }
        }
        if (extensions.isPlayList(fi)) {
            item = openPlaylist(parent, fi);
        } else {
            item = new TPlaylistWidgetItem(parent, fi.absoluteFilePath(),
                fi.fileName(), 0, false, iconProvider.icon(fi));
        }
    } else {
        QString name;
        TDiscName disc(fi.fileName());
        if (disc.valid) {
            // See also TTitleData::getDisplayName() from titletracks.cpp
            if (disc.protocol == "cdda" || disc.protocol == "vcd") {
                name = tr("Track %1").arg(QString::number(disc.title));
            } else {
                name = tr("Title %1").arg(QString::number(disc.title));
            }
        }
        item = new TPlaylistWidgetItem(parent, fi.fileName(), name, 0, false,
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
                                                   QFileInfo& fi) {
    logger()->debug("addDirectory: '" + fi.filePath() + "'");

    static QStringList dirFilterList = extensions.multimedia().forDirFilter()
                                       << "*.lnk";

    emit displayMessage(fi.filePath(), 0);

    QDir directory(fi.absoluteFilePath());
    if (blackListed(directory)) {
        return 0;
    }
    if (directory.exists(TConfig::WZPLAYLIST)) {
        fi.setFile(directory.path(), TConfig::WZPLAYLIST);
        return openPlaylist(parent, fi);
    }

    // for Windows ".lnk" files QDir::System is needed
    directory.setFilter(QDir::Dirs
                        | QDir::AllDirs
                        | QDir::Files
                        | QDir::Drives
                        | QDir::NoDotAndDotDot
                        | QDir::Readable
#ifdef Q_OS_WIN
                        | QDir::System
#endif
                        );
    directory.setNameFilters(dirFilterList);

    // TODO: check sort order tree view
    // directory.setSorting(QDir::Name);

    TPlaylistWidgetItem* dirItem = new TPlaylistWidgetItem(0, directory.path(),
        directory.dirName(), 0, true, iconProvider.folderIcon);

    foreach(const QString& filename, directory.entryList()) {
        if (stopRequested) {
            break;
        }
        fi.setFile(directory.path(), filename);
        if (fi.isDir()) {
            if (recurse) {
                addDirectory(dirItem, fi);
            }
        } else {
            addFile(dirItem, fi);
        }
    }

    if (dirItem->childCount()) {
        latestDir = directory.path();
        parent->addChild(dirItem);
    } else {
        logger()->debug("addDirectory: found no playable items in '"
                        + directory.path() + "'");
        delete dirItem;
        dirItem = 0;
    }

    whiteList();

    return dirItem;
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
            result = addDirectory(root, fi);
        } else {
            result = addFile(root, fi);
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

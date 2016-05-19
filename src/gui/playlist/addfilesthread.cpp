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
                                 bool recurseSubDirs) :
    QThread(parent),
    files(aFiles),
    root(0),
    currentItem(0),
    abortRequested(false),
    stopRequested(false),
    recurse(recurseSubDirs) {
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    playlistPath = QDir::toNativeSeparators(QDir::current().path());
    root = new TPlaylistWidgetItem(0, playlistPath, "", 0, true,
                                   iconProvider.folderIcon);
    root->setFlags(ROOT_FLAGS);

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
        logger()->warn("blackListed: ignoring '%1', it would create an infinite"
                       " playlist", filename);
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

TPlaylistWidgetItem* TAddFilesThread::createPath(TPlaylistWidgetItem* parent,
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
        QString filename = path + "/" + fi.fileName();
        logger()->trace("createPath: creating '%1' in '%2'",
                        filename, parent->filename());
        TPlaylistWidgetItem* w = new TPlaylistWidgetItem(parent,
            filename, name, duration, false, iconProvider.icon(fi));
        // Protect name
        if (protectName) {
            w->setEdited(true);
        }
        return w;
    }

    QString dir = path.mid(parentPathPlus.length());
    int i = dir.indexOf("/");
    if (i >= 0) {
        dir = dir.left(i);
    }
    if (dir.isEmpty()) {
        logger()->error("createPath: invalid path");
        return 0;
    }
    path = parentPathPlus + dir;

    // Only check last child to preserve order of playlist
    if (parent->childCount()) {
        TPlaylistWidgetItem* child = parent->plChild(parent->childCount() - 1);
        if (child->path() == path) {
            createPath(child, fi, name, duration, protectName);
            return child;
        }
    }

    logger()->debug("createPath: creating '%1'", path);
    emit displayMessage(path, 0);
    TPlaylistWidgetItem* folder = new TPlaylistWidgetItem(parent, path, dir,
        0, true, iconProvider.folderIcon);

    createPath(folder, fi, name, duration, protectName);
    return folder;
}

TPlaylistWidgetItem* TAddFilesThread::addItemNotFound(
        TPlaylistWidgetItem* parent,
        const QString& filename,
        QString name,
        const QFileInfo& fi,
        bool ignoreNotFound) {

    bool setFailed = false;

    TDiscName disc(filename);
    if (disc.valid) {
        if (name.isEmpty()) {
            // TODO: share with mdat and TTitleData::getDisplayName()
            QFileInfo fid(disc.device);
            name = fid.fileName();
            // fileName() return empty if name ends in /
            if (name.isEmpty()) {
                name = disc.device;
            }
            if (disc.title > 0) {
                name += " - ";
                if (disc.protocol == "cdda" || disc.protocol == "vcd") {
                    name += tr("track %1").arg(QString::number(disc.title));
                } else {
                    name += tr("title %1").arg(QString::number(disc.title));
                }
            }
        }
    } else {
        QUrl url(filename);
        if (url.scheme().isEmpty()) {
            if (ignoreNotFound) {
                logger()->info("addItemNotFound: ignoring no longer existing "
                               " item '%1'", filename);
                return 0;
            }
            logger()->warn("addItemNotFound: '%1' not found", filename);
            setFailed = true;
        } else if (name.isEmpty()) {
            QString path = url.path();
            if (!path.isEmpty()) {
                name = QFileInfo(path).fileName();
            }
        }
    }

    TPlaylistWidgetItem* item = new TPlaylistWidgetItem(parent, filename, name,
                                                        0, false,
                                                        iconProvider.icon(fi));
    if (setFailed) {
        item->setState(PSTATE_FAILED);
    }
    return item;
}

TPlaylistWidgetItem* TAddFilesThread::addItem(TPlaylistWidgetItem* parent,
                                              QString filename,
                                              QString name,
                                              double duration,
                                              bool ignoreNotFound) {

    bool protectName = !name.isEmpty();

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);
    if (fi.exists()) {
        logger()->trace("addItem: found '%1'", fi.fileName());
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            logger()->trace("addItem: found relative path to '%1'",
                            fi.fileName());
        } else if (fi.fileName() == TConfig::WZPLAYLIST) {
            // Try directory
            fi.setFile(fi.dir().path());
            if (!fi.exists()) {
                fi.setFile(playlistPath, fi.dir().path());
            }
            if (fi.exists()) {
                logger()->info("addItem: '%1' no longer exists. Linking to"
                               " directory '%2' instead",
                               filename, fi.absoluteFilePath());
            } else {
                logger()->info("addItem: ignoring no longer existing"
                               " playlist '%1'", filename);
                return 0;
            }
        } else {
            return addItemNotFound(parent, filename, name, fi, ignoreNotFound);
        }
    }

    if (fi.isSymLink()) {
        logger()->debug("addItem: following '%1' to '%2'",
                        fi.fileName(), fi.symLinkTarget());
        if (name.isEmpty()) {
            name = fi.fileName();
        }
        fi.setFile(fi.symLinkTarget());
        if (!fi.exists()) {
            logger()->warn("addItem: ignoring link '%1' pointing to non"
                           " existing target '%2'", filename, fi.filePath());
            return 0;
        }
    }

    QString savedPlaylistPath = playlistPath;
    TPlaylistWidgetItem* item;
    if (fi.isDir()) {
        item = addDirectory(parent, fi);
    } else {
        if (extensions.isPlayList(fi)) {
            item = openPlaylist(parent, fi);
        } else {
            if (name.isEmpty()) {
                name = fi.fileName();
            }
            item = createPath(parent, fi, name, duration, protectName);
            name = "";
        }
    }

    if (item && !name.isEmpty() && name != item->name()) {
        logger()->debug("addItem: replacing name '%1' with '%2'",
                        item->name(), name);
        item->setName(name);
        if (protectName) {
            item->setEdited(true);
        }
    }

    playlistPath = savedPlaylistPath;
    return item;
}

bool TAddFilesThread::openM3u(TPlaylistWidgetItem* playlistItem,
                              const QFileInfo& fi,
                              bool utf8) {

    bool ignoreNotFound = false;
    if (fi.fileName() == TConfig::WZPLAYLIST) {
        ignoreNotFound = true;
        playlistItem->setName(TPlaylistItem::cleanName(fi.dir().dirName()));
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
            addItem(playlistItem, line, name, duration, ignoreNotFound);
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
        addItem(playlistItem, filename, name, duration);
    }

    set.endGroup();

    return true;
}

TPlaylistWidgetItem* TAddFilesThread::openPlaylist(TPlaylistWidgetItem *parent,
                                                   QFileInfo& fi) {
    logger()->info("openPlaylist: '%1'", fi.filePath());

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
            logger()->warn("openPlaylist: found no playable items in playlist"
                           " '%1'", fi.absoluteFilePath());
            delete playlistItem;
            playlistItem = 0;
        }
    } else {
        logger()->error("openPlaylist: failed to open '%1'",
                        fi.absoluteFilePath());
        emit displayMessage(tr("Failed to open %1").arg(fi.absoluteFilePath()),
                            TConfig::ERROR_MESSAGE_DURATION);
        delete playlistItem;
        playlistItem = 0;
    }

    whiteList();

    return playlistItem;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              QFileInfo& fi) {

    TPlaylistWidgetItem* item;
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
                                       fi.fileName(), 0, false,
                                       iconProvider.icon(fi));
    }

    return item;
}

TPlaylistWidgetItem* TAddFilesThread::addDirectory(TPlaylistWidgetItem* parent,
                                                   QFileInfo& fi) {
    logger()->debug("addDirectory: '%1'", fi.filePath());

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
        logger()->debug("addDirectory: found no playable items in '%1'",
                        directory.path());
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
        TPlaylistWidgetItem* result = addItem(root, filename);
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

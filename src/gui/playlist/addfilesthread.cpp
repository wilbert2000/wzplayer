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
#include "helper.h"


namespace Gui {
namespace Playlist {

// TODO: find the file sys func reporting case
Qt::CaseSensitivity caseSensitiveNames =
#ifdef Q_OS_WIN
        Qt::CaseInsensitive;
#else
        Qt::CaseSensitive;
#endif


class TFileLock {
public:
    bool locked;

    TFileLock(const QFileInfo& fi, QStringList& lfiles);
    TFileLock(const QDir& dir, QStringList& lfiles);
    virtual ~TFileLock();

private:
    QStringList& lockedFiles;

    bool acquire(QString filename);
};

bool TFileLock::acquire(QString filename) {

    if (filename.isEmpty()) {
        Log4Qt::Logger::logger("Gui::Playlist::TFileLock")->error(
            "acquire: ignoring empty filename");
        return false;
    }

    filename = QDir::toNativeSeparators(filename);

    if (lockedFiles.contains(filename, caseSensitiveNames)) {
        Log4Qt::Logger::logger("Gui::Playlist::TFileLock")->warn(
            "acquire: skipping '%1', creating an infinite playlist", filename);
        return false;
    }

    lockedFiles.append(filename);
    return true;
}

TFileLock::TFileLock(const QFileInfo& fi, QStringList& lfiles) :
    lockedFiles(lfiles) {
    locked = acquire(fi.canonicalFilePath());
}

TFileLock::TFileLock(const QDir& dir, QStringList& lfiles) :
    lockedFiles(lfiles) {
    locked = acquire(dir.canonicalPath());
}

TFileLock::~TFileLock() {
    if (locked) {
        lockedFiles.removeLast();
    }
}


// for Windows ".lnk" files QDir::System is needed
QDir::Filters dirFilter = QDir::Dirs
                          | QDir::AllDirs
                          | QDir::Files
                          | QDir::Drives
                          | QDir::NoDotAndDotDot
                          | QDir::Readable
#ifdef Q_OS_WIN
                          | QDir::System
#endif
                          ;

TAddFilesThread::TAddFilesThread(QObject *parent,
                                 const QStringList& aFiles,
                                 const QStringList& nameBlacklist,
                                 bool recurseSubDirs,
                                 bool videoFiles,
                                 bool audioFiles,
                                 bool playlists,
                                 bool images) :
    QThread(parent),
    debug(logger()),
    files(aFiles),
    root(0),
    currentItem(0),
    abortRequested(false),
    stopRequested(false),
    recurse(recurseSubDirs) {

    foreach(const QString& name, nameBlacklist) {
        if (!name.isEmpty()) {
            QRegExp* rx = new QRegExp(name, Qt::CaseInsensitive);
            if (rx->isValid()) {
                logger()->info("TAddFilesThread: precompiled '%1' for"
                               " blacklist", rx->pattern());
                rxNameBlacklist << rx;
            } else {
                delete rx;
                logger()->error("TAddFilesThread: failed to parse regular"
                                " expression '%1'", name);
            }
        }
    }

    ExtensionList exts;
    if (videoFiles) {
        exts = extensions.videoAndAudio();
    }
    if (audioFiles) {
        exts.addList(extensions.audio());
    }
    if (playlists) {
        exts.addList(extensions.playlists());
    }
    if (images) {
        exts.addList(extensions.images());
    }
    nameFilterList = exts.forDirFilter();

#ifdef Q_OS_WIN
    nameFilterList << "*.lnk";
#endif

    if (logger()->isDebugEnabled()) {
        debug << "TAddFilesThread: searching for:" << nameFilterList;
        debug << debug;
    }
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    playlistPath = QDir::toNativeSeparators(QDir::current().path());
    logger()->debug("run: running in '%1'", playlistPath);

    root = new TPlaylistWidgetItem(0, playlistPath, "", 0, true);
    root->setFlags(ROOT_FLAGS);

    addFiles();

    if (abortRequested) {
        delete root;
        root = 0;
    } else {
        root->setFilename("");
        root->setName("");
    }

    logger()->debug("run: exiting. stopped %1 aborted %2",
                    stopRequested, abortRequested);
}

bool TAddFilesThread::nameBlackListed(const QString& name) {

    foreach(QRegExp* rx, rxNameBlacklist) {
        if (rx->indexIn(name) >= 0) {
            logger()->info("nameBlackListed: skipping '%1' on '%2'",
                            name, rx->pattern());
            return true;
        }
    }

    return false;
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

    if (path == parentPath) {
        QString filename = path + "/" + fi.fileName();
        logger()->trace("createPath: creating '%1' in '%2'",
                        filename, parent->filename());
        return new TPlaylistWidgetItem(parent,
                                       filename,
                                       name,
                                       duration,
                                       false,
                                       protectName);
    }

    if (!path.startsWith(parentPathPlus)) {
        QString filename = fi.absoluteFilePath();
        logger()->trace("createPath: creating '%1' in '%2'",
                        filename, parent->filename());
        return new TPlaylistWidgetItem(parent,
                                       filename,
                                       name,
                                       duration,
                                       false,
                                       protectName);
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
    TPlaylistWidgetItem* folder = new TPlaylistWidgetItem(parent,
                                                          path,
                                                          dir,
                                                          0,
                                                          true);

    createPath(folder, fi, name, duration, protectName);
    return folder;
}

TPlaylistWidgetItem* TAddFilesThread::addItemNotFound(
        TPlaylistWidgetItem* parent,
        const QString& filename,
        QString name,
        bool protectName,
        bool wzplaylist) {

    bool setFailed = false;

    TDiscName disc(filename);
    if (disc.valid) {
        if (name.isEmpty()) {
            name = disc.displayName();
        }
    } else if (QUrl(filename).scheme().isEmpty()) {
        if (wzplaylist) {
            logger()->info("addItemNotFound: ignoring no longer existing "
                               " item '%1'", filename);
            parent->setModified();
            return 0;
        }
        logger()->error("addItemNotFound: '%1' not found", filename);
        setFailed = true;
    }

    TPlaylistWidgetItem* item = new TPlaylistWidgetItem(parent,
                                                        filename,
                                                        name,
                                                        0,
                                                        false,
                                                        protectName);
    if (setFailed) {
        item->setState(PSTATE_FAILED);
    }
    return item;
}

TPlaylistWidgetItem* TAddFilesThread::addItem(TPlaylistWidgetItem* parent,
                                              QString filename,
                                              QString name,
                                              double duration,
                                              bool wzplaylist) {

    bool protectName = !name.isEmpty();

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);
    if (fi.exists()) {
        logger()->trace("addItem: found '%1'", fi.absoluteFilePath());
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            logger()->trace("addItem: found relative path to '%1'",
                            fi.fileName());
        } else if (fi.fileName() == TConfig::WZPLAYLIST) {
            parent->setModified();
            // Try the directory
            QString path = fi.dir().path();
            if (path.isEmpty()) {
                logger()->error("addItem: self referencing playlist in '%1'",
                                filename);
                return 0;
            }

            fi.setFile(path);
            if (fi.isRelative()) {
                fi.setFile(playlistPath, path);
            }

            if (fi.exists()) {
                logger()->info("addItem: '%1' no longer exists. Trying"
                               " to link to directory '%2' instead",
                               filename, fi.absoluteFilePath());
            } else {
                logger()->info("addItem: ignoring no longer existing"
                               " playlist '%1'", filename);
                return 0;
            }
        } else {
            return addItemNotFound(parent, filename, name, protectName,
                                   wzplaylist);
        }
    }

    if (name.isEmpty()) {
        name = fi.fileName();
    }

    // For Windows shortcuts, follow the link
    if (fi.isSymLink() && fi.suffix().toLower() == "lnk") {
        fi.setFile(fi.symLinkTarget());
    }

    QString savedPlaylistPath = playlistPath;
    TPlaylistWidgetItem* item;
    if (fi.isDir()) {
        item = addDirectory(parent, fi, name, protectName);
    } else if (extensions.isPlaylist(fi)) {
        item = openPlaylist(parent, fi, name, protectName, true);
    } else {
        item = createPath(parent, fi, name, duration, protectName);
    }
    playlistPath = savedPlaylistPath;
    return item;
}

void TAddFilesThread::addNewItems(TPlaylistWidgetItem* playlistItem,
                                  const QFileInfo& playlistInfo) {
    logger()->debug("addNewItems: '%1'", playlistInfo.filePath());

    emit displayMessage(playlistInfo.filePath(), 0);

    // Collect relative file names
    QStringList files;
    QString path = playlistPath;
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }
    for(int c = 0; c < playlistItem->childCount(); c++) {
        QString fn = playlistItem->plChild(c)->filename();
        if (fn.startsWith(path)) {
            fn = fn.mid(path.length());
            int i = fn.indexOf(QDir::separator());
            if (i >= 0) {
                fn = fn.left(i);
            }
        }
        files << fn;
    }

    QStringList blacklist = playlistItem->getBlacklist();

    QDir directory(playlistInfo.dir().path());
    directory.setFilter(dirFilter);
    directory.setNameFilters(nameFilterList);
    directory.setSorting(getSortFlags());

    foreach(QFileInfo fi, directory.entryInfoList()) {
        if (stopRequested) {
            break;
        }

        QString filename = fi.fileName();

        if (files.contains(filename, caseSensitiveNames)) {
            continue;
        }

        int i = blacklist.indexOf(QRegExp(filename, caseSensitiveNames,
                                          QRegExp::FixedString));
        if (i >= 0) {
            logger()->info("addNewItems: '%1' is blacklisted", filename);
            blacklist.removeAt(i);
            continue;
        }

        if (filename == TConfig::WZPLAYLIST) {
            continue;
        }

        if (nameBlackListed(path + filename)) {
            continue;
        }

        if (fi.isSymLink()
            && files.contains(fi.symLinkTarget(), caseSensitiveNames)) {
            continue;
        }

        TPlaylistWidgetItem* w = 0;
        if (fi.isDir()) {
            if (recurse) {
                logger()->info("addNewItems: adding folder '%1'", filename);
                w = addDirectory(playlistItem, fi, filename, false, false);
            }
        } else {
            logger()->info("addNewItems: adding file '%1'", filename);
            w = addFile(playlistItem, fi);
            // Sort new item into place
            if (w && playlistItem->childCount() > 1) {
                playlistItem->takeChild(playlistItem->childCount() - 1);
                int i = playlistItem->childCount();
                while (i > 0 && *w < *(playlistItem->plChild(i - 1))) {
                    i--;
                }
                playlistItem->insertChild(i, w);
            }
        }
        if (w) {
            w->setModified();
        }
    }

    if (!stopRequested) {
        foreach(const QString& filename, blacklist) {
            logger()->info("addNewItems: '%1' not matched, removing it from"
                           " blacklist", filename);
            playlistItem->whitelist(filename);
            playlistItem->setModified();
        }
    }
}

bool TAddFilesThread::openM3u(TPlaylistWidgetItem* playlistItem,
                              const QFileInfo& fi,
                              bool utf8) {

    bool wzplaylist = false;
    if (fi.fileName() == TConfig::WZPLAYLIST) {
        wzplaylist = true;
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
    static QRegExp rx("^#EXTINF:\\s*(\\d+)\\s*,\\s*(.*)");

    QString path = playlistPath;
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

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
            addItem(playlistItem, line, name, duration, wzplaylist);
            name = "";
            duration = 0;
        } else if (line.startsWith("#WZP-blacklist:")) {
            QString fn = line.mid(15);
            if (fn.startsWith(path)) {
                fn = fn.mid(path.length());
            }
            playlistItem->blacklist(fn);
        }
    }

    file.close();

    if (!stopRequested && wzplaylist) {
        addNewItems(playlistItem, fi);
    }
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
                                                   QFileInfo& fi,
                                                   const QString& name,
                                                   bool protectName,
                                                   bool append) {
    logger()->info("openPlaylist: '%1'", fi.filePath());

    if (fi.isSymLink()) {
        fi.setFile(fi.symLinkTarget());
    }

    emit displayMessage(fi.filePath(), 0);

    TFileLock lock(fi, lockedFiles);
    if (!lock.locked) {
        return 0;
    }

    // Path to use for relative filenames in playlist, use QDir to remove .. ///
    playlistPath = QDir::toNativeSeparators(fi.dir().path());

    // Put playlist in a folder
    TPlaylistWidgetItem* playlistItem = new TPlaylistWidgetItem(0,
        playlistPath + QDir::separator() + fi.fileName(),
        name, 0, true, protectName);

    bool result;
    QString ext = fi.suffix().toLower();
    if (ext == "pls") {
        result = openPls(playlistItem, fi.absoluteFilePath());
    } else {
        result = openM3u(playlistItem, fi, ext != "m3u");
    }

    // Handle result
    if (result) {
        if (playlistItem->childCount()) {
            addFolder(playlistPath, parent, playlistItem, append);
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

    return playlistItem;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              QFileInfo& fi) {

    QString name = fi.fileName();

    if (extensions.isPlaylist(fi)) {
        return openPlaylist(parent, fi, name, false, true);
    }

    if (fi.isSymLink()) {
        QFileInfo target(fi.symLinkTarget());

        if (fi.suffix().toLower() == "lnk") {
            return new TPlaylistWidgetItem(parent, target.absoluteFilePath(),
                                           name, 0, false);
        }

        if (extensions.isMultiMedia(target)) {
            return new TPlaylistWidgetItem(parent, fi.absoluteFilePath(),
                                           name, 0, false);
        }

        return 0;
    }

    return new TPlaylistWidgetItem(parent, fi.absoluteFilePath(), name, 0,
                                   false);
}

QDir::SortFlags TAddFilesThread::getSortFlags() {

    QDir::SortFlags flags = QDir::Name | QDir::DirsLast | QDir::LocaleAware;
    // Note: QString::localeAwareCompare in TPlaylistWidgetItem::operator <
    // has no IgnoreCase...
    // if (!caseSensitiveNames) {
    //    flags |= QDir::IgnoreCase;
    // }
    return flags;
}

void TAddFilesThread::addFolder(const QString& path,
                                TPlaylistWidgetItem* parent,
                                TPlaylistWidgetItem* item,
                                bool append) {

    latestDir = path;
    if (append) {
        parent->addChild(item);
    } else {
        // Sort folder into place
        int i = parent->childCount();
        while (i > 0 && *item < *(parent->plChild(i - 1))) {
            i--;
        }
        parent->insertChild(i, item);
    }

    // Propagate modified
    if (item->modified()) {
        parent->setModified(true, false, true);
    }
}

TPlaylistWidgetItem* TAddFilesThread::addDirectory(TPlaylistWidgetItem* parent,
                                                   QFileInfo& fi,
                                                   const QString& name,
                                                   bool protectName,
                                                   bool append) {
    logger()->debug("addDirectory: '%1'", fi.absoluteFilePath());

    emit displayMessage(fi.absoluteFilePath(), 0);

    QDir directory(fi.absoluteFilePath());
    TFileLock lock(directory, lockedFiles);
    if (!lock.locked) {
        return 0;
    }

    if (directory.exists(TConfig::WZPLAYLIST)) {
        fi.setFile(directory.path(), TConfig::WZPLAYLIST);
        return openPlaylist(parent, fi, name, protectName, append);
    }

    directory.setFilter(dirFilter);
    directory.setNameFilters(nameFilterList);
    directory.setSorting(getSortFlags());

    QString path = QDir::toNativeSeparators(directory.path());

    TPlaylistWidgetItem* dirItem = new TPlaylistWidgetItem(0, path,
        name.isEmpty() ? directory.dirName() : name,
        0, true);

    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    foreach(fi, directory.entryInfoList()) {
        if (stopRequested) {
            break;
        }

        // Use full name for name blacklist
        if (nameBlackListed(path + fi.fileName())) {
            continue;
        }

        if (fi.isDir()) {
            if (recurse) {
                addDirectory(dirItem, fi, fi.fileName(), false, true);
            }
        } else {
            addFile(dirItem, fi);
        }
    }

    if (dirItem->childCount()) {
        addFolder(directory.path(), parent, dirItem, append);
    } else {
        logger()->debug("addDirectory: found no playable items in '%1'",
                        directory.path());
        delete dirItem;
        dirItem = 0;
    }

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

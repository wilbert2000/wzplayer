#include "gui/playlist/addfilesthread.h"

#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QRegExp>
#include <QTextStream>
#include <QTextCodec>

#include "gui/playlist/playlistitem.h"
#include "discname.h"
#include "name.h"
#include "extensions.h"
#include "config.h"


namespace Gui {
namespace Playlist {

class TFileLock {
public:
    bool locked;
    TFileLock(const QFileInfo& fi, QStringList& lfiles);
    virtual ~TFileLock();
private:
    QStringList& lockedFiles;
    bool acquire(QString filename);
};

bool TFileLock::acquire(QString filename) {

    if (filename.isEmpty()) {
        Log4Qt::Logger::logger("Gui::Playlist::TFileLock")->error(
            "Acquire Ignoring empty filename");
        return false;
    }

    filename = QDir::toNativeSeparators(filename);

    // TODO: case
    if (lockedFiles.contains(filename, caseSensitiveFileNames)) {
        Log4Qt::Logger::logger("Gui::Playlist::TFileLock")->info(
            "Acquire Skipping circular reference '%1'", filename);
        return false;
    }

    lockedFiles.append(filename);
    return true;
}

TFileLock::TFileLock(const QFileInfo& fi, QStringList& lfiles) :
    lockedFiles(lfiles) {
    locked = acquire(fi.canonicalFilePath());
}

TFileLock::~TFileLock() {

    if (locked) {
        lockedFiles.removeLast();
    }
}


QDir::Filters dirFilter = QDir::Dirs
                          | QDir::AllDirs
                          | QDir::Files
                          | QDir::Drives
                          | QDir::NoDotAndDotDot
                          | QDir::Readable
// for Windows ".lnk" files QDir::System is needed
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
                                 bool images,
                                 bool favList) :
    QThread(parent),
    debug(logger()),
    files(aFiles),
    root(0),
    abortRequested(false),
    stopRequested(false),
    recurse(recurseSubDirs),
    addImages(images),
    isFavList(favList) {

    rxNameBlacklist.reserve(nameBlacklist.count());
    QRegExp rx("", Qt::CaseInsensitive);

    for(int i = nameBlacklist.count() - 1; i >=0; i--) {
        const QString& name = nameBlacklist.at(i);
        if (!name.isEmpty()) {
            rx.setPattern(name);
            if (rx.isValid()) {
                WZINFO("Compiled '" + rx.pattern() + "' for blacklist");
                rxNameBlacklist.append(rx);
            } else {
                WZERROR("Failed to parse regular expression '" + name + "'");
            }
        }
    }

    TExtensionList exts;
    if (videoFiles) {
        exts = extensions.videoAndAudio();
    }
    if (audioFiles) {
        exts.addList(extensions.audio());
    }
    if (playlists) {
        exts.addList(extensions.playlists());
    }
    if (addImages) {
        exts.addList(extensions.images());
    }
    nameFilterList = exts.forDirFilter();

#ifdef Q_OS_WIN
    nameFilterList << "*.lnk";
#endif

    if (logger()->isDebugEnabled()) {
        debug << "TAddFilesThread Searching for:" << nameFilterList;
        debug << debug;
    }
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    playlistPath = QDir::toNativeSeparators(QDir::current().path());
    WZDEBUG(QString("Running in directory '%1'").arg(playlistPath));

    root = new TPlaylistItem(0, playlistPath, "", 0);
    root->setFlags(ROOT_FLAGS);

    addFiles();

    if (abortRequested) {
        delete root;
        root = 0;
    } else {
        // TPlaylist uses empty filename to determine whether to save playlist
        root->setFilename("");
    }

    WZINFO(QString("Run done. Stopped %1, aborted %2")
           .arg(stopRequested).arg(abortRequested));
    if (abortRequested) {
        emit displayMessage(tr("Scan aborted"), TConfig::MESSAGE_DURATION);
    } else if (stopRequested) {
        emit displayMessage(tr("Scan stopped"), TConfig::MESSAGE_DURATION);
    } else {
        emit displayMessage("", 1); // Clear last msg
    }
}

bool TAddFilesThread::nameBlackListed(const QString& name) {

    for(int i = rxNameBlacklist.size() - 1; i >= 0; i--) {
        const QRegExp& rx = rxNameBlacklist.at(i);
        if (rx.indexIn(name) >= 0) {
            WZINFO("Skipping '" + name + "' on '" + rx.pattern() + "'");
            return true;
        }
    }

    return false;
}

TPlaylistItem* TAddFilesThread::createPath(TPlaylistItem* parent,
                                           const QFileInfo& fi,
                                           const QString& name,
                                           double duration,
                                           bool protectName) {

    QString parentPath = parent->playlistPath();
    QString parentPathPlusSep = parentPath;
    if (!parentPathPlusSep.endsWith("/")) {
        parentPathPlusSep += "/";
    }

    // Remove extra slashes and dots from path
    QString path = fi.dir().absolutePath();

    // File residing inside parent directory
    if (path == parentPath) {
        QString filename = parentPathPlusSep + fi.fileName();
        //WZTRACE("creating file '" + filename + "'");
        return new TPlaylistItem(parent,
                                 filename,
                                 name,
                                 duration,
                                 protectName);
    }

    // File residing outside parent directory from symbolic link or playlist
    if (!path.startsWith(parentPathPlusSep)) {
        QString filename = fi.absoluteFilePath();
        WZTRACE(QString("Creating link in '%1' to '%2'")
                .arg(parent->filename()).arg(filename));
        return new TPlaylistItem(parent,
                                 filename,
                                 name,
                                 duration,
                                 protectName);
    }

    // File residing in subdir of parent
    QString dir = path.mid(parentPathPlusSep.length());
    int i = dir.indexOf("/");
    if (i >= 0) {
        dir = dir.left(i);
    }
    if (dir.isEmpty()) {
        WZERROR("Invalid path");
        return 0;
    }
    path = parentPathPlusSep + dir;

    // Only check last child to preserve order of playlist
    if (parent->childCount()) {
        TPlaylistItem* child = parent->plChild(parent->childCount() - 1);
        if (child->playlistPath() == path) {
            createPath(child, fi, name, duration, protectName);
            return child;
        }
    }

    WZDEBUG("Creating folder '" + path + "'");
    emit displayMessage(path, 0);
    TPlaylistItem* folder = new TPlaylistItem(parent, path, dir, 0);
    createPath(folder, fi, name, duration, protectName);
    return folder;
}

void TAddFilesThread::addNewItems(TPlaylistItem* playlistItem) {

    emit displayMessage(playlistPath, 0);

    // Collect relative file names loaded from playlist
    QStringList files;
    QString path = playlistPath;
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }
    for(int c = 0; c < playlistItem->childCount(); c++) {
        QString fn = playlistItem->plChild(c)->filename();
        if (fn.startsWith(path)) {
            // Remove path creating relative file name
            fn = fn.mid(path.length());
            // Reduce paths to containing subdirectory
            int i = fn.indexOf(QDir::separator());
            if (i >= 0) {
                fn = fn.left(i);
            }
        }
        files << fn;
    }

    QStringList blacklist = playlistItem->getBlacklist();

    QDir directory(playlistPath);
    directory.setFilter(dirFilter);
    directory.setNameFilters(nameFilterList);
    directory.setSorting(getSortFlags());

    foreach(QFileInfo fi, directory.entryInfoList()) {
        if (stopRequested) {
            break;
        }

        QString filename = fi.fileName();
        if (files.contains(filename, caseSensitiveFileNames)) {
            continue;
        }

        int i = blacklist.indexOf(QRegExp(filename,
                                          caseSensitiveFileNames,
                                          QRegExp::FixedString));
        if (i >= 0) {
            WZINFO("'" + filename + "' is blacklisted");
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
            && files.contains(fi.symLinkTarget(), caseSensitiveFileNames)) {
            continue;
        }

        TPlaylistItem* newItem = 0;
        if (fi.isDir()) {
            if (recurse) {
                WZINFO("Adding new folder '" + filename + "'");
                newItem = addDirectory(playlistItem, fi, filename, false);
            }
        } else {
            WZINFO("Adding new file '" + filename + "'");
            newItem = addFile(playlistItem, fi);
        }
        if (newItem) {
            newItem->setModified();
        }
    }

    // Remove no longer existing items from blacklist
    if (!stopRequested) {
        foreach(const QString& filename, blacklist) {
            playlistItem->whitelist(filename);
            playlistItem->setModified();
        }
    }
}

bool TAddFilesThread::openM3u(TPlaylistItem* playlistItem,
                              const QString& fileName) {

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream stream(&file);
    if (playlistItem->extension() == "m3u") {
        stream.setCodec(QTextCodec::codecForLocale());
    } else {
        stream.setCodec("UTF-8");
    }

    QString name;
    double duration = 0;
    bool edited;
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
            edited = !name.isEmpty() && name != TName::baseNameForURL(line);
            addItem(playlistItem, line, name, duration, edited, true);
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

    if (!stopRequested && playlistItem->isWZPlaylist()) {
        addNewItems(playlistItem);
    }

    return true;
}

TPlaylistItem* TAddFilesThread::openPlaylist(TPlaylistItem *parent,
                                             const QFileInfo& fi,
                                             const QString& name,
                                             bool protectName) {
    WZDEBUG("'" + fi.filePath() + "'");

    TFileLock lock(fi, lockedFiles);
    if (!lock.locked) {
        return 0;
    }

    emit displayMessage(fi.filePath(), 0);

    // Use native names for playlistPath and fileName
    QString sourceFileName;
    QString fileName;
    if (fi.isSymLink()) {
        sourceFileName = fi.symLinkTarget();
        playlistPath = QDir::toNativeSeparators(
                           QFileInfo(sourceFileName).dir().absolutePath());
        fileName = QDir::toNativeSeparators(fi.dir().absolutePath())
                   + QDir::separator() + fi.fileName();
    } else {
        playlistPath = QDir::toNativeSeparators(fi.dir().absolutePath());
        fileName = playlistPath + QDir::separator() + fi.fileName();
        sourceFileName = fileName;
    }

    // Put playlist in a folder
    TPlaylistItem* playlistItem = new TPlaylistItem(
        parent,
        fileName, // native file name
        name, // name passed to openPlaylist()
        0, // duration
        protectName);

    if (openM3u(playlistItem, sourceFileName)) {
        if (playlistItem->childCount()) {
            latestDir = playlistPath;
        } else {
            WZINFO("Found no playable items in '" + sourceFileName + "'");
            delete playlistItem;
            playlistItem = 0;
        }
    } else {
        WZERROR("Failed to open '" + sourceFileName + "'");
        emit displayMessage(tr("Failed to open '%1'").arg(sourceFileName),
                            TConfig::ERROR_MESSAGE_DURATION);
        delete playlistItem;
        playlistItem = 0;
    }

    return playlistItem;
}

TPlaylistItem* TAddFilesThread::addFile(TPlaylistItem* parent,
                                        const QFileInfo& fi) {

    QString name = fi.completeBaseName();

    if (extensions.isPlaylist(fi)) {
        return openPlaylist(parent, fi, name, false);
    }

    if (fi.isSymLink()) {
        QFileInfo target(fi.symLinkTarget());

        if (fi.suffix().toLower() == "lnk") {
            return new TPlaylistItem(parent, target.absoluteFilePath(), name, 0);
        }

        if (extensions.isMultiMedia(target)) {
            return new TPlaylistItem(parent, fi.absoluteFilePath(), name, 0);
        }

        return 0;
    }

    return new TPlaylistItem(parent, fi.absoluteFilePath(), name, 0);
}

QDir::SortFlags TAddFilesThread::getSortFlags() {
    return QDir::NoSort;
}

TPlaylistItem* TAddFilesThread::addDirectory(TPlaylistItem* parent,
                                             QFileInfo& fi,
                                             QString name,
                                             bool protectName) {
    WZDEBUG("'" + fi.absoluteFilePath() + "'");

    TFileLock lock(fi, lockedFiles);
    if (!lock.locked) {
        return 0;
    }

    if (protectName) {
        WZINFO(QString("Protecting name '%1' for '%2'")
               .arg(name).arg(fi.absoluteFilePath()));
    }

    emit displayMessage(fi.absoluteFilePath(), 0);

    QDir directory(fi.absoluteFilePath());

    if (name.isEmpty()) {
        name = directory.dirName();
    }

    if (directory.exists(TConfig::WZPLAYLIST)) {
        fi.setFile(directory.path(), TConfig::WZPLAYLIST);
        return openPlaylist(parent, fi, name, protectName);
    }

    directory.setFilter(dirFilter);
    directory.setNameFilters(nameFilterList);
    directory.setSorting(getSortFlags());

    QString path = QDir::toNativeSeparators(directory.path());

    TPlaylistItem* dirItem = new TPlaylistItem(parent, path, name, 0,
                                               protectName);

    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    foreach(QFileInfo f, directory.entryInfoList()) {
        // Stop collecting files when stop requested
        if (stopRequested) {
            break;
        }

        // Check against blacklist
        if (nameBlackListed(path + f.fileName())) {
            continue;
        }

        if (f.isDir()) {
            if (recurse) {
                addDirectory(dirItem, f, f.fileName(), false);
            } else {
                WZTRACE("Skipping directory '" + f.fileName() + "'");
            }
        } else {
            addFile(dirItem, f);
        }
    }

    if (dirItem->childCount()) {
        latestDir = directory.path();
    } else if (isFavList && parent->baseName() == "Favorites") {
        // Keep empty folders inside favorites directory
        WZINFO("Found no playable items in '" + dirItem->filename() + "'");
    } else {
        WZDEBUG("Found no playable items in '" + dirItem->filename() + "'");
        delete dirItem;
        dirItem = 0;
    }

    return dirItem;
}

TPlaylistItem* TAddFilesThread::addItem(TPlaylistItem* parent,
                                        QString filename,
                                        QString name,
                                        double duration,
                                        bool protectName,
                                        bool useBlackList) {

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    if (protectName) {
        WZINFO(QString("Protecting name '%1' for '%2'")
               .arg(name).arg(filename));
    }

    QFileInfo fi(playlistPath, filename);
    if (!fi.exists()) {
        if (fi.fileName().compare(TConfig::WZPLAYLIST, caseSensitiveFileNames)
                == 0) {
            // Non-existing wzplaylist
            parent->setModified();

            // Try the directory
            QDir dir = fi.dir();
            if (dir.exists()) {
                if (dir == QDir(playlistPath)) {
                    WZERROR(QString("Skipping self referencing folder in"
                                    " playlist '%1'").arg(filename));
                    return 0;
                }

                fi.setFile(dir.absolutePath());
                name = "";
                protectName = false;
                WZINFO(QString("'%1' does not exists. Adding directory '%2'"
                               " instead")
                       .arg(filename).arg(fi.absoluteFilePath()));
            } else {
                WZINFO("Ignoring no longer existing playlist '" + filename + "'");
                return 0;
            }
        } else {
            TDiscName disc(filename);
            if (disc.valid) {
                if (name.isEmpty()) {
                    name = disc.displayName();
                }
                WZTRACE(QString("Adding disc '%1'").arg(filename));
            } else if (QUrl(filename).scheme().isEmpty()) {
                // Non existing local file
                if (!isFavList && parent->isWZPlaylist()) {
                    WZINFO(QString("Ignoring no longer existing file '%1'")
                           .arg(filename));
                    parent->setModified();
                    return 0;
                }
                WZINFO(QString("Adding non-existing file '%1'").arg(filename));
                // Don't set item to failed yet, might be temporarely
                // unaccesible or a syntax the player understands, but which
                // is not a valid filename.
            } else {
                WZTRACE(QString("Adding URL '%1'").arg(filename));
            }
            return new TPlaylistItem(parent, filename, name, duration,
                                     protectName);
        }
    }

    // Existing item
    // Check against blacklist
    if (useBlackList && nameBlackListed(fi.absoluteFilePath())) {
        // An item in the playlist matches the blacklist. Signal we dropped it.
        parent->setModified();
        return 0;
    }

    QString savedPlaylistPath = playlistPath;

    TPlaylistItem* item;
    if (fi.isDir()) {
        item = addDirectory(parent, fi, name, protectName);
    } else if (extensions.isPlaylist(fi)) {
        item = openPlaylist(parent, fi, name, protectName);
    } else {
        // For Windows shortcuts, follow the link
        if (fi.isSymLink() && fi.suffix().toLower() == "lnk") {
            fi.setFile(fi.symLinkTarget());
        }

        // Skip images
        if (!addImages && extensions.isImage(fi)) {
            return 0;
        }

        item = createPath(parent, fi, name, duration, protectName);
    }

    playlistPath = savedPlaylistPath;
    return item;
}

void TAddFilesThread::addFiles() {

    for(int i = 0; i < files.count(); i++) {
        if (stopRequested) {
            break;
        }

        QString filename = files.at(i);
        if (filename.isEmpty()) {
            continue;
        }

        // Note: explicitly added items are not blacklisted
        addItem(root,
                filename,
                "" /* name */,
                0 /* duartion */,
                false, /* protect name */
                false /* use blacklist */);
    }
}

} // namespace Playlist
} // namespace Gui

#include "gui/playlist/addfilesthread.h"

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
            "acquire ignoring empty filename");
        return false;
    }

    filename = QDir::toNativeSeparators(filename);

    if (lockedFiles.contains(filename, caseSensitiveFileNames)) {
        Log4Qt::Logger::logger("Gui::Playlist::TFileLock")->info(
            "acquire skipping '%1', would create an infinite list", filename);
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
                                 bool images) :
    QThread(parent),
    debug(logger()),
    files(aFiles),
    root(0),
    abortRequested(false),
    stopRequested(false),
    recurse(recurseSubDirs),
    addImages(images) {

    rxNameBlacklist.reserve(nameBlacklist.count());
    QRegExp rx("", Qt::CaseInsensitive);

    for(int i = nameBlacklist.count() - 1; i >=0; i--) {
        const QString& name = nameBlacklist.at(i);
        if (!name.isEmpty()) {
            rx.setPattern(name);
            if (rx.isValid()) {
                WZINFO("compiled '" + rx.pattern() + "' for blacklist");
                rxNameBlacklist.append(rx);
            } else {
                WZERROR("failed to parse regular expression '" + name + "'");
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
        debug << "TAddFilesThread searching for:" << nameFilterList;
        debug << debug;
    }
}

TAddFilesThread::~TAddFilesThread() {
    delete root;
}

void TAddFilesThread::run() {

    playlistPath = QDir::toNativeSeparators(QDir::current().path());
    WZDEBUG("running in '" + playlistPath + "'");

    root = new TPlaylistWidgetItem(0, playlistPath, "", 0);
    root->setFlags(ROOT_FLAGS);

    addFiles();

    if (abortRequested) {
        delete root;
        root = 0;
    } else {
        // TPlaylist uses empty filename to determine whether to save playlist
        root->setFilename("");
    }

    WZDEBUG(QString("exiting. stopped %1 aborted %2")
            .arg(stopRequested).arg(abortRequested));
}

bool TAddFilesThread::nameBlackListed(const QString& name) {

    for(int i = rxNameBlacklist.size() - 1; i >= 0; i--) {
        const QRegExp& rx = rxNameBlacklist.at(i);
        if (rx.indexIn(name) >= 0) {
            WZINFO("skipping '" + name + "' on '" + rx.pattern() + "'");
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
        return new TPlaylistWidgetItem(parent,
                                       filename,
                                       name,
                                       duration,
                                       protectName);
    }

    // File residing outside parent directory from symbolic link or playlist
    if (!path.startsWith(parentPathPlusSep)) {
        QString filename = fi.absoluteFilePath();
        WZTRACE("creating link '" + filename + "' in '" + parent->filename()
                + "'");
        return new TPlaylistWidgetItem(parent,
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
        WZERROR("invalid path");
        return 0;
    }
    path = parentPathPlusSep + dir;

    // Only check last child to preserve order of playlist
    if (parent->childCount()) {
        TPlaylistWidgetItem* child = parent->plChild(parent->childCount() - 1);
        if (child->playlistPath() == path) {
            createPath(child, fi, name, duration, protectName);
            return child;
        }
    }

    WZDEBUG("creating folder '" + path + "'");
    emit displayMessage(path, 0);
    TPlaylistWidgetItem* folder = new TPlaylistWidgetItem(parent, path, dir, 0);
    createPath(folder, fi, name, duration, protectName);
    return folder;
}

void TAddFilesThread::addNewItems(TPlaylistWidgetItem* playlistItem) {

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

        TPlaylistWidgetItem* w = 0;
        if (fi.isDir()) {
            if (recurse) {
                WZINFO("adding folder '" + filename + "'");
                w = addDirectory(playlistItem, fi, filename, false);
            }
        } else {
            WZINFO("adding file '" + filename + "'");
            w = addFile(playlistItem, fi);
        }
        if (w) {
            w->setModified();
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

bool TAddFilesThread::openM3u(TPlaylistWidgetItem* playlistItem,
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
            addItem(playlistItem, line, name, duration, true);
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

TPlaylistWidgetItem* TAddFilesThread::openPlaylist(TPlaylistWidgetItem *parent,
                                                   const QFileInfo& fi,
                                                   const QString& name,
                                                   bool protectName) {
    WZINFO("'" + fi.filePath() + "'");

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
    TPlaylistWidgetItem* playlistItem = new TPlaylistWidgetItem(
        parent,
        fileName, // native file name
        name, // name passed to openPlaylist()
        0, // duration
        protectName);

    if (openM3u(playlistItem, sourceFileName)) {
        if (playlistItem->childCount()) {
            latestDir = playlistPath;
        } else {
            WZINFO("found no playable items in '" + sourceFileName + "'");
            delete playlistItem;
            playlistItem = 0;
        }
    } else {
        WZERROR("failed to open '" + sourceFileName + "'");
        emit displayMessage(tr("Failed to open '%1'").arg(sourceFileName),
                            TConfig::ERROR_MESSAGE_DURATION);
        delete playlistItem;
        playlistItem = 0;
    }

    return playlistItem;
}

TPlaylistWidgetItem* TAddFilesThread::addFile(TPlaylistWidgetItem* parent,
                                              const QFileInfo& fi) {

    QString name = fi.completeBaseName();

    if (extensions.isPlaylist(fi)) {
        return openPlaylist(parent, fi, name, false);
    }

    if (fi.isSymLink()) {
        QFileInfo target(fi.symLinkTarget());

        if (fi.suffix().toLower() == "lnk") {
            return new TPlaylistWidgetItem(parent, target.absoluteFilePath(),
                                           name, 0);
        }

        if (extensions.isMultiMedia(target)) {
            return new TPlaylistWidgetItem(parent, fi.absoluteFilePath(), name,
                                           0);
        }

        return 0;
    }

    return new TPlaylistWidgetItem(parent, fi.absoluteFilePath(), name, 0);
}

QDir::SortFlags TAddFilesThread::getSortFlags() {
    return QDir::NoSort;
}

TPlaylistWidgetItem* TAddFilesThread::addDirectory(TPlaylistWidgetItem* parent,
                                                   QFileInfo& fi,
                                                   QString name,
                                                   bool protectName) {
    WZDEBUG("'" + fi.absoluteFilePath() + "'");

    TFileLock lock(fi, lockedFiles);
    if (!lock.locked) {
        return 0;
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

    TPlaylistWidgetItem* dirItem = new TPlaylistWidgetItem(parent, path, name,
                                                           0, protectName);

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
    } else {
        WZDEBUG("found no playable items in '" + dirItem->filename() + "'");
        delete dirItem;
        dirItem = 0;
    }

    return dirItem;
}

TPlaylistWidgetItem* TAddFilesThread::addItemNotFound(
        TPlaylistWidgetItem* parent,
        const QString& filename,
        QString name,
        bool protectName) {

    TDiscName disc(filename);
    if (disc.valid) {
        if (name.isEmpty()) {
            name = disc.displayName();
        }
    } else if (parent->isWZPlaylist()){

#ifdef Q_OS_WIN
        bool localFile = true;
#else
        bool localFile = QUrl(filename).scheme().isEmpty();
#endif

        if (localFile) {
            WZINFO("ignoring no longer existing playlist item '" + filename
                   + "'");
            parent->setModified();
            return 0;
         }
    }

    return new TPlaylistWidgetItem(parent, filename, name, 0, protectName);
}

TPlaylistWidgetItem* TAddFilesThread::addItem(TPlaylistWidgetItem* parent,
                                              QString filename,
                                              QString name,
                                              double duration,
                                              bool useBlackList) {

    bool protectName = !name.isEmpty();

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(playlistPath, filename);
    if (fi.exists()) {
        //WZTRACE("found '" + fi.absoluteFilePath() + "'");
    } else if (fi.fileName().compare(TConfig::WZPLAYLIST,
                                     caseSensitiveFileNames) == 0) {
        // Non-existing wzplaylist
        parent->setModified();

        // Try the directory
        if (fi.dir().exists()) {
            QString dir = fi.dir().absolutePath();
            if (dir.compare(playlistPath, caseSensitiveFileNames) == 0) {
                WZERROR("skipping self referencing folder '" + dir
                        + "' in playlist '" + filename + "'");
                return 0;
            }

            fi.setFile(dir);
            name = "";
            protectName = false;
            WZINFO("'" + filename + "' no longer exists. Adding directory '"
                   + fi.absoluteFilePath() + "' instead");
        } else {
            WZINFO("ignoring no longer existing playlist '" + filename + "'");
            return 0;
        }
    } else {
        return addItemNotFound(parent, filename, name, protectName);
    }

    // Check against blacklist
    if (useBlackList && nameBlackListed(fi.absoluteFilePath())) {
        if (parent->isWZPlaylist()) {
            parent->setModified();
        }
        return 0;
    }

    QString savedPlaylistPath = playlistPath;

    TPlaylistWidgetItem* item;
    if (fi.isDir()) {
        item = addDirectory(parent, fi, name, protectName);
    } else {
        if (name.isEmpty()) {
            name = fi.completeBaseName();
        }
        if (extensions.isPlaylist(fi)) {
            item = openPlaylist(parent, fi, name, protectName);
        } else {
            // For Windows shortcuts, follow the link...
            if (fi.isSymLink() && fi.suffix().toLower() == "lnk") {
                fi.setFile(fi.symLinkTarget());
            }

            // Skip images
            if (!addImages && extensions.isImage(fi)) {
                return 0;
            }

            item = createPath(parent, fi, name, duration, protectName);
        }
    }

    playlistPath = savedPlaylistPath;
    return item;
}

void TAddFilesThread::addFiles() {

    foreach(const QString& filename, files) {
        if (stopRequested) {
            break;
        }
        if (filename.isEmpty()) {
            continue;
        }
        TPlaylistWidgetItem* result = addItem(root, filename,
                                              "" /* name */,
                                              0 /* duartion */,
                                              false /* use black list */);
        if (result) {
            result->setSelected(true);
        }
    }
}

} // namespace Playlist
} // namespace Gui

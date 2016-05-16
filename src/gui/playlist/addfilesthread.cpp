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
        logger()->warn("createPath: invalid path");
        return;
    }
    path = parentPathPlus + dir;

    // Only check last child to preserve order of playlist
    if (parent->childCount()) {
        TPlaylistWidgetItem* child = static_cast<TPlaylistWidgetItem*>(
            parent->child(parent->childCount() - 1));
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

void TAddFilesThread::cleanAndAddItem(
        QString filename,
        QString name,
        double duration,
        TPlaylistWidgetItem* parent) {

    bool protect_name = !name.isEmpty();
    QString alt_name;

    if (filename.startsWith("file:")) {
        filename = QUrl(filename).toLocalFile();
    }

    QFileInfo fi(filename);
    if (fi.exists()) {
        logger()->trace("cleanAndAddItem: found '" + fi.fileName() + "'");
        alt_name = fi.fileName();
    } else {
        // Try relative path
        fi.setFile(playlistPath, filename);
        if (fi.exists()) {
            logger()->trace("cleanAndAddItem: found relative path to '"
                            + fi.fileName() + "'");
            alt_name = fi.fileName();
        } else {
            logger()->trace("cleanAndAddItem: adding '" + filename + "'");
            new TPlaylistWidgetItem(parent, filename, name, duration, false,
                                    iconProvider.icon(fi));
            return;
        }
    }

#ifdef Q_OS_WIN
    if (fi.isSymLink()) {
        logger()->debug("cleanAndAddItem: following '" + fi.fileName()
                        + "' to '" + fi.symLinkTarget() + "'");
        fi.setFile(fi.symLinkTarget());
    }
#endif

    if (fi.isDir()) {
        TPlaylistWidgetItem* dir = addDirectory(parent, fi.absoluteFilePath());
        if (dir && !name.isEmpty() && name != dir->name()) {
            dir->setName(name);
            dir->setEdited(true);
        }
        return;
    }

    if (name.isEmpty()) {
        name = alt_name;
    }

    createPath(parent, fi, name, duration, protect_name);
    return;
}

TPlaylistWidgetItem* TAddFilesThread::openM3u(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {
    logger()->info("openM3u: '" + playlistFileName + "'");

    QFileInfo fi(playlistFileName);
    QFile file(fi.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        emit displayMessage(tr("Failed to open %1").arg(fi.absoluteFilePath()),
                            TConfig::ERROR_MESSAGE_DURATION);
        return 0;
    }

    // Path to use for relative filenames in playlist, use QDir to remove .. ///
    playlistPath = fi.dir().path();
    QString savedPlaylistPath = playlistPath;

    // Put playlist in a folder
    TPlaylistWidgetItem* playlistItem = new TPlaylistWidgetItem(0,
        playlistPath + "/" + fi.fileName(), fi.fileName(), 0, true,
        iconProvider.icon(fi));

    QTextStream stream(&file);
    if (fi.suffix().toLower() != "m3u") {
        stream.setCodec("UTF-8");
    } else {
        stream.setCodec(QTextCodec::codecForLocale());
    }

    QString name;
    double duration = 0;

    QRegExp rx("^#EXTINF:\\s*(\\d+)\\s*,\\s*(.*)");

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        // Ignore empty lines
        if (line.isEmpty()) {
            continue;
        }
        if (rx.indexIn(line) >= 0) {
            duration = rx.cap(1).toDouble();
            name = rx.cap(2).simplified();
        } else if (!line.startsWith("#")) {
            cleanAndAddItem(line, name, duration, playlistItem);
            playlistPath = savedPlaylistPath;
            name = "";
            duration = 0;
        }
    }

    file.close();

    if (playlistItem->childCount())  {
        parent->addChild(playlistItem);
        latestDir = fi.absoluteFilePath();
    } else {
        delete playlistItem;
        playlistItem = 0;
    }

    return playlistItem;
}

TPlaylistWidgetItem* TAddFilesThread::openPls(const QString& playlistFileName,
                                              TPlaylistWidgetItem* parent) {
    logger()->info("openPls: '" + playlistFileName + "'");

    QFileInfo fi(playlistFileName);
    QSettings set(fi.absoluteFilePath(), QSettings::IniFormat);
    set.beginGroup("playlist");
    if (set.status() != QSettings::NoError) {
        emit displayMessage(tr("Failed to open %1").arg(fi.absoluteFilePath()),
                            TConfig::ERROR_MESSAGE_DURATION);
        return 0;
    }

    // Path to use for relative filenames in playlist, use QDir to remove .. ///
    playlistPath = fi.dir().path();
    QString savedPlaylistPath = playlistPath;

    // Put playlist in a folder
    TPlaylistWidgetItem* playlistItem = new TPlaylistWidgetItem(0,
        playlistPath + "/" + fi.fileName(), fi.fileName(), 0, true,
        iconProvider.icon(fi));

    QString filename;
    QString name;
    double duration;

    int num_items = set.value("NumberOfEntries", 0).toInt();
    for (int n = 1; n <= num_items; n++) {
        QString ns = QString::number(n);
        filename = set.value("File" + ns, "").toString();
        name = set.value("Title" + ns, "").toString().simplified();
        duration = (double) set.value("Length" + ns, 0).toInt();
        cleanAndAddItem(filename, name, duration, playlistItem);
        playlistPath = savedPlaylistPath;
    }

    set.endGroup();

    if (playlistItem->childCount())  {
        parent->addChild(playlistItem);
        latestDir = fi.absoluteFilePath();
    } else {
        delete playlistItem;
        playlistItem = 0;
    }

    return playlistItem;
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
        QString ext = fi.suffix().toLower();
        if (ext == "pls") {
            item = openPls(filename, parent);
        } else if (ext == "m3u8" || ext == "m3u") {
            item = openM3u(filename, parent);
        } else {
            item = new TPlaylistWidgetItem(parent, fi.absoluteFilePath(),
                fi.fileName(), 0, false, iconProvider.icon(fi));
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
        item = new TPlaylistWidgetItem(parent, filename, name, 0, false,
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
    TPlaylistWidgetItem* item = new TPlaylistWidgetItem(0, directory.path(),
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

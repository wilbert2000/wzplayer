#include "gui/playlist/playlistwidgetitem.h"
#include "gui/playlist/playlistwidget.h"
#include "player/player.h"
#include "wzdebug.h"
#include "name.h"
#include "extensions.h"
#include "iconprovider.h"
#include "wztime.h"
#include "config.h"

#include <QApplication>
#include <QDir>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>


namespace Gui {
namespace Playlist {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistWidgetItem)

// TODO: find the file sys func reporting case
Qt::CaseSensitivity caseSensitiveFileNames =
        #ifdef Q_OS_WIN
                    Qt::CaseInsensitive;
        #else
                    Qt::CaseSensitive;
        #endif

// Text alignment for columns
const int TEXT_ALIGN_NAME = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;
const int TEXT_ALIGN_TYPE = Qt::AlignLeft | Qt::AlignVCenter;
const int TEXT_ALIGN_TIME = Qt::AlignRight | Qt::AlignVCenter;
const int TEXT_ALIGN_ORDER = Qt::AlignRight | Qt::AlignVCenter;

// Level of the root node in the tree view, where level means the number of
// icons indenting the item. With root decoration on, toplevel items appear on
// level 2, being ROOT_NODE_LEVEL + 1.
const int TPlaylistWidgetItem::ROOT_NODE_LEVEL = 1;
// Width of name column. Updated by TPlaylistWidget event handlers.
int TPlaylistWidgetItem::gNameColumnWidth = 0;
// Set by TPlaylistWidget constructor
QFontMetrics TPlaylistWidgetItem::gNameFontMetrics = QFontMetrics(QFont());


// Generate a timestamp for played items
class TTimeStamp : public QTime {
public:
    TTimeStamp();
    virtual ~TTimeStamp();

    int getStamp();
};

TTimeStamp::TTimeStamp() : QTime() {
}

TTimeStamp::~TTimeStamp() {
}

int TTimeStamp::getStamp() {

    if (isNull()) {
        start();
        return 1;
    }
    return elapsed();
}

static TTimeStamp timeStamper;


QString TPlaylistWidgetItem::playlistItemState(TPlaylistWidgetItemState state) {

    switch (state) {
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_LOADING: return "loading";
        case PSTATE_PLAYING: return "playing";
        case PSTATE_FAILED: ;
    }
    return "failed";
}


// Constructor used for root item
TPlaylistWidgetItem::TPlaylistWidgetItem() :
    QTreeWidgetItem(),
    mDuration(0),
    mOrder(1),
    mFolder(true),
    mPlaylist(false),
    mWZPlaylist(false),
    mSymLink(false),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(false),
    mModified(false),
    mPlayedTime(0) {

    setFlags(ROOT_FLAGS);
    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_EXT, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_TIME, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);
}

// Copy constructor
TPlaylistWidgetItem::TPlaylistWidgetItem(const TPlaylistWidgetItem& item) :
    QTreeWidgetItem(item),
    mFilename(item.filename()),
    mBaseName(item.baseName()),
    mExt(item.extension()),
    mDuration(item.duration()),
    mOrder(item.order()),

    mFolder(item.isFolder()),
    mPlaylist(item.isPlaylist()),
    mWZPlaylist(item.isWZPlaylist()),
    mSymLink(item.isSymLink()),
    mTarget(item.target()),

    mState(item.state()),
    mPlayed(item.played()),
    mEdited(item.edited()),
    mModified(item.modified()),
    mPlayedTime(item.playedTime()),
    mBlacklist(item.getBlacklist()),

    itemIcon(item.itemIcon) {
}

// Used for every item except the root
TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool protectName) :
    QTreeWidgetItem(parent),
    mFilename(QDir::toNativeSeparators(filename)),
    mBaseName(name),
    mDuration(duration),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(protectName),
    mModified(false),
    mPlayedTime(0) {

    if (parent) {
        mOrder = parent->childCount();
    } else {
        mOrder = 1;
    }
    if (mBaseName.isEmpty()) {
        // setFileInfo removes the extension
        mBaseName = TName::nameForURL(mFilename);
    }
    setFileInfo();

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsEnabled
                          | Qt::ItemIsDragEnabled;

    if (mFolder) {
        setFlags(flags | Qt::ItemIsDropEnabled);
    } else {
        setFlags(flags);
    }

    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_EXT, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_TIME, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);

    setIcon(COL_NAME, itemIcon);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

// Update fields depending on file name
void TPlaylistWidgetItem::setFileInfo() {

    QFileInfo fi(mFilename);
    mPlaylist = extensions.isPlaylist(fi);
    mSymLink = fi.isSymLink();
    mTarget = fi.symLinkTarget();
    if (mSymLink) {
        fi.setFile(fi.symLinkTarget());
    }
    // Root item uses empty filename
    if (mFilename.isEmpty() || fi.isDir()) {
        mExt = "";
        mFolder = true;
        mWZPlaylist = false;
    } else {
        mExt = fi.suffix().toLower();

        // Remove extension from base name
        if (!mExt.isEmpty() && mBaseName.endsWith(mExt, Qt::CaseInsensitive)) {
            mBaseName = mBaseName.left(mBaseName.length() - mExt.length() - 1);
        }

        // TPlaylist::onNewMediaStartedPlaying set mFolder for disc
        mFolder = mPlaylist;
        mWZPlaylist = fi.fileName().compare(TConfig::WZPLAYLIST,
                                            caseSensitiveFileNames) == 0;
        if (mWZPlaylist) {
            mBaseName = fi.dir().dirName();
        }
    }

    // Icon
    if (mFolder) {
        if (mSymLink) {
            itemIcon = iconProvider.folderLinkIcon;
        } else {
            itemIcon = iconProvider.folderIcon;
        }
    } else if (mSymLink) {
        itemIcon = iconProvider.fileLinkIcon;
    } else {
        itemIcon = iconProvider.fileIcon;
    }
}

void TPlaylistWidgetItem::renameDir(const QString& dir, const QString& newDir) {

    if (mFilename.startsWith(dir)) {
        mFilename = newDir + mFilename.mid(dir.length());
    }

    for(int i = 0; i < childCount(); i++) {
        plChild(i)->renameDir(dir, newDir);
    }
}

bool TPlaylistWidgetItem::renameFile(const QString& newName) {

    // Fully qualify name and newName
    QString dir = QDir::toNativeSeparators(QFileInfo(mFilename).absolutePath());
    if (!dir.endsWith(QDir::separator())) {
        dir += QDir::separator();
    }
    QString name;
    if (mWZPlaylist) {
        name = dir + mBaseName;
    } else {
        name = mFilename;
    }

    QString newFullName;
    if (QFileInfo(newName).isAbsolute()) {
        newFullName = newName;
    } else {
        newFullName = dir + newName;
    }

    // Use compare instead of localized compare and case sensitive, to allow
    // changing case or encoding
    if (newFullName.compare(name, Qt::CaseSensitive) == 0) {
        return true;
    }

    // Stop player if item is playing
    bool restartPlayer = false;
    if (mState == PSTATE_LOADING || mState == PSTATE_PLAYING) {
        restartPlayer = true;
        player->saveRestartTime();
        WZDEBUG("Stopping player");
        player->stop();
    }

    // Rename file
    if (!QFile::rename(name, newFullName)) {
        WZERROR("Failed to rename '" + name + "' to '" + newFullName + "'");
        QMessageBox::warning(treeWidget(),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                            "Failed to rename '%1' to '%2'"
                            ).arg(name).arg(newFullName));
        // Don't restart a stopped player
        return false;
    }

    // Update tree view
    setFilename(newFullName, QFileInfo(newName).completeBaseName());
    if (mFolder) {
        renameDir(name + QDir::separator(), newFullName + QDir::separator());
    }
    plTreeWidget()->setModified(this);

    // Restart player
    if (restartPlayer) {
        QTimer::singleShot(0, plTreeWidget()->parent(), SLOT(play()));
    }

    WZINFO("Renamed '" + name + "' to '" + newFullName + "'");
    return true;
}

QString TPlaylistWidgetItem::editName() const {

    QString name = mBaseName;
    if (!mWZPlaylist && !mExt.isEmpty()) {
        name += "." + mExt;
    }
    return name;
}

bool TPlaylistWidgetItem::rename(const QString &newName) {
    WZDEBUG(newName);

    if (newName.isEmpty()) {
        return false;
    }

    QString name = editName();
    if (name == newName) {
        return true;
    }

    if (QFileInfo(mFilename).exists()) {
        return renameFile(newName);
    }

    if (mFolder) {
        // A folder needs to exist
        WZERROR("Folder '" + mFilename + "' not found");
        QMessageBox::warning(treeWidget(),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                            "Folder '%1' not found").arg(mFilename));

        return false;
    }

    if (plParent()->isPlaylist()) {
        // Set name and extension of this playlist item and protect name
        QFileInfo fi(newName);
        setName(fi.completeBaseName(), fi.suffix(), true);
        plTreeWidget()->setModified(this);
        return true;
    }

    // Cannot change name of non-existing item when parent not a playlist
    WZERROR("Parent '" + mFilename + "' is not a playlist");
    QMessageBox::warning(treeWidget(),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                        "Parent '%1' is not a playlist").arg(mFilename));
    return false;
}

bool TPlaylistWidgetItem::renameDroppedFile() {

    QString name;
    if (isWZPlaylist()) {
        name = QFileInfo(mFilename).absolutePath();
    } else {
        name = mFilename;
    }

    if (QFileInfo(name).exists()) {
        QString newName = plParent()->pathPlusSep() + editName();
        return renameFile(newName);
    }

    // URLs etc.
    return true;
}

void TPlaylistWidgetItem::setData(int column, int role, const QVariant& value) {

    QTreeWidgetItem::setData(column, role, value);
    if (role == Qt::EditRole) {
        if (column == COL_NAME) {
            if (!rename(value.toString())) {
                // Restore old name
                emitDataChanged();
            }
        }
    }
}

QVariant TPlaylistWidgetItem::data(int column, int role) const {

    if (role == Qt::DisplayRole) {
        if (column == COL_NAME) {
            if (mModified) {
                return QVariant(mBaseName + "*");
            }
            return QVariant(mBaseName);
        }
        if (column == COL_EXT) {
            QString ext = mExt;
            if (ext.length() > 8) {
                ext = ext.left(5) + "...";
            }
            return QVariant(ext);
        }
        if (column == COL_TIME) {
            QString s;
            if (mDuration > 0) {
                s = TWZTime::formatTime(qRound(mDuration));
            }
            return QVariant(s);
        }
        if (column == COL_ORDER) {
            return QVariant(mOrder);
        }
    } else if (role == Qt::EditRole) {
        if (column == COL_NAME) {
            return QVariant(editName());
        }
        if (column == COL_EXT) {
            return QVariant(mExt);
        }
    } else if (role == Qt::ToolTipRole) {
        if (column == COL_NAME) {
            if (mSymLink) {
                return QVariant(qApp->translate(
                    "Gui::Playlist::TPlaylistWidgetItem", "Links to %1")
                    .arg(mTarget));
            }
            return QVariant(mFilename);
        }
        if (column == COL_EXT) {
            if (mExt.length() > 8) {
                return QVariant(mExt);
            }
        }
    }
    return QTreeWidgetItem::data(column, role);
}

int TPlaylistWidgetItem::getLevel() const {

    if (plParent() == 0) {
        return ROOT_NODE_LEVEL;
    }
    return plParent()->getLevel() + 1;
}

void TPlaylistWidgetItem::setFilename(const QString& fileName,
                                      const QString& baseName) {
    mFilename = fileName;
    mBaseName = baseName;
    setFileInfo();
}

void TPlaylistWidgetItem::setName(const QString& baseName,
                                  const QString& ext,
                                  bool protectName) {
    mBaseName = baseName;
    mExt = ext;
    if (protectName) {
        mEdited = true;
    }
    setSzHint(getLevel());
}

void TPlaylistWidgetItem::setStateIcon() {

    switch (mState) {
        case PSTATE_STOPPED:
            if (mPlayed) {
                setIcon(COL_NAME, iconProvider.okIcon);
            } else {
                setIcon(COL_NAME, itemIcon);
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_NAME, iconProvider.loadingIcon);
            break;
        case PSTATE_PLAYING:
            setIcon(COL_NAME, iconProvider.playIcon);
            break;
        case PSTATE_FAILED:
            setIcon(COL_NAME, iconProvider.failedIcon);
            break;
    }
}

void TPlaylistWidgetItem::setState(TPlaylistWidgetItemState state) {
    WZDEBUG("'" + mFilename + "' to " + playlistItemState(state));

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper.getStamp();
        WZDEBUG(QString("stamped %1 on '%2'").arg(mPlayedTime).arg(mFilename));
    }
    mState = state;

    setStateIcon();
}

void TPlaylistWidgetItem::setPlayed(bool played) {

    mPlayed = played;
    if (mState == PSTATE_STOPPED) {
        if (mPlayed) {
            setIcon(COL_NAME, iconProvider.okIcon);
        } else {
            setIcon(COL_NAME, itemIcon);
        }
    }
}

void TPlaylistWidgetItem::setModified(bool modified,
                                      bool recurse,
                                      bool markParents) {
    WZDEBUG(QString("set modified to %1 for '%2'")
            .arg(modified).arg(mFilename));

    mModified = modified;

    if (recurse) {
        for(int c = 0; c < childCount(); c++) {
            TPlaylistWidgetItem* child = plChild(c);
            if (child->modified() != modified) {
                child->setModified(modified, recurse, false);
            }
        }
    }

    if (mModified && markParents && plParent() && !plParent()->modified()) {
        plParent()->setModified(true, false, true);
    }

    emitDataChanged();
}

QString TPlaylistWidgetItem::path() const {

    QFileInfo fi(mFilename);
    if (mPlaylist) {
        return fi.absolutePath();
    }
    return fi.absoluteFilePath();
}

QString TPlaylistWidgetItem::pathPlusSep() const {

    QString p = path();
    if (p.endsWith(QDir::separator())) {
        return p;
    }
    return p + QDir::separator();
}

QString TPlaylistWidgetItem::fname() const {

    QString fn = mFilename;
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        if (mWZPlaylist) {
            fn = fi.dir().dirName();
        } else {
            fn = fi.fileName();
        }
    }
    return fn;
}

TPlaylistWidget* TPlaylistWidgetItem::plTreeWidget() const {
    return static_cast<TPlaylistWidget*>(treeWidget());
}

bool TPlaylistWidgetItem::blacklisted(const QString& filename) const {
    return mBlacklist.contains(filename, caseSensitiveFileNames);
}

bool TPlaylistWidgetItem::whitelist(const QString& filename) {

    int i = mBlacklist.indexOf(QRegExp(filename,
                                       caseSensitiveFileNames,
                                       QRegExp::FixedString));
    if (i >= 0) {
        mBlacklist.removeAt(i);
        WZINFO("removed '" + filename + "' from blacklist");
        return true;
    }

    WZWARN("'" + filename + "' not found in blacklist");
    return false;
}

// static, return the size of the name column
QSize TPlaylistWidgetItem::sizeColumnName(int width,
                                          const QString& text,
                                          const QFontMetrics& fm,
                                          const QSize& iconSize,
                                          int level) {

    // TODO: get from style
    const int hMargin = 4; // horizontal margin
    const int vMargin = 2; // vertical margin
    const QSize minSize = iconSize;

    // Get availlable width for text
    int w = width - level * (iconSize.width() + hMargin) - 2 * hMargin;

    // Return minimal size if no space available
    if (w <= minSize.width()) {
        return minSize;
    }

    // Get bounding rect of text
    int maxHeight = 4 * fm.lineSpacing();
    QRect r = QRect(QPoint(), QSize(w, maxHeight));
    QRect br = fm.boundingRect(r, TEXT_ALIGN_NAME, text);

    // Pick up the height from the bounding rect
    int h = br.height() + 2 * vMargin;
    if (h < minSize.height()) {
        h = minSize.height();
    }

    // Return original width and new height needed by text
    return QSize(width, h);
}

void TPlaylistWidgetItem::setSzHint(int level) {

    if (parent()) {
        setSizeHint(COL_NAME,
                    sizeColumnName(gNameColumnWidth,
                                   text(COL_NAME),
                                   gNameFontMetrics,
                                   iconProvider.iconSize,
                                   level));
    }
}

bool TPlaylistWidgetItem::operator <(const QTreeWidgetItem& other) const {

    const TPlaylistWidgetItem* o = static_cast<const TPlaylistWidgetItem*>
                                   (&other);

    if (o == 0) {
        return false;
    }

    if (mFolder) {
        if (o->isFolder()) {
            return QString::localeAwareCompare(mFilename, o->filename()) < 0;
        }
        return false;
    }

    if (o->isFolder()) {
        return true;
    }

    if (parent() != o->parent()) {
        // Sort on path
        int i = QString::localeAwareCompare(
                    QFileInfo(mFilename).absolutePath(),
                    QFileInfo(o->filename()).absolutePath());
        if (i < 0) {
            return true;
        }
        if (i > 0) {
            return false;
        }
    }

    // Sort on section
    int section;
    if (treeWidget()) {
        section = treeWidget()->header()->sortIndicatorSection();
    } else {
        section = COL_ORDER;
    }
    switch (section) {
    case COL_NAME:
        return QString::localeAwareCompare(mBaseName, o->baseName()) < 0;
    case COL_EXT: return QString::localeAwareCompare(mExt, o->extension()) < 0;
    case COL_TIME: return mDuration < o->duration();
    default: return mOrder < o->order();
    }
}

int TPlaylistWidgetItem::compareFilename(const TPlaylistWidgetItem& item) const {
    return mFilename.compare(item.mFilename, caseSensitiveFileNames);
}

} // namespace Playlist
} // namespace Gui

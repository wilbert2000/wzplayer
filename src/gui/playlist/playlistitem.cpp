#include "gui/playlist/playlistitem.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/msg.h"
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
#include <QStack>
#include <QThread>


namespace Gui {
namespace Playlist {

LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistItem)

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
const int TPlaylistItem::ROOT_NODE_LEVEL = 1;
// Width of name column. Updated by TPlaylistWidget event handlers.
int TPlaylistItem::gNameColumnWidth = 0;
// Set by TPlaylistWidget constructor
QFontMetrics TPlaylistItem::gNameFontMetrics = QFontMetrics(QFont());


static int timeStamper = 0;

// Constructor used for root item
TPlaylistItem::TPlaylistItem() :
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
TPlaylistItem::TPlaylistItem(const TPlaylistItem& item) :
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
TPlaylistItem::TPlaylistItem(QTreeWidgetItem* parent,
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

TPlaylistItem *TPlaylistItem::clone() const {

    TPlaylistItem* root = 0;

    QStack<const TPlaylistItem*> stack;
    QStack<TPlaylistItem*> parentStack;
    stack.push(this);
    parentStack.push(0);

    while (!stack.isEmpty()) {
        // Get current item and its parent
        const TPlaylistItem* item = stack.pop();
        TPlaylistItem* parent = parentStack.pop();

        // Copy the item
        TPlaylistItem* copy = new TPlaylistItem(*item);

        // Remember root
        if (!root) {
            root = copy;
        }

        // Add copy to parent
        if (parent) {
            parent->insertChild(0, copy);
        }

        // Push the kids with copy as parent
        for (int i = 0; i < item->childCount(); ++i) {
            stack.push(item->plChild(i));
            parentStack.push(copy);
        }
    }

    return root;
}

TPlaylistItem::~TPlaylistItem() {
}

// Update fields depending on file name
void TPlaylistItem::setFileInfo() {

    // Root item uses empty filename
    if (mFilename.isEmpty()) {
        mBaseName = "";
        mExt = "";
        mFolder = true;
        mPlaylist = false;
        mWZPlaylist = false;
        mSymLink = false;
        mTarget = "";
    } else {
        QFileInfo fi(mFilename);
        mSymLink = fi.isSymLink();
        mTarget = fi.symLinkTarget();
        if (mSymLink) {
            fi.setFile(fi.symLinkTarget());
        }

        if (fi.isDir()) {
            mExt = "";
            mFolder = true;
            mPlaylist = false;
            mWZPlaylist = false;
        } else {
            mExt = fi.suffix().toLower();
            // Remove extension from base name
            if (!mExt.isEmpty() && mBaseName.endsWith(mExt, Qt::CaseInsensitive)) {
                mBaseName = mBaseName.left(mBaseName.length() - mExt.length() - 1);
            }

            // Note: Handle non-existing playlists as URL, to correctly handle
            // http://bladiblo/blob.m3u8 etc.
            mPlaylist = fi.exists() && extensions.playlists().contains(mExt);
            // TPlaylist::onNewMediaStartedPlaying sets mFolder for discs too
            mFolder = mPlaylist;
            mWZPlaylist = fi.fileName().compare(TConfig::WZPLAYLIST,
                                                caseSensitiveFileNames) == 0;
            if (mWZPlaylist) {
                mBaseName = fi.dir().dirName();
            }
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

QString TPlaylistItem::stateString() {

    switch (mState) {
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_LOADING: return "loading";
        case PSTATE_PLAYING: return "playing";
        case PSTATE_FAILED: ;
    }
    return "failed";
}

void TPlaylistItem::renameDir(const QString& dir, const QString& newDir) {

    if (mFilename.startsWith(dir)) {
        mFilename = newDir + mFilename.mid(dir.length());
    }

    for(int i = 0; i < childCount(); i++) {
        plChild(i)->renameDir(dir, newDir);
    }
}

void TPlaylistItem::updateFilename(const QString& source, const QString& dest) {

    if (mWZPlaylist) {
        setFilename(dest + QDir::separator() + TConfig::WZPLAYLIST);
    } else {
        setFilename(dest);
    }
    if (childCount()) {
        renameDir(source + QDir::separator(), dest + QDir::separator());
    }
}

bool TPlaylistItem::renameFile(const QString& newName) {

    QFileInfo fi(path());
    QString dir;
    dir = QDir::toNativeSeparators(fi.absolutePath());
    if (!dir.endsWith(QDir::separator())) {
        dir += QDir::separator();
    }

    QString source;
    if (mWZPlaylist) {
        source = dir + mBaseName;
    } else {
        source = mFilename;
    }
    QString displaySource = fi.fileName();

    QString dest = dir + newName;
    QString displayDest = newName;

    // Use compare instead of localized compare and case sensitive, to allow
    // changing case or encoding
    if (dest.compare(source, Qt::CaseSensitive) == 0) {
        return true;
    }

    // Stop player if item is playing
    bool restartPlayer = false;
    if (mState == PSTATE_LOADING || mState == PSTATE_PLAYING) {
        if (player->state() != Player::STATE_STOPPED) {
            WZDEBUG("Stopping player");
            restartPlayer = true;
            player->saveRestartState();
            player->stop();
        }
    }

    // Show message in statusbar
    msg(qApp->translate("Gui::Playlist::TPlaylistItem", "Renaming '%1' to '%2'")
        .arg(displaySource).arg(displayDest), 0);

    // Rename file
    QFile file(source);
    bool result = file.rename(dest);
    if (result) {
        updateFilename(source, dest);
        setModified();
        WZINFO("Renamed '" + source + "' to '" + dest + "'");
    } else {
        QString emsg = file.errorString();
        WZERROR("Failed to rename '" + source + "' to '" + dest + "'. " + emsg);
        QMessageBox::warning(treeWidget(),
            qApp->translate("Gui::Playlist::TPlaylistItem", "Error"),
            qApp->translate("Gui::Playlist::TPlaylistItem",
                            "Failed to rename '%1' to '%2'. %3")
                             .arg(source).arg(dest).arg(emsg));
    }

    // Restart the player
    if (restartPlayer) {
        QTimer::singleShot(0, plTreeWidget()->parent(), SLOT(play()));
    } else {
        msgClear();
    }

    return result;
}

QString TPlaylistItem::editName() const {

    QString name = mBaseName;
    if (!mWZPlaylist && !mExt.isEmpty()) {
        name += "." + mExt;
    }
    return name;
}

bool TPlaylistItem::rename(const QString &newName) {
    WZDEBUG(newName);

    if (newName.isEmpty()) {
        return false;
    }

    QString name = editName();
    if (name == newName) {
        return true;
    }

    if (QFileInfo(path()).exists()) {
        return renameFile(newName);
    }

    if (plParent()->isFolder()) {
        // Set name and extension of this playlist item and protect name
        QFileInfo fi(newName);
        setName(fi.completeBaseName(), fi.suffix(), true);
        setModified();
        return true;
    }

    // Cannot change name of non-existing item when parent not a playlist
    WZERROR(QString("Cannot rename '%1' when it is not in a playlist")
            .arg(mFilename));
    QMessageBox::warning(treeWidget(),
        qApp->translate("Gui::Playlist::TPlaylistItem", "Error"),
        qApp->translate("Gui::Playlist::TPlaylistItem",
                        "Cannot rename '%1' when it is not in a playlist")
                         .arg(name));
    return false;
}

void TPlaylistItem::setData(int column, int role, const QVariant& value) {

    QTreeWidgetItem::setData(column, role, value);
    if (role == Qt::EditRole && column == COL_NAME) {
        if (!rename(value.toString())) {
            // Restore old name
            emitDataChanged();
        }
    }
}

QVariant TPlaylistItem::data(int column, int role) const {

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
                return QVariant(qApp->translate("Gui::Playlist::TPlaylistItem",
                                                "Links to %1").arg(mTarget));
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

int TPlaylistItem::getLevel() const {

    if (plParent() == 0) {
        return ROOT_NODE_LEVEL;
    }
    return plParent()->getLevel() + 1;
}

void TPlaylistItem::setFilename(const QString& fileName) {
    mFilename = QDir::toNativeSeparators(fileName);
    mBaseName = QFileInfo(fileName).completeBaseName();
    setFileInfo();
}

void TPlaylistItem::setFilename(const QString& fileName,
                                      const QString& baseName) {
    mFilename = QDir::toNativeSeparators(fileName);
    mBaseName = baseName;
    setFileInfo();
}

void TPlaylistItem::setName(const QString& baseName,
                                  const QString& ext,
                                  bool protectName) {
    mBaseName = baseName;
    mExt = ext;
    if (protectName) {
        mEdited = true;
    }
    setSzHint(getLevel());
}

void TPlaylistItem::setStateIcon() {

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

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper++;
        WZDEBUG(QString("Set state to playing at %1 for '%2'")
                .arg(mPlayedTime).arg(mFilename));
    } else {
        WZDEBUG(QString("Setting state to %1 for '%2'")
                .arg(stateString()).arg(mFilename));
    }

    mState = state;
    setStateIcon();
}

void TPlaylistItem::setPlayed(bool played) {

    mPlayed = played;
    if (mState == PSTATE_STOPPED) {
        if (mPlayed) {
            setIcon(COL_NAME, iconProvider.okIcon);
        } else {
            setIcon(COL_NAME, itemIcon);
        }
    }
}

void TPlaylistItem::setModified(bool modified,
                                bool recurse,
                                bool markParents) {

    bool modifiedChanged = modified != mModified;
    mModified = modified;

    if (recurse) {
        for(int c = 0; c < childCount(); c++) {
            TPlaylistItem* child = plChild(c);
            if (child->modified() != modified) {
                child->setModified(modified, recurse, false);
            }
        }
    }

    if (mModified && markParents && plParent() && !plParent()->modified()) {
        plParent()->setModified(true, false, true);
    }

    if (modifiedChanged) {
        bool onMainThread = QThread::currentThread() == qApp->thread();
        // TODO: remove "while..."
        WZINFO(QString("Modified %1 for '%2'%3")
               .arg(modified ? "set" : "cleared")
               .arg(mFilename)
               .arg(onMainThread ? "" : " while not running on main thread"));
        if (onMainThread) {
            TPlaylistWidget* tree = plTreeWidget();
            if (tree) {
                emitDataChanged();
                if (plParent() == 0) {
                    tree->emitModifiedChanged();
                }
            }
        }
    }
}

QString TPlaylistItem::path() const {

    if (mWZPlaylist) {
        QFileInfo fi(mFilename);
        return fi.absolutePath();
    }
    return mFilename;
}

QString TPlaylistItem::playlistPath() const {

    QFileInfo fi(mFilename);
    if (mPlaylist) {
        return fi.absolutePath();
    }
    return fi.absoluteFilePath();
}

QString TPlaylistItem::playlistPathPlusSep() const {

    QString p = playlistPath();
    if (p.endsWith(QDir::separator())) {
        return p;
    }
    return p + QDir::separator();
}

QString TPlaylistItem::fname() const {

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

TPlaylistWidget* TPlaylistItem::plTreeWidget() const {
    return static_cast<TPlaylistWidget*>(treeWidget());
}

bool TPlaylistItem::blacklisted(const QString& filename) const {
    return mBlacklist.contains(filename, caseSensitiveFileNames);
}

bool TPlaylistItem::whitelist(const QString& filename) {

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
QSize TPlaylistItem::sizeColumnName(int width,
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

void TPlaylistItem::setSzHint(int level) {

    if (parent()) {
        setSizeHint(COL_NAME,
                    sizeColumnName(gNameColumnWidth,
                                   text(COL_NAME),
                                   gNameFontMetrics,
                                   iconProvider.iconSize,
                                   level));
    }
}

bool TPlaylistItem::operator <(const QTreeWidgetItem& other) const {

    const TPlaylistItem* o = static_cast<const TPlaylistItem*>(&other);
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

int TPlaylistItem::compareFilename(const TPlaylistItem& item) const {
    return mFilename.compare(item.mFilename, caseSensitiveFileNames);
}

} // namespace Playlist
} // namespace Gui

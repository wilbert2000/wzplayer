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

static int timeStamper = 0;

const int USER_TYPE = QTreeWidgetItem::UserType + 1;

// Constructor used for root item
TPlaylistItem::TPlaylistItem() :
    QTreeWidgetItem(USER_TYPE),
    mDuration(0),
    mOrder(1),
    mFolder(true),
    mPlaylist(false),
    mWZPlaylist(false),
    mSymLink(false),
    mURL(false),
    mEditURL(false),
    mState(PSTATE_STOPPED),
    mPlayed(false),
    mEdited(false),
    mModified(false),
    mPlayedTime(0) {

    setFlags(ROOT_FLAGS);
    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_EXT, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_LENGTH, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);

    setIcon(COL_NAME, iconProvider.folderIcon);
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
    mURL(item.isUrl()),
    mEditURL(item.editURL()),

    mState(item.state()),
    mPlayed(item.played()),
    mEdited(item.edited()),
    mModified(item.modified()),
    mPlayedTime(item.playedTime()),
    mBlacklist(item.getBlacklist()),

    itemIcon(item.itemIcon) {

    // TODO: Copy constructor QTreeWidgetItem does not copy type()
    // Post "upgrade" action?
    WZWARN(QString("Copy constructor called on '%1'").arg(mFilename));
}

// Used for every item except the root
TPlaylistItem::TPlaylistItem(QTreeWidgetItem* parent,
                             const QString& filename,
                             const QString& name,
                             double duration,
                             bool protectName) :
    QTreeWidgetItem(parent, USER_TYPE),
    mFilename(QDir::toNativeSeparators(filename)),
    mBaseName(name),
    mDuration(duration),
    mEditURL(false),
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
                          | Qt::ItemIsDragEnabled
                          | Qt::ItemIsEditable;

    if (mFolder) {
        setFlags(flags | Qt::ItemIsDropEnabled);
    } else {
        setFlags(flags);
    }

    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_EXT, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_LENGTH, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);

    setIcon(COL_NAME, itemIcon);

    if (treeWidget()) {
        setSizeHintName();
    }
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
        // TODO: copy type
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

void TPlaylistItem::setItemIcon() {

    bool addOnIcon;
    if (mFolder) {
        addOnIcon = true;
        if (mSymLink) {
            itemIcon = iconProvider.folderLinkIcon;
        } else {
            itemIcon = iconProvider.folderIcon;
        }
    } else {
        addOnIcon = false;
        if (mSymLink) {
            itemIcon = iconProvider.fileLinkIcon;
        } else if (mURL) {
            itemIcon = iconProvider.urlIcon;
        } else {
            itemIcon = iconProvider.fileIcon;
        }
    }

    if (mEdited) {
        itemIcon = iconProvider.getIconEdited(itemIcon, addOnIcon);
    }
    if (mBlacklist.count()) {
        itemIcon = iconProvider.getIconBlacklist(itemIcon);
    }
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
        mURL = false;
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
            mURL = false;
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
                fi.setFile(fi.absolutePath());
                if (fi.isSymLink()) {
                    mSymLink = true;
                    mTarget = fi.symLinkTarget();
                }
                mURL = false;
            } else if (fi.exists()) {
                mURL = false;
            } else {
                QUrl url(mFilename);
                mURL = url.isValid() && !url.scheme().isEmpty();
            }
        }
    }

    setItemIcon();
}

QString TPlaylistItem::stateString(TPlaylistItemState state) {

    switch (state) {
        case PSTATE_STOPPED: return "stopped";
        case PSTATE_LOADING: return "loading";
        case PSTATE_PLAYING: return "playing";
        case PSTATE_FAILED: ;
    }
    return "failed";
}

QString TPlaylistItem::stateString() const {
    return stateString(mState);
}

QString TPlaylistItem::tr(const char* s) {
    return qApp->translate("Gui::Playlist::TPlaylistItem", s);
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
        QString d = dest + QDir::separator() + TConfig::WZPLAYLIST;
        WZTRACE(QString("Setting '%1' to '%2'").arg(source).arg(d));
        setFilename(d);
    } else {
        WZTRACE(QString("Setting '%1' to '%2'").arg(source).arg(dest));
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

    bool pathChanged = false;
    QString dest;
    QString displayDest;
    if (mEditURL) {
        QFileInfo fid(newName);
        dest = QDir::toNativeSeparators(fid.absoluteFilePath());
        displayDest = fid.fileName();
        pathChanged = fid.dir() != fi.dir();
        if (pathChanged) {
            if (QMessageBox::question(treeWidget(), tr("Confirm move"),
                    tr("Are you sure you want to move '%1' to '%2'?")
                    .arg(source).arg(dest),
                    QMessageBox::Yes,
                    QMessageBox::No | QMessageBox::Default
                    | QMessageBox::Escape)
                    != QMessageBox::Yes) {
                return false;
            }
        }
    } else {
        dest = dir + newName;
        displayDest = newName;
    }

    // Use compare instead of localized compare and case sensitive to allow
    // changing case.
    if (source.compare(dest, Qt::CaseSensitive) == 0) {
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
    msg(tr("Renaming '%1' to '%2'").arg(displaySource).arg(displayDest), 0);

    // Rename file
    QFile file(source);
    bool result = file.rename(dest);
    if (result) {
        updateFilename(source, dest);
        setModified();
        WZINFO("Renamed '" + source + "' to '" + dest + "'");
        if (pathChanged) {
            plTreeWidget()->itemToUpdatePath = this;
            QTimer::singleShot(0, plTreeWidget(),
                               &TPlaylistWidget::updateItemPath);
        }
    } else {
        QString emsg = file.errorString();
        WZERROR("Failed to rename '" + source + "' to '" + dest + "'. " + emsg);
        QMessageBox::warning(treeWidget(), tr("Error"),
                             tr("Failed to rename '%1' to '%2'. %3")
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

bool TPlaylistItem::rename(QString newName) {
    WZDEBUG(newName);

    if (newName.isEmpty()) {
        if (mEditURL || !mEdited) {
            WZDEBUG("Canceling empty filename");
            return false;
        }

        // Reset name to original name
        QFileInfo fi(mFilename);
        if (mWZPlaylist) {
            fi.setFile(fi.absolutePath());
        }
        newName = fi.fileName();
        mEdited = false;
        WZDEBUG(QString("Name reset to '%1'").arg(newName));
    }

    QString name = mEditURL ? mFilename : editName();
    if (name == newName) {
        WZTRACE("New name equals old name");
        return true;
    }

    if (!isLink()) {
        return renameFile(newName);
    }

    if (mEditURL) {
        WZINFO(QString("Setting URL link to '%1'").arg(newName));
        setFilename(newName);
        setModified();
        if (!mURL && !QFileInfo(newName).exists()) {
            QMessageBox::warning(treeWidget(), tr("File not found"),
                tr("Filename set to '%1', but it does not seem to exist.")
                .arg(newName));
        }
        return true;
    }

    // Set name and extension of this playlist item and protect name
    WZDEBUG(QString("Setting name link to '%1'").arg(newName));
    QFileInfo fi(newName);
    newName = fi.completeBaseName();
    setName(newName, fi.suffix(), newName != TName::baseNameForURL(mFilename));
    setModified();
    return true;
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
        if (column == COL_LENGTH) {
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
            if (mEditURL) {
                return QVariant(mFilename);
            }
            return QVariant(editName());
        }
        if (column == COL_EXT) {
            return QVariant(mExt);
        }
    } else if (role == Qt::ToolTipRole) {
        if (column == COL_NAME) {
            QString tip;
            if (mSymLink) {
                tip = tr("Links to %1").arg(mTarget);
            } else {
                tip = mFilename;
            }
            for(int i = 0; i < mBlacklist.count(); i++) {
                if (i == 0) {
                    tip += "\n" + tr("Blacklisted: ");
                }
                tip += "\n" + mBlacklist.at(i);
            }
            if (mEdited) {
                tip += "\n" + tr("Name has been edited");
            }
            return QVariant(tip);
        }
        if (column == COL_EXT) {
            if (mExt.length() > 8) {
                return QVariant(mExt);
            }
        }
    }
    return QTreeWidgetItem::data(column, role);
}


}

void TPlaylistItem::setFilename(const QString& fileName,
                                const QString& baseName) {

    mFilename = QDir::toNativeSeparators(fileName);
    mBaseName = baseName;
    setFileInfo();
}

void TPlaylistItem::setFilename(const QString& fileName) {

    mFilename = QDir::toNativeSeparators(fileName);
    if (mFilename.isEmpty()) {
        mBaseName = "";
    } else if (!mEdited) {
        mBaseName = TName::nameForURL(mFilename);
    }
    setFileInfo();
}

void TPlaylistItem::setName(const QString& baseName,
                            const QString& ext,
                            bool protectName) {

    mBaseName = baseName;
    mExt = ext.toLower();
    mEdited = protectName;
    setItemIcon();
    setStateIcon();
    setSizeHintName();

}

void TPlaylistItem::setStateIcon() {

    switch (mState) {
        case PSTATE_STOPPED:
            if (mPlayed) {
                setIcon(COL_NAME, iconProvider.iconPlayed);
            } else {
                setIcon(COL_NAME, itemIcon);
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_NAME, iconProvider.iconLoading);
            break;
        case PSTATE_PLAYING:
            setIcon(COL_NAME, iconProvider.iconPlaying);
            break;
        case PSTATE_FAILED:
            setIcon(COL_NAME, iconProvider.iconFailed);
            break;
    }
}

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        mPlayed = true;
        mPlayedTime = timeStamper++;
    }
    WZTRACE(QString("Changing state from %1 to %2 for '%3'")
            .arg(stateString(mState)).arg(stateString(state)).arg(mBaseName));

    mState = state;
    setStateIcon();
}

void TPlaylistItem::setPlayed(bool played) {

    mPlayed = played;
    if (mState == PSTATE_STOPPED) {
        if (mPlayed) {
            setIcon(COL_NAME, iconProvider.iconPlayed);
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

    TPlaylistItem* parent = plParent();
    if (mModified && markParents && parent && !parent->modified()) {
        parent->setModified(true, false, true);
    }

    if (modifiedChanged) {
        WZINFO(QString("Modified %1 for '%2'")
               .arg(modified ? "set" : "cleared").arg(mFilename));
        TPlaylistWidget* tree = plTreeWidget();
        if (tree) {
            setSizeHintName();
            emitDataChanged();
            if (parent == 0) {
                tree->emitModifiedChanged();
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

bool TPlaylistItem::isLink() const {

    if (mURL) {
        return true;
    }
    if (mFilename.isEmpty()) {
        WZWARN("isLink() called on empty filename");
        return true;
    }

    QFileInfo fi(mFilename);
    if (mWZPlaylist) {
        fi.setFile(fi.absolutePath());
    }
    if (!fi.exists()) {
        return true;
    }

    TPlaylistItem* parent = plParent();
    QFileInfo fip(parent->filename());
    if (parent->isPlaylist()) {
        if (parent->isWZPlaylist()) {
            return fip.dir() != fi.dir();
        }
        return true;
    }
    if (fip.isDir()) {
        return QDir(fip.absoluteFilePath()) != fi.dir();
    }

    WZINFO(QString("Marking '%1' as link").arg(mFilename));
    return true;
}

TPlaylistWidget* TPlaylistItem::plTreeWidget() const {
    return static_cast<TPlaylistWidget*>(treeWidget());
}

void TPlaylistItem::updateIcon() {

    setItemIcon();
    setStateIcon();
}

void TPlaylistItem::blacklist(const QString& filename) {
    WZINFO(QString("Blacklisting '%1' in '%2'")
           .arg(filename).arg(mFilename));

    mBlacklist.append(filename);
    if (mBlacklist.count() == 1) {
        updateIcon();
    }
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
        if (mBlacklist.count() == 0) {
            updateIcon();
        }
        WZINFO("Removed '" + filename + "' from blacklist");
        return true;
    }

    WZWARN("'" + filename + "' not found in blacklist");
    return false;
}

int TPlaylistItem::getLevel() const {

    if (plParent() == 0) {
        return 0;
    }
    return plParent()->getLevel() + 1;
}

// Initialised in constructor TPlaylistWidget by calling setSpacing() on root
int TPlaylistItem::hSpacing = 28;
int TPlaylistItem::vSpacing = 6;
int TPlaylistItem::bounding = 2;

void TPlaylistItem::setSpacing() {

    QString txt = "Hello";
    setText(COL_NAME, txt);
    QStyleOptionViewItem opt;
    opt.features = QStyleOptionViewItem::HasDisplay
            | QStyleOptionViewItem::HasDecoration;
    opt.text = txt;
    opt.font = font(COL_NAME);
    opt.fontMetrics = QFontMetrics(opt.font);
    opt.icon = icon(COL_NAME);
    opt.decorationSize = treeWidget()->iconSize();
    opt.index = treeWidget()->model()->index(0, 0);
    QAbstractItemDelegate* del = treeWidget()->itemDelegate();
    QSize size = del->sizeHint(opt, opt.index);

    QRect r(QPoint(), QSize(512, 512));
    QRect br = opt.fontMetrics.boundingRect(r, TEXT_ALIGN_NAME, txt);

    int ffm = treeWidget()->style()->pixelMetric(
                QStyle::PM_FocusFrameHMargin, 0, treeWidget());
    // Note: QItemDelegate adds 2 * (QStyle::PM_FocusFrameHMargin + 1) to
    // the width of the text
    hSpacing = size.width() - br.width() - 2 * (ffm + 1);
    vSpacing = size.height() - br.height() - ffm;
    bounding = br.height() - opt.fontMetrics.lineSpacing();

    setText(COL_NAME, "");
}

// Return the size of the name column given the available width
QSize TPlaylistItem::getSizeColumnName(int width,
                                       const QString& text,
                                       const QFontMetrics& fm) {

    // Return minimal size if no space available
    if (width <= hSpacing) {
        return QSize(hSpacing, iconProvider.actualIconSize.height() + vSpacing);
    }

    // Substract spacing from availlable width for text
    int maxWidth = width - hSpacing;
    // Allow 4 lines of text
    int maxHeight = 4 * fm.lineSpacing() + bounding;

    // Get bounding rect of text
    QRect r = QRect(QPoint(), QSize(maxWidth, maxHeight));
    QRect br = fm.boundingRect(r, TEXT_ALIGN_NAME, text);

    if (br.height() > maxHeight) {
       br.setHeight(maxHeight);
    }

    return QSize(width, br.height() + vSpacing);
}

QSize TPlaylistItem::getSizeHintName(int level) const {

    QTreeWidget* tree = treeWidget();
    if (!tree) {
        return QSize(-1, -1);
    }

    return getSizeColumnName(tree->header()->sectionSize(COL_NAME)
                             - level * tree->indentation(),
                             text(COL_NAME),
                             QFontMetrics(font(COL_NAME)));
}

void TPlaylistItem::setSizeHintName(int level) {
    setSizeHint(COL_NAME, getSizeHintName(level));
}

void TPlaylistItem::setSizeHintName() {
    setSizeHintName(getLevel());
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
    case COL_LENGTH: return mDuration < o->duration();
    default: return mOrder < o->order();
    }
}

int TPlaylistItem::compareFilename(const TPlaylistItem& item) const {
    return mFilename.compare(item.mFilename, caseSensitiveFileNames);
}

} // namespace Playlist
} // namespace Gui

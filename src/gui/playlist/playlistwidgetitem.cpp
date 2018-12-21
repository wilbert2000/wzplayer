#include "gui/playlist/playlistwidgetitem.h"
#include "gui/playlist/playlistwidget.h"
#include "player/player.h"
#include "wzdebug.h"
#include "iconprovider.h"
#include "wztime.h"
#include "config.h"

#include <QApplication>
#include <QDir>
#include <QHeaderView>
#include <QMessageBox>


namespace Gui {
namespace Playlist {


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistWidgetItem)

// Text alignment for columns
const int TEXT_ALIGN_NAME = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;
const int TEXT_ALIGN_TYPE = Qt::AlignLeft | Qt::AlignVCenter;
const int TEXT_ALIGN_TIME = Qt::AlignRight | Qt::AlignVCenter;
const int TEXT_ALIGN_ORDER = Qt::AlignRight | Qt::AlignVCenter;

// Level of the root node in the tree view, where level means the number of
// icons indenting the item. With root decoration on, toplevel items appear on
// level 2, being ROOT_NODE_LEVEL + 1.
const int ROOT_NODE_LEVEL = 1;

// Minimum name column width
const int MIN_NAME_COL_WIDTH = 32;

// Updated by TPlaylistWidget event handlers
int gNameColumnWidth = MIN_NAME_COL_WIDTH;

// Set by TPlaylistWidget constructor
QFontMetrics gNameFontMetrics = QFontMetrics(QFont());


// Constructor used for root item
TPlaylistWidgetItem::TPlaylistWidgetItem() :
    QTreeWidgetItem(),
    mModified(false) {

    playlistItem.setFolder(true);
    setFlags(ROOT_FLAGS);
    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_TYPE, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_TIME, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);
}

// Used for every item except the root
TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool protectName) :
    QTreeWidgetItem(parent),
    playlistItem(QDir::toNativeSeparators(filename),
                 name,
                 duration,
                 protectName),
    mModified(false) {

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsEnabled
                          | Qt::ItemIsDragEnabled;

    if (isFolder()) {
        setFlags(flags | Qt::ItemIsDropEnabled);
    } else {
        setFlags(flags);
    }

    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setTextAlignment(COL_TYPE, TEXT_ALIGN_TYPE);
    setTextAlignment(COL_TIME, TEXT_ALIGN_TIME);
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

void TPlaylistWidgetItem::refresh(const QString& dir, const QString& newDir) {

    {
        QString fn = filename();
        if (fn.startsWith(dir)) {
            playlistItem.setFName(newDir + fn.mid(dir.length()));
        }
    }

    for(int i = 0; i < childCount(); i++) {
        plChild(i)->refresh(dir, newDir);
    }
}

bool TPlaylistWidgetItem::renameFile(const QString& newName) {

    // Stop player if item is playing
    if (state() == PSTATE_LOADING || state() == PSTATE_PLAYING) {
        WZDEBUG("Stopping playing item");
        player->stop();
    }

    QString dir = QDir::toNativeSeparators(QFileInfo(filename()).absolutePath());
    if (!dir.endsWith(QDir::separator())) {
        dir += QDir::separator();
    }
    QString name;
    if (isWZPlaylist()) {
        name = dir + baseName();
    } else {
        name = filename();
    }
    QString nn;
    if (newName.startsWith('/')) {
        nn = newName;
    } else {
        nn = dir + newName;
    }

    if (QFile::rename(name, nn)) {
        setFilename(nn, QFileInfo(newName).completeBaseName());
        if (isFolder()) {
            refresh(name + QDir::separator(), nn + QDir::separator());
        }
        plTreeWidget()->setModified(this);
        WZINFO("Renamed '" + name + "' to '" + nn + "'");
        return true;
    }

    WZERROR("Failed to rename '" + name + "' to '" + nn + "'");
    QMessageBox::warning(treeWidget(),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                        "Failed to rename '%1' to '%2'").arg(name).arg(nn));
    return false;
}

bool TPlaylistWidgetItem::rename(const QString &newName) {
    WZDEBUG(newName);

    if (newName.isEmpty()) {
        return false;
    }

    QString name = playlistItem.editName();
    if (name == newName) {
        return true;
    }

    if (QFileInfo(filename()).exists()) {
        return renameFile(newName);
    }

    if (isFolder()) {
        // A folder needs to exist
        WZERROR("Folder '" + filename() + "' not found");
        QMessageBox::warning(treeWidget(),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
            qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                            "Folder '%1' not found").arg(filename()));

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
    WZERROR("Parent '" + filename() + "' is not a playlist");
    QMessageBox::warning(treeWidget(),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem", "Error"),
        qApp->translate("Gui::Playlist::TPlaylistWidgetItem",
                        "Parent '%1' is not a playlist").arg(filename()));
    return false;
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
                return QVariant(baseName() + "*");
            }
            return QVariant(baseName());
        }
        if (column == COL_TYPE) {
            QString ext = extension();
            if (ext.length() > 8) {
                ext = ext.left(5) + "...";
            }
            return QVariant(ext);
        }
        if (column == COL_TIME) {
            QString s;
            double d = playlistItem.duration();
            if (d > 0) {
                s = TWZTime::formatTime(qRound(d));
            }
            return QVariant(s);
        }
        if (column == COL_ORDER) {
             return QVariant(QString::number(order()));
        }
    } else if (role == Qt::EditRole) {
        if (column == COL_NAME) {
            return QVariant(playlistItem.editName());
        }
        if (column == COL_TYPE) {
            return QVariant(extension());
        }
    } else if (role == Qt::ToolTipRole) {
        if (column == COL_NAME) {
            if (isSymLink()) {
                return QVariant(qApp->translate(
                    "Gui::Playlist::TPlaylistWidgetItem", "Links to %1")
                    .arg(target()));
            }
            return QVariant(filename());
        }
        if (column == COL_TYPE) {
            QString ext = extension();
            if (ext.length() > 8) {
                return QVariant(ext);
            }
        }
    }
    return QTreeWidgetItem::data(column, role);
}

bool TPlaylistWidgetItem::isRoot() const {
    return parent() == 0;
}

int TPlaylistWidgetItem::getLevel() const {

    if (plParent() == 0) {
        return ROOT_NODE_LEVEL;
    }
    return plParent()->getLevel() + 1;
}

void TPlaylistWidgetItem::setFilename(const QString& fileName,
                                      const QString& baseName) {
    playlistItem.setFilename(fileName, baseName);
}

void TPlaylistWidgetItem::setName(const QString& baseName,
                                  const QString& ext,
                                  bool protectName) {

    playlistItem.setBaseName(baseName, protectName);
    playlistItem.setExtension(ext);
    setSzHint(getLevel());
}

QIcon TPlaylistWidgetItem::getIcon() {

    if (itemIcon.isNull()) {
        itemIcon = iconProvider.icon(filename());
    }
    return itemIcon;
}

void TPlaylistWidgetItem::loadIcon() {

    if (icon(COL_NAME).isNull()) {
        setStateIcon();
    }
}

void TPlaylistWidgetItem::setStateIcon() {

    switch (state()) {
        case PSTATE_STOPPED:
            if (playlistItem.played()) {
                setIcon(COL_NAME, iconProvider.okIcon);
            } else {
                setIcon(COL_NAME, getIcon());
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

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    WZDEBUG("'" + filename() + "' to "
            + TPlaylistItem::playlistItemState(state));

    playlistItem.setState(state);
    setStateIcon();
}

void TPlaylistWidgetItem::setDuration(double d) {
    playlistItem.setDuration(d);
}

void TPlaylistWidgetItem::setOrder(int order) {
    playlistItem.setOrder(order);
}

void TPlaylistWidgetItem::setPlayed(bool played) {

    playlistItem.setPlayed(played);
    if (playlistItem.state() == PSTATE_STOPPED) {
        if (played) {
            setIcon(COL_NAME, iconProvider.okIcon);
        } else {
            setIcon(COL_NAME, getIcon());
        }
    }
}

void TPlaylistWidgetItem::setModified(bool modified,
                                      bool recurse,
                                      bool markParents) {
    WZDEBUG(QString("set modified to %1 for '%2'")
            .arg(modified).arg(filename()));

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

    QFileInfo fi(filename());
    if (isPlaylist()) {
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

    QString fn = filename();
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        if (isWZPlaylist()) {
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

bool TPlaylistWidgetItem::whitelist(const QString& filename) {
    return playlistItem.whitelist(filename);
}

// static, return size of the name column
QSize TPlaylistWidgetItem::sizeColumnName(const QString& text,
                                          int width,
                                          const QFontMetrics& fm,
                                          const QSize& iconSize,
                                          int level) {

    // TODO: get from where?
    const int hMargin = 4; // horizontal margin
    const int vMargin = 2; // vertical margin
    const int minHeight = iconSize.height(); // Minimal height

    // Get availlable width for text
    int w = width - level * (iconSize.width() + hMargin) - 2 * hMargin;

    // Return minimal size if no space available
    if (w <= MIN_NAME_COL_WIDTH) {
        return QSize(MIN_NAME_COL_WIDTH, minHeight);
    }

    // Get bounding rect of text
    int maxHeight = 4 * fm.lineSpacing();
    QRect r = QRect(QPoint(), QSize(w, maxHeight));
    QRect br = fm.boundingRect(r, TEXT_ALIGN_NAME, text);

    // Pick up height from bounding rect
    int h = br.height() + 2 * vMargin;
    if (h < minHeight) {
        h = minHeight;
    }

    return QSize(width, h);
};

void TPlaylistWidgetItem::setSzHint(int level) {

    if (parent()) {
        setSizeHint(COL_NAME,
                    sizeColumnName(text(COL_NAME),
                                   gNameColumnWidth,
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

    if (isFolder()) {
        if (o->isFolder()) {
            return QString::localeAwareCompare(filename(), o->filename()) < 0;
        }
        return false;
    }

    if (o->isFolder()) {
        return true;
    }

    // Sort on path
    int i = QString::localeAwareCompare(QFileInfo(filename()).absolutePath(),
                                        QFileInfo(o->filename()).absolutePath());
    if (i < 0) {
        return true;
    }
    if (i > 0) {
        return false;
    }

    int section;
    QTreeWidget* tree = treeWidget();
    if (tree) {
        section = tree->header()->sortIndicatorSection();
    } else {
        section = COL_ORDER;
    }

    if (section == COL_NAME) {
        return QString::localeAwareCompare(baseName(), o->baseName()) < 0;
    }
    if (section == COL_TYPE) {
        return QString::localeAwareCompare(extension(), o->extension()) < 0;
    }
    if (section == COL_TIME) {
        return duration() < o->duration();
    }

    return order() < o->order();
}

} // namespace Playlist
} // namespace Gui

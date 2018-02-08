#include "gui/playlist/playlistwidgetitem.h"

#include <QApplication>
#include <QDir>
#include <QHeaderView>

#include "wzdebug.h"
#include "iconprovider.h"
#include "wztime.h"
#include "config.h"


namespace Gui {
namespace Playlist {


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistWidgetItem)

// Alignment text fields
const int TEXT_ALIGN_NAME = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;
const int TEXT_ALIGN_TYPE = Qt::AlignLeft | Qt::AlignVCenter;
const int TEXT_ALIGN_TIME = Qt::AlignRight | Qt::AlignVCenter;
const int TEXT_ALIGN_ORDER = Qt::AlignRight | Qt::AlignVCenter;

// Level of the root node in the tree view, where level means the number of
// icons indenting the item. With root decoration on, toplevel items appear on
// level 2, being gRootNodeLevel + 1.
int gRootNodeLevel = 1;
// Set by TPlaylistWidget event handlers
int gNameColumnWidth = 0;
// Set by TPlaylistWidget constructor
QFontMetrics gNameFontMetrics = QFontMetrics(QFont());


// Used as root
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
        // TODO: setFlags(flags | Qt::ItemIsEditable);
        setFlags(flags);
    }

    setTextAlignment(COL_NAME, TEXT_ALIGN_NAME);
    setNameText(false);
    if (isSymLink()) {
        setToolTip(COL_NAME, qApp->translate(
            "Gui::Playlist::TPlaylistWidgetItem", "Links to %1").arg(target()));
    } else {
        setToolTip(COL_NAME, this->filename());
    }

    setTextAlignment(COL_TYPE, TEXT_ALIGN_TYPE);
    setExtensionText();
    setTextAlignment(COL_TIME, TEXT_ALIGN_TIME);
    setDurationText();
    setTextAlignment(COL_ORDER, TEXT_ALIGN_ORDER);
    setOrderText();
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

bool TPlaylistWidgetItem::isRoot() const {
    return parent() == 0;
}

int TPlaylistWidgetItem::getLevel() const {

    if (plParent() == 0) {
        return gRootNodeLevel;
    }
    return plParent()->getLevel() + 1;
}

void TPlaylistWidgetItem::setFilename(const QString& fileName,
                                      const QString& baseName) {
    playlistItem.setFilename(fileName, baseName);
}

void TPlaylistWidgetItem::setNameText(bool setSizeHint) {

    QString n = baseName();
    if (mModified) {
        n += "*";
    }
    setText(COL_NAME, n);
    if (setSizeHint) {
        setSzHint(getLevel());
    }
}

void TPlaylistWidgetItem::setExtensionText() {

    QString ext = extension();
    if (ext.length() > 8) {
        ext = ext.left(5) + "...";
    }
    setText(COL_TYPE, ext);
}

void TPlaylistWidgetItem::setName(const QString& baseName,
                                  const QString& ext,
                                  bool protectName,
                                  bool setSizeHint) {

    playlistItem.setBaseName(baseName, protectName);
    playlistItem.setExtension(ext);
    setNameText(setSizeHint);
    setExtensionText();
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

void TPlaylistWidgetItem::setDurationText() {

    QString s;
    double d = playlistItem.duration();
    if (d > 0) {
        s = TWZTime::formatTime(qRound(d));
    }
    setText(COL_TIME, s);
}

void TPlaylistWidgetItem::setDuration(double d) {

    playlistItem.setDuration(d);
    setDurationText();
}

void TPlaylistWidgetItem::setOrderText() {

    setText(COL_ORDER, QString::number(order()));
}

void TPlaylistWidgetItem::setOrder(int order) {

    playlistItem.setOrder(order);
    setOrderText();
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
    setNameText(true);

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

bool TPlaylistWidgetItem::whitelist(const QString& filename) {
    return playlistItem.whitelist(filename);
}

// static
QSize TPlaylistWidgetItem::itemSize(const QString& text,
                                    int width,
                                    const QFontMetrics& fm,
                                    const QSize& iconSize,
                                    int level) {

    // TODO: get from where?
    const int hm = 4;
    const int vm = 1;

    int w = width - level * (iconSize.width() + hm) - 2 * hm;
    if (w <= 32) {
        return QSize(width, iconSize.height());
    }

    int maxh = 4 * fm.lineSpacing();
    QRect r = QRect(QPoint(), QSize(w, maxh));
    QRect br = fm.boundingRect(r, TEXT_ALIGN_NAME, text);

    int h = br.height() + 2 * vm;
    if (h < iconSize.height()) {
        h = iconSize.height();
    }

    return QSize(width, h);
};

void TPlaylistWidgetItem::setSzHint(int level) {

    if (parent()) {
        //QSize iconSize = icon(COL_NAME).actualSize(QSize(22, 22));
        setSizeHint(COL_NAME, itemSize(text(COL_NAME),
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

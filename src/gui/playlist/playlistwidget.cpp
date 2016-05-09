#include "gui/playlist/playlistwidget.h"

#include <QDebug>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QDropEvent>
#include <QFontMetrics>
#include <QTimer>

#include "gui/playlist/playlistwidgetitem.h"
#include "images.h"
#include "iconprovider.h"

namespace Gui {
namespace Playlist {


class TWordWrapItemDelegate : public QStyledItemDelegate {
public:
    TWordWrapItemDelegate(TPlaylistWidget* parent) :
        QStyledItemDelegate(parent),
        header(parent->header()) {
    }

    virtual ~TWordWrapItemDelegate() {
    }

    static int getLevel(const QModelIndex& index) {

        if (index.parent() == QModelIndex()) {
            return gRootNodeLevel + 1;
        }
        return getLevel(index.parent()) + 1;
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {

        if (index.column() != TPlaylistWidgetItem::COL_NAME) {
            return QStyledItemDelegate::sizeHint(option, index);
        }
        QString text = index.model()->data(index).toString();
        if (text.isEmpty()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        return TPlaylistWidgetItem::itemSize(
                    text,
                    header->sectionSize(TPlaylistWidgetItem::COL_NAME),
                    option.fontMetrics,
                    option.decorationSize,
                    getLevel(index));
    };

private:
    QHeaderView* header;
};


TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    playing_item(0) {

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAutoExpandDelay(750);

    //setRootIsDecorated(false);
    setColumnCount(TPlaylistWidgetItem::COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Length"));
    header()->setStretchLastSection(false);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_NAME,
                                   QHeaderView::Stretch);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_TIME,
                                   QHeaderView::ResizeToContents);
#else
    header()->setResizeMode(TPlaylistWidgetItem::COL_NAME,
                            QHeaderView::Stretch);
    header()->setResizeMode(TPlaylistWidgetItem::COL_TIME,
                            QHeaderView::ResizeToContents);
#endif

    // Icons
    gIconSize = iconProvider.folderIcon.actualSize(QSize(22, 22));
    setIconSize(gIconSize);

    okIcon = Images::icon("ok", gIconSize.width());
    loadingIcon = Images::icon("loading", gIconSize.width());
    playIcon = Images::icon("play", gIconSize.width());
    failedIcon = Images::icon("failed", gIconSize.width());

    // Sort
    enableSort(false);
    connect(header(), SIGNAL(sectionClicked(int)),
            this, SLOT(onSectionClicked(int)));

    // Drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    //setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);

    // Wordwrap
    gNameFontMetrics = fontMetrics();

    setWordWrap(true);
    setUniformRowHeights(false);
    setItemDelegate(new TWordWrapItemDelegate(this));
    wordWrapTimer = new QTimer();
    wordWrapTimer->setInterval(750);
    wordWrapTimer->setSingleShot(true);

    connect(wordWrapTimer, SIGNAL(timeout()),
            this, SLOT(resizeRows()));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onSectionResized(int,int,int)));

    setContextMenuPolicy(Qt::CustomContextMenu);
}

TPlaylistWidget::~TPlaylistWidget() {
}

void TPlaylistWidget::clr() {

    playing_item = 0;
    clear();
}

int TPlaylistWidget::countItems(QTreeWidgetItem* w) const {

    int count = w->childCount();
    for(int c = 0; c < w->childCount(); c++) {
        count += countItems(w->child(c));
    }
    return count;
}

int TPlaylistWidget::countItems() const {
    return countItems(root());
}

int TPlaylistWidget::countChildren(QTreeWidgetItem* w) const {

    if (w->childCount()) {
        int count = 0;
        for(int c = 0; c < w->childCount(); c++) {
            count += countChildren(w->child(c));
        }
        return count;
    }
    return 1;
}

int TPlaylistWidget::countChildren() const {
    return countChildren(root());
}

TPlaylistWidgetItem* TPlaylistWidget::currentPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(currentItem());
}

QTreeWidgetItem* TPlaylistWidget::playlistWidgetFolder(QTreeWidgetItem* w) const {

    if (w) {
        if (w->childCount()) {
            return w;
        }
        if (w->parent()) {
            return w->parent();
        }
    }
    return root();
}

QTreeWidgetItem* TPlaylistWidget::currentPlaylistWidgetFolder() const {
    return playlistWidgetFolder(currentItem());
}

TPlaylistWidgetItem* TPlaylistWidget::firstPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(topLevelItem(0));
}

TPlaylistWidgetItem* TPlaylistWidget::lastPlaylistWidgetItem() const {

     TPlaylistWidgetItem* last = static_cast<TPlaylistWidgetItem*>(
                                     topLevelItem(topLevelItemCount() - 1));
     while (last && last->isFolder()) {
         last = static_cast<TPlaylistWidgetItem*>(last->child(last->childCount() - 1));
     }
     return last;
}

QString TPlaylistWidget::playingFile() const {
    return playing_item ? playing_item->filename() : "";
}

QString TPlaylistWidget::currentFile() const {

    TPlaylistWidgetItem* w = currentPlaylistWidgetItem();
    return w ? w->filename() : "";
}

TPlaylistWidgetItem* TPlaylistWidget::findFilename(const QString &filename) {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (i->filename() == filename) {
            return i;
        }
        ++it;
    }

    return 0;
}

void TPlaylistWidget::setPlayingItem(TPlaylistWidgetItem* item,
                                     TPlaylistItemState state) {
    qDebug() << "Gui::TPlaylistWidget::setPlayingItem";

    bool setCurrent = true;
    if (playing_item) {
        // Set state previous playing item
        if (playing_item != item
            && playing_item->state() != PSTATE_STOPPED
            && playing_item->state() != PSTATE_FAILED) {
            playing_item->setState(PSTATE_STOPPED);
        }
        setCurrent = playing_item == currentItem();
    }

    playing_item = item;

    if (playing_item) {
        playing_item->setState(state);
    }

    if (playing_item && setCurrent) {
        setCurrentItem(playing_item);
    } else {
        // Hack to trigger playlist enableActions...
        emit currentItemChanged(currentItem(), currentItem());
    }
}

void TPlaylistWidget::clearPlayed() {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            i->setPlayed(false);
        }
        ++it;
    }
}

TPlaylistWidgetItem* TPlaylistWidget::getNextItem(TPlaylistWidgetItem* w,
                                                  bool allowChild) const {
    // See also itemBelow()

    if (w == 0) {
        return firstPlaylistWidgetItem();
    }

    if (allowChild && w->childCount() > 0) {
        return static_cast<TPlaylistWidgetItem*>(w->child(0));
    }

    QTreeWidgetItem* parent = w->parent();
    if(parent) {
       int idx = parent->indexOfChild(w) + 1;
       if (idx < parent->childCount()) {
           return static_cast<TPlaylistWidgetItem*>(parent->child(idx));
       }
       return getNextItem(static_cast<TPlaylistWidgetItem*>(parent), false);
    }

    return static_cast<TPlaylistWidgetItem*>(
                topLevelItem(indexOfTopLevelItem(w) + 1));
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem(TPlaylistWidgetItem* i) const {

    do {
        i = getNextItem(i);
    } while (i && i->isFolder());

    return i;
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = playing_item;
    if (item == 0) {
        item = currentPlaylistWidgetItem();
    }
    return getNextPlaylistWidgetItem(item);
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousItem(TPlaylistWidgetItem* w,
                                                      bool allowChild) const {
    // See also itemAbove()

    if (w == 0) {
        return lastPlaylistWidgetItem();
    }

    int c = w->childCount();
    if (allowChild && c > 0) {
        return static_cast<TPlaylistWidgetItem*>(w->child(c - 1));
    }

    QTreeWidgetItem* parent = w->parent();
    if(parent) {
       int idx = parent->indexOfChild(w) - 1;
       if (idx >= 0) {
           return static_cast<TPlaylistWidgetItem*>(parent->child(idx));
       }
       return getPreviousItem(static_cast<TPlaylistWidgetItem*>(parent), false);
    }

    return static_cast<TPlaylistWidgetItem*>(
                topLevelItem(indexOfTopLevelItem(w) - 1));
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(
        TPlaylistWidgetItem* i) const {

    do {
        i = getPreviousItem(i);
    } while (i && i->isFolder());

    return i;
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = playing_item;
    if (item == 0) {
        item = currentPlaylistWidgetItem();
    }
    return getPreviousPlaylistWidgetItem(item);
}

TPlaylistWidgetItem* TPlaylistWidget::findPreviousPlayedTime(
        TPlaylistWidgetItem* w) {

    TPlaylistWidgetItem* result = 0;
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (((result == 0) || (i->playedTime() >= result->playedTime()))
            && (i->playedTime() < w->playedTime())) {
            result = i;
        }
        ++it;
    }

    return result;
}

// Fix Qt not selecting the drop
void TPlaylistWidget::dropEvent(QDropEvent *e) {
    qDebug() << "Gui::TPlaylistWidget::dropEvent";

    QTreeWidgetItem* current = currentItem();
    QList<QTreeWidgetItem*>	selection = selectedItems();

    QTreeWidget::dropEvent(e);

    if (e->isAccepted()) {
        clearSelection();
        setCurrentItem(current);
        for(int i = 0; i < selection.count(); i++) {
            selection[i]->setSelected(true);
        }
        emit modified();
    }
}

void TPlaylistWidget::enableSort(bool enable) {

    setSortingEnabled(enable);
    if (enable) {
        header()->setSortIndicator(TPlaylistWidgetItem::COL_NAME,
                                   Qt::AscendingOrder);
    } else {
        header()->setSortIndicator(-1, Qt::AscendingOrder);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionsClickable(true);
#else
        header()->setClickable(true);
#endif
    }
}

void TPlaylistWidget::onSectionClicked(int) {

    // TODO: see sortItems/sortColumn
    if (!isSortingEnabled()) {
        enableSort(true);
    }
}

void TPlaylistWidget::resizeRows(QTreeWidgetItem* w, int level) {

    level++;
    for(int c = 0; c < w->childCount(); c++) {
        TPlaylistWidgetItem* cw = static_cast<TPlaylistWidgetItem*>(w->child(c));
        if (cw) {
            cw->setSzHint(level);
            if (cw->isExpanded() && cw->childCount()) {
                resizeRows(cw, level);
            }
        }
    }
}

void TPlaylistWidget::resizeRows() {

    gNameColumnWidth = header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
    qDebug() << "Gui::Playlist::TPlaylistWidget::resizeRows: width name"
             << gNameColumnWidth;
    resizeRows(root(), 2);
}

void TPlaylistWidget::onSectionResized(int logicalIndex, int, int newSize) {

    if (logicalIndex == TPlaylistWidgetItem::COL_NAME) {
        wordWrapTimer->start();
        gNameColumnWidth = newSize;
        //qDebug() << "Gui::Playlist::TPlaylistWidget::onSectionResized:"
        //         << old << newSize;
    }
}

void TPlaylistWidget::onItemExpanded(QTreeWidgetItem* w) {
    qDebug() << "Gui::Playlist::TPlaylistWidget::onItemExpanded:"
             << w->text(0);

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(w);
    if (i && !wordWrapTimer->isActive()) {
        gNameColumnWidth = header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
        resizeRows(i, i->getLevel());
    }
}

QString TPlaylistWidget::add(TPlaylistWidgetItem* item,
                             QTreeWidgetItem* target,
                             QTreeWidgetItem* current) {
    qDebug() << "Gui::Playlist::TPlaylistWidget::add";

    enableSort(false);

    // Setup parent and child index into parent
    QTreeWidgetItem* parent;
    int idx = -1;
    if (target) {
        parent = playlistWidgetFolder(target);
        if (parent == target) {
            idx = 0;
        } else {
            idx = parent->indexOfChild(target);
        }
    } else {
        parent = root();
    }
    if (idx < 0) {
        idx = parent->childCount();
    }

    QString filename;
    if (parent == root()
        && parent->childCount() == 0
        && item->childCount() == 1
        && item->child(0)->childCount()) {

        TPlaylistWidgetItem* old = item;
        item = static_cast<TPlaylistWidgetItem*>(item->takeChild(0));
        delete old;
        current = item->child(0);
        idx = 0;
        filename = item->filename();
    }

    QList<QTreeWidgetItem*> children;
    while (item->childCount()) {
        children << item->takeChild(0);
    }
    delete item;

    clearSelection();
    parent->insertChildren(idx, children);
    setCurrentItem(current);

    return filename;
}


} // namespace Playlist
} // namespace Gui

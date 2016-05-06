#include "gui/playlist/playlistwidget.h"

#include <QDebug>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QDropEvent>
//#include <QTextDocument>
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>
#include <QRectF>

#include "gui/playlist/playlistwidgetitem.h"
#include "images.h"

namespace Gui {
namespace Playlist {

const int TEXT_ALIGN = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap;

int iconWidth = 0;
int nameColumnWidth = 0;

const int hm = 4;
const int vm = 1;

//QTextDocument doc;

class TWordWrapItemDelegate : public QStyledItemDelegate {
public:
    TWordWrapItemDelegate(TPlaylistWidget* parent) :
        QStyledItemDelegate(parent) {
    }

    virtual ~TWordWrapItemDelegate() {
    }

    virtual void paint(QPainter* painter,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const {

        //if (1 || index.column() != TPlaylistWidgetItem::COL_NAME) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        //}

        /*
        QString text = index.data().toString();

        //doc.setPlainText(text);
        //doc.setTextWidth(option.rect.width());

        painter->save();
        painter->setPen(Qt::SolidLine);
        painter->drawRect(option.rect);

        //painter->translate(option.rect.x(), option.rect.y());
        //doc.drawContents(painter);
        painter->restore();

        QRectF r = option.rect;
        qDebug() << "draw before adjust" << r;
        r.adjust(hm, vm, -hm, -vm);
        qDebug() << "draw after" << r << text;
        painter->drawText(r, TEXT_ALIGN, text);
        */
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {

        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        QString text = index.model()->data(index).toString();
        qDebug() << "Gui::Playlist::TWordWrapItemDelegate::sizeHint: text"
                 << text
                 << "option.rect" << option.rect
                 << "hint base" << hint;

        if (text.isEmpty()) {
            qDebug() << "Gui::Playlist::TWordWrapItemDelegate::sizeHint:"
                        " returning hint base for empty" << hint;
            return hint;
        }

        if (index.column() != TPlaylistWidgetItem::COL_NAME) {
            qDebug() << "Gui::Playlist::TWordWrapItemDelegate::sizeHint:"
                        " returning hint base for time col" << hint;
            return hint;
        }

        QRect r = option.rect;
        if (!r.isValid()) {
            if (nameColumnWidth <= 0) {
                return hint;
            }
            r = QRect(QPoint(), QSize(nameColumnWidth, 1000));
        }

        /*
        doc.setPlainText(text);
        doc.setTextWidth(option.rect.width());
        return QtCore.QSize(document.idealWidth(), document.size().height())
        */

        r.adjust(hm + iconWidth, vm, -hm, -vm);
        QRect br = option.fontMetrics.boundingRect(r, TEXT_ALIGN, text);
        hint = br.size() + QSize(hm, vm) * 2;
        qDebug() << "Gui::Playlist::TWordWrapItemDelegate::bounding rect br"
                 << br
                 << "hint" << hint;
        return hint;
    };
};


TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    playing_item(0) {

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

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

    setAutoExpandDelay(750);

    setWordWrap(true);
    setUniformRowHeights(false);
    wordWrapItemDelegate = new TWordWrapItemDelegate(this);
    setItemDelegate(wordWrapItemDelegate);

    setSelectionMode(QAbstractItemView::ExtendedSelection);

    enableSort(false);

    connect(header(), SIGNAL(sectionClicked(int)),
            this, SLOT(onSectionClicked(int)));

    // Icons
    folderIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style()->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);
    notPlayedIcon.addPixmap(style()->standardPixmap(QStyle::SP_FileIcon));
    QSize iconSize = folderIcon.actualSize(QSize(22,22));
    setIconSize(iconSize);
    iconWidth = iconSize.width();
    okIcon = Images::icon("ok", iconSize.width());
    loadingIcon = Images::icon("loading", iconSize.width());
    playIcon = Images::icon("play", iconSize.width());
    failedIcon = Images::icon("failed", iconSize.width());

    // Drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    //setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);

    setContextMenuPolicy(Qt::CustomContextMenu);
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

    if (w && w != root()) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(w);
        if (i->isFolder()) {
            return i;
        } else if (i->parent()) {
            return i->parent();
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

void TPlaylistWidget::setPlayingItem(TPlaylistWidgetItem* item) {
    qDebug() << "Gui::TPlaylistWidget::setPlayingItem";

    bool setCurrent;
    if (playing_item) {
        // Set state previous playing item
        if (playing_item != item
            && (playing_item->state() == PSTATE_PLAYING
            || playing_item->state() == PSTATE_LOADING)) {
            playing_item->setState(PSTATE_STOPPED);
        }
        setCurrent = playing_item == currentItem();
    } else {
        setCurrent = true;
    }

    playing_item = item;
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

    if (allowChild && w->isFolder() && w->childCount() > 0) {
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
    if (allowChild && w->isFolder() && c > 0) {
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

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(TPlaylistWidgetItem* i) const {

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
    qDebug() << "Gui::Playlist::TPlaylistWidget::findPreviousPlayedTime:"
             << w->filename() << w->playedTime();

    TPlaylistWidgetItem* result = 0;
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        qDebug() << i->filename() << i->playedTime();
        if (((result == 0) || (i->playedTime() >= result->playedTime()))
            && (i->playedTime() < w->playedTime())) {
            qDebug() << "selected";
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
        header()->setClickable(true);
    }
}

void TPlaylistWidget::onSectionClicked(int) {
    qDebug() << "Gui::TPlaylistWidget: onSectionClicked";

    // TODO: see sortItems/sortColumn
    if (!isSortingEnabled()) {
        enableSort(true);
    }
}

void TPlaylistWidget::resizeRows() {

    //resizeColumnToContents(TPlaylistWidgetItem::COL_TIME);
    //doItemsLayout();

    /*
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        qDebug() << i->name();
        ++it;
    }
    */
}

void TPlaylistWidget::resizeEvent(QResizeEvent* event) {

    nameColumnWidth = columnWidth(TPlaylistWidgetItem::COL_NAME);
    qDebug() << "Gui::Playlist::TPlaylistWidget::resizeEvent: set col width to"
             << nameColumnWidth;
    QTreeWidget::resizeEvent(event);
}

} // namespace Playlist
} // namespace Gui

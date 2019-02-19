#include "gui/playlist/playlistwidget.h"

#include "gui/playlist/playlistwidgetitem.h"
#include "gui/action/menu/menuexec.h"
#include "gui/msg.h"
#include "images.h"
#include "iconprovider.h"
#include "wzdebug.h"

#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QDropEvent>
#include <QFontMetrics>
#include <QTimer>
#include <QSettings>


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
            return TPlaylistWidgetItem::ROOT_NODE_LEVEL;
        }
        return getLevel(index.parent()) + 1;
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {

        // Return default size hint for all but column name
        if (index.column() != TPlaylistWidgetItem::COL_NAME) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        // Column name
        QString text = index.model()->data(index).toString();

        // Return default for no name
        if (text.isEmpty()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        // Return the size of column name
        return TPlaylistWidgetItem::sizeColumnName(
                    header->sectionSize(TPlaylistWidgetItem::COL_NAME),
                    text,
                    option.fontMetrics,
                    option.decorationSize,
                    getLevel(index));
    }

private:
    QHeaderView* header;
}; // class TWordWrapItemDelegate


TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    debug(logger()),
    playing_item(0),
    sortSection(-1),
    sortOrder(Qt::AscendingOrder),
    mModified(false) {

    setObjectName("playlistwidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setColumnCount(TPlaylistWidgetItem::COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Ext") << tr("Length")
                    << tr("#"));
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_NAME,
                                   QHeaderView::Stretch);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_EXT,
                                   QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_TIME,
                                   QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_ORDER,
                                   QHeaderView::ResizeToContents);

    // Create a TPlaylistWidgetItem root
    addTopLevelItem(new TPlaylistWidgetItem());
    setRootIndex(model()->index(0, 0));

    setSelectionMode(QAbstractItemView::ExtendedSelection);

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
    setIconSize(iconProvider.iconSize);
    TPlaylistWidgetItem::gNameFontMetrics = fontMetrics();
    setWordWrap(true);
    setUniformRowHeights(false);
    setItemDelegate(new TWordWrapItemDelegate(this));
    wordWrapTimer = new QTimer(this);
    wordWrapTimer->setInterval(500);
    wordWrapTimer->setSingleShot(true);
    connect(wordWrapTimer, &QTimer::timeout,
            this, &TPlaylistWidget::resizeRowsEx);

    setAutoExpandDelay(750);
    connect(this, &TPlaylistWidget::itemExpanded,
            this, &TPlaylistWidget::onItemExpanded);
    connect(header(), &QHeaderView::sectionResized,
            this, &TPlaylistWidget::onSectionResized);

    header()->setSectionsClickable(true);
    connect(header(), &QHeaderView::sectionClicked,
            this, &TPlaylistWidget::onSectionClicked);

    // Columns menu
    columnsMenu = new Gui::Action::Menu::TMenuExec(this);
    columnsMenu->menuAction()->setObjectName("pl_columns_menu");
    columnsMenu->menuAction()->setText(tr("View columns"));
    QStringList colnames = QStringList() << "name" << "ext" << "length"
                        << "order";
    for(int i = 0; i < columnCount(); i++) {
        QAction* a = new QAction(headerItem()->text(i), columnsMenu);
        a->setObjectName("pl_toggle_col_" + colnames.at(i));
        a->setCheckable(true);
        a->setData(i);
        columnsMenu->addAction(a);
    }
    connect(columnsMenu, &QMenu::triggered,
            this, &TPlaylistWidget::onColumnMenuTriggered);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested,
            columnsMenu, &Gui::Action::Menu::TMenuExec::execSlot);
}

TPlaylistWidget::~TPlaylistWidget() {
}

void TPlaylistWidget::clr() {

    playing_item = 0;
    mModified = false;

    clear();

    // Create a TPlaylistWidgetItem root
    addTopLevelItem(new TPlaylistWidgetItem());
    setRootIndex(model()->index(0, 0));
}

void TPlaylistWidget::setModified(QTreeWidgetItem* item,
                                  bool modified,
                                  bool recurse) {

    TPlaylistWidgetItem* i = dynamic_cast<TPlaylistWidgetItem*>(item);
    if (i == 0) {
        i = root();
    }
    if (i) {
        i->setModified(modified, recurse);
        if (mModified != modified) {
            mModified = modified;
            emit modifiedChanged();
        }
    }
}

int TPlaylistWidget::countItems(QTreeWidgetItem* w) const {

    int count = 0;
    if (w) {
        count = w->childCount();
        for(int c = 0; c < w->childCount(); c++) {
            count += countItems(w->child(c));
        }
    }
    return count;
}

int TPlaylistWidget::countItems() const {
    return countItems(root());
}

int TPlaylistWidget::countChildren(QTreeWidgetItem* w) const {

    if (w) {
        if (w->childCount()) {
            int count = 0;
            for(int c = 0; c < w->childCount(); c++) {
                count += countChildren(w->child(c));
            }
            return count;
        }
        if (w != root()) {
            return 1;
        }
    }
    return 0;
}

int TPlaylistWidget::countChildren() const {
    return countChildren(root());
}

bool TPlaylistWidget::hasItems() const {

    TPlaylistWidgetItem* r = root();
    return r && r->childCount();
}

bool TPlaylistWidget::hasSingleItem() const {

    TPlaylistWidgetItem* r = root();
    return r && r->childCount() == 1 && r->child(0)->childCount() == 0;
}

TPlaylistWidgetItem* TPlaylistWidget::currentPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(currentItem());
}

TPlaylistWidgetItem* TPlaylistWidget::firstPlaylistWidgetItem() const {

    TPlaylistWidgetItem* r = root();
    if (r) {
        return r->plChild(0);
    }
    return 0;
}

TPlaylistWidgetItem* TPlaylistWidget::lastPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = root();
    while (item && item->childCount()) {
        item = item->plChild(item->childCount() - 1);
    }
    if (item == root()) {
        return 0;
    }
    return item;
}

QString TPlaylistWidget::playingFile() const {

    if (hasItems() && playing_item) {
        return playing_item->filename();
    }
    return "";
}

QString TPlaylistWidget::currentFile() const {

    TPlaylistWidgetItem* w = currentPlaylistWidgetItem();
    return w ? w->filename() : "";
}

TPlaylistWidgetItem* TPlaylistWidget::findFilename(const QString &filename) {

    if (filename.isEmpty()) {
        return 0;
    }

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
                                     TPlaylistWidgetItemState state) {
    WZDEBUG("");

    bool setCurrent = true;
    if (playing_item) {
        // Set state previous playing item
        if (playing_item != item
            && playing_item->state() != PSTATE_STOPPED
            && playing_item->state() != PSTATE_FAILED) {
            playing_item->setState(PSTATE_STOPPED);
        }
        // Only set current, when playing_item was current
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
        return w->plChild(0);
    }

    TPlaylistWidgetItem* parent = static_cast<TPlaylistWidgetItem*>(w->parent());
    if(parent) {
       int idx = parent->indexOfChild(w) + 1;
       if (idx < parent->childCount()) {
           return parent->plChild(idx);
       }
       return getNextItem(parent, false);
    }

    return 0;
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem(
        TPlaylistWidgetItem* item) const {

    do {
        item = getNextItem(item);
    } while (item && item->childCount());

    return item;
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
        return w->plChild(c - 1);
    }

    TPlaylistWidgetItem* parent = w->plParent();
    if(parent) {
       int idx = parent->indexOfChild(w) - 1;
       if (idx >= 0) {
           return parent->plChild(idx);
       }
       return getPreviousItem(parent, false);
    }

    return lastPlaylistWidgetItem();
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(
        TPlaylistWidgetItem* i) const {

    do {
        i = getPreviousItem(i);
    } while (i && i->childCount());

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
        if (i->childCount() == 0
            && ((result == 0) || (i->playedTime() >= result->playedTime()))
            && (i->playedTime() < w->playedTime())) {
            result = i;
        }
        ++it;
    }

    return result;
}

void TPlaylistWidget::dropEvent(QDropEvent* e) {
    WZDEBUG("");

    QTreeWidgetItem* current = currentItem();
    QList<QTreeWidgetItem*> sel = selectedItems();
    QList<QTreeWidgetItem*> modified;

    // Collect parents of the selection to mark as modified if drop succeeds
    for(int i = 0; i < sel.count(); i++) {
        QTreeWidgetItem* p = sel.at(i)->parent();
        if (!modified.contains(p)) {
            modified.append(p);
        }
    }

    // Need to read event->dropAction() here, cause QTreeWidget will modify it
    bool moved = e->source() == this
            && (e->dropAction() == Qt::MoveAction
                || dragDropMode() == QAbstractItemView::InternalMove);

    // Handle the drop
    QTreeWidget::dropEvent(e);

    if (e->isAccepted()) {
        WZDEBUG("Drop accepted");

        if (moved) {
            // Restore current item and selection
            clearSelection();
            setCurrentItem(current);
            for(int i = 0; i < sel.count(); i++) {
                sel.at(i)->setSelected(true);
            }

            if (!sel.isEmpty()) {
                // Add new parent to modified items
                TPlaylistWidgetItem* parent = static_cast<TPlaylistWidgetItem*>(
                            sel.at(0)->parent());
                if (!modified.contains(parent)) {
                    modified.append(parent);
                }

                // Rename dropped files if WZPlaylist
                if (parent->isWZPlaylist()) {
                    bool failed = false;
                    for(int i = 0; i < sel.count(); i++) {
                        TPlaylistWidgetItem* item =
                                static_cast<TPlaylistWidgetItem*>(sel.at(i));
                        if (!item->renameDroppedFile()) {
                            // Delete the failed item, so it won't be saved
                            int idx = parent->indexOfChild(item);
                            delete parent->takeChild(idx);
                            failed = true;
                        }
                    }

                    // Refresh the playlist if failed
                    if (failed) {
                        emit refresh();
                    }
                }
            }
        } else {
            // TODO: currently copy is blocked by drag drop settings
        }

        // Set modified
        for(int i = 0; i < modified.count(); i++) {
            setModified(static_cast<TPlaylistWidgetItem*>(modified.at(i)));
        }
    } else {
        WZDEBUG("Drop not accepted");
    }
}

void TPlaylistWidget::rowsAboutToBeRemoved(const QModelIndex &parent,
                                           int start, int end) {
    WZDEBUG(QString("'%1' %2 %3")
            .arg(parent.data().toString()).arg(start).arg(end));

    QTreeWidget::rowsAboutToBeRemoved(parent, start, end);

    // Because of setRootIndex(model()->index(0, 0)) in add(),
    // root item that needs action is always valid.
    if (!parent.isValid()) {
        return;
    }
    TPlaylistWidgetItem* p = static_cast<TPlaylistWidgetItem*>(
                parent.internalPointer());

    if (sortSection == TPlaylistWidgetItem::COL_ORDER) {
        int d = end - start + 1;
        if (sortOrder == Qt::AscendingOrder) {
            for (int i = end + 1; i < p->childCount(); i++) {
                p->plChild(i)->setOrder(i - d);
            }
        } else {
            d = p->childCount() - d;
            for (int i = start - 1; i >= 0; i--) {
                p->plChild(i)->setOrder(d - i);
            }
        }
    } else {
        for (int r = start; r <= end; r++) {
            int order = p->plChild(r)->order();
            for (int i = p->childCount() - 1; i >= 0; i--) {
                TPlaylistWidgetItem* item = p->plChild(i);
                int o = item->order();
                if (o > order) {
                    item->setOrder(o - 1);
                }
            }
        }
    }
}

void TPlaylistWidget::rowsInserted(const QModelIndex &parent,
                                   int start, int end) {
    WZDEBUG(QString("'%1' %2 %3")
            .arg(parent.data().toString()).arg(start).arg(end));
    QTreeWidget::rowsInserted(parent, start, end);

    // Because of setRootIndex(model()->index(0, 0)) in add(),
    // root item that needs action is always valid.
    if (!parent.isValid()) {
        return;
    }

    TPlaylistWidgetItem* p = static_cast<TPlaylistWidgetItem*>(
                parent.internalPointer());

    if (sortSection == TPlaylistWidgetItem::COL_ORDER) {
        if (sortOrder == Qt::AscendingOrder) {
            for (int i = p->childCount() - 1; i >= start; i--) {
                p->plChild(i)->setOrder(i + 1);
            }
        } else {
            int order = p->childCount();
            for (int i = 0; i <= end; i++) {
                p->plChild(i)->setOrder(order--);
            }
        }
    } else {
        int d = end - start + 1;
        for (int i = p->childCount() - 1; i > end; i--) {
            TPlaylistWidgetItem* item = p->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
        for (int i = end; i >= start; i--) {
            p->plChild(i)->setOrder(i + 1);
        }
        for (int i = start - 1; i >= 0; i--) {
            TPlaylistWidgetItem* item = p->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
    }
}

void TPlaylistWidget::setSort(int section, Qt::SortOrder order) {

    sortSection = section;
    sortOrder = order;
    header()->setSortIndicator(sortSection, sortOrder);
    if (sortSection >= 0) {
        if (!isSortingEnabled()) {
            setSortingEnabled(true);
        }
    } else if (isSortingEnabled()) {
        setSortingEnabled(false);
    }
}

void TPlaylistWidget::onColumnMenuTriggered(QAction* action) {

    int col = action->data().toInt();
    setColumnHidden(col, !action->isChecked());

    // Prevent all columns hidden
    for (int i = 0; i < columnCount(); i++) {
        if (!isColumnHidden(i)) {
            return;
        }
    }
    showColumn(col);
    action->setChecked(true);
}

void TPlaylistWidget::onSectionClicked(int section) {

    if (section == sortSection) {
        if (sortOrder == Qt::AscendingOrder) {
            sortOrder = Qt::DescendingOrder;
        } else {
            sortOrder = Qt::AscendingOrder;
        }
    } else {
        sortSection = section;
        sortOrder = Qt::AscendingOrder;
    }

    setSort(sortSection, sortOrder);
}

void TPlaylistWidget::resizeRows(QTreeWidgetItem* w, int level) {

    if (w) {
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
}

void TPlaylistWidget::resizeRowsEx() {

    TPlaylistWidgetItem::gNameColumnWidth =
            header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
    resizeRows(root(), TPlaylistWidgetItem::ROOT_NODE_LEVEL);
}

void TPlaylistWidget::onSectionResized(int logicalIndex, int, int newSize) {

    if (logicalIndex == TPlaylistWidgetItem::COL_NAME) {
        TPlaylistWidgetItem::gNameColumnWidth = newSize;
        wordWrapTimer->start();
    }
}

void TPlaylistWidget::onItemExpanded(QTreeWidgetItem* w) {
    WZDEBUG("'" + w->text(TPlaylistWidgetItem::COL_NAME) + "'");

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(w);
    if (i == 0) {
        return;
    }

    // Resize rows of expanded item
    if (!wordWrapTimer->isActive()) {
        TPlaylistWidgetItem::gNameColumnWidth =
                header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
        resizeRows(i, i->getLevel());
    }
}

TPlaylistWidgetItem* TPlaylistWidget::validateItem(TPlaylistWidgetItem* item) {

    if (item) {
        QTreeWidgetItemIterator it(this);
        while (*it) {
            if (*it == item) {
                return item;
            }
            ++it;
        }
    }

    return 0;
}

TPlaylistWidgetItem* TPlaylistWidget::add(TPlaylistWidgetItem* item,
                                          TPlaylistWidgetItem* target) {
    WZDEBUG("");

    // Get parent and child index into parent
    TPlaylistWidgetItem* parent;
    int idx = 0;

    // Validate target is still valid
    target = validateItem(target);

    if (target) {
        if (target->childCount()) {
            parent = target;
        } else {
            parent = target->plParent();
            if (parent) {
                idx = parent->indexOfChild(target);
            }
        }
    } else {
        parent = root();
        if (parent) {
            idx = parent->childCount();
        }
    }

    bool newModified;
    if (parent == 0 || (parent == root() && parent->childCount() == 0)) {
        // Remove single folder in root
        if (item->childCount() == 1 && item->child(0)->childCount()) {
            WZDEBUG("removing single folder in root");
            TPlaylistWidgetItem* old = item;
            item = static_cast<TPlaylistWidgetItem*>(item->takeChild(0));
            delete old;
        }

        // Invalidate playing_item
        playing_item = 0;

        // Delete old root
        setRootIndex(QModelIndex());
        delete takeTopLevelItem(0);

        // Disable sort
        setSort(-1, sortOrder);

        // Set item as root
        item->setFlags(ROOT_FLAGS);
        addTopLevelItem(item);
        setRootIndex(model()->index(0, 0));

        // Sort
        if (item->isWZPlaylist() || !item->isPlaylist()) {
            sortSection = TPlaylistWidgetItem::COL_NAME;
            sortOrder = Qt::AscendingOrder;
        } else {
            sortSection = TPlaylistWidgetItem::COL_ORDER;
            sortOrder = Qt::AscendingOrder;
        }
        setSort(sortSection, sortOrder);

        if (item->childCount()) {
            setCurrentItem(item->child(0));
            // call onItemExpanded on root (loads icons etc.)
            onItemExpanded(item);
        }

        newModified = item->modified();
    } else {
        bool itemModified = item->modified();

        // Collect children of item in QList
        QList<QTreeWidgetItem*> children;
        while (item->childCount()) {
            children << item->takeChild(0);
        }

        // Cleanup item
        clearSelection();
        delete item;
        item = 0;

        // Disable sort
        int savedSortSection = sortSection;
        setSort(-1, sortOrder);

        // Insert children
        parent->insertChildren(idx, children);

        // Sort
        setSort(savedSortSection, sortOrder);

        // Select first as current item
        setCurrentItem(children.at(0));

        if (itemModified) {
            // Update modified
            newModified = itemModified;
            parent->setModified();
        } else {
            // Leave modified unchanged
            newModified  = mModified;
        }
    }

    // Signal modified changed
    if (newModified != mModified) {
        mModified = newModified;
        emit modifiedChanged();
    }

    return item;
}

void TPlaylistWidget::saveSettings(QSettings* pref) {

    for(int c = 0; c < columnCount(); c++) {
        pref->setValue("COL_" + QString::number(c), !isColumnHidden(c));
    }
}

void TPlaylistWidget::loadSettings(QSettings* pref) {

    QList<QAction*> actions = columnsMenu->actions();
    for(int c = 0; c < columnCount(); c++) {
        bool show = pref->value("COL_" + QString::number(c), true).toBool();
        actions.at(c)->setChecked(show);
        setColumnHidden(c, !show);
    }
}

} // namespace Playlist
} // namespace Gui

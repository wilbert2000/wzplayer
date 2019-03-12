#include "gui/playlist/playlistwidget.h"

#include "gui/playlist/playlistitem.h"
#include "gui/action/menu/menuexec.h"
#include "qtfilecopier/qtfilecopier.h"
#include "qtfilecopier/qtcopydialog.h"
#include "player/player.h"
#include "gui/msg.h"
#include "wzfiles.h"
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
#include <QMessageBox>
#include <QDir>
#include <QMimeData>
#include <QApplication>


namespace Gui {
namespace Playlist {

TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    debug(logger()),
    playingItem(0),
    sortSection(-1),
    sortOrder(Qt::AscendingOrder),
    fileCopier(0),
    copyDialog(0) {

    setObjectName("playlistwidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setIconSize(iconProvider.iconSize);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::EditKeyPressed);

    setColumnCount(TPlaylistItem::COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Ext") << tr("Length")
                    << tr("#"));
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(TPlaylistItem::COL_NAME,
                                   QHeaderView::Stretch);
    header()->setSectionResizeMode(TPlaylistItem::COL_EXT,
                                   QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(TPlaylistItem::COL_TIME,
                                   QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(TPlaylistItem::COL_ORDER,
                                   QHeaderView::ResizeToContents);

    // Create a TPlaylistItem root
    TPlaylistItem* root = new TPlaylistItem();
    addTopLevelItem(root);
    // Setup spacing playlist items for wordwrap
    root->setSpacing();
    setRootIndex(model()->index(0, 0));

    // Drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);

    // Wordwrap
    setWordWrap(true);
    setUniformRowHeights(false);
    wordWrapTimer.setSingleShot(true);
    connect(&wordWrapTimer, &QTimer::timeout,
            this, &TPlaylistWidget::resizeNameColumnAll);
    connect(header(), &QHeaderView::sectionResized,
            this, &TPlaylistWidget::onSectionResized);

    setAutoExpandDelay(750);
    connect(this, &TPlaylistWidget::itemExpanded,
            this, &TPlaylistWidget::onItemExpanded);

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

    delete copyDialog;
    copyDialog = 0;
    if (fileCopier) {
        fileCopier->cancelAll();
        delete fileCopier;
        fileCopier = 0;
    }
}

void TPlaylistWidget::clr() {

    playingItem = 0;
    clear();

    // Create a TPlaylistItem root
    addTopLevelItem(new TPlaylistItem());
    setRootIndex(model()->index(0, 0));
}

// Called by root item to signal modified changed
void TPlaylistWidget::emitModifiedChanged() {
    emit modifiedChanged();
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

    TPlaylistItem* r = root();
    return r && r->childCount();
}

bool TPlaylistWidget::hasSingleItem() const {

    TPlaylistItem* r = root();
    return r && r->childCount() == 1 && r->child(0)->childCount() == 0;
}

TPlaylistItem* TPlaylistWidget::plCurrentItem() const {
    return static_cast<TPlaylistItem*>(currentItem());
}

TPlaylistItem* TPlaylistWidget::firstPlaylistItem() const {

    TPlaylistItem* r = root();
    if (r && r->childCount()) {
        return r->plChild(0);
    }
    return 0;
}

TPlaylistItem* TPlaylistWidget::lastPlaylistItem() const {

    TPlaylistItem* item = root();
    while (item && item->childCount()) {
        item = item->plChild(item->childCount() - 1);
    }
    if (item == root()) {
        return 0;
    }
    return item;
}

QString TPlaylistWidget::playingFile() const {

    if (hasItems() && playingItem) {
        return playingItem->filename();
    }
    return QString();
}

QString TPlaylistWidget::currentFile() const {

    TPlaylistItem* w = plCurrentItem();
    return w ? w->filename() : "";
}

TPlaylistItem* TPlaylistWidget::findFilename(const QString &filename) {

    if (filename.isEmpty()) {
        return 0;
    }

    QString search1 = QDir::toNativeSeparators(filename);
    QString search2;
    if (QFileInfo(filename).isDir()) {
        search2 = search1 + QDir::separator() + TConfig::WZPLAYLIST;
    }
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        QString fn = i->filename();
        if (fn == search1 || (!search2.isEmpty() && fn == search2)) {
            return i;
        }
        ++it;
    }

    return 0;
}

void TPlaylistWidget::setPlayingItem(TPlaylistItem* item,
                                     TPlaylistItemState state) {

    bool setCurrent = true;
    if (playingItem) {
        // Set state previous playing item
        if (playingItem != item
            && playingItem->state() != PSTATE_STOPPED
            && playingItem->state() != PSTATE_FAILED) {
            playingItem->setState(PSTATE_STOPPED);
        }
        // Only set current, when playing_item was current item
        setCurrent = playingItem == currentItem();
    }

    playingItem = item;

    if (playingItem) {
        playingItem->setState(state);
    }

    if (playingItem && setCurrent) {
        setCurrentItem(playingItem);
    } else {
        // Hack to trigger playlist enableActions...
        // TODO: remove hack
        emit currentItemChanged(currentItem(), currentItem());
    }
}

void TPlaylistWidget::clearPlayed() {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (!i->isFolder()) {
            i->setPlayed(false);
        }
        ++it;
    }
}

TPlaylistItem* TPlaylistWidget::getNextItem(TPlaylistItem* w,
                                            bool allowChild) const {
    // See also itemBelow()

    if (w == 0) {
        return firstPlaylistItem();
    }

    if (allowChild && w->childCount() > 0) {
        return w->plChild(0);
    }

    TPlaylistItem* parent = static_cast<TPlaylistItem*>(w->parent());
    if(parent) {
       int idx = parent->indexOfChild(w) + 1;
       if (idx < parent->childCount()) {
           return parent->plChild(idx);
       }
       return getNextItem(parent, false);
    }

    return 0;
}

TPlaylistItem* TPlaylistWidget::getNextPlaylistItem(TPlaylistItem* item) const {

    do {
        item = getNextItem(item);
    } while (item && item->childCount());

    return item;
}

TPlaylistItem* TPlaylistWidget::getNextPlaylistItem() const {

    TPlaylistItem* item = playingItem;
    if (item == 0) {
        item = plCurrentItem();
    }
    return getNextPlaylistItem(item);
}

TPlaylistItem* TPlaylistWidget::getPreviousItem(TPlaylistItem* w,
                                                bool allowChild) const {
    // See also itemAbove()

    if (w == 0) {
        return lastPlaylistItem();
    }

    int c = w->childCount();
    if (allowChild && c > 0) {
        return w->plChild(c - 1);
    }

    TPlaylistItem* parent = w->plParent();
    if(parent) {
       int idx = parent->indexOfChild(w) - 1;
       if (idx >= 0) {
           return parent->plChild(idx);
       }
       return getPreviousItem(parent, false);
    }

    return lastPlaylistItem();
}

TPlaylistItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(
        TPlaylistItem* i) const {

    do {
        i = getPreviousItem(i);
    } while (i && i->childCount());

    return i;
}

TPlaylistItem* TPlaylistWidget::getPreviousPlaylistWidgetItem() const {

    TPlaylistItem* item = playingItem;
    if (item == 0) {
        item = plCurrentItem();
    }
    return getPreviousPlaylistWidgetItem(item);
}

TPlaylistItem* TPlaylistWidget::findPreviousPlayedTime(
        TPlaylistItem* w) {

    TPlaylistItem* result = 0;
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (i->childCount() == 0
            && ((result == 0) || (i->playedTime() >= result->playedTime()))
            && (i->playedTime() < w->playedTime())) {
            result = i;
        }
        ++it;
    }

    return result;
}

void TPlaylistWidget::editName() {
    WZDEBUG("");

    TPlaylistItem* cur = plCurrentItem();
    if (cur) {
        // Select COL_NAME
        setCurrentItem(cur, TPlaylistItem::COL_NAME);
        // Start editor
        QKeyEvent event(QEvent::KeyPress, Qt::Key_F2, Qt::NoModifier);
        keyPressEvent(&event);
        // Unselect extension
        if (!cur->isWZPlaylist()) {
            QString ext = cur->extension();
            if (!ext.isEmpty()) {
                QKeyEvent ev(QEvent::KeyPress, Qt::Key_Left, Qt::ShiftModifier);
                QWidget* editor = qApp->focusWidget();
                for(int i = ext.length(); i >= 0; i--) {
                    qApp->sendEvent(editor, &ev);
                }
            }
        }
    }
}

bool TPlaylistWidget::droppingOnItself(QDropEvent *event,
                                       const QModelIndex &index) {

    Qt::DropAction dropAction = event->dropAction();
    if (dragDropMode() == QAbstractItemView::InternalMove)
        dropAction = Qt::MoveAction;
    if (event->source() == this
        && event->possibleActions() & Qt::MoveAction
        && dropAction == Qt::MoveAction) {
        QModelIndexList selected = selectedIndexes();
        QModelIndex root = rootIndex();
        QModelIndex child = index;
        while (child.isValid() && child != root) {
            if (selected.contains(child))
                return true;
            child = child.parent();
        }
    }
    return false;
}

/*!
    If the event hasn't already been accepted, determines the index to drop on.
    if (row == -1 && col == -1)
        // append to this drop index
    else
        // place at row, col in drop index
    If it returns true a drop can be done, and dropRow, dropCol and dropIndex
    reflects the position of the drop.
 */
bool TPlaylistWidget::dropOn(QDropEvent *event, int *dropRow, int *dropCol,
                             QModelIndex *dropIndex) {

    if (event->isAccepted()) {
        return false;
    }

    QModelIndex index;
    QModelIndex root = rootIndex();
    if (viewport()->rect().contains(event->pos())) {
        index = indexAt(event->pos());
        if (!index.isValid() || !visualRect(index).contains(event->pos()))
            index = root;
    }

    // If we are allowed to do the drop
    if (model()->supportedDropActions() & event->dropAction()) {
        int row = -1;
        int col = -1;
        if (index != root) {
            switch (dropIndicatorPosition()) {
            case QAbstractItemView::AboveItem:
                row = index.row();
                col = index.column();
                index = index.parent();
                break;
            case QAbstractItemView::BelowItem:
                row = index.row() + 1;
                col = index.column();
                index = index.parent();
                break;
            case QAbstractItemView::OnItem:
            case QAbstractItemView::OnViewport:
                break;
            }
        }
        *dropIndex = index;
        *dropRow = row;
        *dropCol = col;
        if (!droppingOnItself(event, index))
            return true;
    }

    return false;
}

void TPlaylistWidget::onDropDone(bool error) {
    WZDEBUG(QString("Error %1").arg(error));

    // The copy dialog should already be hidden by QtCopyDialogPrivate::done()
    delete copyDialog;
    copyDialog = 0;
    fileCopier->deleteLater();
    fileCopier = 0;
}

bool TPlaylistWidget::addDroppedItem(const QString& source,
                                     const QString& dest,
                                     TPlaylistItem* item) {

    QFileInfo fid(dest);
    if (fid.exists()) {
        TPlaylistItem* parent = findFilename(fid.absolutePath());
        if (parent) {
            // Update item
            item->updateFilename(source, dest);

            // Remove existing items
            for(int i = parent->childCount() - 1; i >= 0; i--) {
                if (dest.compare(parent->plChild(i)->path(),
                                 caseSensitiveFileNames) == 0) {
                    delete parent->takeChild(i);
                }
            }

            // Add item
            parent->addChild(item);
            parent->setModified();
            item->setSelected(true);
            return true;
        }
        WZERROR(QString("Destination '%1' not found in playlist.")
                .arg(fid.absoluteFilePath()));
    } else {
        WZERROR(QString("Destination '%1' not found on disk.").arg(dest));
    }

    delete item;
    return false;
}

void TPlaylistWidget::onCopyFinished(int id, bool error) {

    QString source = QDir::toNativeSeparators(fileCopier->sourceFilePath(id));
    QString dest = QDir::toNativeSeparators(fileCopier->destinationFilePath(id));

    if (error) {
        WZINFO(QString("Canceled copy '%1' to '%2'").arg(source).arg(dest));
    } else {
        TPlaylistItem* sourceItem = findFilename(source);
        if (sourceItem) {
            // Add clone to parent
            TPlaylistItem* destItem = sourceItem->clone();
            if (addDroppedItem(source, dest, destItem)) {
                destItem->setState(PSTATE_STOPPED);
                destItem->setModified(true, true);
                if (sourceItem == plCurrentItem()) {
                    setCurrentItem(destItem);
                }
                WZINFO(QString("Copied '%1' to '%2'").arg(source).arg(dest));
            }
        } else {
            WZWARN(QString("Source '%1' not found in playlist").arg(source));
        }
    }
}

void TPlaylistWidget::onMoveAboutToStart(int id) {

    bool stopPlayer = false;
    if (playingItem  && player->state() != Player::STATE_STOPPED) {
        QFileInfo fip(playingItem->filename());
        QFileInfo fis(fileCopier->sourceFilePath(id));
        if (fis.isDir()) {
            QString dir = fis.canonicalFilePath();
            if (!dir.endsWith("/")) {
                dir += "/";
            }
            if (fip.canonicalFilePath().startsWith(dir)) {
                stopPlayer = true;
            }
        } else if (fis == fip) {
            stopPlayer = true;
        }
    }

    if (stopPlayer) {
        WZDEBUG("Stopping player");
        stoppedFilename = QDir::toNativeSeparators(
                    fileCopier->destinationFilePath(id));
        stoppedItem = playingItem;
        player->saveRestartState();
        player->stop();
    }
}

void TPlaylistWidget::onMoveFinished(int id, bool error) {

    QString source = QDir::toNativeSeparators(fileCopier->sourceFilePath(id));
    QString dest = QDir::toNativeSeparators(fileCopier->destinationFilePath(id));

    if (error) {
        WZINFO(QString("Canceled move from '%1' to '%2'").arg(source).arg(dest));
    } else {
        TPlaylistItem* sourceItem = findFilename(source);
        if (sourceItem) {
            // Remove it from parent
            bool isCurrentItem = sourceItem == plCurrentItem();
            TPlaylistItem* parent = sourceItem->plParent();
            int idx = parent->indexOfChild(sourceItem);
            parent->takeChild(idx);
            parent->setModified();

            // Add it to dest
            if (addDroppedItem(source, dest, sourceItem)) {
                if (stoppedFilename == dest) {
                    if (player->state() == Player::STATE_STOPPED) {
                        isCurrentItem = false;
                        setCurrentItem(stoppedItem);
                        WZDEBUG("Posting restart player");
                        QTimer::singleShot(0, this->parent(), SLOT(play()));
                    }
                    stoppedFilename = "";
                }
                if (isCurrentItem) {
                    setCurrentItem(sourceItem);
                }
                WZINFO(QString("Moved '%1' to '%2'").arg(source).arg(dest));
            }
        } else {
            WZWARN(QString("Source '%1' not found in playlist").arg(source));
        }
    }
}

void TPlaylistWidget::dropSelection(TPlaylistItem* target,
                                    Qt::DropAction action) {
    WZDEBUG(QString("Selected target '%1'").arg(target->filename()));

    if (fileCopier) {
        copyDialog->show();
        copyDialog->raise();
        QMessageBox::information(this, tr("Information"),
            tr("A copy or move action is still in progress. You can retry after"
               " it has finished."));
        return;
    }

    // Setup the file copier
    fileCopier = new QtFileCopier(this);
    copyDialog = new QtCopyDialog(this);
    copyDialog->setFileCopier(fileCopier);
    connect(fileCopier, &QtFileCopier::done,
            this, &TPlaylistWidget::onDropDone,
            Qt::QueuedConnection);

    // Collect the dropped files
    QStringList files;
    QList<QTreeWidgetItem*> sel = selectedItems();
    for(int i = 0; i < sel.length(); i++) {
        TPlaylistItem* item = static_cast<TPlaylistItem*>(sel.at(i));
        files.append(item->path());
        WZDEBUG(QString("Adding '%1'").arg(item->path()));
    }

    // The finished event handlers will select the items
    clearSelection();

    // Pass files to copier
    if (action == Qt::MoveAction) {
        stoppedFilename = "";
        connect(fileCopier, &QtFileCopier::aboutToStart,
                this, &TPlaylistWidget::onMoveAboutToStart);
        connect(fileCopier, &QtFileCopier::finished,
                this, &TPlaylistWidget::onMoveFinished);
        fileCopier->moveFiles(files, target->path(), 0);
    } else {
        connect(fileCopier, &QtFileCopier::finished,
                this, &TPlaylistWidget::onCopyFinished);
        fileCopier->copyFiles(files, target->path(), 0);
    }

    return;
}

void TPlaylistWidget::dropEvent(QDropEvent* event) {
    debug << "dropEvent" << event->mimeData()->formats();
    debug << debug;

    QModelIndex index;
    int col = -1;
    int row = -1;
    if (dropOn(event, &row, &col, &index)) {
        TPlaylistItem* target = static_cast<TPlaylistItem*>(
                    index.internalPointer());
        QString fn = target->path();
        if (!fn.isEmpty() && QFileInfo(fn).isDir()) {
            dropSelection(target, event->dropAction());
            // Don't want QAbstractItemView to delete src because it was "moved"
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
    }

    QTreeWidgetItem* current = currentItem();
    QList<QTreeWidgetItem*> sel;
    QList<QTreeWidgetItem*> modified;

    // Handle non filesystem stuff that is gona move
    bool moved = !event->isAccepted()
            && event->source() == this
            && event->dropAction() == Qt::MoveAction;

    // Collect parents of selection to mark as modified if drop succeeds
    sel = selectedItems();
    if (moved) {
        for(int i = 0; i < sel.count(); i++) {
            QTreeWidgetItem* p = sel.at(i)->parent();
            if (!modified.contains(p)) {
                modified.append(p);
            }
        }
    }

    // Handle the drop
    QTreeWidget::dropEvent(event);

    if (event->isAccepted()) {
        if (moved) {
            WZDEBUG("Selection moved");
            // Restore current item and selection
            clearSelection();
            setCurrentItem(current);
            for(int i = 0; i < sel.count(); i++) {
                sel.at(i)->setSelected(true);
            }

            // Add new parent to modified items
            if (!sel.isEmpty()) {
                TPlaylistItem* parent = static_cast<TPlaylistItem*>(
                            sel.at(0)->parent());
                if (!modified.contains(parent)) {
                    modified.append(parent);
                }
            }
        } else {
            WZDEBUG("Selection did not move");
        }

        // Add new parent to modified items
        if (!sel.isEmpty()) {
            TPlaylistItem* parent = static_cast<TPlaylistItem*>(
                        sel.at(0)->parent());
            if (!modified.contains(parent)) {
                modified.append(parent);
            }
        }

        // Set modified
        for(int i = 0; i < modified.count(); i++) {
            static_cast<TPlaylistItem*>(modified.at(i))->setModified();
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
    TPlaylistItem* p = static_cast<TPlaylistItem*>(parent.internalPointer());

    if (sortSection == TPlaylistItem::COL_ORDER) {
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
                TPlaylistItem* item = p->plChild(i);
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

    TPlaylistItem* p = static_cast<TPlaylistItem*>(parent.internalPointer());

    if (sortSection == TPlaylistItem::COL_ORDER) {
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
            TPlaylistItem* item = p->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
        for (int i = end; i >= start; i--) {
            p->plChild(i)->setOrder(i + 1);
        }
        for (int i = start - 1; i >= 0; i--) {
            TPlaylistItem* item = p->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
    }
}

bool TPlaylistWidget::removeFromDisk(const QString& filename,
                                     const QString& playingFile) {

    QFileInfo fi(filename);
    if (!fi.exists()) {
        if (fi.fileName() == TConfig::WZPLAYLIST) {
            fi.setFile(fi.absolutePath());
            if (!fi.exists()) {
                return true;
            }
        } else {
            return true;
        }
    }

    if (!fi.isSymLink() && fi.isDir()) {
        if (!TWZFiles::directoryIsEmpty(filename)) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Skipping delete of directory '%1',"
                   " it does not seem to be empty.").arg(filename));
            return false;
        }
    }

    // Ask for confirmation
    int res = QMessageBox::question(this, tr("Confirm delete from disk"),
        tr("You're about to delete '%1' from your disk.").arg(filename)
        + "<br>"
        + tr("This action cannot be undone.")
        + "<br>"
        + "Are you sure you want to proceed?",
        QMessageBox::Yes, QMessageBox::No);

    if (res != QMessageBox::Yes) {
        WZINFO("Canceled delete of '" + filename + "'");
        return false;
    }

    if (fi.isSymLink() || fi.isFile()) {
        // Cannot delete file on Windows when in use.
        // MPV will go to 100% cpu when deleting a used image file.
        if (filename == playingFile
            && player->state() != Player::STATE_STOPPED) {
            player->stop();
        }

        if (QFile::remove(filename)) {
            WZINFO("Removed file '" + filename + "' from disk");
            return true;
        }
    } else if (fi.dir().rmdir(fi.fileName())) {
        WZINFO("Removed directory '" + filename + "' from disk");
        return true;
    }

    WZERROR("Failed to remove '" + filename + "' from disk");
    QMessageBox::warning(this, tr("Delete failed"),
                         tr("Failed to delete '%1' from disk").arg(filename));
    return false;
}

void TPlaylistWidget::removeSelected(bool deleteFromDisk) {
    WZDEBUG("");

    // Save currently playing item, which might be deleted
    QString playing = playingFile();

    // Move new current out of selection
    TPlaylistItem* newCurrent = plCurrentItem();
    do {
        newCurrent = getNextItem(newCurrent, false);
    } while (newCurrent && newCurrent->isSelected());

    // Delete the selection
    TPlaylistItem* root = this->root();
    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (i != root
            && (!deleteFromDisk
                || removeFromDisk(i->filename(), playing))) {
            WZINFO("removing '" + i->filename() + "' from playlist");

            TPlaylistItem* parent = i->plParent();

            // Blacklist item if WZPlaylist
            if (!deleteFromDisk
                    && parent
                    && parent->isWZPlaylist()
                    && QFileInfo(parent->filename()).absolutePath().compare(
                        QFileInfo(i->filename()).absolutePath(),
                        caseSensitiveFileNames) == 0) {

                // A playlist may contain multiple identical items
                bool multipleItems = false;
                for(int c = 0; c < parent->childCount(); c++) {
                    TPlaylistItem* s = parent->plChild(c);
                    if (s != i && s->compareFilename(*i) == 0) {
                        multipleItems = true;
                        break;
                    }
                }

                if (!multipleItems) {
                    parent->blacklist(i->fname());
                }
            }
            delete i;

            // Clean up wzplaylists in empty folders
            while (parent && parent->childCount() == 0 && parent != root) {
                TPlaylistItem* gp = parent->plParent();
                if (parent == newCurrent) {
                    newCurrent = gp;
                }
                if (parent->isWZPlaylist()
                        && QFile::remove(parent->filename())) {
                    WZINFO("Removed playlist '" + parent->filename()
                           + "' from disk");
                }

                delete parent;
                parent = gp;
            }

            if (parent) {
                parent->setModified();
            }
        }
        it++;
    }

    playingItem = findFilename(playing);
    if (newCurrent && newCurrent != root) {
        setCurrentItem(newCurrent);
    } else {
        setCurrentItem(firstPlaylistItem());
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

void TPlaylistWidget::resizeNameColumn(TPlaylistItem* item, int level) {

    if (item) {
        level++;
        for(int i = 0; i < item->childCount(); i++) {
            TPlaylistItem* child = item->plChild(i);
            child->setSizeHintName(level);
            if (child->isExpanded() && child->childCount()) {
                resizeNameColumn(child, level);
            }
        }
    }
}

void TPlaylistWidget::resizeNameColumnAll() {
    resizeNameColumn(root(), 0);
}

void TPlaylistWidget::onItemExpanded(QTreeWidgetItem* i) {

    TPlaylistItem* item = static_cast<TPlaylistItem*>(i);
    if (!wordWrapTimer.isActive()) {
        resizeNameColumn(item, item->getLevel());
    }
}

void TPlaylistWidget::onSectionResized(int logicalIndex, int, int) {

    if (logicalIndex == TPlaylistItem::COL_NAME) {
        wordWrapTimer.start();
    }
}

TPlaylistItem* TPlaylistWidget::validateItem(TPlaylistItem* item) {

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

TPlaylistItem* TPlaylistWidget::add(TPlaylistItem* item,
                                    TPlaylistItem* target) {
    WZTRACE(QString("Child count %1").arg(item->childCount()));

    // Validate target is still valid
    target = validateItem(target);

    // Get parent to insert into and index into parent
    TPlaylistItem* parent;
    int idx = 0;
    if (target) {
        if (target->isFolder()) {
            parent = target;
        } else {
            parent = target->plParent();
            if (parent) {
                idx = parent->indexOfChild(target);
            }
        }
    } else {
        parent = root();
        idx = parent->childCount();
    }

    if (parent == 0 || (parent == root() && parent->childCount() == 0)) {
        // Drop into empty root

        // Remove single folder in root
        if (item->childCount() == 1 && item->child(0)->childCount()) {
            WZTRACE("Removing single folder in root");
            TPlaylistItem* old = item;
            item = static_cast<TPlaylistItem*>(item->takeChild(0));
            delete old;
        }

        // Invalidate playing_item
        playingItem = 0;

        // Delete old root
        setRootIndex(QModelIndex());
        delete takeTopLevelItem(0);
        clearSelection();

        // Disable sort
        setSort(-1, sortOrder);

        // Set new item as root
        item->setFlags(ROOT_FLAGS);
        addTopLevelItem(item);
        setRootIndex(model()->index(0, 0));

        // Sort
        if (item->isWZPlaylist() || !item->isPlaylist()) {
            sortSection = TPlaylistItem::COL_NAME;
            sortOrder = Qt::AscendingOrder;
        } else {
            sortSection = TPlaylistItem::COL_ORDER;
            sortOrder = Qt::AscendingOrder;
        }
        setSort(sortSection, sortOrder);

        if (item->childCount()) {
            setCurrentItem(item->child(0));
            wordWrapTimer.start();
        }

        if (item->modified()) {
            emit modifiedChanged();
        }
    } else {
        // Collect and prepare children of item
        clearSelection();
        QList<QTreeWidgetItem*> children;
        while (item->childCount()) {
            TPlaylistItem* child = item->plTakeChild(0);
            child->setModified(true, true, false);
            child->setSelected(true);
            children.append(child);
        }

        delete item;
        // Notify TPlaylist::onThreadFinished() root is still alive
        item = 0;

        // Disable sort
        int savedSortSection = sortSection;
        setSort(-1, sortOrder);

        // Insert children
        parent->insertChildren(idx, children);
        setSort(savedSortSection, sortOrder);

        // Update size hint name column
        int level = parent->getLevel() + 1;
        for(int i = 0; i < children.count(); i++) {
            static_cast<TPlaylistItem*>(children.at(i))->setSizeHintName(level);
        }

        parent->setModified();
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

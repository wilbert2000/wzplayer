#include "gui/playlist/playlistwidget.h"

#include "gui/playlist/addfilesthread.h"
#include "gui/playlist/playlistitem.h"
#include "gui/mainwindow.h"
#include "gui/msg.h"
#include "player/player.h"
#include "qtfilecopier/qtcopydialog.h"
#include "qtfilecopier/qtfilecopier.h"
#include "wzfiles.h"
#include "images.h"
#include "iconprovider.h"
#include "extensions.h"
#include "wztimer.h"
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


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPlaylistWidget)

namespace Gui {
namespace Playlist {

TPlaylistWidget::TPlaylistWidget(QWidget* parent,
                                 TMainWindow* mainWindow,
                                 const QString& name,
                                 const QString& shortName,
                                 const QString& tranName,
                                 bool favList) :
    QTreeWidget(parent),
    playingItem(0),
    sortSection(-1),
    sortOrder(Qt::AscendingOrder),
    addFilesThread(0),
    addFilesRestartThread(false),
    isFavList(favList),
    sortSectionSaved(-1),
    sortOrderSaved(Qt::AscendingOrder),
    scrollToCurrent(false),
    fileCopier(0),
    copyDialog(0) {

    setObjectName(name);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setIconSize(iconProvider.iconSize);

    setFocusPolicy(Qt::StrongFocus);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setColumnCount(TPlaylistItem::COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Ext") << tr("Length")
                    << tr("#"));
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(TPlaylistItem::COL_NAME,
                                   QHeaderView::Stretch);
    header()->setSectionResizeMode(TPlaylistItem::COL_EXT,
                                   QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(TPlaylistItem::COL_LENGTH,
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
    wordWrapTimer.setInterval(0);
    connect(&wordWrapTimer, &QTimer::timeout,
            this, &TPlaylistWidget::onWordWrapTimeout);

    // Scroll to current item
    scrollToCurrentItemTimer = new TWZTimer(this, shortName
                                            + "_scroll_to_current_timer");
    scrollToCurrentItemTimer->setSingleShot(true);
    scrollToCurrentItemTimer->setInterval(50);
    connect(scrollToCurrentItemTimer, &QTimer::timeout,
            this, &TPlaylistWidget::scrollToCurrentItem);
    connect(mainWindow, &TMainWindow::resizedMainWindow,
            scrollToCurrentItemTimer, &TWZTimer::logStart);

    connect(header(), &QHeaderView::sectionResized,
            this, &TPlaylistWidget::onSectionResized);

    setAutoExpandDelay(750);
    connect(this, &TPlaylistWidget::itemExpanded,
            this, &TPlaylistWidget::onItemExpanded);

    header()->setSectionsClickable(true);
    connect(header(), &QHeaderView::sectionClicked,
            this, &TPlaylistWidget::onSectionClicked);

    // Columns menu
    columnsMenu = new Gui::Action::Menu::TMenu(this,
        shortName + "_columns_menu",
        tr("View columns %1").arg(tranName.toLower()));
    QStringList colnames = QStringList() << "name" << "ext" << "length"
                        << "order";
    for(int i = 0; i < columnCount(); i++) {
        QAction* a = new QAction(headerItem()->text(i), columnsMenu);
        a->setObjectName(shortName + "_toggle_col_" + colnames.at(i));
        a->setCheckable(true);
        a->setChecked(i == 0);
        a->setData(i);
        columnsMenu->addAction(a);
    }
    connect(columnsMenu, &QMenu::triggered,
            this, &TPlaylistWidget::onColumnMenuTriggered);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested,
            columnsMenu, &Gui::Action::Menu::TMenu::execSlot);
}

TPlaylistWidget::~TPlaylistWidget() {

    // Prevent onThreadFinished handling results
    addFilesThread = 0;
    abortFileCopier();
}

void TPlaylistWidget::abort() {

    if (playingItem) {
        setPlayingItem(0);
    }
    bool b = isBusy();
    abortFileCopier();
    abortAddFilesThread();
    if (b != isBusy()) {
        emit busyChanged();
    }
}

void TPlaylistWidget::abortAddFilesThread() {

    if (addFilesThread) {
        WZINFOOBJ("Aborting add files thread");
        addFilesStartPlay = false;
        addFilesRestartThread = false;
        addFilesThread->abort();
        addFilesThread = 0;
    }
}

void TPlaylistWidget::onAddFilesThreadFinished() {
    WZTRACEOBJ("");

    if (addFilesThread == 0) {
        // Only during destruction, no need to enable actions
        WZDEBUGOBJ("Add files thread is gone");
        return;
    }

    // Get items from thread
    TPlaylistItem* root = addFilesThread->root;
    addFilesThread->root = 0;
    if (!addFilesThread->latestDir.isEmpty()) {
        emit latestDirChanged(addFilesThread->latestDir);
    }

    // Clean up
    delete addFilesThread;
    addFilesThread = 0;

    if (root == 0) {
        // Thread aborted
        if (addFilesRestartThread) {
            WZDEBUGOBJ("Thread aborted, starting new request");
            addFilesStartThread();
        } else {
            WZDEBUGOBJ("Thread aborted");
            addFileList.clear();
            emit busyChanged();
        }
        return;
    }

    QString msg = addFileList.count() == 1 ? addFileList.at(0) : "";
    addFileList.clear();

    // Found nothing to play?
    if (root->childCount() == 0) {
        delete root;
        if (msg.isEmpty()) {
            msg = tr("Found nothing to play.");
        } else {
            msg = tr("Found no files to play in '%1'.").arg(msg);
        }
        WZINFO(msg);
        emit nothingToPlay(msg);
        emit busyChanged();
        return;
    }

    // add() returns a newly created root when all items are replaced
    // or 0 when the new items are inserted into the existing root.
    root = add(root, addFilesTarget, addFilesTargetIndex);
    if (root) {
        WZINFOOBJ("emit rootFilenameChanged(\"" + root->filename() + "\")");
        emit rootFilenameChanged(root->filename());
    }

    emit busyChanged();
    emit addedItems();

    if (addFilesStartPlay) {
        if (!addFilesFileToPlay.isEmpty()) {
            TPlaylistItem* item = findFilename(addFilesFileToPlay);
            if (item) {
                emit playItem(item);
                return;
            }
        }
        emit startPlay();
    }
}

void TPlaylistWidget::addFilesStartThread() {

    if (addFilesThread) {
        // Thread still running, abort it and restart it in
        // onAddFilesThreadFinished()
        WZDEBUGOBJ("Add files thread still running. Aborting it...");
        addFilesRestartThread = true;
        addFilesThread->abort();
    } else {
        WZDEBUGOBJ("Starting add files thread");
        addFilesRestartThread = false;

        // Allow single image
        bool addImages = Settings::pref->addImages
                || ((addFileList.count() == 1)
                    && extensions.isImage(addFileList.at(0)));

        addFilesThread = new TAddFilesThread(this,
                                             addFileList,
                                             Settings::pref->nameBlacklist,
                                             Settings::pref->addDirectories,
                                             Settings::pref->addVideo,
                                             Settings::pref->addAudio,
                                             Settings::pref->addPlaylists,
                                             addImages,
                                             isFavList);

        connect(addFilesThread, &TAddFilesThread::finished,
                this, &TPlaylistWidget::onAddFilesThreadFinished);
        connect(addFilesThread, &TAddFilesThread::displayMessage,
                msgSlot, &TMsgSlot::msg);

        addFilesThread->start();
        emit busyChanged();
    }
}

void TPlaylistWidget::addFiles(const QStringList& files,
                               TPlaylistItem* target,
                               int targetIndex,
                               bool startPlay,
                               const QString& fileToPlay) {
    WZDOBJ << files << "startPlay" << startPlay;

    addFileList = files;
    addFilesTarget = target;
    addFilesTargetIndex = targetIndex;
    addFilesStartPlay = startPlay;
    addFilesFileToPlay = fileToPlay;

    addFilesStartThread();
}

void TPlaylistWidget::abortFileCopier() {

    if (copyDialog) {
        copyDialog->deleteLater();
        copyDialog = 0;
    }
    if (fileCopier) {
        WZINFOOBJ("Aborting file copier");
        fileCopier->cancelAll();
        fileCopier->deleteLater();
        fileCopier = 0;
    }
}

void TPlaylistWidget::clr() {
    WZTRACEOBJ("");

    abort();
    clear();

    // Create a TPlaylistItem root
    addTopLevelItem(new TPlaylistItem());
    setRootIndex(model()->index(0, 0));
}

// Called by root item to signal modified changed
void TPlaylistWidget::emitModifiedChanged() {
    WZTRACEOBJ("");
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

int TPlaylistWidget::countChildren(TPlaylistItem* w) const {

    if (w) {
        if (w->childCount()) {
            int count = 0;
            for(int c = 0; c < w->childCount(); c++) {
                count += countChildren(w->plChild(c));
            }
            return count;
        }
        if (!w->isFolder()) {
            return 1;
        }
    }
    return 0;
}

int TPlaylistWidget::countChildren() const {
    return countChildren(root());
}

bool TPlaylistWidget::hasItems() const {
    return root()->childCount();
}

bool TPlaylistWidget::hasPlayableItems(TPlaylistItem* item) const {

    for(int i = 0; i < item->childCount(); i++) {
        TPlaylistItem* child = item->plChild(i);
        if (!child->isFolder() || hasPlayableItems(child)) {
            return true;
        }
    }
    return false;
}

bool TPlaylistWidget::hasPlayableItems() const {
    return hasPlayableItems(root());
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

    if (playingItem) {
        return playingItem->filename();
    }
    return "";
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
        if (playingItem == item) {
            setCurrent = false;
        } else {
            // Only set current item, when playingItem was current item
            // or when current = 0
            setCurrent = playingItem == currentItem()
                    || currentItem() == 0;
            // Set state previous playing item
            if (playingItem->state() != PSTATE_STOPPED
                    && playingItem->state() != PSTATE_FAILED) {
                playingItem->setState(PSTATE_STOPPED);
            }
        }
    }

    bool changed = item != playingItem;
    bool updated = changed || (playingItem && playingItem->state() != state);

    playingItem = item;

    if (playingItem) {
        if (state != playingItem->state()) {
            playingItem->setState(state);
        }
        if (setCurrent) {
            setCurrentItem(playingItem);
            if (wordWrapTimer.isActive()) {
                scrollToCurrent = true;
            } else {
                scrollToCurrentItemTimer->logStart();
            }
        }
    }

    if (updated) {
        if (changed) {
            emit playingItemChanged(playingItem);
        }
        emit playingItemUpdated(playingItem);
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

void TPlaylistWidget::editStart(TPlaylistItem* current) {
    WZTRACEOBJ("");

    // Select COL_NAME
    setCurrentItem(current, TPlaylistItem::COL_NAME);
    // Start editor
    QKeyEvent event(QEvent::KeyPress, Qt::Key_F2, Qt::NoModifier);
    keyPressEvent(&event);
    // Unselect the extension
    if (!current->isWZPlaylist()) {
        QString ext = current->extension();
        if (!ext.isEmpty() && ext.length() < 6) {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Left, Qt::ShiftModifier);
            QWidget* editor = qApp->focusWidget();
            for(int i = ext.length(); i >= 0; i--) {
                qApp->sendEvent(editor, &e);
            }
        }
    }
}

void TPlaylistWidget::editName() {
    WZTRACEOBJ("");

    TPlaylistItem* current = plCurrentItem();
    if (current) {
        current->setEditName();
        editStart(current);
    }
}

void TPlaylistWidget::editURL() {
    WZTRACEOBJ("");

    TPlaylistItem* current = plCurrentItem();
    if (current) {
        current->setEditURL();
        editStart(current);
    }
}

void TPlaylistWidget::updateItemPath() {
    WZTRACEOBJ("");

    TPlaylistItem* parent = itemToUpdatePath->plParent();
    int idx = parent->indexOfChild(itemToUpdatePath);
    parent->takeChild(idx);
    // Modified already set

    QString path = QFileInfo(itemToUpdatePath->path()).absolutePath();
    parent = findFilename(path);
    if (parent) {
        parent->addChild(itemToUpdatePath);
        parent->setModified();
        setCurrentItem(itemToUpdatePath);
        scrollToItem(itemToUpdatePath);
        WZINFO(QString("Item moved to '%1'").arg(path));
    } else {
        WZINFO(QString("Item no longer in playlist"));
        QMessageBox::information(this, tr("Item moved"),
            tr("Item moved to folder '%2' and is no longer part of this"
               " playlist.")
            .arg(path));
        delete itemToUpdatePath;
    }
    itemToUpdatePath = 0;
}

bool TPlaylistWidget::droppingOnItself(QDropEvent *event,
                                       const QModelIndex &index) {

    if (event->source() == this
        && event->possibleActions() & Qt::MoveAction
        && event->dropAction() == Qt::MoveAction) {
        QModelIndexList selected = selectedIndexes();
        QModelIndex root = rootIndex();
        QModelIndex child = index;
        while (child.isValid() && child != root) {
            if (selected.contains(child)) {
                WZDEBUG("Dropping on itself");
                return true;
            }
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
    if (supportedDropActions() & event->dropAction()) {
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
    WZTRACEOBJ(QString("Error %2").arg(error));

    // The copy dialog should already be hidden by QtCopyDialogPrivate::done()
    delete copyDialog;
    copyDialog = 0;
    fileCopier->deleteLater();
    fileCopier = 0;
    emit busyChanged();
}

bool TPlaylistWidget::addDroppedItem(const QString& source,
                                     const QString& dest,
                                     TPlaylistItem* item) {

    QFileInfo fid(dest);
    if (fid.exists()) {
        TPlaylistItem* parent = findFilename(fid.absolutePath());
        if (parent) {
            // Remove existing items
            // TODO: check
            for(int i = parent->childCount() - 1; i >= 0; i--) {
                if (dest.compare(parent->plChild(i)->path(),
                                 caseSensitiveFileNames) == 0) {
                    delete parent->takeChild(i);
                }
            }

            // Add item
            parent->addChild(item);
            item->updateFilename(source, dest);

            // Don't mark playlists as modified
            if (item->isFolder()) {
                parent->setModified();
            } else {
                item->setModified();
            }

            // Set first as currentItem
            if (selectedItems().count() == 0) {
                setCurrentItem(item);
            }
            item->setSelected(true);

            // Accept item
            return true;
        }
        WZERROR(QString("Destination '%1' not found in playlist.")
                .arg(fid.absolutePath()));
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
        WZINFOOBJ(QString("Canceled copy '%1' to '%2'").arg(source).arg(dest));
    } else {
        TPlaylistItem* sourceItem = findFilename(source);
        if (sourceItem) {
            // Add clone to parent
            TPlaylistItem* destItem = sourceItem->clone();
            if (addDroppedItem(source, dest, destItem)) {
                destItem->setState(PSTATE_STOPPED);
                WZINFOOBJ(QString("Copied '%1' to '%2'").arg(source).arg(dest));
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
            // Remove source from parent
            TPlaylistItem* parent = sourceItem->plParent();
            int idx = parent->indexOfChild(sourceItem);
            parent->takeChild(idx);
            parent->setModified();

            // Add it to dest
            if (addDroppedItem(source, dest, sourceItem)) {
                if (stoppedFilename == dest) {
                    if (player->state() == Player::STATE_STOPPED) {
                        setCurrentItem(stoppedItem);
                        WZDEBUG("Posting play");
                        QTimer::singleShot(0, this->parent(), SLOT(play()));
                    }
                    stoppedFilename = "";
                }
                WZINFO(QString("Moved '%1' to '%2'").arg(source).arg(dest));
            }
        } else {
            WZWARN(QString("Source '%1' not found in playlist").arg(source));
        }
    }
}

void TPlaylistWidget::moveItem(TPlaylistItem* item,
                               TPlaylistItem* target,
                               int& targetIndex) {
    WZTRACEOBJ(QString("Move '%1' to '%2' idx %3")
               .arg(item->filename())
               .arg(target->filename())
               .arg(targetIndex));

    bool isCurrentItem = item == plCurrentItem();
    bool isLink = item->isLink(); // Get before remove from parent

    // Remove item from parent
    {
        TPlaylistItem* parent = item->plParent();
        if (parent) {
            int idx = parent->indexOfChild(item);
            if (idx < targetIndex) {
                WZTRACEOBJ("Decrementing target index");
                targetIndex--;
            } else if (idx == targetIndex) {
                WZDEBUG("Drop on self");
                targetIndex++;
                item->setSelected(true);
                return;
            }
            parent->takeChild(idx);
        }
    }

    if (!isLink) {
        QString source = QDir::toNativeSeparators(item->path());
        QString dest = QDir::toNativeSeparators(target->path());
        if (!dest.endsWith(QDir::separator())) {
            dest += QDir::separator();
        }
        dest += QFileInfo(source).fileName();
        if (source == dest) {
            // TODO: request to set sort to COL_ORDER
            WZTRACEOBJ("Moving idx only");
        } else {
            item->updateFilename(source, dest);
        }
    }

    WZTRACEOBJ(QString("Inserting item '%1' at idx %2 into '%3'")
               .arg(item->filename()).arg(targetIndex).arg(target->filename()));
    target->insertChild(targetIndex, item);

    // Don't mark playlists as modified
    if (item->isFolder()) {
        target->setModified();
    } else {
        item->setModified();
    }
    item->setSelected(true);
    if (isCurrentItem) {
        setCurrentItem(item);
    }
    targetIndex++;
}

void TPlaylistWidget::copyItem(TPlaylistItem* item,
                               TPlaylistItem* target,
                               int& targetIndex) {
    WZTRACEOBJ(QString("Copy '%1' to '%2' idx %3")
               .arg(item->filename())
               .arg(target->filename())
               .arg(targetIndex));

    bool isCurrentItem = item == plCurrentItem();
    TPlaylistItem* destItem;
    if (dropSourceWidget == 0) {
        destItem = item;
    } else {
        destItem = item->clone();
    }
    target->insertChild(targetIndex, destItem);

    QString source = QDir::toNativeSeparators(item->path());
    QFileInfo fi(source);
    if (fi.exists()) {
        QString dest = QDir::toNativeSeparators(target->path());
        if (!dest.endsWith(QDir::separator())) {
            dest += QDir::separator();
        }
        dest += fi.fileName();
        if (source != dest) {
            item->updateFilename(source, dest);
        } // else set sort to COL_ORDER?
    }

    // Don't mark playlists as modified
    if (destItem->isFolder()) {
        target->setModified();
    } else {
        destItem->setModified();
    }
    destItem->setSelected(true);
    if (isCurrentItem) {
        setCurrentItem(destItem);
    }
    targetIndex++;
    WZINFO(QString("Moved '%1' to '%2' inside '%3'")
           .arg(source).arg(destItem->filename()).arg(target->filename()));
}

QList<QTreeWidgetItem*> TPlaylistWidget::getItemsFromMimeData(
        const QMimeData* mimeData) {

    QList<QTreeWidgetItem*> items;
    QByteArray bytes = mimeData->data(
                "application/x-qabstractitemmodeldatalist");
    QDataStream stream(&bytes, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        TPlaylistItem* item = new TPlaylistItem();
        stream >> *item;
        items << item;
    }

    return items;
}

bool TPlaylistWidget::drop(TPlaylistItem* target,
                           int targetIndex,
                           QDropEvent* event) {
    WZDEBUG(QString("Selected target '%1'").arg(target->filename()));

    // Collect the dropped files
    QString targetFilename = target->path();
    QDir targetDir(targetFilename);
    bool targetIsDir = !targetFilename.isEmpty() && targetDir.exists();
    QStringList files;
    QList<TPlaylistItem*> nonFSItems;

    {
        QList<QTreeWidgetItem*> sourceItems;
        if (dropSourceWidget == 0) {
            WZDEBUGOBJ("Drop originates from other app");
            // Items not from this app. Get items from mimeData
            sourceItems = getItemsFromMimeData(event->mimeData());
        } else {
            WZDOBJ << "Drop originates from TPlaylistWidget"
                   << dropSourceWidget->objectName();
            // TODO: there seems to be a "draggable" selection
            sourceItems = dropSourceWidget->selectedItems();
        }

        for(int i = 0; i < sourceItems.length(); i++) {
            TPlaylistItem* item = static_cast<TPlaylistItem*>(sourceItems.at(i));
            if (targetIsDir && !item->isLink()) {
                QFileInfo source(item->path());
                if (event->dropAction() == Qt::CopyAction
                        || event->dropAction() == Qt::LinkAction
                        || source.dir() != targetDir) {
                    files.append(source.absoluteFilePath());
                    WZTRACEOBJ(QString("Added '%1' to file copier")
                               .arg(source.absoluteFilePath()));
                    continue;
                }
            }
            // No need for file system action
            WZTOBJ << "No file system" << event->dropAction()
                   << "for" << item->filename();
            nonFSItems.append(item);
        }
    }

    // move/copyItem() and the finished event handlers will select the new items
    clearSelection();

    // Handle the items that don't need a file sytem copy/move/link
    for(int i = 0; i < nonFSItems.count(); i++) {
        if(event->dropAction() == Qt::MoveAction) {
            moveItem(nonFSItems.at(i), target, targetIndex);
        } else {
            copyItem(nonFSItems.at(i), target, targetIndex);
        }
    }

    if (files.count() == 0) {
        WZDEBUG("No files left to copy, move or link");
        return true;
    }

    // Setup the file copier
    fileCopier = new QtFileCopier(this);
    copyDialog = new QtCopyDialog(this);
    copyDialog->setFileCopier(fileCopier);
    connect(fileCopier, &QtFileCopier::done,
            this, &TPlaylistWidget::onDropDone,
            Qt::QueuedConnection);

    emit busyChanged();

    // Pass files to copier
    if (event->dropAction() == Qt::MoveAction) {
        stoppedFilename = "";
        connect(fileCopier, &QtFileCopier::aboutToStart,
                this, &TPlaylistWidget::onMoveAboutToStart);
        connect(fileCopier, &QtFileCopier::finished,
                this, &TPlaylistWidget::onMoveFinished);
        fileCopier->moveFiles(files, target->path(), 0);
    } else {
        connect(fileCopier, &QtFileCopier::finished,
                this, &TPlaylistWidget::onCopyFinished);
        QtFileCopier::CopyFlags flags;
        if (event->dropAction() == Qt::LinkAction) {
            flags = QtFileCopier::MakeLinks;
        } else {
            flags = 0;
        }
        fileCopier->copyFiles(files, target->path(), flags);
    }

    return true;
}

void TPlaylistWidget::dropURLs(TPlaylistItem* target,
                               int row,
                               const QMimeData* mimeData) {

    QStringList files;
    foreach(const QUrl& url, mimeData->urls()) {
        files.append(url.toString());
    }

    if (files.count()) {
        addFiles(files, target, row);
    }
}

bool TPlaylistWidget::hasModelMimeType(const QMimeData* mime) {
    return mime->hasFormat("application/x-qabstractitemmodeldatalist");
}

void TPlaylistWidget::dropEvent(QDropEvent* event) {
    WZDOBJ << event->mimeData()->formats();

    bool gotModelMimeType = hasModelMimeType(event->mimeData());
    if (gotModelMimeType || event->mimeData()->hasUrls()) {
        QModelIndex index;
        int col = -1;
        int row = -1;
        if (dropOn(event, &row, &col, &index)) {
            event->accept();
            if (copyDialog) {
                copyDialog->show();
                copyDialog->raise();
                QMessageBox::information(copyDialog, tr("Information"),
                    tr("A copy or move action is still in progress. You can"
                       " retry after it has finished."));
            } else {
                TPlaylistItem* target = static_cast<TPlaylistItem*>(
                            index.internalPointer());
                dropSourceWidget = dynamic_cast<TPlaylistWidget*>(
                            event->source());
                if (gotModelMimeType) {
                    drop(target, row, event);
                } else {
                    dropURLs(target, row, event->mimeData());
                }
                // Don't want QAbstractItemView to delete src
                // because it was "moved"
                event->setDropAction(Qt::CopyAction);
            }
        }
    }

    QTreeWidget::dropEvent(event);
}

void TPlaylistWidget::dragEnterEvent(QDragEnterEvent* event) {
    WZT << "Poss" << event->possibleActions()
        << "Prop" << event->proposedAction()
        << "Mime types" << event->mimeData()->formats();

    // See QAbstractItemView::dragEnterEvent()
    if (event->isAccepted()) {
        WZTOBJ << "Event already accepted";
    } else if (hasModelMimeType(event->mimeData())) {
        setState(DraggingState);
        if (event->source() != 0) {
            if (event->proposedAction()) {
                event->acceptProposedAction();
                WZTOBJ << "Accepted proposed" << event->proposedAction();
            } else if (event->source() == this) {
                event->setDropAction(Qt::MoveAction);
                event->accept();
                WZTOBJ << "Accepted move";
            } else {
                event->setDropAction(Qt::LinkAction);
                event->accept();
                WZTOBJ << "Accepted link";
            }
        } else if (event->proposedAction() & (Qt::CopyAction | Qt::LinkAction)) {
            event->acceptProposedAction();
            WZTOBJ << "Accepted proposed" << event->proposedAction();
        } else {
            event->setDropAction(Qt::CopyAction);
            event->accept();
            WZTOBJ << "Accepted copy";
        }
    } else if (event->mimeData()->hasUrls()) {
        if (event->proposedAction() & (Qt::CopyAction | Qt::LinkAction)) {
            setState(DraggingState);
            event->acceptProposedAction();
            WZTOBJ << "Accepted proposed" << event->proposedAction();
        } else if (event->possibleActions() & Qt::CopyAction) {
            setState(DraggingState);
            event->setDropAction(Qt::CopyAction);
            event->accept();
            WZTOBJ << "Accepted copy";
        } else if (event->possibleActions() & Qt::LinkAction) {
            setState(DraggingState);
            event->setDropAction(Qt::LinkAction);
            event->accept();
            WZTOBJ << "Accepted link";
        } else {
            event->ignore();
            WZTOBJ << "Ignoring proposed" << event->proposedAction();
        }
    } else {
        WZTOBJ << "Ignoring mime types" << event->mimeData()->formats();
        event->ignore();
    }
}

Qt::DropActions TPlaylistWidget::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

QStringList TPlaylistWidget::mimeTypes() const {

    QStringList s = QTreeWidget::mimeTypes();
    s.append("text/uri-list");
    //s.append("text/plain");
    return s;
}


bool TPlaylistWidget::dropMimeData(QTreeWidgetItem *parent,
                                   int index,
                                   const QMimeData *data,
                                   Qt::DropAction action) {
    WZTOBJ;
    return QTreeWidget::dropMimeData(parent, index, data, action);
}

void TPlaylistWidget::rowsAboutToBeRemoved(const QModelIndex &parent,
                                           int start, int end) {

    QTreeWidget::rowsAboutToBeRemoved(parent, start, end);

    if (!parent.isValid()) {
        return;
    }

    TPlaylistItem* parentItem = static_cast<TPlaylistItem*>(
                parent.internalPointer());

    if (sortSection == TPlaylistItem::COL_ORDER) {
        int d = end - start + 1;
        if (sortOrder == Qt::AscendingOrder) {
            d--;
            for (int i = end + 1; i < parentItem->childCount(); i++) {
                parentItem->plChild(i)->setOrder(i - d);
            }
        } else {
            d = parentItem->childCount() - d;
            for (int i = start - 1; i >= 0; i--) {
                parentItem->plChild(i)->setOrder(d - i);
            }
        }
    } else {
        for (int r = start; r <= end; r++) {
            int order = parentItem->plChild(r)->order();
            for (int i = parentItem->childCount() - 1; i >= 0; i--) {
                TPlaylistItem* item = parentItem->plChild(i);
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

    QTreeWidget::rowsInserted(parent, start, end);

    if (!parent.isValid()) {
        return;
    }

    TPlaylistItem* parentItem = static_cast<TPlaylistItem*>(
                parent.internalPointer());

    if (sortSection == TPlaylistItem::COL_ORDER) {
        if (sortOrder == Qt::AscendingOrder) {
            for (int i = parentItem->childCount() - 1; i >= start; i--) {
                parentItem->plChild(i)->setOrder(i + 1);
            }
        } else {
            int order = parentItem->childCount();
            for (int i = 0; i <= end; i++) {
                parentItem->plChild(i)->setOrder(order--);
            }
        }
    } else {
        int d = end - start + 1;
        for (int i = parentItem->childCount() - 1; i > end; i--) {
            TPlaylistItem* item = parentItem->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
        for (int i = end; i >= start; i--) {
            parentItem->plChild(i)->setOrder(i + 1);
        }
        for (int i = start - 1; i >= 0; i--) {
            TPlaylistItem* item = parentItem->plChild(i);
            int o = item->order();
            if (o > start) {
                item->setOrder(o + d);
            }
        }
    }
}

bool TPlaylistWidget::removeItemFromDisk(TPlaylistItem* item) {
    WZTRACEOBJ("");

    QFileInfo fi(item->path());

    // Test dir empty
    QString filename = QDir::toNativeSeparators(fi.absoluteFilePath());
    if (!fi.isSymLink() && fi.isDir()) {
        if (!TWZFiles::directoryIsEmpty(filename)) {
            QMessageBox::warning(this, tr("Warning"),
                tr("Skipping delete of directory '%1',"
                   " it does not seem to be empty.").arg(filename));
            return false;
        }
    }

    if (!yesToAll) {
        // Ask for confirmation
        int res = QMessageBox::question(this, tr("Confirm delete from disk"),
            tr("You're about to delete %1'%2' from disk.")
            .arg(fi.isSymLink() ? tr("symbolic link ") : "").arg(filename)
            + "<br>"
            + tr("This action cannot be undone.")
            + "<br>"
            + "Are you sure you want to proceed?",
            QMessageBox::Yes, QMessageBox::YesToAll,
            QMessageBox::No | QMessageBox::Default | QMessageBox::Escape);

        if (res == QMessageBox::No) {
            WZINFO("Canceled delete of '" + filename + "'");
            return false;
        }
        if (res == QMessageBox::YesToAll) {
            WZDEBUG("Setting yesToAll");
            yesToAll = true;
        }
    }

    if (fi.isSymLink() || fi.isFile()) {
        if (QFile::remove(filename)) {
            WZINFO("Removed file '" + filename + "' from disk");
            return true;
        }
    } else if (fi.dir().rmdir(fi.fileName())) {
        WZINFO("Removed directory '" + filename + "' from disk");
        return true;
    }

    QString emsg = strerror(errno);
    WZERROR(QString("Failed to remove '%1' from disk. %2")
            .arg(filename).arg(emsg));
    QMessageBox::warning(this, tr("Delete failed"),
                         tr("Failed to delete '%1' from disk. %2")
                         .arg(filename).arg(emsg));
    return false;
}

void TPlaylistWidget::removeItem(TPlaylistItem* item,
                                 TPlaylistItem*& newCurrent,
                                 bool deleteFromDisk) {
    WZTRACEOBJ("");

    if (item == root()) {
        return;
    }

    if (item == playingItem) {
        setPlayingItem(0);
        player->stop();
    }

    if (deleteFromDisk && !item->isLink()) {
        if (!removeItemFromDisk(item)) {
            return;
        }
    }

    WZINFOOBJ("Removing '" + item->filename() + "' from playlist");

    // Blacklist item if parent is WZPlaylist
    TPlaylistItem* parent = item->plParent();
    if (!deleteFromDisk && parent && parent->isWZPlaylist()) {
        QFileInfo fi(item->filename());
        if (fi.exists() && QFileInfo(parent->filename()).dir() == fi.dir()) {

            // A playlist may contain multiple identical items
            bool multipleItems = false;
            for(int c = 0; c < parent->childCount(); c++) {
                TPlaylistItem* child = parent->plChild(c);
                if (child != item
                        && child->filename().compare(item->filename(),
                                                     caseSensitiveFileNames)
                        == 0) {
                    multipleItems = true;
                    break;
                }
            }

            if (!multipleItems) {
                parent->blacklist(item->fname());
            }
        }
    }

    delete item;

    // Clean up wzplaylists in empty folders
    while (parent && parent->childCount() == 0 && parent != root()) {
        TPlaylistItem* gp = parent->plParent();
        if (parent == newCurrent) {
            newCurrent = gp;
        }
        if (parent->isWZPlaylist()
                && QFile::remove(parent->filename())) {
            WZINFOOBJ("Removed '" + parent->filename() + "' from disk");
        }

        delete parent;
        parent = gp;
    }

    if (parent) {
        parent->setModified();
    }
}

void TPlaylistWidget::removeSelected(bool deleteFromDisk) {
    WZTRACEOBJ("");

    // Move current out of selection
    TPlaylistItem* newCurrent = plCurrentItem();
    while (newCurrent && newCurrent->isSelected()) {
        newCurrent = getNextItem(newCurrent, false);
    }

    // Delete the selection
    yesToAll = false;
    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::Selected);
    while (*it) {
        removeItem(static_cast<TPlaylistItem*>(*it),
                   newCurrent, deleteFromDisk);
    }

    // Set new current
    if (newCurrent && newCurrent != root()) {
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

void TPlaylistWidget::disableSort() {
    setSort(-1, sortOrder);
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

void TPlaylistWidget::scrollToCurrentItem() {

    scrollToCurrent = false;
    if (currentItem()) {
        WZTRACEOBJ("Scrolling to current item");
        scrollToItem(currentItem());
    } else {
        WZTRACEOBJ("No current item");
    }
}

void TPlaylistWidget::onWordWrapTimeout() {

    resizeNameColumnAll();
    if (scrollToCurrent) {
        scrollToCurrentItemTimer->logStart();
    }
}

void TPlaylistWidget::startWordWrap() {

    if (root()->childCount()) {
        wordWrapTimer.start();
    } else {
        wordWrapTimer.stop();
    }
}

void TPlaylistWidget::onItemExpanded(QTreeWidgetItem* i) {
    WZTRACEOBJ("");

    if (!wordWrapTimer.isActive()) {
        TPlaylistItem* item = static_cast<TPlaylistItem*>(i);
        resizeNameColumn(item, item->getLevel());
    }
}

void TPlaylistWidget::onSectionResized(int logicalIndex, int, int) {

    if (logicalIndex == TPlaylistItem::COL_NAME) {
        startWordWrap();
    }
}

TPlaylistItem* TPlaylistWidget::validateItem(TPlaylistItem* folder,
                                             TPlaylistItem* item) const {

    for(int i = 0; i < folder->childCount(); i++) {
        TPlaylistItem* child = folder->plChild(i);
        if (child == item) {
            return child;
        }
        if (child->childCount()) {
            child = validateItem(child, item);
            if (child) {
                return child;
            }
        }
    }

    return 0;
}

TPlaylistItem* TPlaylistWidget::validateItem(TPlaylistItem* item) const {

    if (item) {
        return validateItem(root(), item);
    }
    return 0;
}

TPlaylistItem* TPlaylistWidget::add(TPlaylistItem* item,
                                    TPlaylistItem* target,
                                    int index) {

    // Validate target is still valid
    target = validateItem(target);

    // Get parent to insert into and index into parent
    TPlaylistItem* parent;
    int idx;
    if (target) {
        if (target->isFolder()) {
            parent = target;
            if (index >= 0) {
                if (index <= parent->childCount()) {
                    idx = index;
                } else {
                    idx = parent->childCount();
                }
            } else if (sortSection == TPlaylistItem::COL_ORDER) {
                idx = parent->childCount();
            } else if (sortOrder == Qt::AscendingOrder) {
                idx = parent->childCount();
                while (idx > 0 && *(parent->plChild(idx)) < *item) {
                    idx--;
                }
            } else {
                idx = 0;
                while (idx < parent->childCount()
                       && *item < *(parent->plChild(idx))) {
                    idx++;
                }
            }
        } else {
            parent = target->plParent();
            if (parent) {
                idx = parent->indexOfChild(target);
            } else {
                // Not used
                idx = 0;
            }
        }
    } else {
        parent = root();
        idx = parent->childCount();
    }

    WZTOBJ << "target" << (target ? target->filename() : QString())
           << "index" << index << "idx" << idx;

    // Disable sort
    int currentSortSection = sortSection;
    disableSort();

    if (parent == 0 || (parent == root() && parent->childCount() == 0)) {
        // Drop into empty root

        // Remove single folder in root
        if (item->childCount() == 1 && item->child(0)->childCount()) {
            WZTRACEOBJ("Removing single folder in root");
            TPlaylistItem* old = item;
            item = static_cast<TPlaylistItem*>(item->takeChild(0));
            delete old;
        }

        WZTRACEOBJ(QString("New root '%1'").arg(item->filename()));

        // Invalidate playing_item
        if (playingItem) {
            playingItem = 0;
            emit playingItemChanged(playingItem);
            emit playingItemUpdated(playingItem);
        }

        // Delete old root
        setRootIndex(QModelIndex());
        delete takeTopLevelItem(0);
        clearSelection();

        // Set new item as root
        item->setFlags(ROOT_FLAGS);
        addTopLevelItem(item);
        setRootIndex(model()->index(0, 0));

        // Sort
        if (item->isWZPlaylist() || !item->isPlaylist()) {
            if (currentSortSection < 0) {
                // Sort not set yet. Playlist only,
                // favList has its sortOrder set in constructor
                sortSection = TPlaylistItem::COL_NAME;
                sortOrder = Qt::AscendingOrder;
                sortSectionSaved = -1;
            } else if (sortSectionSaved >= 0) {
                // Current sort is from non-wzplaylist playlist,
                // restore saved sort
                sortSection = sortSectionSaved;
                sortOrder = sortOrderSaved;
                sortSectionSaved = -1;
            } else {
                // Keep current sort
                sortSection = currentSortSection;
            }
        } else if (sortSectionSaved >= 0) {
            // Keep current sort
            sortSection = currentSortSection;
        } else {
            // Save sort for when switching back to wzplaylist
            if (currentSortSection >= 0) {
                sortSectionSaved = currentSortSection;
                sortOrderSaved = sortOrder;
            } else {
                sortSectionSaved = TPlaylistItem::COL_NAME;
                sortOrderSaved = Qt::AscendingOrder;
            }
            sortSection = TPlaylistItem::COL_ORDER;
            sortOrder = Qt::AscendingOrder;
        }
        setSort(sortSection, sortOrder);

        if (item->childCount()) {
            setCurrentItem(item->child(0));
            startWordWrap();
        }

        if (item->modified()) {
            WZTRACEOBJ("New root is modified");
            emit modifiedChanged();
        } else {
            WZTRACEOBJ("New root is not modified");
        }
    } else {
        WZTRACEOBJ(QString("Dropping %1 items into '%2'")
                   .arg(item->childCount()).arg(parent->filename()));

        // Collect and prepare children of item
        clearSelection();
        QList<QTreeWidgetItem*> children;
        while (item->childCount()) {
            TPlaylistItem* child = item->plTakeChild(0);
            child->setModified(true, true, false);
            children.append(child);
        }

        delete item;
        // Notify TPlaylist::onThreadFinished() root is still alive
        item = 0;

        // Insert children
        if (children.count()) {
            parent->insertChildren(idx, children);
            setCurrentItem(children.at(0));
        }

        // Select drop and update size hint name column
        int level = parent->getLevel() + 1;
        for(int i = 0; i < children.count(); i++) {
            TPlaylistItem* child = static_cast<TPlaylistItem*>(children.at(i));
            child->setSelected(true);
            child->setSizeHintName(level);
        }

        // Restore sort
        setSort(currentSortSection, sortOrder);

        WZTRACEOBJ("Setting parent modified");
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
        QAction* action = actions.at(c);
        bool show = action->isChecked();
        show = pref->value("COL_" + QString::number(c), show).toBool();
        action->setChecked(show);
        setColumnHidden(c, !show);
    }
}

} // namespace Playlist
} // namespace Gui

#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include "gui/playlist/playlistitem.h"
#include <QTreeWidget>
#include <QTimer>


class QSettings;
class QtFileCopier;
class QtCopyDialog;
class TWZTimer;

namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {
class TMenu;
}
}

namespace Playlist {

class TAddFilesThread;

class TPlaylistWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit TPlaylistWidget(QWidget* parent,
                             TMainWindow* mainWindow,
                             const QString& name,
                             const QString& shortName,
                             const QString& tranName,
                             bool favList);
    virtual ~TPlaylistWidget() override;

    void addFiles(const QStringList& files,
                  TPlaylistItem* target = 0,
                  int targetIndex = -1,
                  bool startPlay = false,
                  const QString& fileToPlay = "");
    void abort();

    TPlaylistItem* playingItem;
    void setPlayingItem(TPlaylistItem* item,
                        TPlaylistItemState state = PSTATE_STOPPED);

    int sortSection;
    Qt::SortOrder sortOrder;
    TPlaylistItem* itemToUpdatePath;

    TPlaylistItem* root() const {
        return static_cast<TPlaylistItem*>(topLevelItem(0));
    }

    int countItems() const;
    int countChildren() const;
    bool hasItems() const;
    bool hasPlayableItems() const;
    bool hasSingleItem() const;

    TPlaylistItem* plCurrentItem() const;
    TPlaylistItem* firstPlaylistItem() const;
    TPlaylistItem* lastPlaylistItem() const;

    QString playingFile() const;
    TPlaylistItem* findFilename(const QString& filename);

    TPlaylistItem* getNextItem(TPlaylistItem* w, bool allowChild = true) const;
    TPlaylistItem* getNextPlaylistItem(TPlaylistItem* item) const;
    TPlaylistItem* getNextPlaylistItem() const;

    TPlaylistItem* getPreviousPlaylistWidgetItem(TPlaylistItem* w) const;
    TPlaylistItem* getPreviousPlaylistWidgetItem() const;

    TPlaylistItem* findPreviousPlayedTime(TPlaylistItem* w);

    void clearPlayed();
    void clr();

    bool isBusy() const { return addFilesThread || fileCopier; }
    bool isModified() const { return root()->modified(); }
    void clearModified() { root()->setModified(false, true); }
    void emitModifiedChanged();

    bool isEditing() const {
        return state() == QAbstractItemView::EditingState;
    }
    void editName();
    void editURL();

    TPlaylistItem* validateItem(TPlaylistItem* folder, TPlaylistItem* item) const;
    TPlaylistItem* validateItem(TPlaylistItem* item) const;

    TPlaylistItem* add(TPlaylistItem* item, TPlaylistItem* target, int index);
    void removeSelected(bool deleteFromDisk);

    void setSort(int section, Qt::SortOrder order);
    void disableSort();

    void saveSettings(QSettings* pref);
    void loadSettings(QSettings* pref);

public slots:
    void updateItemPath();

signals:
    void busyChanged();
    void modifiedChanged();
    void playingItemChanged(TPlaylistItem* item);
    void playingItemUpdated(TPlaylistItem* item);
    void latestDirChanged(QString dir);
    void nothingToPlay(QString msg);
    void rootFilenameChanged(QString rootFilename);
    void addedItems();
    void startPlay();
    void playItem(TPlaylistItem* item);

protected:
    virtual Qt::DropActions supportedDropActions() const override;
    virtual QStringList mimeTypes() const override;
    virtual void dragEnterEvent(QDragEnterEvent*event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual bool dropMimeData(QTreeWidgetItem *parent,
                              int index,
                              const QMimeData *data,
                              Qt::DropAction action) override;

protected slots:
    virtual void rowsAboutToBeRemoved(const QModelIndex& parent,
                                      int start, int end) override;
    virtual void rowsInserted(const QModelIndex& parent,
                              int start, int end) override;

private:
    TAddFilesThread* addFilesThread;
    QStringList addFileList;
    TPlaylistItem* addFilesTarget;
    int addFilesTargetIndex;
    bool addFilesStartPlay;
    QString addFilesFileToPlay;
    bool addFilesRestartThread;
    const bool isFavList;

    int sortSectionSaved;
    Qt::SortOrder sortOrderSaved;

    Action::Menu::TMenu* columnsMenu;

    QTimer wordWrapTimer;
    TWZTimer* scrollToCurrentItemTimer;
    bool scrollToCurrent;

    QtFileCopier *fileCopier;
    QtCopyDialog *copyDialog;
    QString stoppedFilename;
    TPlaylistItem* stoppedItem;

    bool yesToAll;

    void addFilesStartThread();
    void abortAddFilesThread();
    void abortFileCopier();

    int countItems(QTreeWidgetItem* w) const;
    int countChildren(TPlaylistItem* w) const;
    bool hasPlayableItems(TPlaylistItem* item) const;

    TPlaylistItem* getPreviousItem(TPlaylistItem* w,
                                   bool allowChild = true) const;

    void editStart(TPlaylistItem* current);

    bool droppingOnItself(QDropEvent *event, const QModelIndex &index);
    bool dropOn(QDropEvent *event, int *dropRow, int *dropCol,
                QModelIndex *dropIndex);
    bool addDroppedItem(const QString& source,
                        const QString& dest,
                        TPlaylistItem* item);
    void moveItem(TPlaylistItem* item,
                  TPlaylistItem* target,
                  int& targetIndex);
    void copyItem(TPlaylistItem* item,
                  TPlaylistItem* target,
                  int targetIndex);
    void handleItem(TPlaylistWidget* sourceWidget,
                    TPlaylistItem* item,
                    TPlaylistItem* target,
                    int& targetIndex,
                    Qt::DropAction action);
    QList<QTreeWidgetItem*> getItemsFromMimeData(const QMimeData* mimeData);
    void getItems(TPlaylistWidget* sourceWidget,
                     TPlaylistItem* target,
                     QStringList& filesForCopier,
                     QList<TPlaylistItem*>& internalItems,
                     QDropEvent* event);
    void dropItems(TPlaylistItem* target,
                   int targetIndex,
                   QDropEvent* event,
                   QStringList& filesForCopier);
    void drop(TPlaylistItem* target, int targetIndex, QDropEvent* event);
    void dropURLs(TPlaylistItem* target, int row, const QMimeData* mimeData);
    bool hasModelMimeType(const QMimeData* mime);

    void resizeNameColumn(TPlaylistItem* item, int level);
    void startWordWrap();

    bool removeItemFromDisk(TPlaylistItem* item);
    void removeItem(TPlaylistItem* item,
                    TPlaylistItem*& newCurrent,
                    bool deleteFromDisk);

private slots:
    void onAddFilesThreadFinished();
    void scrollToCurrentItem();
    void onWordWrapTimeout();
    void resizeNameColumnAll();
    void onItemExpanded(QTreeWidgetItem* i);
    void onSectionClicked(int section);
    void onSectionResized(int logicalIndex, int oldSize, int newSize);
    void onColumnMenuTriggered(QAction* action);

    void onDropDone(bool error);
    void onCopyFinished(int id, bool error);
    void onMoveAboutToStart(int id);
    void onMoveFinished(int id, bool error);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H

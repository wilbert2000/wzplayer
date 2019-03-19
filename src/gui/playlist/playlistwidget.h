#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistitem.h"
#include "wzdebug.h"
#include <QTreeWidget>
#include <QTimer>


class QSettings;
class QtFileCopier;
class QtCopyDialog;

namespace Gui {

namespace Action { namespace Menu {
    class TMenuExec;
}}

namespace Playlist {

class TPlaylistWidget : public QTreeWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylistWidget(QWidget* parent,
                             const QString& name,
                             const QString& shortName);
    virtual ~TPlaylistWidget();

    TPlaylistItem* playingItem;
    void setPlayingItem(TPlaylistItem* item,
                        TPlaylistItemState state = PSTATE_STOPPED);

    TPlaylistItem* root() const {
        return static_cast<TPlaylistItem*>(topLevelItem(0));
    }

    int countItems() const;
    int countChildren() const;
    bool hasItems() const;
    bool hasSingleItem() const;

    TPlaylistItem* plCurrentItem() const;
    TPlaylistItem* firstPlaylistItem() const;
    TPlaylistItem* lastPlaylistItem() const;

    QString playingFile() const;
    QString currentFile() const;
    TPlaylistItem* findFilename(const QString& filename);

    TPlaylistItem* getNextItem(TPlaylistItem* w, bool allowChild = true) const;
    TPlaylistItem* getNextPlaylistItem(TPlaylistItem* item) const;
    TPlaylistItem* getNextPlaylistItem() const;

    TPlaylistItem* getPreviousPlaylistWidgetItem(TPlaylistItem* w) const;
    TPlaylistItem* getPreviousPlaylistWidgetItem() const;

    TPlaylistItem* findPreviousPlayedTime(TPlaylistItem* w);

    void clearPlayed();
    void clr();

    bool isBusy() const { return fileCopier; }
    bool isModified() const { return root()->modified(); }
    void clearModified() { root()->setModified(false, true); }
    void emitModifiedChanged();

    TPlaylistItem* validateItem(TPlaylistItem* item);

    TPlaylistItem* add(TPlaylistItem* item, TPlaylistItem* target);
    void removeSelected(bool deleteFromDisk);

    void setSort(int section, Qt::SortOrder order);
    void disableSort();

    void saveSettings(QSettings* pref);
    void loadSettings(QSettings* pref);

public slots:
    void editName();

signals:
    void modifiedChanged();
    void playingItemChanged(TPlaylistItem* item);

protected:
    virtual void dropEvent(QDropEvent* event) override;

protected slots:
    virtual void rowsAboutToBeRemoved(const QModelIndex& parent,
                                      int start, int end) override;
    virtual void rowsInserted(const QModelIndex& parent,
                              int start, int end) override;

private:
    int sortSection;
    Qt::SortOrder sortOrder;
    int sortSectionSaved;
    Qt::SortOrder sortOrderSaved;
    Action::Menu::TMenuExec* columnsMenu;

    QTimer wordWrapTimer;

    QtFileCopier *fileCopier;
    QtCopyDialog *copyDialog;
    QString stoppedFilename;
    TPlaylistItem* stoppedItem;

    int countItems(QTreeWidgetItem* w) const;
    int countChildren(QTreeWidgetItem* w) const;

    TPlaylistItem* getPreviousItem(TPlaylistItem* w,
                                   bool allowChild = true) const;

    bool droppingOnItself(QDropEvent *event, const QModelIndex &index);
    bool dropOn(QDropEvent *event, int *dropRow, int *dropCol,
                QModelIndex *dropIndex);
    bool addDroppedItem(const QString& source,
                        const QString& dest,
                        TPlaylistItem* item);
    void dropSelection(TPlaylistItem* target, Qt::DropAction action);

    void resizeNameColumn(TPlaylistItem* item, int level);
    bool removeFromDisk(const QString& filename, const QString& playingFile);

private slots:
    void resizeNameColumnAll();
    void onItemExpanded(QTreeWidgetItem* i);
    void onSectionClicked(int section);
    void onSectionResized(int logicalIndex, int oldSize, int newSize);
    void onColumnMenuTriggered(QAction* action);
    void onCopyFinished(int id, bool error);
    void onMoveAboutToStart(int id);
    void onMoveFinished(int id, bool error);
    void onDropDone(bool error);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H

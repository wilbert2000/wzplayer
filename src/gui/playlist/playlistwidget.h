#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistitem.h"
#include "wzdebug.h"
#include <QTreeWidget>


class QTimer;
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
    explicit TPlaylistWidget(QWidget* parent);
    virtual ~TPlaylistWidget();

    TPlaylistItem* playing_item;

    void setPlayingItem(TPlaylistItem* item,
                        TPlaylistItemState state = PSTATE_STOPPED);

    TPlaylistItem* root() const {
        return static_cast<TPlaylistItem*>(topLevelItem(0));
    }

    int countItems() const;
    int countChildren() const;
    bool hasItems() const;
    bool hasSingleItem() const;

    TPlaylistItem* currentPlaylistItem() const;
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

    bool modified() { return mModified; }
    void setModified(QTreeWidgetItem* item,
                     bool modified = true,
                     bool recurse = false);
    void clearModified() {
        setModified(root(), false, true);
    }

    TPlaylistItem* validateItem(TPlaylistItem* item);

    TPlaylistItem* add(TPlaylistItem* item, TPlaylistItem* target);
    void removeSelected(bool deleteFromDisk);

    void setSort(int section, Qt::SortOrder order);

    void saveSettings(QSettings* pref);
    void loadSettings(QSettings* pref);

signals:
    void modifiedChanged();
    void refresh();

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
    bool mModified;
    QTimer* wordWrapTimer;
    Gui::Action::Menu::TMenuExec* columnsMenu;

    QtFileCopier *fileCopier;
    QtCopyDialog *copyDialog;

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

    void resizeRows(QTreeWidgetItem* w, int level);
    bool removeFromDisk(const QString& filename, const QString& playingFile);

private slots:
    void onItemExpanded(QTreeWidgetItem*w);
    void onSectionClicked(int section);
    void onSectionResized(int, int, int);
    void onColumnMenuTriggered(QAction* action);
    void resizeRowsEx();
    void onCopyFinished(int id, bool error);
    void onMoveFinished(int id, bool error);
    void onDropDone(bool error);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H

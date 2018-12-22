#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include "gui/playlist/playlistwidgetitem.h"
#include "wzdebug.h"
#include <QTreeWidget>

class QTimer;

namespace Gui {
namespace Playlist {

class TPlaylistWidget : public QTreeWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TPlaylistWidget(QWidget* parent);
    virtual ~TPlaylistWidget();

    TPlaylistWidgetItem* playing_item;

    void setPlayingItem(TPlaylistWidgetItem* item,
                        TPlaylistWidgetItemState state = PSTATE_STOPPED);

    TPlaylistWidgetItem* root() const {
        return static_cast<TPlaylistWidgetItem*>(topLevelItem(0));
    }

    int countItems() const;
    int countChildren() const;
    bool hasItems() const;
    bool hasSingleItem() const;

    TPlaylistWidgetItem* currentPlaylistWidgetItem() const;
    TPlaylistWidgetItem* firstPlaylistWidgetItem() const;
    TPlaylistWidgetItem* lastPlaylistWidgetItem() const;

    QString playingFile() const;
    QString currentFile() const;
    TPlaylistWidgetItem* findFilename(const QString& filename);

    TPlaylistWidgetItem* getNextItem(TPlaylistWidgetItem* w,
                                     bool allowChild = true) const;
    TPlaylistWidgetItem* getNextPlaylistWidgetItem(TPlaylistWidgetItem* item) const;
    TPlaylistWidgetItem* getNextPlaylistWidgetItem() const;

    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem(TPlaylistWidgetItem* w) const;
    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem() const;

    TPlaylistWidgetItem* findPreviousPlayedTime(TPlaylistWidgetItem* w);

    void clearPlayed();
    void clr();

    bool modified() { return mModified; }
    void setModified(QTreeWidgetItem* item,
                     bool modified = true,
                     bool recurse = false);
    void clearModified() {
        setModified(root(), false, true);
    }

    TPlaylistWidgetItem* validateItem(TPlaylistWidgetItem* item);

    TPlaylistWidgetItem* add(TPlaylistWidgetItem* item,
                             TPlaylistWidgetItem* target);

    void setSort(int section, Qt::SortOrder order);

signals:
    void modifiedChanged();

protected:
    virtual void dropEvent(QDropEvent*);

private:
    int sortSection;
    Qt::SortOrder sortOrder;
    bool mModified;
    QTimer* wordWrapTimer;

    int countItems(QTreeWidgetItem* w) const;
    int countChildren(QTreeWidgetItem* w) const;

    TPlaylistWidgetItem* getPreviousItem(TPlaylistWidgetItem* w,
                                         bool allowChild = true) const;

    void setOrder(TPlaylistWidgetItem* item, int& order);
    void setOrder();
    void resizeRows(QTreeWidgetItem* w, int level);

private slots:
    void onSectionClicked(int section);
    void onItemExpanded(QTreeWidgetItem*w);
    void onSectionResized(int, int, int);
    void resizeRowsEx();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H

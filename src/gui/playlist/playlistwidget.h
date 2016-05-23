#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include <QTreeWidget>
#include <gui/playlist/playlistwidgetitem.h>
#include "wzdebug.h"


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
                        TPlaylistItemState state = PSTATE_STOPPED);

    TPlaylistWidgetItem* root() const {
        return static_cast<TPlaylistWidgetItem*>(topLevelItem(0));
    }

    int countItems() const;
    int countChildren() const;
    bool hasItems() const;

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
    void enableSort(bool enable);

    bool modified() { return mModified; }
    void setModified(QTreeWidgetItem* item,
                     bool modified = true,
                     bool recurse = false);
    void clearModified() {
        setModified(root(), false, true);
    }

    TPlaylistWidgetItem* add(TPlaylistWidgetItem* item,
                             TPlaylistWidgetItem* target,
                             TPlaylistWidgetItem* current);

signals:
    void modifiedChanged();

protected:
    virtual void dropEvent(QDropEvent*);

private:
    bool mModified;
    QTimer* wordWrapTimer;

    int countItems(QTreeWidgetItem* w) const;
    int countChildren(QTreeWidgetItem* w) const;

    TPlaylistWidgetItem* getPreviousItem(TPlaylistWidgetItem* w,
                                         bool allowChild = true) const;

    void resizeRows(QTreeWidgetItem* w, int level);

private slots:
    void onSectionClicked(int);
    void onItemExpanded(QTreeWidgetItem*w);
    void onSectionResized(int, int, int);
    void resizeRows();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H

#include "gui/playlist/menuaddremoved.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"


namespace Gui {
namespace Playlist {

TMenuAddRemoved::TMenuAddRemoved(TPlaylist* playlist, TMainWindow* w,
                                 TPlaylistWidget* plw) :
    TMenu(playlist, w, "pl_add_removed_menu", tr("Add removed item"), "noicon"),
    playlistWidget(plw) {

    menuAction()->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    connect(this, &TMenuAddRemoved::triggered,
            this, &TMenuAddRemoved::onTriggered);
    connect(this, &TMenuAddRemoved::addRemovedItem,
            playlist, &TPlaylist::addRemovedItem);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TMenuAddRemoved::onCurrentItemChanged);

    setEnabled(false);
}

TMenuAddRemoved::~TMenuAddRemoved() {
}

void TMenuAddRemoved::onAboutToShow() {

    clear();
    int c = 0;
    item = playlistWidget->plCurrentItem();
    if (item) {
        if (!item->isFolder()) {
            item = item->plParent();
        }
        if (item) {
            foreach(const QString& s, item->getBlacklist()) {
                QAction* action = new QAction(s, this);
                action->setData(s);
                addAction(action);
                c++;
            }
        }
    }

    if (c == 0) {
        QAction* action = new QAction(tr("No removed items"), this);
        action->setEnabled(false);
        addAction(action);
    }
}

void TMenuAddRemoved::onTriggered(QAction* action) {

    QString s = action->data().toString();
    if (!s.isEmpty()) {
        // Check item still valid
        item = playlistWidget->validateItem(item);
        if (item && item->whitelist(s)) {
            item->setModified();
            // TODO:
            emit addRemovedItem(s);
        }
    }
}

void TMenuAddRemoved::onCurrentItemChanged(QTreeWidgetItem* current,
                                           QTreeWidgetItem*) {

    bool e = false;
    if (current) {
        TPlaylistItem* c = static_cast<TPlaylistItem*>(current);
        if (c->isFolder()) {
            e = c->getBlacklistCount();
        } else {
            c = c->plParent();
            if (c) {
                e = c->getBlacklistCount();
            }
        }
    }
    setEnabled(e);
}

} // namespace Playlist
} // namespace Gui

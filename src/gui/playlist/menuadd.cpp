#include "gui/playlist/menuadd.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/action.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Playlist {


TAddRemovedMenu::TAddRemovedMenu(QWidget* parent, TMainWindow* w,
                                 TPlaylist* playlist) :
    TMenu(parent, w, "pl_add_removed_menu", tr("Add removed item")),
    playlistWidget(playlist->getPlaylistWidget()) {

    connect(this, &TAddRemovedMenu::triggered,
            this, &TAddRemovedMenu::onTriggered);
    connect(this, &TAddRemovedMenu::addRemovedItem,
            playlist, &TPlaylist::addRemovedItem);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TAddRemovedMenu::onCurrentItemChanged);

    setEnabled(false);
}

TAddRemovedMenu::~TAddRemovedMenu() {
}

void TAddRemovedMenu::onAboutToShow() {

    clear();
    int c = 0;
    item = playlistWidget->currentPlaylistItem();
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

void TAddRemovedMenu::onTriggered(QAction* action) {

    QString s = action->data().toString();
    if (!s.isEmpty()) {
        // Check item still valid
        item = playlistWidget->validateItem(item);
        if (!item) {
            WZWARN("Owner of blacklist no longer existing");
        } else if (item->whitelist(s)) {
            playlistWidget->setModified(item);
            emit addRemovedItem(s);
        }
    }
}

void TAddRemovedMenu::onCurrentItemChanged(QTreeWidgetItem* current,
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


TMenuAdd::TMenuAdd(TPlaylist* playlist, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(playlist, mw, "pl_add_menu",
                             tr("Add to playlist"), "plus") {

    using namespace Gui::Action;

    TAction* a = new TAction(this, "pl_add_current", tr("Add playing file"));
    connect(a, &TAction::triggered, playlist, &TPlaylist::addCurrentFile);

    a = new TAction(this, "pl_add_files", tr("Add file(s)..."));
    connect(a, &TAction::triggered, playlist, &TPlaylist::addFilesDialog);

    a = new TAction(this, "pl_add_directory", tr("Add directory..."));
    connect(a, &TAction::triggered, playlist, &TPlaylist::addDirectory);

    a = new TAction(this, "pl_add_urls", tr("Add URL(s)..."));
    connect(a, &TAction::triggered, playlist, &TPlaylist::addUrls);

    // Add removed sub menu
    addMenu(new TAddRemovedMenu(this, main_window, playlist));
}

TMenuAdd::~TMenuAdd() {
}

} // namespace Playlist
} // namespace Gui

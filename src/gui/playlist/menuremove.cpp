#include "gui/playlist/menuremove.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidgetitem.h"
#include "gui/action/action.h"


namespace Gui {
namespace Playlist {

TMenuRemove::TMenuRemove(TPlaylist* pl, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(pl, mw, "pl_remove_menu",
                             tr("&Remove from playlist"), "minus"),
    playlist(pl) {

    removeSelectedAct = new Gui::Action::TAction(this, "pl_remove_selected",
        tr("&Remove from list"), "", Qt::Key_Delete);
    connect(removeSelectedAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::removeSelected);

    removeSelectedFromDiskAct = new Gui::Action::TAction(this,
        "pl_delete_from_disk", tr("&Delete from disk..."), "",
        Qt::SHIFT | Qt::Key_Delete);
    connect(removeSelectedFromDiskAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::removeSelectedFromDisk);

    removeAllAct = new Gui::Action::TAction(this, "pl_remove_all",
        tr("&Clear playlist"), "", Qt::CTRL | Qt::Key_Delete);
    connect(removeAllAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::removeAll);

    playlist->addActions(actions());
}

void TMenuRemove::enableActions() {

    bool e = playlist->isPlaylistEnabled() && playlist->hasItems();
    removeSelectedAct->setEnabled(e);

    TPlaylistWidgetItem* currentItem = playlist->currentPlaylistWidgetItem();
    removeSelectedFromDiskAct->setEnabled(e
        && currentItem
        && (currentItem->isSymLink()
            || !currentItem->isFolder()
            || currentItem->childCount() == 0));
            // Leaving test for non media files to deleteFileFromDisk()

    removeAllAct->setEnabled(e);
}

void TMenuRemove::onAboutToShow() {
    enableActions();
}

} // namespace Playlist
} // namespace Gui

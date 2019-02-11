#include "gui/playlist/menuremove.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/action.h"


namespace Gui {
namespace Playlist {

TMenuRemove::TMenuRemove(TPlaylist* pl, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(pl, mw, "pl_remove_menu",
                             tr("&Remove from playlist"), "minus"),
    playlist(pl) {

    using namespace Gui::Action;

    removeSelectedAct = new TAction(this, "pl_remove_selected",
        tr("&Remove from playlist"), "", Qt::Key_Delete);
    connect(removeSelectedAct, &TAction::triggered,
            playlist, &TPlaylist::removeSelected);

    removeSelectedFromDiskAct = new TAction(this, "pl_delete_from_disk",
        tr("&Delete from disk..."), "", Qt::SHIFT | Qt::Key_Delete);
    connect(removeSelectedFromDiskAct, &TAction::triggered,
            playlist, &TPlaylist::removeSelectedFromDisk);
    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::currentItemChanged,
            this, &TMenuRemove::enableRemoveFromDiskAction);


    removeAllAct = new TAction(this, "pl_remove_all", tr("&Clear playlist"),
                               "", Qt::CTRL | Qt::Key_Delete);
    connect(removeAllAct, &TAction::triggered, playlist, &TPlaylist::removeAll);

    addActionsTo(playlist);
}

void TMenuRemove::enableRemoveFromDiskAction() {

    TPlaylistWidgetItem* current = playlist->currentPlaylistWidgetItem();
    removeSelectedFromDiskAct->setEnabled(
                !playlist->isLoading()
                && current
                && (!current->isWZPlaylist()
                    || current->isSymLink()
                    || current->childCount() == 0));
    // Leaving test for non media files in directory to deleteFileFromDisk()
}

void TMenuRemove::enableActions() {

    bool e = !playlist->isLoading() && playlist->hasItems();
    removeSelectedAct->setEnabled(e);
    removeAllAct->setEnabled(e);
    enableRemoveFromDiskAction();
}

void TMenuRemove::onAboutToShow() {
    enableActions();
}

} // namespace Playlist
} // namespace Gui

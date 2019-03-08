#include "gui/playlist/menuremove.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/action.h"


namespace Gui {
namespace Playlist {

TMenuRemove::TMenuRemove(TPlaylist* pl, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(pl, mw, "pl_remove_menu",
                             tr("Remove from playlist"), "noicon"),
    playlist(pl) {

    menuAction()->setIcon(style()->standardPixmap(QStyle::SP_DialogCancelButton));

    using namespace Gui::Action;

    removeSelectedAct = new TAction(this, "pl_delete",
        tr("Delete from playlist"), "noicon", Qt::Key_Delete);
    removeSelectedAct->setIcon(style()->standardPixmap(QStyle::SP_TrashIcon));
    connect(removeSelectedAct, &TAction::triggered,
            playlist, &TPlaylist::removeSelected);

    removeSelectedFromDiskAct = new TAction(this, "pl_delete_from_disk",
        tr("Delete from disk..."), "noicon", Qt::SHIFT | Qt::Key_Delete);
    removeSelectedFromDiskAct->setIcon(style()->standardPixmap(
                                           QStyle::SP_DialogDiscardButton));
    connect(removeSelectedFromDiskAct, &TAction::triggered,
            playlist, &TPlaylist::removeSelectedFromDisk);
    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::currentItemChanged,
            this, &TMenuRemove::enableRemoveFromDiskAction);


    removeAllAct = new TAction(this, "pl_clear", tr("Clear playlist"),
                               "noicon", Qt::CTRL | Qt::Key_Delete);
    removeAllAct->setIcon(style()->standardPixmap(QStyle::SP_DialogResetButton));
    connect(removeAllAct, &TAction::triggered, playlist, &TPlaylist::removeAll);
}

void TMenuRemove::enableRemoveFromDiskAction() {

    TPlaylistItem* current = playlist->plCurrentItem();
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

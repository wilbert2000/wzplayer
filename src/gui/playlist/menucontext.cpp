#include "gui/playlist/menucontext.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/menuadd.h"
#include "gui/playlist/menuremove.h"
#include "gui/action/action.h"
#include "gui/mainwindow.h"
#include "player/player.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>


namespace Gui {
namespace Playlist {


TMenuContext::TMenuContext(TPlaylist* pl, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(pl, mw, "", ""),
    playlist(pl) {

    using namespace Gui::Action;

    // Edit name
    editNameAct = new TAction(this, "pl_edit_name", tr("&Edit name..."), "",
                              Qt::Key_F2);
    connect(editNameAct, &TAction::triggered, playlist, &TPlaylist::editName);
    playlist->addAction(editNameAct);

    // New folder
    newFolderAct = new TAction(this, "pl_new_folder", tr("&New folder"), "",
                               Qt::Key_F10);
    connect(newFolderAct, &TAction::triggered, playlist, &TPlaylist::newFolder);
    playlist->addAction(newFolderAct);

    // Find playing
    findPlayingAct = new TAction(this, "pl_find_playing",
                                 tr("&Find playing item"), "", Qt::Key_F3);
    connect(findPlayingAct, &TAction::triggered,
            playlist, &TPlaylist::findPlayingItem);
    main_window->addAction(findPlayingAct);

    addSeparator();
    // Cut
    cutAct = new TAction(this, "pl_cut", tr("&Cut file name(s)"), "",
                         QKeySequence("Ctrl+X"));
    connect(cutAct, &TAction::triggered, playlist, &TPlaylist::cut);
    playlist->addAction(cutAct);

    // Copy
    copyAct = new TAction(this, "pl_copy", tr("&Copy file name(s)"), "",
                          QKeySequence("Ctrl+C"));
    connect(copyAct, &TAction::triggered, playlist, &TPlaylist::copySelected);
    main_window->addAction(copyAct);

    // Paste
    pasteAct = new TAction(this, "pl_paste", tr("&Paste file name(s)"), "",
                           QKeySequence("Ctrl+V"));
    connect(pasteAct, &TAction::triggered, playlist, &TPlaylist::paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &TMenuContext::enablePaste);
    main_window->addAction(pasteAct);

    addSeparator();
    // Add menu
    addToPlaylistMenu = new TMenuAdd(playlist, main_window);
    addMenu(addToPlaylistMenu);

    // Remove menu
    removeFromPlaylistMenu = new TMenuRemove(playlist, main_window);
    addMenu(removeFromPlaylistMenu);

    //connect(pw, &TPlaylistWidget::currentItemChanged)
}

void TMenuContext::enablePaste() {
    pasteAct->setEnabled(!playlist->isLoading()
                         && QApplication::clipboard()->mimeData()->hasText());
}

void TMenuContext::enableActions() {

    bool current = playlist->currentPlaylistWidgetItem();
    bool enable = !playlist->isLoading();

    editNameAct->setEnabled(enable && current);
    newFolderAct->setEnabled(enable);
    findPlayingAct->setEnabled(playlist->hasPlayingItem());

    cutAct->setEnabled(enable && current);
    copyAct->setEnabled(current || player->mdat.filename.count());
    enablePaste();
}

void TMenuContext::onAboutToShow() {
    enableActions();
}

} // namespace Playlist
} // namespace Gui

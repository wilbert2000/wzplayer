#include "gui/playlist/menucontext.h"
#include "gui/playlist/playlist.h"
#include "gui/action/action.h"
#include "player/player.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>


namespace Gui {
namespace Playlist {


TMenuContext::TMenuContext(TPlaylist* pl, TMainWindow* mw) :
    Gui::Action::Menu::TMenu(pl, mw, "pl_context_menu", ""),
    playlist(pl) {

    // Edit name
    editNameAct = new Gui::Action::TAction(this, "pl_edit_name",
                                           tr("&Edit name..."), "", Qt::Key_F2);
    connect(editNameAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::editName);

    // New folder
    newFolderAct = new Gui::Action::TAction(this, "pl_new_folder",
                                            tr("&New folder"), "", Qt::Key_F10);
    connect(newFolderAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::newFolder);

    // Find playing
    findPlayingAct = new Gui::Action::TAction(this, "pl_find_playing",
                                              tr("&Find playing item"));
    connect(findPlayingAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::findPlayingItem);

    addSeparator();
    // Cut
    cutAct = new Gui::Action::TAction(this, "pl_cut", tr("&Cut file name(s)"),
                                      "", QKeySequence("Ctrl+X"));
    connect(cutAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::cut);

    // Copy
    copyAct = new Gui::Action::TAction(this, "pl_copy",
                                       tr("&Copy file name(s)"), "",
                                       QKeySequence("Ctrl+C"));
    connect(copyAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::copySelected);

    // Paste
    pasteAct = new Gui::Action::TAction(this, "pl_paste",
                                        tr("&Paste file name(s)"), "",
                                        QKeySequence("Ctrl+V"));
    connect(pasteAct, &Gui::Action::TAction::triggered,
            playlist, &TPlaylist::paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &TMenuContext::enablePaste);

    // Add actions to playlist
    playlist->addActions(actions());
}

void TMenuContext::enablePaste() {
    pasteAct->setEnabled(playlist->isPlaylistEnabled()
                         && QApplication::clipboard()->mimeData()->hasText());
}

void TMenuContext::enableActions() {

    bool e = playlist->isPlaylistEnabled();
    TPlaylistWidgetItem* current = playlist->currentPlaylistWidgetItem();

    editNameAct->setEnabled(e && current);
    newFolderAct->setEnabled(e);
    findPlayingAct->setEnabled(playlist->hasPlayingItem());

    // Note: there is always something selected if there are items
    bool haveItems = playlist->hasItems();
    cutAct->setEnabled(e && haveItems);
    copyAct->setEnabled(haveItems || player->mdat.filename.count());
    enablePaste();
}

void TMenuContext::onAboutToShow() {
    enableActions();
}


} // namespace Playlist
} // namespace Gui

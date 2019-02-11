#include "gui/action/menu/menufile.h"

#include "gui/mainwindow.h"
#include "gui/action/menu/favorites.h"
#include "gui/action/action.h"
#include "player/player.h"
#include "settings/paths.h"

#include <QMessageBox>


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuDisc : public TMenu {
public:
    explicit TMenuDisc(TMainWindow* parent);
};

TMenuDisc::TMenuDisc(TMainWindow* parent)
    : TMenu(parent, parent, "disc_menu", tr("Open d&isc"), "open_disc") {

    // DVD
    TAction* a = new TAction(this, "open_dvd", tr("&DVD from drive"), "dvd");
    connect(a, &TAction::triggered, parent, &TMainWindow::openDVD);

    a = new TAction(this, "open_dvd_iso", tr("D&VD from ISO file..."),
                    "dvd_iso");
    connect(a, &TAction::triggered, parent, &TMainWindow::openDVDFromISO);

    a = new TAction(this, "open_dvd_folder", tr("DVD &from folder..."),
                    "dvd_hd");
    connect(a, &TAction::triggered, parent, &TMainWindow::openDVDFromFolder);


    addSeparator();
    // BluRay
    a = new TAction(this, "open_bluray", tr("&Blu-ray from drive"), "bluray");
    connect(a, &TAction::triggered, parent, &TMainWindow::openBluRay);

    a = new TAction(this, "open_bluray_iso", tr("B&lu-&ray from ISO file..."),
                    "bluray_iso");
    connect(a, &TAction::triggered, parent, &TMainWindow::openBluRayFromISO);

    a = new TAction(this, "open_bluray_folder", tr("Bl&u-ray from folder..."),
                    "bluray_hd");
    connect(a, &TAction::triggered, parent, &TMainWindow::openBluRayFromFolder);


    addSeparator();
    // VCD
    a = new TAction(this, "open_vcd", tr("V&ideo CD"), "vcd");
    connect(a, &TAction::triggered, parent, &TMainWindow::openVCD);

    // Audio
    a = new TAction(this, "open_audio_cd", tr("&Audio CD"), "cdda");
    connect(a, &TAction::triggered, parent, &TMainWindow::openAudioCD);

    addActionsTo(parent);
}


TMenuFile::TMenuFile(TMainWindow* mw) :
    TMenu(mw, mw, "file_menu", tr("&File"), "noicon") {

    // Favorites
    TFavorites* fav = new TFavorites(main_window,
                                     "favorites_menu",
                                     tr("Fa&vorites"),
                                     "open_favorites",
                                     TPaths::configPath() + "/favorites.m3u8");
    fav->getEditAct()->setObjectName("favorites_edit");
    main_window->addAction(fav->getEditAct());
    fav->getAddAct()->setObjectName("favorites_add");
    main_window->addAction(fav->getAddAct());
    fav->getJumpAct()->setObjectName("favorites_jump");
    main_window->addAction(fav->getJumpAct());
    addMenu(fav);


    // Recents
    recentfiles_menu = new TMenu(main_window, main_window, "recent_menu",
                                 tr("&Recent files"), "recents");
    clearRecentsAct = new TAction(this, "clear_recents", tr("&Clear"),
                                  "delete", 0, false);
    main_window->addAction(clearRecentsAct);
    connect(clearRecentsAct, &TAction::triggered,
            this, &TMenuFile::clearRecentsList);
    addMenu(recentfiles_menu);
    updateRecents();
    connect(main_window, &TMainWindow::preferencesChanged,
            this, &TMenuFile::onPreferencesChanged);

    addSeparator();


    // Open URL
    TAction* a = new TAction(this, "open_url", tr("Open &URL..."), "",
                             QKeySequence("Ctrl+U"));
    connect(a, &TAction::triggered, main_window, &TMainWindow::openURL);
    main_window->addAction(a);

    // Open file
    a  = new TAction(this, "open_file", tr("&Open file..."), "open",
                              Qt::CTRL | Qt::Key_F);
    connect(a, &TAction::triggered, main_window, &TMainWindow::openFile);
    main_window->addAction(a);

    // Open dir
    a = new TAction(this, "open_directory", tr("Open &directory..."),
                             "", QKeySequence("Ctrl+D"));
    connect(a, &TAction::triggered, main_window, &TMainWindow::openDirectory);
    main_window->addAction(a);

    // Disc submenu
    addMenu(new TMenuDisc(main_window));

    addSeparator();

    // Playlist
    addAction(main_window->findChild<TAction*>("pl_open"));
    addAction(main_window->findChild<TAction*>("pl_save"));
    addAction(main_window->findChild<TAction*>("pl_saveas"));
    addAction(main_window->findChild<TAction*>("pl_refresh"));

    addSeparator();

    // Browse dir
    addAction(main_window->findChild<TAction*>("pl_browse_dir"));

    // Save thumbnail
    saveThumbnailAct  = new TAction(this, "save_thumbnail",
                                    tr("&Save thumbnail"), "",
                                    Qt::CTRL | Qt::Key_I);
    connect(saveThumbnailAct, &TAction::triggered,
            main_window, &TMainWindow::saveThumbnail);
    main_window->addAction(saveThumbnailAct);

    addSeparator();

    // Close
    a = new TAction(this, "close", tr("&Close"));
    main_window->addAction(a);
    connect(a, &TAction::triggered, main_window, &TMainWindow::closeWindow);
}

TMenuFile::~TMenuFile() {
}

void TMenuFile::enableActions() {
    saveThumbnailAct->setEnabled(player
        && player->mdat.selected_type == TMediaData::TYPE_FILE
        && !player->mdat.filename.isEmpty());
}

void TMenuFile::updateRecents() {

    recentfiles_menu->clear();

    int current_items = 0;

    if (pref->history_recents.count() > 0) {
        for (int n = 0; n < pref->history_recents.count(); n++) {
            QString i = QString::number(n+1);
            QString fullname = pref->history_recents.getURL(n);
            QString filename = fullname;
            QFileInfo fi(fullname);
            // Let's see if it looks like a file (no dvd://1 or something)
            if (fullname.indexOf("://") < 0) {
                filename = fi.fileName();
            }
            if (filename.size() > 85) {
                filename = filename.left(80) + "...";
            }

            QString show_name = filename;
            QString title = pref->history_recents.getTitle(n);
            if (!title.isEmpty())
                show_name = title;

            QAction* a = recentfiles_menu->addAction(QString("%1. " + show_name)
                .arg(i.insert(i.size()-1, '&'), 3, ' '));
            a->setStatusTip(fullname);
            a->setData(n);
            connect(a, &QAction::triggered,
                    main_window, &TMainWindow::openRecent);
            current_items++;
        }
    } else {
        QAction* a = recentfiles_menu->addAction(tr("<empty>"));
        a->setEnabled(false);
    }

    recentfiles_menu->menuAction()->setVisible(current_items > 0);
    if (current_items  > 0) {
        recentfiles_menu->addSeparator();
        recentfiles_menu->addAction(clearRecentsAct);
    }
}

void TMenuFile::onPreferencesChanged() {
    // Number of recent items might have changed
    updateRecents();
}

void TMenuFile::clearRecentsList() {

    int ret = QMessageBox::question(main_window,
                                    tr("Confirm deletion - WZPlayer"),
                                    tr("Delete the list of recent files?"),
                                    QMessageBox::Cancel, QMessageBox::Ok);

    if (ret == QMessageBox::Ok) {
        // Delete items in menu
        pref->history_recents.clear();
        updateRecents();
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui

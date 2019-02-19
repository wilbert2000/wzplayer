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
    explicit TMenuDisc(TMainWindow* mw);
};

TMenuDisc::TMenuDisc(TMainWindow* mw)
    : TMenu(mw, mw, "disc_menu", tr("Open disc"), "open_disc") {

    // DVD
    TAction* a = new TAction(mw, "open_dvd", tr("DVD from drive"), "dvd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVD);

    a = new TAction(mw, "open_dvd_iso", tr("DVD from ISO file..."), "dvd_iso");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVDFromISO);

    a = new TAction(mw, "open_dvd_folder", tr("DVD from folder..."), "dvd_hd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVDFromFolder);


    addSeparator();
    // BluRay
    a = new TAction(mw, "open_bluray", tr("Blu-ray from drive"), "bluray");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRay);

    a = new TAction(mw, "open_bluray_iso", tr("Blu-ray from ISO file..."),
                    "bluray_iso");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRayFromISO);

    a = new TAction(mw, "open_bluray_folder", tr("Blu-ray from folder..."),
                    "bluray_hd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRayFromFolder);


    addSeparator();
    // VCD
    a = new TAction(mw, "open_vcd", tr("Video CD"), "vcd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openVCD);

    // Audio
    a = new TAction(mw, "open_audio_cd", tr("Audio CD"), "cdda");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openAudioCD);
}


TMenuFile::TMenuFile(TMainWindow* mw) :
    TMenu(mw, mw, "file_menu", tr("File"), "noicon") {

    // Favorites
    TFavorites* fav = new TFavorites(mw, "favorites_menu", tr("Favorites"), "",
                                     TPaths::configPath() + "/favorites.m3u8");
    fav->getAddAct()->setObjectName("favorites_add");
    mw->addAction(fav->getAddAct());
    fav->getEditAct()->setObjectName("favorites_edit");
    mw->addAction(fav->getEditAct());
    addMenu(fav);

    // Recents
    recentfiles_menu = new TMenu(mw, mw, "recent_menu", tr("Recent files"));
    clearRecentsAct = new TAction(mw, "recents_clear", tr("Clear recents"),
                                  "delete");
    connect(clearRecentsAct, &TAction::triggered,
            this, &TMenuFile::clearRecentsList);
    addMenu(recentfiles_menu);
    updateRecents();
    connect(mw, &TMainWindow::settingsChanged,
            this, &TMenuFile::onSettingsChanged);

    addSeparator();


    // Open URL
    TAction* a = new TAction(mw, "open_url", tr("Open URL..."), "",
                             QKeySequence("Ctrl+U"));
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openURL);

    // Open file
    a  = new TAction(mw, "open_file", tr("Open file..."), "open",
                     Qt::CTRL | Qt::Key_F);
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openFile);

    // Open dir
    a = new TAction(mw, "open_directory", tr("Open directory..."), "",
                    QKeySequence("Ctrl+D"));
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDirectory);

    // Disc submenu
    addMenu(new TMenuDisc(mw));

    addSeparator();

    // Playlist
    addAction(mw->findChild<TAction*>("pl_open"));
    addAction(mw->findChild<TAction*>("pl_save"));
    addAction(mw->findChild<TAction*>("pl_saveas"));
    addAction(mw->findChild<TAction*>("pl_refresh"));

    addSeparator();

    // Browse dir
    addAction(mw->findChild<TAction*>("pl_browse_dir"));

    // Save thumbnail
    saveThumbnailAct  = new TAction(mw, "save_thumbnail", tr("Save thumbnail"),
                                    "", Qt::CTRL | Qt::Key_I);
    connect(saveThumbnailAct, &TAction::triggered,
            mw, &TMainWindow::saveThumbnail);
    addAction(saveThumbnailAct);

    addSeparator();

    // Close
    // Memo: Quit added by TMainwindowTray
    a = new TAction(mw, "close", tr("Close"));
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::closeWindow);
}

TMenuFile::~TMenuFile() {
}

void TMenuFile::enableActions() {
    saveThumbnailAct->setEnabled(
                player->mdat.selected_type == TMediaData::TYPE_FILE
                && !player->mdat.filename.isEmpty());
}

void TMenuFile::updateRecents() {

    recentfiles_menu->clear();

    int current_items = 0;

    if (pref->history_recents.count() > 0) {
        for (int n = 0; n < pref->history_recents.count(); n++) {
            QString fullname = pref->history_recents.getURL(n);
            QString show_name;
            // Let's see if it looks like a file (no dvd://1 or something)
            if (fullname.indexOf("://") < 0) {
                show_name = QFileInfo(fullname).fileName();
            } else {
                show_name = fullname;
            }
            if (show_name.size() > 35) {
                show_name = show_name.left(32) + "...";
            }

            QString title = pref->history_recents.getTitle(n);
            if (!title.isEmpty())
                show_name = title;

            QAction* a = recentfiles_menu->addAction(show_name);
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

void TMenuFile::onSettingsChanged() {
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

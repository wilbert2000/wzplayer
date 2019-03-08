#include "gui/action/menu/menufile.h"

#include "gui/mainwindow.h"
#include "gui/playlist/playlist.h"
#include "gui/action/menu/favorites.h"
#include "gui/action/action.h"
#include "player/player.h"
#include "settings/paths.h"
#include "name.h"

#include <QMessageBox>
#include <QStyle>


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuDisc : public TMenu {
public:
    explicit TMenuDisc(TMainWindow* mw);
};

TMenuDisc::TMenuDisc(TMainWindow* mw)
    : TMenu(mw, mw, "opem_disc_menu", tr("Open disc"), "open_disc") {

    // DVD
    TAction* a = new TAction(mw, "open_dvd", tr("Open DVD"), "dvd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVD);

    a = new TAction(mw, "open_dvd_iso", tr("Open DVD ISO file..."), "dvd_iso");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVDFromISO);

    a = new TAction(mw, "open_dvd_folder", tr("Open DVD folder..."), "dvd_hd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openDVDFromFolder);


    addSeparator();
    // BluRay
    a = new TAction(mw, "open_bluray", tr("Open Blu-ray"), "bluray");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRay);

    a = new TAction(mw, "open_bluray_iso", tr("Open Blu-ray ISO file..."),
                    "bluray_iso");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRayFromISO);

    a = new TAction(mw, "open_bluray_folder", tr("Open Blu-ray folder..."),
                    "bluray_hd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openBluRayFromFolder);


    addSeparator();
    // VCD
    a = new TAction(mw, "open_vcd", tr("Open video CD"), "vcd");
    addAction(a);
    connect(a, &TAction::triggered, mw, &TMainWindow::openVCD);

    // Audio
    a = new TAction(mw, "open_audio_cd", tr("Open audio CD"), "cdda");
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
    recentFilesMenu = new TMenu(mw, mw, "recent_menu", tr("Recent files"));
    clearRecentsAct = new TAction(mw, "recents_clear", tr("Clear recents"),
                                  "delete");
    connect(clearRecentsAct, &TAction::triggered,
            this, &TMenuFile::clearRecentsList);
    addMenu(recentFilesMenu);
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
    connect(a, &TAction::triggered,
            mw->getPlaylist(), &Playlist::TPlaylist::openFileDialog);

    // Open dir
    a = new TAction(mw, "open_directory", tr("Open directory..."), "",
                    QKeySequence("Ctrl+D"));
    addAction(a);
    connect(a, &TAction::triggered,
            mw->getPlaylist(), &Playlist::TPlaylist::openDirectoryDialog);

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

#ifdef Q_OS_LINUX
    // Save thumbnail
    saveThumbnailAct  = new TAction(mw, "save_thumbnail", tr("Save thumbnail"),
                                    "", Qt::CTRL | Qt::Key_I);
    connect(saveThumbnailAct, &TAction::triggered,
            mw, &TMainWindow::saveThumbnail);
    addAction(saveThumbnailAct);
#endif

    addSeparator();

    // Close
    // Memo: Quit added by TMainwindowTray
    a = new TAction(mw, "close", tr("Close"), "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
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

void TMenuFile::openRecent() {

    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        int i = action->data().toInt();
        QString filename = pref->history_recents.getURL(i);
        if (!filename.isEmpty()) {
            main_window->getPlaylist()->open(filename);
        }
    }
}

void TMenuFile::updateRecents() {

    recentFilesMenu->clear();

    for (int i = 0; i < pref->history_recents.count(); i++) {
        QString url = pref->history_recents.getURL(i);
        QString name = pref->history_recents.getTitle(i);
        if (name.isEmpty()) {
            name = TName::nameForURL(url);
            if (name.isEmpty()) {
                continue;
            }
        }
        if (name.size() > 35) {
            name = name.left(32) + "...";
        }

        QAction* a = new QAction(name, recentFilesMenu);
        a->setStatusTip(url);
        a->setData(i);
        connect(a, &QAction::triggered, this, &TMenuFile::openRecent);
        recentFilesMenu->addAction(a);
    }

    int count = recentFilesMenu->actions().count();
    if (count > 0) {
        recentFilesMenu->addSeparator();
        recentFilesMenu->addAction(clearRecentsAct);
    } else {
        recentFilesMenu->menuAction()->setVisible(false);
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

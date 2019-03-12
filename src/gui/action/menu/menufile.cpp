#include "gui/action/menu/menufile.h"

#include "gui/mainwindow.h"
#include "gui/playlist/playlist.h"
#include "gui/action/menu/favorites.h"
#include "gui/action/action.h"
#include "player/player.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "name.h"

#include <QMessageBox>
#include <QStyle>


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuDisc : public TMenu {
public:
    explicit TMenuDisc(QWidget* parent, TMainWindow* mw);
};

TMenuDisc::TMenuDisc(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "opem_disc_menu", tr("Open disc"), "open_disc") {

    addAction(mw->getAction("open_dvd_disc"));
    addAction(mw->getAction("open_dvd_iso"));
    addAction(mw->getAction("open_dvd_folder"));
    addSeparator();
    addAction(mw->getAction("open_bluray_disc"));
    addAction(mw->getAction("open_bluray_iso"));
    addAction(mw->getAction("open_bluray_folder"));
    addSeparator();
    addAction(mw->getAction("open_vcd"));
    addAction(mw->getAction("open_audio_cd"));
}


TMenuFile::TMenuFile(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, mw, "file_menu", tr("File"), "noicon") {

    // Favorites
    TFavorites* fav = new TFavorites(this, mw, "favorites_menu",
        tr("Favorites"), "", TPaths::dataPath() + "/favorites.m3u8");
    fav->getAddAct()->setObjectName("favorites_add");
    mw->addAction(fav->getAddAct());
    fav->getEditAct()->setObjectName("favorites_edit");
    mw->addAction(fav->getEditAct());
    addMenu(fav);

    // Recents
    recentFilesMenu = new TMenu(this, mw, "recent_menu", tr("Recent files"));
    updateRecents();
    addMenu(recentFilesMenu);
    connect(pref, &TPreferences::recentsChanged,
            this, &TMenuFile::onRecentsChanged,
            Qt::QueuedConnection);
    connect(mw, &TMainWindow::settingsChanged,
            this, &TMenuFile::onSettingsChanged);

    addSeparator();
    addAction(mw->getAction("open_url"));
    addAction(mw->getAction("open_file"));
    addAction(mw->getAction("open_directory"));

    // Disc submenu
    addMenu(new TMenuDisc(this, mw));

    // Playlist
    addSeparator();
    addAction(mw->getAction("pl_open"));
    addAction(mw->getAction("pl_save"));
    addAction(mw->getAction("pl_saveas"));
    addAction(mw->getAction("pl_refresh"));

    addSeparator();
    // Browse dir
    addAction(mw->getAction("pl_browse_dir"));
    // Save thumbnail
#ifdef Q_OS_LINUX
    addAction(mw->getAction("save_thumbnail"));
#endif

    // Close
    addSeparator();
    addAction(mw->getAction("close"));
    // Note: Quit added by TMainwindowTray
}

TMenuFile::~TMenuFile() {
}

void TMenuFile::updateRecents() {

    recentFilesMenu->clear();

    for (int i = 0; i < pref->history_recents.count(); i++) {
        QString url = pref->history_recents.getURL(i);
        if (url.isEmpty()) {
            continue;
        }
        QString name = pref->history_recents.getTitle(i);
        if (name.isEmpty()) {
            name = TName::nameForURL(url);
            if (name.isEmpty()) {
                continue;
            }
        }
        if (name.size() > 50) {
            name = name.left(47) + "...";
        }

        QAction* a = new QAction(name, recentFilesMenu);
        a->setStatusTip(url);
        a->setData(i);
        connect(a, &QAction::triggered, main_window, &TMainWindow::openRecent);
        recentFilesMenu->addAction(a);
    }

    int count = recentFilesMenu->actions().count();
    if (count > 0) {
        recentFilesMenu->addSeparator();
    }
    recentFilesMenu->addAction(main_window->getAction("recents_clear"));
}

void TMenuFile::onRecentsChanged() {
    updateRecents();
}

void TMenuFile::onSettingsChanged() {
    // Number of recent items might have changed
    updateRecents();
}

} // namespace Menu
} // namespace Action
} // namespace Gui

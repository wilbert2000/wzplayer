#include "gui/action/menu/menufile.h"

#include "gui/mainwindow.h"
#include "gui/playlist/playlist.h"
#include "gui/action/action.h"
#include "player/player.h"
#include "settings/paths.h"
#include "settings/preferences.h"
#include "name.h"
#include "iconprovider.h"

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

    addAction(mw->findAction("open_dvd_disc"));
    addAction(mw->findAction("open_dvd_iso"));
    addAction(mw->findAction("open_dvd_folder"));
    addSeparator();
    addAction(mw->findAction("open_bluray_disc"));
    addAction(mw->findAction("open_bluray_iso"));
    addAction(mw->findAction("open_bluray_folder"));
    addSeparator();
    addAction(mw->findAction("open_vcd"));
    addAction(mw->findAction("open_audio_cd"));
}


TMenuFile::TMenuFile(QWidget* parent, TMainWindow* mw, TMenu* favMenu) :
    TMenu(parent, mw, "file_menu", tr("File"), "noicon") {

    // Favorites
    addMenu(favMenu);

    // Recents
    recentFilesMenu = new TMenu(this, mw, "recent_menu", tr("Recent files"),
                                "noicon");
    recentFilesMenu->menuAction()->setIcon(iconProvider.recentIcon);
    updateRecents();
    addMenu(recentFilesMenu);
    connect(pref, &TPreferences::recentsChanged,
            this, &TMenuFile::onRecentsChanged,
            Qt::QueuedConnection);
    connect(mw, &TMainWindow::settingsChanged,
            this, &TMenuFile::onSettingsChanged);

    addSeparator();
    addAction(mw->findAction("open_url"));
    addAction(mw->findAction("open_file"));
    addAction(mw->findAction("open_directory"));

    // Disc submenu
    addMenu(new TMenuDisc(this, mw));

    // Playlist
    addSeparator();
    addAction(mw->findAction("pl_open"));
    addAction(mw->findAction("pl_save"));
    addAction(mw->findAction("pl_saveas"));
    addAction(mw->findAction("pl_refresh"));

    addSeparator();
    // Browse dir
    addAction(mw->findAction("pl_browse_dir"));
    // Save thumbnail
#ifdef Q_OS_LINUX
    addAction(mw->findAction("save_thumbnail"));
#endif

    // Close
    addSeparator();
    addAction(mw->findAction("close"));
    // Note: Quit added by TMainwindowTray
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
    recentFilesMenu->addAction(main_window->findAction("recents_clear"));
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

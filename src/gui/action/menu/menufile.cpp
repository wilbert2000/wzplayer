#include "gui/action/menu/menufile.h"

#include "gui/mainwindow.h"
#include "settings/preferences.h"
#include "name.h"
#include "iconprovider.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuDisc : public TMenu {
public:
    explicit TMenuDisc(QWidget* parent, TMainWindow* mw);
};

TMenuDisc::TMenuDisc(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "opem_disc_menu", tr("Open disc"), "open_disc") {

    addAction(mw->requireAction("open_dvd_disc"));
    addAction(mw->requireAction("open_dvd_iso"));
    addAction(mw->requireAction("open_dvd_folder"));
    addSeparator();
    addAction(mw->requireAction("open_bluray_disc"));
    addAction(mw->requireAction("open_bluray_iso"));
    addAction(mw->requireAction("open_bluray_folder"));
    addSeparator();
    addAction(mw->requireAction("open_vcd"));
    addAction(mw->requireAction("open_audio_cd"));
}


TMenuFile::TMenuFile(QWidget* parent, TMainWindow* mw, TMenu* favMenu) :
    TMenu(parent, "file_menu", tr("File"), "noicon") {

    // Favorites
    addMenu(favMenu);

    // Recents
    recentFilesMenu = new TMenu(this, "recent_menu", tr("Recent files"),
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
    addAction(mw->requireAction("open_url"));
    addAction(mw->requireAction("open_file"));
    addAction(mw->requireAction("open_directory"));

    // Disc submenu
    addMenu(new TMenuDisc(this, mw));

    // Playlist
    addSeparator();
    addAction(mw->requireAction("pl_open"));
    addAction(mw->requireAction("pl_save"));
    addAction(mw->requireAction("pl_save_as"));
    addAction(mw->requireAction("pl_refresh"));

    addSeparator();
    // Browse dir
    addAction(mw->requireAction("pl_browse_dir"));
    // Save thumbnail
#ifdef Q_OS_LINUX
    addAction(mw->requireAction("save_thumbnail"));
#endif

    // Close & quit
    addSeparator();
    addAction(mw->requireAction("close"));
    addAction(mw->requireAction("quit"));
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
        connect(a, &QAction::triggered,
                Gui::mainWindow, &TMainWindow::openRecent);
        recentFilesMenu->addAction(a);
    }

    int count = recentFilesMenu->actions().count();
    if (count > 0) {
        recentFilesMenu->addSeparator();
    }
    recentFilesMenu->addAction(mainWindow->requireAction("recents_clear"));
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

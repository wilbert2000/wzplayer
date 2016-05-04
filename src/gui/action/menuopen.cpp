#include "gui/action/menuopen.h"
#include <QMessageBox>
#include <QFileInfo>

#include "gui/base.h"
#include "gui/playlist/playlist.h"
#include "gui/action/action.h"
#include "gui/action/favorites.h"
#include "gui/action/tvlist.h"
#include "settings/paths.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {
namespace Action {

class TMenuDisc : public TMenu {
public:
	explicit TMenuDisc(TBase* parent);
};

TMenuDisc::TMenuDisc(TBase* parent)
    : TMenu(parent, parent, "disc_menu", tr("Open d&isc"), "open_disc") {

	// DVD
	TAction* a = new TAction(this, "open_dvd", tr("&DVD from drive"), "dvd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVD()));
	a = new TAction(this, "open_dvd_folder", tr("D&VD from folder..."), "dvd_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVDFromFolder()));
	// BluRay
	a = new TAction(this, "open_bluray", tr("&Blu-ray from drive"), "bluray");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRay()));
	a = new TAction(this, "open_bluray_folder", tr("Blu-&ray from folder..."), "bluray_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRayFromFolder()));
	// VCD and audio
	a = new TAction(this, "open_vcd", tr("V&CD"), "vcd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openVCD()));
	a = new TAction(this, "open_audio_cd", tr("&Audio CD"), "cdda");
	connect(a, SIGNAL(triggered()), parent, SLOT(openAudioCD()));

	addActionsTo(parent);
}


TMenuOpen::TMenuOpen(TBase* mw)
    : TMenu(mw, mw, "open_menu", tr("&Open"), "noicon") {

	// Open URL
    TAction* a = new TAction(this, "open_url", tr("Open &URL..."), "",
                             QKeySequence("Ctrl+U"));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openURL()));

	addSeparator();

	// Open file
    a = new TAction(this, "open_file", tr("Open &file..."), "open",
                    Qt::CTRL | Qt::Key_F);
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openFile()));

	// Open dir
    a = new TAction(this, "open_directory", tr("Open &directory..."), "",
                    QKeySequence("Ctrl+D"));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openDirectory()));

	// Open playlist
    addAction(main_window->getPlaylist()->findChild<TAction*>("pl_open"));

	// Disc submenu
	addMenu(new TMenuDisc(main_window));

	addSeparator();

	// Recents
    recentfiles_menu = new TMenu(main_window, main_window, "recent_menu",
                                 tr("&Recent files"), "recents");
    clearRecentsAct = new TAction(this, "clear_recents", tr("&Clear"),
                                  "delete", 0, false);
	main_window->addAction(clearRecentsAct);
	connect(clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()));
	addMenu(recentfiles_menu);
	updateRecents();

	// Favorites
    TFavorites* fav = new TFavorites(main_window, "favorites_menu",
                                     tr("F&avorites"), "open_favorites",
                                     TPaths::configPath() + "/favorites.m3u8");
	fav->editAct()->setObjectName("edit_fav_list");
	fav->jumpAct()->setObjectName("jump_fav_list");
	fav->nextAct()->setObjectName("next_fav");
	fav->previousAct()->setObjectName("previous_fav");
	fav->addCurrentAct()->setObjectName("add_current_fav");
	main_window->addAction(fav->editAct());
	main_window->addAction(fav->jumpAct());
	main_window->addAction(fav->nextAct());
	main_window->addAction(fav->previousAct());
	main_window->addAction(fav->addCurrentAct());
	addMenu(fav);
	connect(fav, SIGNAL(activated(const QString&)),
			main_window, SLOT(open(const QString&)));
	connect(main_window, SIGNAL(mediaFileTitleChanged(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	addSeparator();

	// TV
    fav = new TTVList(main_window, "tv_menu", tr("&TV"), "open_tv",
					  TPaths::configPath() + "/tv.m3u8",
					  pref->check_channels_conf_on_startup,
					  TTVList::TV);
	fav->editAct()->setObjectName("edit_tv_list");
	fav->jumpAct()->setObjectName("jump_tv_list");
	fav->nextAct()->setObjectName("next_tv");
    fav->previousAct()->setObjectName("previous_tv");
    fav->addCurrentAct()->setObjectName("add_current_tv");
	main_window->addAction(fav->editAct());
	main_window->addAction(fav->jumpAct());
	main_window->addAction(fav->nextAct());
	main_window->addAction(fav->previousAct());
	main_window->addAction(fav->addCurrentAct());
	addMenu(fav);
	connect(fav, SIGNAL(activated(QString)), main_window, SLOT(open(QString)));
	connect(main_window, SIGNAL(mediaFileTitleChanged(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	// Radio
    fav = new TTVList(main_window, "radio_menu", tr("Radi&o"), "open_radio",
					  TPaths::configPath() + "/radio.m3u8",
					  pref->check_channels_conf_on_startup,
					  TTVList::Radio);
	fav->editAct()->setObjectName("edit_radio_list");
	fav->jumpAct()->setObjectName("jump_radio_list");
	fav->nextAct()->setObjectName("next_radio");
    fav->previousAct()->setObjectName("previous_radio");
    fav->addCurrentAct()->setObjectName("add_current_radio");
	main_window->addAction(fav->editAct());
	main_window->addAction(fav->jumpAct());
	main_window->addAction(fav->nextAct());
	main_window->addAction(fav->previousAct());
	main_window->addAction(fav->addCurrentAct());
	addMenu(fav);
	connect(fav, SIGNAL(activated(QString)), main_window, SLOT(open(QString)));
	connect(main_window, SIGNAL(mediaFileTitleChanged(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	addSeparator();

	// Close
    a = new TAction(this, "close", tr("&Close"), "", QKeySequence("Ctrl+X"));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(closeWindow()));
}

void TMenuOpen::updateRecents() {
	qDebug("Gui::Action::TMenuOpen::updateRecents");

	recentfiles_menu->clear();

	int current_items = 0;

	if (pref->history_recents.count() > 0) {
		for (int n = 0; n < pref->history_recents.count(); n++) {
			QString i = QString::number(n+1);
			QString fullname = pref->history_recents.item(n);
			QString filename = fullname;
			QFileInfo fi(fullname);
			// Let's see if it looks like a file (no dvd://1 or something)
			if (fullname.indexOf(QRegExp("^.*://.*")) == -1) {
				filename = fi.fileName();
			}
			if (filename.size() > 85) {
				filename = filename.left(80) + "...";
			}

			QString show_name = filename;
			QString title = pref->history_recents.title(n);
			if (!title.isEmpty())
				show_name = title;

			QAction* a = recentfiles_menu->addAction(QString("%1. " + show_name).arg(i.insert(i.size()-1, '&'), 3, ' '));
			a->setStatusTip(fullname);
			a->setData(n);
			connect(a, SIGNAL(triggered()), main_window, SLOT(openRecent()));
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

void TMenuOpen::clearRecentsList() {

	int ret = QMessageBox::question(main_window, tr("Confirm deletion - WZPlayer"),
				tr("Delete the list of recent files?"),
				QMessageBox::Cancel, QMessageBox::Ok);

	if (ret == QMessageBox::Ok) {
		// Delete items in menu
		pref->history_recents.clear();
		updateRecents();
	}
}

} // namespace Action
} // namespace Gui

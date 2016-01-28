#include "gui/action/menuopen.h"
#include <QMessageBox>
#include "settings/paths.h"
#include "gui/action/action.h"
#include "gui/action/favorites.h"
#include "gui/action/tvlist.h"
#include "gui/base.h"


namespace Gui {
namespace Action {

class TMenuDisc : public TMenu {
public:
	explicit TMenuDisc(TBase* parent);
};

TMenuDisc::TMenuDisc(TBase* parent)
	: TMenu(parent, this, "disc_menu", QT_TR_NOOP("&Disc"), "open_disc") {

	// DVD
	TAction* a = new TAction(this, "open_dvd", QT_TR_NOOP("&DVD from drive"), "dvd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVD()));
	a = new TAction(this, "open_dvd_folder", QT_TR_NOOP("D&VD from folder..."), "dvd_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openDVDFromFolder()));
	// BluRay
	a = new TAction(this, "open_bluray", QT_TR_NOOP("&Blu-ray from drive"), "bluray");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRay()));
	a = new TAction(this, "open_bluray_folder", QT_TR_NOOP("Blu-&ray from folder..."), "bluray_hd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openBluRayFromFolder()));
	// VCD and audio
	a = new TAction(this, "open_vcd", QT_TR_NOOP("V&CD"), "vcd");
	connect(a, SIGNAL(triggered()), parent, SLOT(openVCD()));
	a = new TAction(this, "open_audio_cd", QT_TR_NOOP("&Audio CD"), "cdda");
	connect(a, SIGNAL(triggered()), parent, SLOT(openAudioCD()));

	addActionsTo(parent);
}


TMenuOpen::TMenuOpen(TBase* parent, TCore* core, QWidget* playlist)
	: TMenu(parent, this, "open_menu", QT_TR_NOOP("&Open"), "noicon")
	, main_window(parent) {

	// Open
	TAction* a = new TAction(this, "open_file", QT_TR_NOOP("&File..."), "open", QKeySequence("Ctrl+F"));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openFile()));

	// Recents
	recentfiles_menu = new TMenu(main_window, this, "recent_menu", QT_TR_NOOP("&Recent files"), "recents");
	clearRecentsAct = new TAction(this, "clear_recents", QT_TR_NOOP("&Clear"), "delete", 0, false);
	main_window->addAction(clearRecentsAct);
	connect(clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()));
	addMenu(recentfiles_menu);
	updateRecents();

	// Favorites
	TFavorites* fav = new TFavorites(main_window, this, "favorites_menu",
									 QT_TR_NOOP("F&avorites"), "open_favorites",
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
	connect(fav, SIGNAL(activated(QString)),
			main_window, SLOT(openFavorite(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	// Open dir
	a = new TAction(this, "open_directory", QT_TR_NOOP("D&irectory..."), "openfolder");
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openDirectory()));

	// Open playlist
	a = new TAction(this, "open_playlist", QT_TR_NOOP("&Playlist..."));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), playlist, SLOT(load()));

	// Disc submenu
	addMenu(new TMenuDisc(main_window));

	// URL
	a = new TAction(this, "open_url", QT_TR_NOOP("&URL..."), "url", QKeySequence("Ctrl+U"));
	main_window->addAction(a);
	connect(a, SIGNAL(triggered()), main_window, SLOT(openURL()));

	// TV
	fav = new TTVList(main_window, this, "tv_menu", QT_TR_NOOP("&TV"), "open_tv",
					  TPaths::configPath() + "/tv.m3u8",
					  pref->check_channels_conf_on_startup,
					  TTVList::TV);
	fav->editAct()->setObjectName("edit_tv_list");
	fav->jumpAct()->setObjectName("jump_tv_list");
	fav->nextAct()->setObjectName("next_tv");
	fav->nextAct()->setShortcut(Qt::Key_H);
	fav->previousAct()->setObjectName("previous_tv");
	fav->previousAct()->setShortcut(Qt::Key_L);
	fav->addCurrentAct()->setObjectName("add_current_tv");
	main_window->addAction(fav->editAct());
	main_window->addAction(fav->jumpAct());
	main_window->addAction(fav->nextAct());
	main_window->addAction(fav->previousAct());
	main_window->addAction(fav->addCurrentAct());
	addMenu(fav);
	connect(fav, SIGNAL(activated(QString)), main_window, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	// Radio
	fav = new TTVList(main_window, this, "radio_menu", QT_TR_NOOP("Radi&o"), "open_radio",
					  TPaths::configPath() + "/radio.m3u8",
					  pref->check_channels_conf_on_startup,
					  TTVList::Radio);
	fav->editAct()->setObjectName("edit_radio_list");
	fav->jumpAct()->setObjectName("jump_radio_list");
	fav->nextAct()->setObjectName("next_radio");
	fav->nextAct()->setShortcut(Qt::SHIFT | Qt::Key_H);
	fav->previousAct()->setObjectName("previous_radio");
	fav->previousAct()->setShortcut(Qt::SHIFT | Qt::Key_L);
	fav->addCurrentAct()->setObjectName("add_current_radio");
	main_window->addAction(fav->editAct());
	main_window->addAction(fav->jumpAct());
	main_window->addAction(fav->nextAct());
	main_window->addAction(fav->previousAct());
	main_window->addAction(fav->addCurrentAct());
	addMenu(fav);
	connect(fav, SIGNAL(activated(QString)), main_window, SLOT(open(QString)));
	connect(core, SIGNAL(mediaPlaying(const QString&, const QString&)),
			fav, SLOT(getCurrentMedia(const QString&, const QString&)));

	// Close
	addSeparator();
	a = new TAction(this, "close", QT_TR_NOOP("C&lose"), "", QKeySequence("Ctrl+X"));
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

	int ret = QMessageBox::question(main_window, tr("Confirm deletion - SMPlayer"),
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

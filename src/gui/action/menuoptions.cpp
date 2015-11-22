#include "gui/action/optionsmenu.h"
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "core.h"


using namespace Settings;

namespace Gui {

class TOSDMenu : public TMenu {
public:
	explicit TOSDMenu(QWidget* parent, TCore* c);
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};

TOSDMenu::TOSDMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "osd_menu", QT_TR_NOOP("&OSD"), "osd")
	, core(c) {

	group = new TActionGroup(this, "osd");
	// Always enabled
	new TActionGroupItem(this, group, "osd_none", QT_TR_NOOP("Subtitles onl&y"), Settings::TPreferences::None);
	new TActionGroupItem(this, group, "osd_seek", QT_TR_NOOP("Volume + &Seek"), Settings::TPreferences::Seek);
	new TActionGroupItem(this, group, "osd_timer", QT_TR_NOOP("Volume + Seek + &Timer"), Settings::TPreferences::SeekTimer);
	new TActionGroupItem(this, group, "osd_total", QT_TR_NOOP("Volume + Seek + Timer + T&otal time"), Settings::TPreferences::SeekTimerTotal);
	group->setChecked(pref->osd_level);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)));
	connect(core, SIGNAL(osdLevelChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "inc_osd_scale", QT_TR_NOOP("Size &+"), "", Qt::SHIFT | Qt::Key_U);
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
	a = new TAction(this, "dec_osd_scale", QT_TR_NOOP("Size &-"), "", Qt::SHIFT | Qt::Key_Y);
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addActionsTo(parent);
}

void TOSDMenu::onAboutToShow() {
	group->setChecked((int) pref->osd_level);
}


class TStayOnTopMenu : public TMenu {
public:
	explicit TStayOnTopMenu(QWidget* parent);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
};

TStayOnTopMenu::TStayOnTopMenu(QWidget *parent) :
	// TODO: rename to stay_on_top_menu?
	TMenu(parent, this, "ontop_menu", QT_TR_NOOP("&Stay on top"), "ontop") {

	group = new TActionGroup(this, "ontop");
	// Always enabled
	new TActionGroupItem(this, group, "on_top_always", QT_TR_NOOP("&Always"), Settings::TPreferences::AlwaysOnTop);
	new TActionGroupItem(this, group, "on_top_never", QT_TR_NOOP("&Never"), Settings::TPreferences::NeverOnTop);
	new TActionGroupItem(this, group, "on_top_playing", QT_TR_NOOP("While &playing"), Settings::TPreferences::WhilePlayingOnTop);
	group->setChecked((int) pref->stay_on_top);
	connect(group , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));
	connect(parent , SIGNAL(stayOnTopChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "toggle_stay_on_top", QT_TR_NOOP("Toggle stay on top"), "");
	connect(a, SIGNAL(triggered()), parent, SLOT(toggleStayOnTop()));

	addActionsTo(parent);
}

void TStayOnTopMenu::onAboutToShow() {
	group->setChecked((int) pref->stay_on_top);
}

TOptionsMenu::TOptionsMenu(QWidget* parent,
						   TCore* core,
						   QMenu* toolBarMenu,
						   QWidget* playlist,
						   QWidget* logWindow)
	: TMenu(parent, this, "options_menu", QT_TR_NOOP("Op&tions"), "noicon") {

	// Ontop submenu
	addMenu(new TStayOnTopMenu(parent));
	// Toolbars
	addMenu(toolBarMenu);
	// OSD
	addMenu(new TOSDMenu(parent, core));

	// Show properties
	addSeparator();
	showPropertiesAct = new TAction(this, "show_file_properties", QT_TR_NOOP("View &info and properties..."), "info", QKeySequence("Ctrl+I"));
	connect(showPropertiesAct, SIGNAL(triggered()), parent, SLOT(showFilePropertiesDialog()));

	// Show playlist
	showPlaylistAct = new TAction(this, "show_playlist", QT_TR_NOOP("&Playlist"), "playlist", QKeySequence("Ctrl+P"));
	showPlaylistAct->setCheckable(true);
	connect(showPlaylistAct, SIGNAL(triggered(bool)), parent, SLOT(showPlaylist(bool)));
	connect(playlist, SIGNAL(visibilityChanged(bool)), showPlaylistAct, SLOT(setChecked(bool)));

	// Show log
	showLogAct = new TAction(this, "show_smplayer_log", QT_TR_NOOP("&View log"), "log", QKeySequence("Ctrl+L"));
	showLogAct->setCheckable(true);
	connect(showLogAct, SIGNAL(triggered(bool)), logWindow, SLOT(setVisible(bool)));
	connect(logWindow, SIGNAL(visibilityChanged(bool)), showLogAct, SLOT(setChecked(bool)));

	// Youtube browser
#ifdef YOUTUBE_SUPPORT
	addSeparator();
	showTubeBrowserAct = new TAction(this, "show_tube_browser", QT_TR_NOOP("&YouTube browser"), "tubebrowser", Qt::Key_F11);
	connect(showTubeBrowserAct, SIGNAL(triggered()), parent, SLOT(showTubeBrowser()));
#endif

	// Preferences
	addSeparator();
	showPreferencesAct = new TAction(this, "show_preferences", QT_TR_NOOP("P&references"), "prefs", QKeySequence("Ctrl+S"));
	connect(showPreferencesAct, SIGNAL(triggered()), parent, SLOT(showPreferencesDialog()));

	showConfigAct = new TAction(this, "show_config", QT_TR_NOOP("&Open configuration folder"));
	connect(showConfigAct, SIGNAL(triggered()), parent, SLOT(showConfigFolder()));
}

} // namespace Gui

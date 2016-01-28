#include "gui/action/menuoptions.h"
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "core.h"


using namespace Settings;

namespace Gui {
namespace Action {

class TMenuOSD : public TMenu {
public:
	explicit TMenuOSD(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
	TAction* showFilenameAct;
	TAction* showTimeAct;
};

TMenuOSD::TMenuOSD(QWidget *parent, TCore* c)
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
	TAction* a = new TAction(this, "next_osd", QT_TR_NOOP("OSD - Next level"), "", Qt::Key_O);
	connect(a, SIGNAL(triggered()), core, SLOT(nextOSDLevel()));

	addSeparator();
	a = new TAction(this, "inc_osd_scale", QT_TR_NOOP("Size &+"), "", Qt::SHIFT | Qt::Key_U);
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
	a = new TAction(this, "dec_osd_scale", QT_TR_NOOP("Size &-"), "", Qt::SHIFT | Qt::Key_Y);
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addSeparator();
	showFilenameAct = new TAction(this, "show_filename", QT_TR_NOOP("Show filename on OSD"), "", Qt::SHIFT | Qt::Key_I);
	connect(showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()));

	showTimeAct = new TAction(this, "show_time", QT_TR_NOOP("Show playback time on OSD"), "", Qt::Key_I);
	connect(showTimeAct, SIGNAL(triggered()), core, SLOT(showTimeOnOSD()));

	addActionsTo(parent);
}

void TMenuOSD::enableActions(bool stopped, bool video, bool) {

	bool enabled = !stopped && video;
	showFilenameAct->setEnabled(enabled);
	showTimeAct->setEnabled(enabled);
}

void TMenuOSD::onAboutToShow() {
	group->setChecked((int) pref->osd_level);
}


class TMenuStayOnTop : public TMenu {
public:
	explicit TMenuStayOnTop(QWidget* parent);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
};

TMenuStayOnTop::TMenuStayOnTop(QWidget *parent)
	: TMenu(parent, this, "stay_on_top_menu", QT_TR_NOOP("&Stay on top"), "ontop") {

	group = new TActionGroup(this, "ontop");
	// Always enabled
	new TActionGroupItem(this, group, "stay_on_top_always", QT_TR_NOOP("&Always"), Settings::TPreferences::AlwaysOnTop);
	new TActionGroupItem(this, group, "stay_on_top_never", QT_TR_NOOP("&Never"), Settings::TPreferences::NeverOnTop);
	new TActionGroupItem(this, group, "stay_on_top_playing", QT_TR_NOOP("While &playing"), Settings::TPreferences::WhilePlayingOnTop);
	group->setChecked((int) pref->stay_on_top);
	connect(group , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));
	connect(parent , SIGNAL(stayOnTopChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "toggle_stay_on_top", QT_TR_NOOP("Toggle stay on top"), "");
	connect(a, SIGNAL(triggered()), parent, SLOT(toggleStayOnTop()));

	addActionsTo(parent);
}

void TMenuStayOnTop::onAboutToShow() {
	group->setChecked((int) pref->stay_on_top);
}

TMenuOptions::TMenuOptions(QWidget* parent,
						   TCore* core,
						   QMenu* toolBarMenu,
						   QWidget* playlist,
						   QWidget* logWindow)
	: TMenu(parent, this, "options_menu", QT_TR_NOOP("Op&tions"), "noicon") {

	// OSD
	addMenu(new TMenuOSD(parent, core));
	// Toolbars
	addMenu(toolBarMenu);
	// Ontop submenu
	addMenu(new TMenuStayOnTop(parent));

	addSeparator();
	// Show playlist
	TAction* a = new TAction(this, "show_playlist", QT_TR_NOOP("&Playlist..."), "playlist", QKeySequence("Ctrl+P"));
	a->setCheckable(true);
	connect(a, SIGNAL(triggered(bool)), parent, SLOT(showPlaylist(bool)));
	connect(playlist, SIGNAL(visibilityChanged(bool)), a, SLOT(setChecked(bool)));

	// Show properties
	a = new TAction(this, "show_file_properties", QT_TR_NOOP("View &info and properties..."), "info", QKeySequence("Ctrl+I"));
	connect(a, SIGNAL(triggered()), parent, SLOT(showFilePropertiesDialog()));

	// Show log
	a = new TAction(this, "show_smplayer_log", QT_TR_NOOP("&View log..."), "log", QKeySequence("Ctrl+L"));
	a->setCheckable(true);
	connect(a, SIGNAL(triggered(bool)), logWindow, SLOT(setVisible(bool)));
	connect(logWindow, SIGNAL(visibilityChanged(bool)), a, SLOT(setChecked(bool)));

	// Youtube browser
#ifdef YOUTUBE_SUPPORT
	addSeparator();
	a = new TAction(this, "show_tube_browser", QT_TR_NOOP("&YouTube browser..."), "tubebrowser", Qt::Key_F11);
	connect(a, SIGNAL(triggered()), parent, SLOT(showTubeBrowser()));
#endif

	// Preferences
	addSeparator();
	a = new TAction(this, "show_config", QT_TR_NOOP("Open &configuration folder..."));
	connect(a, SIGNAL(triggered()), parent, SLOT(showConfigFolder()));

	a = new TAction(this, "show_preferences", QT_TR_NOOP("P&references..."), "prefs", QKeySequence("Ctrl+S"));
	connect(a, SIGNAL(triggered()), parent, SLOT(showPreferencesDialog()));

	addActionsTo(parent);
}

} // namespace Action
} // namespace Gui

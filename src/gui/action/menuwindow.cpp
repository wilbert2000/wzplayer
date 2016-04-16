#include "gui/action/menuwindow.h"
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
    : TMenu(parent, "osd_menu", tr("&OSD"), "osd")
	, core(c) {

	group = new TActionGroup(this, "osd");
	// Always enabled
	new TActionGroupItem(this, group, "osd_none", tr("Subtitles onl&y"), Settings::TPreferences::None);
	new TActionGroupItem(this, group, "osd_seek", tr("Volume + &Seek"), Settings::TPreferences::Seek);
	new TActionGroupItem(this, group, "osd_timer", tr("Volume + Seek + &Timer"), Settings::TPreferences::SeekTimer);
	new TActionGroupItem(this, group, "osd_total", tr("Volume + Seek + Timer + T&otal time"), Settings::TPreferences::SeekTimerTotal);
	group->setChecked(pref->osd_level);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)));
	connect(core, SIGNAL(osdLevelChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "next_osd", tr("OSD - Next level"), "", Qt::Key_O);
	connect(a, SIGNAL(triggered()), core, SLOT(nextOSDLevel()));

	addSeparator();
	a = new TAction(this, "inc_osd_scale", tr("Size &+"), "", Qt::SHIFT | Qt::Key_U);
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
	a = new TAction(this, "dec_osd_scale", tr("Size &-"), "", Qt::SHIFT | Qt::Key_Y);
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addSeparator();
	showFilenameAct = new TAction(this, "show_filename", tr("Show filename on OSD"), "", Qt::SHIFT | Qt::Key_I);
	connect(showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()));

	showTimeAct = new TAction(this, "show_time", tr("Show playback time on OSD"), "", Qt::Key_I);
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
    : TMenu(parent, "stay_on_top_menu", tr("&Stay on top"), "ontop") {

	group = new TActionGroup(this, "ontop");
	// Always enabled
	new TActionGroupItem(this, group, "stay_on_top_always", tr("&Always"), Settings::TPreferences::AlwaysOnTop);
	new TActionGroupItem(this, group, "stay_on_top_never", tr("&Never"), Settings::TPreferences::NeverOnTop);
	new TActionGroupItem(this, group, "stay_on_top_playing", tr("While &playing"), Settings::TPreferences::WhilePlayingOnTop);
	group->setChecked((int) pref->stay_on_top);
	connect(group , SIGNAL(activated(int)), parent, SLOT(changeStayOnTop(int)));
	connect(parent , SIGNAL(stayOnTopChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
	TAction* a = new TAction(this, "toggle_stay_on_top", tr("Toggle stay on top"), "");
	connect(a, SIGNAL(triggered()), parent, SLOT(toggleStayOnTop()));

	addActionsTo(parent);
}

void TMenuStayOnTop::onAboutToShow() {
	group->setChecked((int) pref->stay_on_top);
}

TMenuWindow::TMenuWindow(QWidget* parent,
						   TCore* core,
						   QMenu* toolBarMenu,
						   QWidget* playlist,
						   QWidget* logWindow)
    : TMenu(parent, "window_menu", tr("&Window"), "noicon") {

	// OSD
	addMenu(new TMenuOSD(parent, core));
	// Toolbars
	addMenu(toolBarMenu);
	// Ontop submenu
	addMenu(new TMenuStayOnTop(parent));

	addSeparator();
	// Show playlist
	TAction* a = new TAction(this, "show_playlist", tr("&Playlist..."), "playlist", QKeySequence("Ctrl+P"));
	a->setCheckable(true);
	connect(a, SIGNAL(triggered(bool)), parent, SLOT(showPlaylist(bool)));
	connect(playlist, SIGNAL(visibilityChanged(bool)), a, SLOT(setChecked(bool)));

	// Show properties
	a = new TAction(this, "show_file_properties", tr("&View properties..."), "info", QKeySequence("Ctrl+I"));
	connect(a, SIGNAL(triggered()), parent, SLOT(showFilePropertiesDialog()));

	// Show log
	a = new TAction(this, "show_log", tr("View &log..."), "log", QKeySequence("Ctrl+L"));
	a->setCheckable(true);
	connect(a, SIGNAL(triggered(bool)), logWindow, SLOT(setVisible(bool)));
	connect(logWindow, SIGNAL(visibilityChanged(bool)), a, SLOT(setChecked(bool)));

	// Preferences
	addSeparator();
	a = new TAction(this, "show_config", tr("Open &configuration folder..."));
	connect(a, SIGNAL(triggered()), parent, SLOT(showConfigFolder()));

	a = new TAction(this, "show_preferences", tr("P&references..."), "prefs", QKeySequence("Ctrl+S"));
	connect(a, SIGNAL(triggered()), parent, SLOT(showPreferencesDialog()));

	addActionsTo(parent);
}

} // namespace Action
} // namespace Gui

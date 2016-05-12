#include "gui/action/menuwindow.h"

#include <QDebug>

#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "core.h"
#include "images.h"
#include "gui/base.h"
#include "gui/action/action.h"
#include "images.h"

using namespace Settings;

namespace Gui {
namespace Action {

class TMenuOSD : public TMenu {
public:
    explicit TMenuOSD(TBase* mw, TCore* c);
protected:
    virtual void enableActions();
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
	TAction* showFilenameAct;
	TAction* showTimeAct;
};

TMenuOSD::TMenuOSD(TBase* mw, TCore* c)
    : TMenu(mw, mw, "osd_menu", tr("&OSD"), "osd")
	, core(c) {

    TAction* a = new TAction(this, "next_osd", tr("OSD - Next level"), "", Qt::Key_O);
    connect(a, SIGNAL(triggered()), core, SLOT(nextOSDLevel()));

    addSeparator();
    group = new TActionGroup(this, "osd");
	// Always enabled
    new TActionGroupItem(this, group, "osd_none", tr("Subtitles onl&y"),
        Settings::TPreferences::None, true, false, Qt::SHIFT | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_seek", tr("Volume + &Seek"),
        Settings::TPreferences::Seek, true, false, Qt::CTRL | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_timer", tr("Volume + Seek + &Timer"),
        Settings::TPreferences::SeekTimer, true, false, Qt::ALT | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_total", tr("Volume + Seek + Timer + T&otal time"),
        Settings::TPreferences::SeekTimerTotal, true, false, Qt::META | Qt::Key_O);
	group->setChecked(pref->osd_level);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeOSDLevel(int)));
	connect(core, SIGNAL(osdLevelChanged(int)), group, SLOT(setChecked(int)));

	addSeparator();
    a = new TAction(this, "inc_osd_scale", tr("Size &+"), "", QKeySequence(")"));
	connect(a, SIGNAL(triggered()), core, SLOT(incOSDScale()));
    a = new TAction(this, "dec_osd_scale", tr("Size &-"), "", QKeySequence("("));
	connect(a, SIGNAL(triggered()), core, SLOT(decOSDScale()));

	addSeparator();
    showFilenameAct = new TAction(this, "show_filename", tr("Show filename on OSD"));
	connect(showFilenameAct, SIGNAL(triggered()), core, SLOT(showFilenameOnOSD()));

    showTimeAct = new TAction(this, "show_time", tr("Show playback time on OSD"));
	connect(showTimeAct, SIGNAL(triggered()), core, SLOT(showTimeOnOSD()));

    addActionsTo(main_window);
}

void TMenuOSD::enableActions() {

    bool enabled = core->statePOP() && core->hasVideo();
	showFilenameAct->setEnabled(enabled);
	showTimeAct->setEnabled(enabled);
}

void TMenuOSD::onAboutToShow() {
	group->setChecked((int) pref->osd_level);
}

static QString stayOnTopToIconString(int stay_on_top) {

    switch (stay_on_top) {
    case TPreferences::NeverOnTop: return "stay_on_top_never";
    case TPreferences::AlwaysOnTop:return "stay_on_top_always";
    case TPreferences::WhilePlayingOnTop:return "stay_on_top_playing";
    default: return "stay_on_top_toggle";
    }
}

TMenuStayOnTop::TMenuStayOnTop(TBase* mw) :
    TMenu(mw, mw, "stay_on_top_menu", tr("&Stay on top")) {

    toggleStayOnTopAct = new TAction(this, "stay_on_top_toggle",
                                     tr("Toggle stay on top"),
                                     stayOnTopToIconString(pref->stay_on_top),
                                     Qt::Key_T);
    setDefaultAction(toggleStayOnTopAct);
    connect(toggleStayOnTopAct, SIGNAL(triggered()),
            main_window, SLOT(toggleStayOnTop()));
    connect(this, SIGNAL(triggered(QAction*)),
            this, SLOT(onTriggered(QAction*)));

    addSeparator();

    group = new TActionGroup(this, "stay_on_top_group");
    new TActionGroupItem(this, group, "stay_on_top_never", tr("&Never"),
        Settings::TPreferences::NeverOnTop, true, true, Qt::SHIFT | Qt::Key_T);
    new TActionGroupItem(this, group, "stay_on_top_always", tr("&Always"),
        Settings::TPreferences::AlwaysOnTop, true, true, Qt::CTRL | Qt::Key_T);
    new TActionGroupItem(this, group, "stay_on_top_playing", tr("While &playing"),
        Settings::TPreferences::WhilePlayingOnTop, true, true, Qt::ALT | Qt::Key_T);
	group->setChecked((int) pref->stay_on_top);
    connect(group , SIGNAL(activated(int)), main_window, SLOT(changeStayOnTop(int)));
    connect(main_window , SIGNAL(stayOnTopChanged(int)), group, SLOT(setChecked(int)));

    addActionsTo(main_window);
}

void TMenuStayOnTop::onTriggered(QAction* action) {

    TPreferences::TOnTop stay_on_top;
    if (action->objectName() == "stay_on_top_toggle") {
        // pref->stay_on_top is not yet updated
        if (pref->stay_on_top == TPreferences::NeverOnTop) {
            stay_on_top = TPreferences::AlwaysOnTop;
        } else {
            stay_on_top = TPreferences::NeverOnTop;
        }
    } else {
        stay_on_top = (TPreferences::TOnTop) action->data().toInt();
    }

    toggleStayOnTopAct->setIcon(Images::icon(stayOnTopToIconString(stay_on_top)));
}

void TMenuStayOnTop::onAboutToShow() {
	group->setChecked((int) pref->stay_on_top);
}

TMenuWindow::TMenuWindow(TBase* mw,
                         TCore* core,
                         QMenu* toolBarMenu,
                         QWidget* playlist,
                         QWidget* logWindow)
    : TMenu(mw, mw, "window_menu", tr("&Window"), "noicon") {

	// OSD
    addMenu(new TMenuOSD(main_window, core));
	// Toolbars
	addMenu(toolBarMenu);
	// Ontop submenu
    addMenu(new TMenuStayOnTop(main_window));

	addSeparator();
    // Show properties
    TAction* a = new TAction(this, "show_file_properties",
        tr("&View properties..."), "info", Qt::SHIFT | Qt::Key_P);
    a->setCheckable(true);
    main_window->addAction(a);
    connect(a, SIGNAL(triggered(bool)),
            main_window, SLOT(showFilePropertiesDialog(bool)));

    // Show playlist
    a = new TAction(this, "show_playlist", tr("View &playlist..."), "playlist",
                    Qt::Key_P);
	a->setCheckable(true);
    main_window->addAction(a);
    connect(a, SIGNAL(triggered(bool)),
            main_window, SLOT(showPlaylist(bool)));
    connect(playlist, SIGNAL(visibilityChanged(bool)),
            a, SLOT(setChecked(bool)));

	// Show log
    a = new TAction(this, "show_log", tr("View &log..."), "log",
                    QKeySequence("Ctrl+L"));
	a->setCheckable(true);
    main_window->addAction(a);
    connect(a, SIGNAL(triggered(bool)), logWindow, SLOT(setVisible(bool)));
    connect(logWindow, SIGNAL(visibilityChanged(bool)),
            a, SLOT(setChecked(bool)));

	// Preferences
	addSeparator();
    a = new TAction(this, "show_config", tr("Open &configuration folder..."));
    main_window->addAction(a);
    connect(a, SIGNAL(triggered()), main_window, SLOT(showConfigFolder()));

    a = new TAction(this, "show_preferences", tr("P&references..."), "prefs",
                    Qt::ALT | Qt::Key_P);
    main_window->addAction(a);
    connect(a, SIGNAL(triggered()), main_window, SLOT(showPreferencesDialog()));
}

} // namespace Action
} // namespace Gui

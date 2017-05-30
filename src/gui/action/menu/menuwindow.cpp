#include "gui/action/menu/menuwindow.h"

#include <QDebug>

#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "gui/autohidetimer.h"
#include "gui/action/actiongroup.h"
#include "gui/action/action.h"
#include "player/player.h"
#include "settings/preferences.h"
#include "images.h"
#include "images.h"

using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuOSD : public TMenu {
public:
    explicit TMenuOSD(TMainWindow* mw);
protected:
    virtual void enableActions();
    virtual void onAboutToShow();
private:
    TActionGroup* group;
    TAction* showFilenameAct;
    TAction* showTimeAct;
};

TMenuOSD::TMenuOSD(TMainWindow* mw)
    : TMenu(mw, mw, "osd_menu", tr("&OSD"), "osd") {

    TAction* a = new TAction(this, "next_osd", tr("OSD - Next level"), "", Qt::Key_O);
    connect(a, SIGNAL(triggered()), player, SLOT(nextOSDLevel()));

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
    connect(group, SIGNAL(activated(int)), player, SLOT(setOSDLevel(int)));
    connect(player, SIGNAL(osdLevelChanged(int)), group, SLOT(setChecked(int)));

    addSeparator();
    a = new TAction(this, "inc_osd_scale", tr("Size &+"), "", QKeySequence(")"));
    connect(a, SIGNAL(triggered()), player, SLOT(incOSDScale()));
    a = new TAction(this, "dec_osd_scale", tr("Size &-"), "", QKeySequence("("));
    connect(a, SIGNAL(triggered()), player, SLOT(decOSDScale()));

    addSeparator();
    showFilenameAct = new TAction(this, "show_filename", tr("Show filename on OSD"));
    connect(showFilenameAct, SIGNAL(triggered()), player, SLOT(showFilenameOnOSD()));

    showTimeAct = new TAction(this, "show_time", tr("Show playback time on OSD"));
    connect(showTimeAct, SIGNAL(triggered()), player, SLOT(showTimeOnOSD()));

    addActionsTo(main_window);
}

void TMenuOSD::enableActions() {

    bool enabled = player->statePOP() && player->hasVideo();
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

TMenuStayOnTop::TMenuStayOnTop(TMainWindow* mw) :
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

TMenuWindow::TMenuWindow(TMainWindow* mw,
                         QMenu* toolBarMenu,
                         TDockWidget* playlistDock,
                         TDockWidget* logDock,
                         TAutoHideTimer* autoHideTimer)
    : TMenu(mw, mw, "window_menu", tr("&Window"), "noicon") {

    // OSD
    addMenu(new TMenuOSD(main_window));
    // Toolbars
    addMenu(toolBarMenu);
    // Ontop submenu
    addMenu(new TMenuStayOnTop(main_window));

    addSeparator();
    // Show properties
    addAction(main_window->findChild<TAction*>("view_properties"));

    // Show playlist
    QAction* q = playlistDock->toggleViewAction();
    q->setObjectName("show_playlist");
    q->setIcon(Images::icon("playlist"));
    q->setShortcut(Qt::Key_P);
    updateToolTip(q);
    addAction(q);
    autoHideTimer->add(q, playlistDock);

    // Show log
    q = logDock->toggleViewAction();
    q->setObjectName("show_log");
    q->setIcon(Images::icon("log"));
    q->setShortcut(QKeySequence("Ctrl+L"));
    updateToolTip(q);
    addAction(q);
    autoHideTimer->add(q, logDock);

    // Preferences
    addSeparator();
    TAction* a = new TAction(this, "show_config",
                             tr("Open &configuration folder..."));
    main_window->addAction(a);
    connect(a, SIGNAL(triggered()), main_window, SLOT(showConfigFolder()));

    a = new TAction(this, "show_preferences", tr("P&references..."), "prefs",
                    Qt::ALT | Qt::Key_P);
    main_window->addAction(a);
    connect(a, SIGNAL(triggered()), main_window, SLOT(showPreferencesDialog()));
}

} // namespace Menu
} // namespace Action
} // namespace Gui

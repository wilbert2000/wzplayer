#include "gui/action/menu/menuview.h"
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

    TAction* a = new TAction(this, "next_osd", tr("OSD - Next level"), "",
                             Qt::Key_O);
    connect(a, &TAction::triggered, player, &Player::TPlayer::nextOSDLevel);

    addSeparator();
    group = new TActionGroup(this, "osd");
    // Always enabled
    new TActionGroupItem(this, group, "osd_none", tr("Subtitles only"),
        Settings::TPreferences::None, true, false, Qt::SHIFT | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_seek", tr("Volume + seek"),
        Settings::TPreferences::Seek, true, false, Qt::CTRL | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_time", tr("Volume + seek + time"),
        Settings::TPreferences::SeekTimer, true, false, Qt::ALT | Qt::Key_O);
    new TActionGroupItem(this, group, "osd_total",
                         tr("Volume + seek + time + length"),
                         Settings::TPreferences::SeekTimerTotal, true, false,
                         Qt::META | Qt::Key_O);
    group->setChecked(pref->osd_level);
    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setOSDLevel);
    connect(player, &Player::TPlayer::osdLevelChanged,
            group, &TActionGroup::setChecked);

    addSeparator();
    a = new TAction(this, "inc_osd_scale", tr("OSD size &+"), "",
                    QKeySequence(")"));
    connect(a, &TAction::triggered, player, &Player::TPlayer::incOSDScale);
    a = new TAction(this, "dec_osd_scale", tr("OSD size &-"), "",
                    QKeySequence("("));
    connect(a, &TAction::triggered, player, &Player::TPlayer::decOSDScale);

    addSeparator();
    showFilenameAct = new TAction(this, "show_filename",
                                  tr("Show filename on OSD"));
    connect(showFilenameAct, &TAction::triggered,
            player, &Player::TPlayer::showFilenameOnOSD);

    showTimeAct = new TAction(this, "show_time",
                              tr("Show playback time on OSD"));
    connect(showTimeAct, &TAction::triggered,
            player, &Player::TPlayer::showTimeOnOSD);
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
    connect(toggleStayOnTopAct, &TAction::triggered,
            main_window, &TMainWindow::toggleStayOnTop);
    connect(this, &TMenuStayOnTop::triggered,
            this, &TMenuStayOnTop::onTriggered);

    addSeparator();

    group = new TActionGroup(this, "stay_on_top_group");
    new TActionGroupItem(this, group, "stay_on_top_never", tr("&Never"),
        Settings::TPreferences::NeverOnTop, true, true, Qt::SHIFT | Qt::Key_T);
    new TActionGroupItem(this, group, "stay_on_top_always", tr("&Always"),
        Settings::TPreferences::AlwaysOnTop, true, true, Qt::CTRL | Qt::Key_T);
    new TActionGroupItem(this, group, "stay_on_top_playing",
                         tr("While &playing"),
                         Settings::TPreferences::WhilePlayingOnTop, true, true,
                         Qt::ALT | Qt::Key_T);
    group->setChecked((int) pref->stay_on_top);
    connect(group , &TActionGroup::activated,
            main_window, &TMainWindow::changeStayOnTop);
    connect(main_window , &TMainWindow::stayOnTopChanged,
            group, &TActionGroup::setChecked);
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

TMenuView::TMenuView(TMainWindow* mw,
                     QMenu* toolBarMenu,
                     TDockWidget* playlistDock,
                     TDockWidget* logDock,
                     TAutoHideTimer* autoHideTimer)
    : TMenu(mw, mw, "view_menu", tr("Vie&w"), "noicon") {

    // OSD
    addMenu(new TMenuOSD(main_window));
    // Toolbars
    addMenu(toolBarMenu);
    // Ontop submenu
    addMenu(new TMenuStayOnTop(main_window));

    addSeparator();
    // Show properties
    propertiesAct = new TAction(this, "view_properties", tr("&Properties..."),
                                "", Qt::SHIFT | Qt::Key_P);
    propertiesAct->setCheckable(true);
    propertiesAct->setEnabled(false);
    connect(propertiesAct, &TAction::triggered,
            main_window, &TMainWindow::showFilePropertiesDialog);

    // Show playlist
    QAction* a = playlistDock->toggleViewAction();
    a->setObjectName("view_playlist");
    a->setIcon(Images::icon("playlist"));
    a->setShortcut(Qt::Key_P);
    updateToolTip(a);
    addAction(a);
    autoHideTimer->add(a, playlistDock);

    // Show log
    a = logDock->toggleViewAction();
    a->setObjectName("view_log");
    a->setIcon(Images::icon("log"));
    a->setShortcut(QKeySequence("Ctrl+L"));
    updateToolTip(a);
    addAction(a);
    autoHideTimer->add(a, logDock);

    // Settings
    addSeparator();
    a = new TAction(this, "open_config_dir",
                    tr("Open &configuration folder..."));
    connect(a, &QAction::triggered,
            main_window, &TMainWindow::showConfigFolder);

    a = new TAction(this, "view_settings", tr("Settings..."), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_S);
    connect(a, &QAction::triggered,
            main_window, &TMainWindow::showSettingsDialog);
}

void TMenuView::onMediaSettingsChanged(Settings::TMediaSettings*) {
    propertiesAct->setEnabled(true);
}

} // namespace Menu
} // namespace Action
} // namespace Gui

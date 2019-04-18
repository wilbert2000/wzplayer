#include "gui/action/menu/menuview.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "settings/preferences.h"
#include "images.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

TOSDGroup::TOSDGroup(TMainWindow *mw) :
    TActionGroup(mw, "osd_group") {

    // Always enabled
    new TActionGroupItem(mw, this, "osd_none", tr("Subtitles only"),
        TPreferences::None, false, Qt::SHIFT | Qt::Key_O);
    new TActionGroupItem(mw, this, "osd_seek", tr("Volume + seek"),
        TPreferences::Seek, false, Qt::CTRL | Qt::Key_O);
    new TActionGroupItem(mw, this, "osd_time", tr("Volume + seek + time"),
        TPreferences::SeekTimer, false, Qt::ALT | Qt::Key_O);
    new TActionGroupItem(mw, this, "osd_total",
                         tr("Volume + seek + time + length"),
                         TPreferences::SeekTimerTotal, false,
                         Qt::META | Qt::Key_O);
    setChecked(pref->osd_level);
    connect(this, &TOSDGroup::triggeredID,
            player, &Player::TPlayer::setOSDLevel);
    connect(player, &Player::TPlayer::osdLevelChanged,
            this, &TOSDGroup::setChecked);
}

class TMenuOSD : public TMenu {
public:
    explicit TMenuOSD(QWidget* parent, TMainWindow* mw);
};

TMenuOSD::TMenuOSD(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "osd_menu", tr("OSD"), "osd") {

    addAction(mw->requireAction("osd_next"));

    addSeparator();
    addActions(mw->findChild<TOSDGroup*>()->actions());

    addSeparator();
    addAction(mw->requireAction("osd_inc_scale"));
    addAction(mw->requireAction("osd_dec_scale"));

    addSeparator();
    addAction(mw->requireAction("osd_show_filename"));
    addAction(mw->requireAction("osd_show_time"));
}


static QString stayOnTopToIconString(int stay_on_top) {

    switch (stay_on_top) {
        case TPreferences::NeverOnTop: return "stay_on_top_never";
        case TPreferences::AlwaysOnTop:return "stay_on_top_always";
        case TPreferences::WhilePlayingOnTop:return "stay_on_top_playing";
        default: return "stay_on_top_toggle";
    }
}

TToggleStayOnTopAction::TToggleStayOnTopAction(TMainWindow* mw) :
    TAction(mw, "stay_on_top_toggle", tr("Toggle stay on top"),
            stayOnTopToIconString(pref->stay_on_top), Qt::Key_T) {

    connect(this, &TToggleStayOnTopAction::triggered,
            mw, &TMainWindow::toggleStayOnTop);
    connect(mw, &TMainWindow::stayOnTopChanged,
            this, &TToggleStayOnTopAction::onStayOnTopChanged);
}

void TToggleStayOnTopAction::onStayOnTopChanged(int stayOnTop) {
    setIcon(Images::icon(stayOnTopToIconString(stayOnTop)));
}

TStayOnTopGroup::TStayOnTopGroup(TMainWindow *mw) :
    TActionGroup(mw, "stay_on_top_group") {

    new TToggleStayOnTopAction(mw);

    new TActionGroupItem(mw, this, "stay_on_top_never", tr("Never"),
        TPreferences::NeverOnTop, true, Qt::SHIFT | Qt::Key_T);
    new TActionGroupItem(mw, this, "stay_on_top_always", tr("Always"),
        TPreferences::AlwaysOnTop, true, Qt::CTRL | Qt::Key_T);
    new TActionGroupItem(mw, this, "stay_on_top_playing", tr("While playing"),
                         TPreferences::WhilePlayingOnTop, true,
                         Qt::ALT | Qt::Key_T);
    setChecked((int) pref->stay_on_top);

    connect(this , &TActionGroup::triggeredID,
            mw, &TMainWindow::changeStayOnTop);
    connect(mw, &TMainWindow::stayOnTopChanged,
            this, &TActionGroup::setChecked);
}

TMenuStayOnTop::TMenuStayOnTop(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "stay_on_top_menu", tr("Stay on top")) {

    TToggleStayOnTopAction* toggleStayOnTopAct =
            mw->findChild<TToggleStayOnTopAction*>();
    addAction(toggleStayOnTopAct);
    setDefaultAction(toggleStayOnTopAct);
    addSeparator();
    addActions(mw->findChild<TStayOnTopGroup*>()->actions());
}


TMenuView::TMenuView(QWidget* parent,
                     TMainWindow* mw,
                     QMenu* toolBarMenu,
                     QMenu* editToolBarMenu)
    : TMenu(parent, "view_menu", tr("View"), "noicon") {

    addMenu(new TMenuOSD(this, mw));
    addMenu(new TMenuStayOnTop(this, mw));

    addSeparator();
    addAction(mw->requireAction("view_playlist"));
    addAction(mw->requireAction("view_favorites"));
    addAction(mw->requireAction("view_log"));
    addAction(mw->requireAction("view_properties"));

    addSeparator();
    addMenu(toolBarMenu);
    addMenu(editToolBarMenu);

    addSeparator();
    addAction(mw->requireAction("browse_config_dir"));
    addAction(mw->requireAction("browse_data_dir"));
    addAction(mw->requireAction("view_settings"));

    // Tray action added by TMainWindowTray
}

} // namespace Menu
} // namespace Action
} // namespace Gui

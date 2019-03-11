#include "gui/action/menu/menuplay.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

// Base class for seeking forward/rewinding
TMenuSeek::TMenuSeek(TMainWindow* mw,
                     const QString& name,
                     const QString& text,
                     int seekIntOffset) :
    TMenu(mw, mw, name, text) {

    setDefaultAction(main_window->seekIntToAction(
                         pref->seeking_current_action + seekIntOffset));
    connect(this, &TMenuSeek::triggered,
            main_window, &TMainWindow::updateSeekDefaultAction);
}

void TMenuSeek::updateDefaultAction(QAction* action) {

    setDefaultAction(action);
    // Update default action associated tool buttons
    emit defaultActionChanged(action);
}


class TMenuSeekForward: public TMenuSeek {
public:
    explicit TMenuSeekForward(TMainWindow* mnw);
};

TMenuSeekForward::TMenuSeekForward(TMainWindow* mw) :
    TMenuSeek(mw, "seek_forward_menu", tr("Seek forward"), 0) {

    addAction(main_window->findChild<TAction*>("seek_forward_frame"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("seek_forward1"));
    addAction(main_window->findChild<TAction*>("seek_forward2"));
    addAction(main_window->findChild<TAction*>("seek_forward3"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("play_next"));

    connect(main_window, &TMainWindow::seekForwardDefaultActionChanged,
            this, &TMenuSeekForward::updateDefaultAction);
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(TMainWindow* mw);
};

TMenuSeekRewind::TMenuSeekRewind(TMainWindow* mw) :
    TMenuSeek(mw, "seek_rewind_menu", tr("Seek backwards"), 5) {

    addAction(main_window->findChild<TAction*>("seek_rewind_frame"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("seek_rewind1"));
    addAction(main_window->findChild<TAction*>("seek_rewind2"));
    addAction(main_window->findChild<TAction*>("seek_rewind3"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("play_prev"));

    connect(main_window, &TMainWindow::seekRewindDefaultActionChanged,
            this, &TMenuSeekRewind::updateDefaultAction);
}

class TMenuPlaySpeed : public TMenu {
public:
    explicit TMenuPlaySpeed(TMainWindow* mw);
};

TMenuPlaySpeed::TMenuPlaySpeed(TMainWindow* mw)
    : TMenu(mw, mw, "play_speed_menu", tr("Play speed")) {

    addAction(main_window->findChild<TAction*>("speed_normal"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("spedd_half"));
    addAction(main_window->findChild<TAction*>("speed_double"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("speed_dec_10"));
    addAction(main_window->findChild<TAction*>("speed_inc_10"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("speed_dec_4"));
    addAction(main_window->findChild<TAction*>("speed_inc_4"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("speed_dec_1"));
    addAction(main_window->findChild<TAction*>("speed_inc_1"));
}

class TMenuInOut : public TMenu {
public:
    explicit TMenuInOut(TMainWindow* mw);
};

TMenuInOut::TMenuInOut(TMainWindow* mw)
    : TMenu(mw, mw, "in_out_menu", tr("In-out points")) {

    addAction(main_window->findChild<TAction*>("set_in"));
    addAction(main_window->findChild<TAction*>("set_out"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("clear_in"));
    addAction(main_window->findChild<TAction*>("clear_out"));
    addAction(main_window->findChild<TAction*>("clear_in_out"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("seek_in"));
    addAction(main_window->findChild<TAction*>("seek_out"));
    addSeparator();
    addAction(main_window->findChild<TAction*>("repeat_in_out"));
    addAction(main_window->findChild<TAction*>("pl_repeat"));
    addAction(main_window->findChild<TAction*>("pl_shuffle"));
}

// Create main play menu
TMenuPlay::TMenuPlay(TMainWindow* mw)
    : TMenu(mw, mw, "play_menu", tr("Play"), "noicon") {

    addAction(main_window->findChild<TAction*>("play_or_pause"));
    addAction(main_window->findChild<TAction*>("stop"));
    addAction(main_window->findChild<TAction*>("play_new_window"));

    addSeparator();
    // Forward menu
    addMenu(new TMenuSeekForward(main_window));
    // Rewind menu
    addMenu(new TMenuSeekRewind(main_window));
    // Seek to time
    addAction(main_window->findChild<TAction*>("seek_to_time"));

    addSeparator();
    // Speed submenu
    addMenu(new TMenuPlaySpeed(main_window));
    // In-out point submenu
    addMenu(new TMenuInOut(main_window));
}

} // namespace Menu
} // namespace Action
} // namespace Gui


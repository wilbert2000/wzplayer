#include "gui/action/menu/menuplay.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

// Base class for seeking forward/rewinding
TMenuSeek::TMenuSeek(QWidget* parent,
                     TMainWindow* mw,
                     const QString& name,
                     const QString& text,
                     int seekIntOffset) :
    TMenu(parent, mw, name, text) {

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
    explicit TMenuSeekForward(QWidget* parent, TMainWindow* mnw);
};

TMenuSeekForward::TMenuSeekForward(QWidget* parent, TMainWindow* mw) :
    TMenuSeek(parent, mw, "seek_forward_menu", tr("Seek forward"), 0) {

    addAction(main_window->getAction("seek_forward_frame"));
    addSeparator();
    addAction(main_window->getAction("seek_forward1"));
    addAction(main_window->getAction("seek_forward2"));
    addAction(main_window->getAction("seek_forward3"));
    addSeparator();
    addAction(main_window->getAction("play_next"));

    connect(main_window, &TMainWindow::seekForwardDefaultActionChanged,
            this, &TMenuSeekForward::updateDefaultAction);
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(QWidget* parent, TMainWindow* mw);
};

TMenuSeekRewind::TMenuSeekRewind(QWidget* parent, TMainWindow* mw) :
    TMenuSeek(parent, mw, "seek_rewind_menu", tr("Seek backwards"), 5) {

    addAction(main_window->getAction("seek_rewind_frame"));
    addSeparator();
    addAction(main_window->getAction("seek_rewind1"));
    addAction(main_window->getAction("seek_rewind2"));
    addAction(main_window->getAction("seek_rewind3"));
    addSeparator();
    addAction(main_window->getAction("play_prev"));

    connect(main_window, &TMainWindow::seekRewindDefaultActionChanged,
            this, &TMenuSeekRewind::updateDefaultAction);
}

class TMenuPlaySpeed : public TMenu {
public:
    explicit TMenuPlaySpeed(QWidget* parent, TMainWindow* mw);
};

TMenuPlaySpeed::TMenuPlaySpeed(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "play_speed_menu", tr("Play speed")) {

    addAction(main_window->getAction("speed_normal"));

    addSeparator();
    addAction(main_window->getAction("spedd_half"));
    addAction(main_window->getAction("speed_double"));

    addSeparator();
    addAction(main_window->getAction("speed_dec_10"));
    addAction(main_window->getAction("speed_inc_10"));

    addSeparator();
    addAction(main_window->getAction("speed_dec_4"));
    addAction(main_window->getAction("speed_inc_4"));

    addSeparator();
    addAction(main_window->getAction("speed_dec_1"));
    addAction(main_window->getAction("speed_inc_1"));
}

class TMenuInOut : public TMenu {
public:
    explicit TMenuInOut(QWidget* parent, TMainWindow* mw);
};

TMenuInOut::TMenuInOut(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "in_out_menu", tr("In-out points")) {

    addAction(main_window->getAction("set_in"));
    addAction(main_window->getAction("set_out"));

    addSeparator();
    addAction(main_window->getAction("clear_in"));
    addAction(main_window->getAction("clear_out"));
    addAction(main_window->getAction("clear_in_out"));

    addSeparator();
    addAction(main_window->getAction("seek_in"));
    addAction(main_window->getAction("seek_out"));

    addSeparator();
    addAction(main_window->getAction("repeat_in_out"));
    addAction(main_window->getAction("pl_repeat"));
    addAction(main_window->getAction("pl_shuffle"));
}

// Create main play menu
TMenuPlay::TMenuPlay(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "play_menu", tr("Play")) {

    addAction(main_window->getAction("play_or_pause"));
    addAction(main_window->getAction("stop"));
    addAction(main_window->getAction("play_in_new_window"));

    addSeparator();
    // Forward menu
    addMenu(new TMenuSeekForward(this, main_window));
    // Rewind menu
    addMenu(new TMenuSeekRewind(this, main_window));
    // Seek to time
    addAction(main_window->getAction("seek_to_time"));

    addSeparator();
    // Speed submenu
    addMenu(new TMenuPlaySpeed(this, main_window));
    // In-out point submenu
    addMenu(new TMenuInOut(this, main_window));
}

} // namespace Menu
} // namespace Action
} // namespace Gui


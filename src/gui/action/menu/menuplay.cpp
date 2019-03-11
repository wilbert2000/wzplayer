#include "gui/action/menu/menuplay.h"
#include "gui/mainwindow.h"
#include "gui/action/menu/menuinoutpoints.h"
#include "gui/action/action.h"
#include "settings/preferences.h"
#include "player/player.h"


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
protected:
    virtual void enableActions();
private:
    QActionGroup* group;
};


TMenuPlaySpeed::TMenuPlaySpeed(TMainWindow* mw)
    : TMenu(mw, mw, "speed_menu", tr("Speed"), "speed") {

    group = new QActionGroup(mw);
    group->setExclusive(false);
    group->setEnabled(false);

    TAction* a = new TAction(mw, "speed_normal", tr("Normal speed"), "",
                             Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::normalSpeed);

    addSeparator();
    a = new TAction(mw, "spedd_half", tr("Half speed"), "",
                    Qt::META | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::halveSpeed);

    a = new TAction(mw, "speed_double", tr("Double speed"), "",
                    Qt::ALT | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::doubleSpeed);

    addSeparator();
    a = new TAction(mw, "speed_dec_10", tr("Speed -10%"), "",
                    Qt::SHIFT | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed10);

    a = new TAction(mw, "speed_inc_10", tr("Speed +10%"), "",
                    Qt::CTRL | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed10);

    addSeparator();
    a = new TAction(mw, "speed_dec_4", tr("Speed -4%"), "",
                    Qt::SHIFT | Qt::CTRL | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed4);

    a = new TAction(mw, "speed_inc_4", tr("Speed +4%"), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed4);

    addSeparator();
    a = new TAction(mw, "speed_dec_1", tr("Speed -1%"), "",
                    Qt::SHIFT | Qt::META | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed1);

    a = new TAction(main_window, "speed_inc_1", tr("Speed +1%"), "",
                    Qt::CTRL | Qt::META | Qt::Key_Z);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed1);
}

void TMenuPlaySpeed::enableActions() {
    // Using mset, so useless to set if stopped
    group->setEnabled(player->statePOP());
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
    // Seek to...
    seekToAct = new TAction(main_window, "seek_to", tr("Seek to..."), "",
                            QKeySequence("Ctrl+G"));
    addAction(seekToAct);
    connect(seekToAct, &TAction::triggered,
            main_window, &TMainWindow::showSeekToDialog);

    // Speed submenu
    addSeparator();
    addMenu(new TMenuPlaySpeed(main_window));

    // In-out point submenu
    addMenu(new TMenuInOut(main_window));
}

void TMenuPlay::enableActions() {

    Player::TState s = player->state();
    seekToAct->setEnabled(s == Player::STATE_PLAYING
                          || s == Player::STATE_PAUSED);
}

} // namespace Menu
} // namespace Action
} // namespace Gui


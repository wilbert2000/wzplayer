#include "gui/action/menu/menuplay.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"
#include "settings/preferences.h"
#include "player/player.h"
#include <QStyle>


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

    addAction(main_window->findAction("seek_forward_frame"));
    addSeparator();
    addAction(main_window->findAction("seek_forward1"));
    addAction(main_window->findAction("seek_forward2"));
    addAction(main_window->findAction("seek_forward3"));
    addSeparator();
    addAction(main_window->findAction("play_next"));

    connect(main_window, &TMainWindow::seekForwardDefaultActionChanged,
            this, &TMenuSeekForward::updateDefaultAction);
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(QWidget* parent, TMainWindow* mw);
};

TMenuSeekRewind::TMenuSeekRewind(QWidget* parent, TMainWindow* mw) :
    TMenuSeek(parent, mw, "seek_rewind_menu", tr("Seek backwards"), 5) {

    addAction(main_window->findAction("seek_rewind_frame"));
    addSeparator();
    addAction(main_window->findAction("seek_rewind1"));
    addAction(main_window->findAction("seek_rewind2"));
    addAction(main_window->findAction("seek_rewind3"));
    addSeparator();
    addAction(main_window->findAction("play_prev"));

    connect(main_window, &TMainWindow::seekRewindDefaultActionChanged,
            this, &TMenuSeekRewind::updateDefaultAction);
}


TPlaySpeedGroup::TPlaySpeedGroup(TMainWindow *mw) :
    QActionGroup(mw) {

    setExclusive(false);
    setEnabled(false);

    TAction* a = new TAction(this, "speed_normal", tr("Normal speed"), "",
                             Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::normalSpeed);

    a = new TAction(this, "spedd_half", tr("Half speed"), "",
                    Qt::META | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::halveSpeed);

    a = new TAction(this, "speed_double", tr("Double speed"), "",
                    Qt::ALT | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::doubleSpeed);

    a = new TAction(this, "speed_dec_10", tr("Speed -10%"), "",
                    Qt::SHIFT | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed10);

    a = new TAction(this, "speed_inc_10", tr("Speed +10%"), "",
                    Qt::CTRL | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed10);

    a = new TAction(this, "speed_dec_4", tr("Speed -4%"), "",
                    Qt::SHIFT | Qt::CTRL | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed4);

    a = new TAction(this, "speed_inc_4", tr("Speed +4%"), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed4);

    a = new TAction(this, "speed_dec_1", tr("Speed -1%"), "",
                    Qt::SHIFT | Qt::META | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed1);

    a = new TAction(this, "speed_inc_1", tr("Speed +1%"), "",
                    Qt::CTRL | Qt::META | Qt::Key_Z);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed1);

    connect(player, &Player::TPlayer::stateChanged,
            this, &TPlaySpeedGroup::onPlayerStateChanged);
}

void TPlaySpeedGroup::onPlayerStateChanged(Player::TState state) {
    setEnabled(state == Player::STATE_PLAYING || state == Player::STATE_PAUSED);
}


class TMenuPlaySpeed : public TMenu {
public:
    explicit TMenuPlaySpeed(QWidget* parent, TMainWindow* mw);
};

TMenuPlaySpeed::TMenuPlaySpeed(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "play_speed_menu", tr("Play speed")) {

    addAction(main_window->findAction("speed_normal"));

    addSeparator();
    addAction(main_window->findAction("spedd_half"));
    addAction(main_window->findAction("speed_double"));

    addSeparator();
    addAction(main_window->findAction("speed_dec_10"));
    addAction(main_window->findAction("speed_inc_10"));

    addSeparator();
    addAction(main_window->findAction("speed_dec_4"));
    addAction(main_window->findAction("speed_inc_4"));

    addSeparator();
    addAction(main_window->findAction("speed_dec_1"));
    addAction(main_window->findAction("speed_inc_1"));
}


TInOutGroup::TInOutGroup(TMainWindow *mw) :
    QActionGroup(mw) {

    setObjectName("inoutgroup");
    setExclusive(false);
    setEnabled(false);

    TAction* a  = new TAction(this, "set_in", tr("Set in"), "",
                              QKeySequence("["));
    connect(a, &TAction::triggered, player, &Player::TPlayer::setInPoint);

    a = new TAction(this, "set_out", tr("Set out and repeat"), "",
                    QKeySequence("]"));
    connect(a, &TAction::triggered, player, &Player::TPlayer::setOutPoint);

    a  = new TAction(this, "clear_in", tr("Clear in"), "",
                     QKeySequence("Shift+["));
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInPoint);

    a = new TAction(this, "clear_out", tr("Clear out and repeat"), "",
                    QKeySequence("Shift+]"));
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearOutPoint);

    a = new TAction(this, "clear_in_out", tr("Clear in-out and repeat"), "",
                    Qt::Key_Backspace);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInOutPoints);

    a  = new TAction(this, "seek_in", tr("Seek to in"), "noicon", Qt::Key_Home);
    a->setIcon(mw->style()->standardIcon(QStyle::SP_DirHomeIcon));
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekInPoint);

    a = new TAction(this, "seek_out", tr("Seek to out"), "", Qt::Key_End);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekOutPoint);

    repeatInOutAct = new TAction(this, "repeat_in_out", tr("Repeat in-out"),
                                 "repeat", Qt::Key_Backslash);
    repeatInOutAct->setCheckable(true);
    repeatInOutAct->setChecked(player->mset.loop);
    connect(repeatInOutAct, &TAction::triggered,
            player, &Player::TPlayer::setRepeat);
    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TInOutGroup::onRepeatInOutChanged);
    connect(player, &Player::TPlayer::mediaSettingsChanged,
            this, &TInOutGroup::onRepeatInOutChanged);

    connect(player, &Player::TPlayer::stateChanged,
            this, &TInOutGroup::onPlayerStateChanged);
}

void TInOutGroup::onPlayerStateChanged(Player::TState state) {
    setEnabled(state == Player::STATE_PLAYING || state == Player::STATE_PAUSED);
}

void TInOutGroup::onRepeatInOutChanged() {
    repeatInOutAct->setChecked(player->mset.loop);
}


class TMenuInOut : public TMenu {
public:
    explicit TMenuInOut(QWidget* parent, TMainWindow* mw);
};

TMenuInOut::TMenuInOut(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "in_out_menu", tr("In-out points")) {

    addAction(main_window->findAction("set_in"));
    addAction(main_window->findAction("set_out"));

    addSeparator();
    addAction(main_window->findAction("clear_in"));
    addAction(main_window->findAction("clear_out"));
    addAction(main_window->findAction("clear_in_out"));

    addSeparator();
    addAction(main_window->findAction("seek_in"));
    addAction(main_window->findAction("seek_out"));

    addSeparator();
    addAction(main_window->findAction("repeat_in_out"));
    addAction(main_window->findAction("pl_repeat"));
    addAction(main_window->findAction("pl_shuffle"));
}

// Create main play menu
TMenuPlay::TMenuPlay(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "play_menu", tr("Play")) {

    addAction(main_window->findAction("stop"));
    addAction(main_window->findAction("play_or_pause"));
    addAction(main_window->findAction("pl_play_in_new_window"));

    addSeparator();
    // Forward menu
    addMenu(new TMenuSeekForward(this, main_window));
    // Rewind menu
    addMenu(new TMenuSeekRewind(this, main_window));
    // Seek to time
    addAction(main_window->findAction("seek_to"));

    addSeparator();
    // Speed submenu
    addMenu(new TMenuPlaySpeed(this, main_window));
    // In-out point submenu
    addMenu(new TMenuInOut(this, main_window));
}

} // namespace Menu
} // namespace Action
} // namespace Gui


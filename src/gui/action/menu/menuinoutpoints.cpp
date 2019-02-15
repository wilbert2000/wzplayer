#include "gui/action/menu/menuinoutpoints.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "gui/action/action.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

// Create in-out points menu
TMenuInOut::TMenuInOut(TMainWindow* mw)
    : TMenu(mw, mw, "in_out_menu", tr("&In-out points")) {

    // Put in group to enable/disable together, if we disable the menu users
    // cannot discover the menu because it won't open.
    group = new QActionGroup(this);
    group->setExclusive(false);
    group->setEnabled(false);

    TAction* a  = new TAction(this, "set_in", tr("Set &in"), "",
                              QKeySequence("["));
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setInPoint);

    a = new TAction(this, "set_out", tr("Set &out and repeat"), "",
                    QKeySequence("]"));
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setOutPoint);

    addSeparator();
    a  = new TAction(this, "clear_in", tr("&Clear in"), "",
                     QKeySequence("Shift+["));
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInPoint);

    a = new TAction(this, "clear_out", tr("C&lear out and repeat"),
                    "", QKeySequence("Shift+]"));
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearOutPoint);

    a = new TAction(this, "clear_in_out", tr("Cle&ar in-out and repeat"), "",
                             Qt::Key_Backspace);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInOutPoints);

    addSeparator();
    a  = new TAction(this, "seek_in", tr("&Seek to in"), "",
                     Qt::Key_Home);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekInPoint);

    a = new TAction(this, "seek_out", tr("S&eek to &out"), "",
                    Qt::Key_End);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekOutPoint);

    addSeparator();
    repeatInOutAct = new TAction(this, "repeat_in_out", tr("&Repeat in-out"),
                                 "repeat", Qt::Key_Backslash);
    repeatInOutAct->setCheckable(true);
    group->addAction(repeatInOutAct);
    connect(repeatInOutAct, &TAction::triggered,
            player, &Player::TPlayer::setRepeat);
    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TMenuInOut::upd);

    // Repeat playlist
    addAction(main_window->findChild<TAction*>("pl_repeat"));

    // Shuffle
    addAction(main_window->findChild<TAction*>("pl_shuffle"));
}

void TMenuInOut::enableActions() {
    group->setEnabled(player->statePOP());
}

void TMenuInOut::upd() {
    repeatInOutAct->setChecked(player->mset.loop);
}

void TMenuInOut::onMediaSettingsChanged(TMediaSettings*) {
    upd();
}

void TMenuInOut::onAboutToShow() {
    upd();
}

} // namespace Menu
} // namespace Action
} // namespace Gui


#include "gui/action/menu/menuinoutpoints.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "gui/action/action.h"
#include <QStyle>


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

// Create in-out points menu
TMenuInOut::TMenuInOut(TMainWindow* mw)
    : TMenu(mw, mw, "in_out_menu", tr("In-out points")) {

    // Put in group to enable/disable together, if we disable the menu users
    // cannot discover the menu because it won't open.
    group = new QActionGroup(mw);
    group->setExclusive(false);
    group->setEnabled(false);

    TAction* a  = new TAction(mw, "set_in", tr("Set in"), "", QKeySequence("["));
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setInPoint);

    a = new TAction(mw, "set_out", tr("Set out and repeat"), "",
                    QKeySequence("]"));
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::setOutPoint);

    addSeparator();
    a  = new TAction(mw, "clear_in", tr("Clear in"), "",
                     QKeySequence("Shift+["));
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInPoint);

    a = new TAction(mw, "clear_out", tr("Clear out and repeat"), "",
                    QKeySequence("Shift+]"));
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearOutPoint);

    a = new TAction(mw, "clear_in_out", tr("Clear in-out and repeat"), "",
                    Qt::Key_Backspace);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::clearInOutPoints);

    addSeparator();
    a  = new TAction(mw, "seek_in", tr("Seek to in"), "noicon", Qt::Key_Home);
    a->setIcon(style()->standardIcon(QStyle::SP_DirHomeIcon));
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekInPoint);

    a = new TAction(mw, "seek_out", tr("Seek to out"), "", Qt::Key_End);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::seekOutPoint);


    addSeparator();
    repeatInOutAct = new TAction(mw, "repeat_in_out", tr("Repeat in-out"),
                                 "repeat", Qt::Key_Backslash);
    repeatInOutAct->setCheckable(true);
    addAction(repeatInOutAct);
    group->addAction(repeatInOutAct);
    connect(repeatInOutAct, &TAction::triggered,
            player, &Player::TPlayer::setRepeat);
    connect(player, &Player::TPlayer::InOutPointsChanged,
            this, &TMenuInOut::upd);

    // Repeat playlist
    addAction(mw->findChild<TAction*>("pl_repeat"));

    // Shuffle
    addAction(mw->findChild<TAction*>("pl_shuffle"));
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


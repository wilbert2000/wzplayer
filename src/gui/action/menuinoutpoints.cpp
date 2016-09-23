#include "gui/action/menuinoutpoints.h"

#include <QDebug>

#include "gui/base.h"
#include "player/player.h"
#include "gui/action/action.h"


using namespace Settings;

namespace Gui {
namespace Action {


// Create in-out points menu
TMenuInOut::TMenuInOut(TBase* mw)
    : TMenu(mw, mw, "in_out_points_menu", tr("&In-out points")) {

    // Put in group to enable/disable together, if we disable the menu users
    // cannot discover the menu because it won't open.
    group = new QActionGroup(this);
	group->setExclusive(false);
    group->setEnabled(false);

    TAction* a = new TAction(this, "clear_in_out_points",
                    tr("Cle&ar in-out points and repeat"), "", Qt::Key_Backspace);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(clearInOutPoints()));

    addSeparator();
    a  = new TAction(this, "set_in_point", tr("Set &in point"), "",
                              QKeySequence("["));
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(setInPoint()));

    a = new TAction(this, "set_out_point", tr("Set &out point and repeat"), "",
                    QKeySequence("]"));
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(setOutPoint()));

    addSeparator();
    a  = new TAction(this, "clear_in_point", tr("&Clear in point"), "",
                     QKeySequence("Shift+["));
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(clearInPoint()));

    a = new TAction(this, "clear_out_point", tr("C&lear out point and repeat"),
                    "", QKeySequence("Shift+]"));
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(clearOutPoint()));

    addSeparator();
    a  = new TAction(this, "seek_in_point", tr("&Seek to in point"), "",
                     Qt::Key_Home);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(seekInPoint()));

    a = new TAction(this, "seek_out_point", tr("S&eek to &out point"), "",
                    Qt::Key_End);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(seekOutPoint()));

    addSeparator();
    repeatInOutAct = new TAction(this, "repeat_in_out", tr("&Repeat in-out"),
                                 "repeat", Qt::Key_Backslash);
    repeatInOutAct->setCheckable(true);
    group->addAction(repeatInOutAct);
    connect(repeatInOutAct, SIGNAL(triggered(bool)), player, SLOT(toggleRepeat(bool)));
    connect(player, SIGNAL(InOutPointsChanged()), this, SLOT(upd()));

    // Repeat playlist
    a = new TAction(this, "pl_repeat", tr("&Repeat playlist"), "",
                    Qt::CTRL | Qt::Key_Backslash);
    a->setCheckable(true);

    // Shuffle
    a = new TAction(this, "pl_shuffle", tr("S&huffle playlist"), "shuffle",
                    Qt::ALT | Qt::Key_Backslash);
    a->setCheckable(true);

    // Don't add actions to main window, playlist will add them
}

void TMenuInOut::enableActions() {
    group->setEnabled(player->statePOP());
    // repeat playlist always enabled
    // shuffle playlist always enabled
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

} // namespace Action
} // namespace Gui


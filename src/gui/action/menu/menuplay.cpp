#include "gui/action/menu/menuplay.h"

#include <QToolButton>

#include "gui/mainwindow.h"
#include "gui/playlist/playlist.h"
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
                     const QString& sign) :
    TMenu(mw, mw, name, text),
    seek_sign(sign) {

    setDefaultAction(menuAction());
    connect(this, &TMenuSeek::triggered, this, &TMenuSeek::onTriggered);
    connect(main_window, &TMainWindow::settingsChanged,
            this, &TMenuSeek::setJumpTexts);
    connect(main_window->getPlaylist(),
            &Playlist::TPlaylist::enablePrevNextChanged,
            this, &TMenuSeek::updateDefaultAction);
}

// Map in to action
TAction* TMenuSeek::intToAction(int i) const {

    switch (i) {
    case 0: return frameAct;
    case 1: return seek1Act;
    case 2: return seek2Act;
    case 3: return seek3Act;
    default: return plAct;
    }
}

// Return int for action
int TMenuSeek::actionToInt(QAction* action) const {

    QString name = action->objectName();
    if (name.startsWith("frame")) {
        return 0;
    }
    if (name.endsWith("1")) {
        return 1;
    }
    if (name.endsWith("2")) {
        return 2;
    }
    if (name.endsWith("3")) {
        return 3;
    }
    return 4;
}

// Set default action and pref when menu triggered
void TMenuSeek::onTriggered(QAction* action) {

    setDefaultAction(action);
    pref->seeking_current_action = actionToInt(action);
}

// Update default actions menu and associated tool buttons
void TMenuSeek::updateDefaultAction() {

    QAction* action = intToAction(pref->seeking_current_action);
    if (!action->isEnabled()) {
        action = menuAction();
    }

    if (action != defaultAction()) {
        setDefaultAction(action);

        // Set default action asscociated tool buttons.
        QString name = menuAction()->objectName() + "_toolbutton";
        QList<QToolButton*> buttons = main_window->findChildren<QToolButton*>(name);
        foreach(QToolButton* button, buttons) {
            button->setDefaultAction(action);
        }
    }
}

void TMenuSeek::enableActions() {

    bool e = player->statePOP();
    frameAct->setEnabled(e);
    seek1Act->setEnabled(e);
    seek2Act->setEnabled(e);
    seek3Act->setEnabled(e);

    updateDefaultAction();
}

// Return seek string to use in menu
QString TMenuSeek::timeForJumps(int secs) const {

    int minutes = (int) secs / 60;
    int seconds = secs % 60;
    QString m = tr("%1 minute(s)").arg(minutes);
    QString s = tr("%1 second(s)").arg(seconds);

    QString txt;
    if (minutes == 0) {
        txt = s;
    } else if (seconds == 0) {
        txt = m;
    } else {
        txt = tr("%1 and %2", "combine minutes (%1) and secs (%2)")
              .arg(m).arg(s);
    }
    return tr("%1%2", "add + or - sign (%1) to seek text (%2)")
            .arg(seek_sign).arg(txt);
}

// Set seek actions text from preferences
void TMenuSeek::setJumpTexts() {

    seek1Act->setTextAndTip(timeForJumps(pref->seeking1));
    seek2Act->setTextAndTip(timeForJumps(pref->seeking2));
    seek3Act->setTextAndTip(timeForJumps(pref->seeking3));
}


class TMenuSeekForward: public TMenuSeek {
public:
    explicit TMenuSeekForward(TMainWindow* mnw);
};

TMenuSeekForward::TMenuSeekForward(TMainWindow* mw) :
    TMenuSeek(mw, "forward_menu", tr("&Forward"),
              tr("+", "sign to use in menu for forward seeking")) {

    frameAct = new TAction(this, "frame_step", tr("Fra&me step"), "",
                           Qt::ALT | Qt::Key_Right);
    connect(frameAct, &TAction::triggered, player, &Player::TPlayer::frameStep);

    addSeparator();
    seek1Act = new TAction(this, "forward1", "", "", Qt::Key_Right);
    seek1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
    connect(seek1Act, &TAction::triggered, player, &Player::TPlayer::forward1);

    seek2Act = new TAction(this, "forward2", "", "", Qt::SHIFT | Qt::Key_Right);
    connect(seek2Act, &TAction::triggered, player, &Player::TPlayer::forward2);

    seek3Act = new TAction(this, "forward3", "", "", Qt::CTRL | Qt::Key_Right);
    connect(seek3Act, &TAction::triggered, player, &Player::TPlayer::forward3);

    addSeparator();
    plAct = main_window->findChild<TAction*>("play_next");
    addAction(plAct);

    setJumpTexts();
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(TMainWindow* mw);
};

// Create rewind menu as descendant from TMenuSeek
TMenuSeekRewind::TMenuSeekRewind(TMainWindow* mw) :
    TMenuSeek(mw, "rewind_menu", tr("&Rewind"),
              tr("-", "sign to use in menu for rewind seeking")) {

    frameAct = new TAction(this, "frame_back_step", tr("Fra&me back step"), "",
                           Qt::ALT | Qt::Key_Left);
    connect(frameAct, &TAction::triggered, player,
            &Player::TPlayer::frameBackStep);

    addSeparator();
    seek1Act = new TAction(this, "rewind1", "", "", Qt::Key_Left);
    seek1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
    connect(seek1Act, &TAction::triggered, player, &Player::TPlayer::rewind1);

    seek2Act = new TAction(this, "rewind2", "", "", Qt::SHIFT | Qt::Key_Left);
    connect(seek2Act, &TAction::triggered, player, &Player::TPlayer::rewind2);

    seek3Act = new TAction(this, "rewind3", "", "", Qt::CTRL | Qt::Key_Left);
    connect(seek3Act, &TAction::triggered, player, &Player::TPlayer::rewind3);

    addSeparator();
    plAct = main_window->findChild<TAction*>("play_prev");
    addAction(plAct);

    setJumpTexts();
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
    : TMenu(mw, mw, "speed_menu", tr("Spee&d"), "speed") {

    group = new QActionGroup(this);
    group->setExclusive(false);
    group->setEnabled(false);


    TAction* a = new TAction(this, "normal_speed", tr("&Normal speed"), "",
                             Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::normalSpeed);

    addSeparator();
    a = new TAction(this, "halve_speed", tr("&Half speed"), "",
                    Qt::META | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::halveSpeed);
    a = new TAction(this, "double_speed", tr("&Double speed"), "",
                    Qt::ALT | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::doubleSpeed);

    addSeparator();
    a = new TAction(this, "dec_speed", tr("Speed &-10%"), "",
                    Qt::SHIFT | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed10);
    a = new TAction(this, "inc_speed", tr("Speed &+10%"), "",
                    Qt::CTRL | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed10);

    addSeparator();
    a = new TAction(this, "dec_speed_4", tr("Speed -&4%"), "",
                    Qt::SHIFT | Qt::CTRL | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed4);
    a = new TAction(this, "inc_speed_4", tr("&Speed +4%"), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed4);

    addSeparator();
    a = new TAction(this, "dec_speed_1", tr("Speed -&1%"), "",
                    Qt::SHIFT | Qt::META | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decSpeed1);
    a = new TAction(this, "inc_speed_1", tr("S&peed +1%"), "",
                    Qt::CTRL | Qt::META | Qt::Key_Z);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incSpeed1);
}

void TMenuPlaySpeed::enableActions() {
    // Using mset, so useless to set if stopped
    group->setEnabled(player->statePOP());
}


// Create main play menu
TMenuPlay::TMenuPlay(TMainWindow* mw)
    : TMenu(mw, mw, "play_menu", tr("&Play"), "noicon") {

    addAction(main_window->findChild<TAction*>("play_or_pause"));
    addAction(main_window->findChild<TAction*>("stop"));
    addAction(main_window->findChild<TAction*>("play_new_window"));

    addSeparator();

    // Forward menu
    TMenuSeek* forward_menu = new TMenuSeekForward(main_window);
    addMenu(forward_menu);
    // Rewind menu
    TMenuSeek* rewind_menu = new TMenuSeekRewind(main_window);
    addMenu(rewind_menu);
    // Let forward and rewind work in tandem
    connect(forward_menu, &TMenuSeek::triggered,
            rewind_menu, &TMenuSeek::updateDefaultAction);
    connect(rewind_menu, &TMenuSeek::triggered,
            forward_menu, &TMenuSeek::updateDefaultAction);

    // Seek to...
    seekToAct = new TAction(this, "seek_to", tr("S&eek to..."), "",
                            QKeySequence("Ctrl+G"));
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


#include "gui/action/menuplay.h"

#include <QDebug>
#include <QToolButton>

#include "gui/base.h"
#include "gui/playlist/playlist.h"
#include "gui/action/widgetactions.h"
#include "gui/action/menuinoutpoints.h"
#include "gui/action/action.h"
#include "settings/preferences.h"
#include "core.h"
#include "images.h"

using namespace Settings;

namespace Gui {
namespace Action {

// Base class for seeking forward/rewinding
TMenuSeek::TMenuSeek(QWidget* parent,
                     TBase* mainwindow,
                     const QString& name,
                     const QString& text,
                     const QString& sign) :
    TMenu(parent, mainwindow, name, text),
    seek_sign(sign) {

    setDefaultAction(menuAction());
    connect(this, SIGNAL(triggered(QAction*)),
            this, SLOT(onTriggered(QAction*)));
    connect(main_window, SIGNAL(preferencesChanged()),
            this, SLOT(setJumpTexts()));
    connect(main_window->getPlaylist(), SIGNAL(enablePrevNextChanged()),
            this, SLOT(updateDefaultAction()));
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
        //qDebug() << "Gui::Action::TMenuseek::updateDefaultAction: updating default action for"
        //         << menuAction()->objectName()
        //         << "from" << defaultAction()->objectName()
        //         << "to" << action->objectName();
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

    bool e = main_window->getCore()->statePOP();
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
    explicit TMenuSeekForward(QWidget* parent,
                              TBase* mainwindow,
                              TCore* core,
                              Gui::Playlist::TPlaylist* playlist);
};

// Create forward menu as descendant from TMenuSeek
TMenuSeekForward::TMenuSeekForward(QWidget* parent,
                                   TBase* mainwindow,
                                   TCore* core,
                                   Gui::Playlist::TPlaylist* playlist) :
    TMenuSeek(parent, mainwindow, "forward_menu", tr("&Forward"),
              tr("+", "sign to use in menu for forward seeking")) {

    frameAct = new TAction(this, "frame_step", tr("Fra&me step"), "", Qt::ALT | Qt::Key_Right);
    connect(frameAct, SIGNAL(triggered()), core, SLOT(frameStep()));

    addSeparator();
    seek1Act = new TAction(this, "forward1", "", "", Qt::Key_Right);
    seek1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
    connect(seek1Act, SIGNAL(triggered()), core, SLOT(sforward()));

    seek2Act = new TAction(this, "forward2", "", "", Qt::SHIFT | Qt::Key_Right);
    connect(seek2Act, SIGNAL(triggered()), core, SLOT(forward()));

    seek3Act = new TAction(this, "forward3", "", "", Qt::CTRL | Qt::Key_Right);
    connect(seek3Act, SIGNAL(triggered()), core, SLOT(fastforward()));

    addSeparator();
    plAct = playlist->findChild<TAction*>("pl_next");
    addAction(plAct);

    setJumpTexts();
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(QWidget* parent,
                             TBase* mainwindow,
                             TCore* core,
                             Gui::Playlist::TPlaylist* playlist);
};

// Create rewind menu as descendant from TMenuSeek
TMenuSeekRewind::TMenuSeekRewind(QWidget* parent,
                                 TBase* mainwindow,
                                 TCore* core,
                                 Gui::Playlist::TPlaylist* playlist) :
    TMenuSeek(parent, mainwindow, "rewind_menu", tr("&Rewind"),
              tr("-", "sign to use in menu for rewind seeking")) {

    frameAct = new TAction(this, "frame_back_step", tr("Fra&me back step"), "", Qt::ALT | Qt::Key_Left);
    connect(frameAct, SIGNAL(triggered()), core, SLOT(frameBackStep()));

    addSeparator();
    seek1Act = new TAction(this, "rewind1", "", "", Qt::Key_Left);
    seek1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
    connect(seek1Act, SIGNAL(triggered()), core, SLOT(srewind()));

    seek2Act = new TAction(this, "rewind2", "", "", Qt::SHIFT | Qt::Key_Left);
    connect(seek2Act, SIGNAL(triggered()), core, SLOT(rewind()));

    seek3Act = new TAction(this, "rewind3", "", "", Qt::CTRL | Qt::Key_Left);
    connect(seek3Act, SIGNAL(triggered()), core, SLOT(fastrewind()));

    addSeparator();
    plAct = playlist->findChild<TAction*>("pl_prev");
    addAction(plAct);

    setJumpTexts();
}


class TMenuPlaySpeed : public TMenu {
public:
    explicit TMenuPlaySpeed(TBase* mw, TCore* c);
protected:
    virtual void enableActions();
private:
	TCore* core;
	QActionGroup* group;
};


TMenuPlaySpeed::TMenuPlaySpeed(TBase* mw, TCore* c)
    : TMenu(mw, mw, "speed_menu", tr("Spee&d"), "speed")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);


    TAction* a = new TAction(this, "normal_speed", tr("&Normal speed"), "",
                             Qt::Key_Z);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(normalSpeed()));

    addSeparator();
    a = new TAction(this, "halve_speed", tr("&Half speed"), "",
                    Qt::ALT | Qt::Key_Z);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(halveSpeed()));
    a = new TAction(this, "double_speed", tr("&Double speed"), "",
                    Qt::META | Qt::Key_Z);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(doubleSpeed()));

    addSeparator();
    a = new TAction(this, "dec_speed", tr("Speed &-10%"), "",
                    Qt::SHIFT | Qt::Key_Z);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(decSpeed10()));
    a = new TAction(this, "inc_speed", tr("Speed &+10%"), "",
                    Qt::CTRL | Qt::Key_Z);
    group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(incSpeed10()));

	addSeparator();
    a = new TAction(this, "dec_speed_4", tr("Speed -&4%"), "",
                    Qt::SHIFT | Qt::CTRL | Qt::Key_Z);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(decSpeed4()));
    a = new TAction(this, "inc_speed_4", tr("&Speed +4%"), "",
                    Qt::ALT | Qt::CTRL | Qt::Key_Z);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed4()));

	addSeparator();
    a = new TAction(this, "dec_speed_1", tr("Speed -&1%"), "",
                    Qt::SHIFT | Qt::META | Qt::Key_Z);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed1()));
    a = new TAction(this, "inc_speed_1", tr("S&peed +1%"), "",
                    Qt::CTRL | Qt::META | Qt::Key_Z);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed1()));

    addActionsTo(main_window);
}

void TMenuPlaySpeed::enableActions() {
	// Using mset, so useless to set if stopped
    group->setEnabled(core->statePOP());
}


// Create main play menu
TMenuPlay::TMenuPlay(TBase* mw, TCore* c, Gui::Playlist::TPlaylist* playlist)
    : TMenu(mw, mw, "play_menu", tr("&Play"), "noicon")
    , core(c) {

    addAction(playlist->findChild<TAction*>("play_or_pause"));
    addAction(playlist->findChild<TAction*>("stop"));

	addSeparator();

    // Forward menu
    QMenu* forward_menu = new TMenuSeekForward(this, main_window, core, playlist);
    addMenu(forward_menu);
    // Rewind menu
    QMenu* rewind_menu = new TMenuSeekRewind(this, main_window, core, playlist);
    addMenu(rewind_menu);
    // Let forward and rewind work in tandem
    connect(forward_menu, SIGNAL(triggered(QAction*)),
            rewind_menu, SLOT(updateDefaultAction()));
    connect(rewind_menu, SIGNAL(triggered(QAction*)),
            forward_menu, SLOT(updateDefaultAction()));

    // Seek to...
    seekToAct = new TAction(this, "seek_to", tr("S&eek to..."), "", QKeySequence("Ctrl+G"));
    connect(seekToAct, SIGNAL(triggered()), main_window, SLOT(showSeekToDialog()));

    // Speed submenu
    addSeparator();
    addMenu(new TMenuPlaySpeed(main_window, core));

    // In-out point submenu
    addMenu(playlist->getInOutMenu());

    addActionsTo(main_window);
}

void TMenuPlay::enableActions() {

    TCoreState s = core->state();
    seekToAct->setEnabled(s == STATE_PLAYING || s == STATE_PAUSED);
}

} // namespace Action
} // namespace Gui


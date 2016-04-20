#include "gui/action/menuplay.h"

#include <QDebug>
#include <QToolButton>

#include "images.h"
#include "core.h"
#include "settings/preferences.h"
#include "gui/action/action.h"
#include "gui/action/widgetactions.h"
#include "gui/playlist.h"


using namespace Settings;

namespace Gui {
namespace Action {

TMenuSeek::TMenuSeek(QWidget* parent,
                     QWidget* mainwindow,
                     const QString& name,
                     const QString& text,
                     const QString& icon,
                     const QString& sign) :
    TMenu(parent, name, text, icon),
    main_window(mainwindow),
    seek_sign(sign) {

    connect(this, SIGNAL(triggered(QAction*)),
            this, SLOT(onTriggered(QAction*)));
    connect(mainwindow, SIGNAL(preferencesChanged()),
            this, SLOT(setJumpTexts()));
}

TAction* TMenuSeek::intToAction(int i) const {

    switch (i) {
    case 0: return frameAct;
    case 1: return seek1Act;
    case 2: return seek2Act;
    case 3: return seek3Act;
    default: return plAct;
    }
}

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

void TMenuSeek::onTriggered(QAction* action) {

    setDefaultAction(action);
    pref->seeking_current_action = actionToInt(action);
}

void TMenuSeek::enableActions(bool stopped, bool, bool) {

    bool e = !stopped;
    frameAct->setEnabled(e);
    seek1Act->setEnabled(e);
    seek2Act->setEnabled(e);
    seek3Act->setEnabled(e);
}

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

void TMenuSeek::setJumpTexts() {

    seek1Act->setTextAndTip(timeForJumps(pref->seeking1));
    seek2Act->setTextAndTip(timeForJumps(pref->seeking2));
    seek3Act->setTextAndTip(timeForJumps(pref->seeking3));
}

void TMenuSeek::peerTriggered(QAction* action) {

    // Set default action for this menu
    QAction* this_action;
    QString name = action->objectName();
    if (name.startsWith("frame")) {
        this_action = frameAct;
    } else if (name.endsWith("1")) {
        this_action = seek1Act;
    } else if (name.endsWith("2")) {
        this_action = seek2Act;
    } else if (name.endsWith("3")) {
        this_action = seek3Act;
    } else {
        this_action = plAct;
    }
    setDefaultAction(this_action);

    // Set default action asscociated tool buttons
    QList<QToolButton*> buttons = main_window->findChildren<QToolButton*>(
                                      objectName() + "_toolbutton");
    foreach(QToolButton* button, buttons) {
        button->setDefaultAction(this_action);
    }
}

class TMenuSeekForward: public TMenuSeek {
public:
    explicit TMenuSeekForward(QWidget* parent,
                              QWidget* mainwindow,
                              TCore* core,
                              TPlaylist* playlist);
};

TMenuSeekForward::TMenuSeekForward(QWidget* parent,
                                   QWidget* mainwindow,
                                   TCore* core,
                                   TPlaylist* playlist) :
    TMenuSeek(parent, mainwindow, "forward_menu", tr("&Forward"), "forward1",
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

    setDefaultAction(intToAction(pref->seeking_current_action));
    setJumpTexts();
}


class TMenuSeekRewind: public TMenuSeek {
public:
    explicit TMenuSeekRewind(QWidget* parent,
                             QWidget* mainwindow,
                             TCore* core,
                             TPlaylist* playlist);
};

TMenuSeekRewind::TMenuSeekRewind(QWidget* parent,
                                 QWidget* mainwindow,
                                 TCore* core,
                                 TPlaylist* playlist) :
    TMenuSeek(parent, mainwindow, "rewind_menu", tr("&Rewind"), "rewind1",
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

    setDefaultAction(intToAction(pref->seeking_current_action));
    setJumpTexts();
}


class TMenuInOut : public TMenu {
public:
    explicit TMenuInOut(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	QActionGroup* group;
	TAction* repeatAct;
};

TMenuInOut::TMenuInOut(QWidget* parent, TCore* c)
    : TMenu(parent, "in_out_points_menu", tr("&In-out points"))
	, core(c) {

    // Put in group to enable/disable together, if we disable the menu users
    // cannot discover the menu because it won't open.
    group = new QActionGroup(this);
	group->setExclusive(false);
    group->setEnabled(false);

    TAction* a  = new TAction(this, "set_in_point", tr("Set &in point"), "", QKeySequence("["));
	group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(setInPoint()));

    a = new TAction(this, "set_out_point", tr("Set &out point"), "", QKeySequence("]"));
	group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(setOutPoint()));

    a = new TAction(this, "clear_in_out_points", tr("&Clear in-out points"), "", Qt::Key_Backspace);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), core, SLOT(clearInOutPoints()));

	addSeparator();
    repeatAct = new TAction(this, "repeat_in_out", tr("&Repeat in-out"), "repeat", Qt::Key_Backslash);
	repeatAct->setCheckable(true);
    group->addAction(repeatAct);
	connect(repeatAct, SIGNAL(triggered(bool)), core, SLOT(toggleRepeat(bool)));
    // Currently no one sets it

	addActionsTo(parent);
}

void TMenuInOut::enableActions(bool stopped, bool, bool) {
    group->setEnabled(!stopped);
}

void TMenuInOut::onMediaSettingsChanged(TMediaSettings* mset) {
	repeatAct->setChecked(mset->loop);
}

void TMenuInOut::onAboutToShow() {
	repeatAct->setChecked(core->mset.loop);
}


class TMenuPlaySpeed : public TMenu {
public:
	explicit TMenuPlaySpeed(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool);
private:
	TCore* core;
	QActionGroup* group;
};


TMenuPlaySpeed::TMenuPlaySpeed(QWidget *parent, TCore *c)
    : TMenu(parent, "speed_menu", tr("Sp&eed"), "speed")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	// TODO: make checkable, to see if normal speed?
	TAction* a = new TAction(this, "normal_speed", tr("&Normal speed"), "", Qt::Key_Backspace);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(normalSpeed()));

	addSeparator();
	a = new TAction(this, "halve_speed", tr("&Half speed"), "", Qt::Key_BraceLeft);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(halveSpeed()));
	a = new TAction(this, "double_speed", tr("&Double speed"), "", Qt::Key_BraceRight);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(doubleSpeed()));

	addSeparator();
	a = new TAction(this, "dec_speed", tr("Speed &-10%"), "", Qt::Key_BracketLeft);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed10()));
	a = new TAction(this, "inc_speed", tr("Speed &+10%"), "", Qt::Key_BracketRight);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed10()));

	addSeparator();
	a = new TAction(this, "dec_speed_4", tr("Speed -&4%"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed4()));
	a = new TAction(this, "inc_speed_4", tr("&Speed +4%"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed4()));

	addSeparator();
	a = new TAction(this, "dec_speed_1", tr("Speed -&1%"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decSpeed1()));
	a = new TAction(this, "inc_speed_1", tr("S&peed +1%"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incSpeed1()));

	addActionsTo(parent);
}

void TMenuPlaySpeed::enableActions(bool stopped, bool, bool) {
	// Using mset, so useless to set if stopped
	group->setEnabled(!stopped);
}


TMenuPlay::TMenuPlay(QWidget* parent, TCore* c, Gui::TPlaylist* plist)
    : TMenu(parent, "play_menu", tr("&Play"), "noicon")
	, core(c)
	, playlist(plist)
	, pauseIcon(Images::icon("pause"))
	, playIcon(Images::icon("play")) {

    playAct = new TAction(this, "play", tr("Play"), "", Qt::Key_MediaPlay, false);
	parent->addAction(playAct);
	connect(playAct, SIGNAL(triggered()), core, SLOT(play()));

	playOrPauseAct = new TAction(this, "play_or_pause", tr("&Play"), "play", Qt::Key_Space);
	playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause")); // MCE remote key
	connect(playOrPauseAct, SIGNAL(triggered()), core, SLOT(playOrPause()));

	pauseAct = new TAction(this, "pause", tr("Pause"), "",
						   QKeySequence("Media Pause"), false); // MCE remote key
	parent->addAction(pauseAct);
	connect(pauseAct, SIGNAL(triggered()), core, SLOT(pause()));

	stopAct = new TAction(this, "stop", tr("&Stop"), "", Qt::Key_MediaStop);
	connect(stopAct, SIGNAL(triggered()), core, SLOT(stop()));

	connect(core, SIGNAL(stateChanged(TCoreState)), this, SLOT(onStateChanged(TCoreState)));

	addSeparator();

    // Forward menu
    QMenu* fw = new TMenuSeekForward(this, parent, core, playlist);
    addMenu(fw);
    // Rewind menu
    QMenu* rw = new TMenuSeekRewind(this, parent, core, playlist);
    addMenu(rw);

    connect(fw, SIGNAL(triggered(QAction*)),
            rw, SLOT(peerTriggered(QAction*)));
    connect(rw, SIGNAL(triggered(QAction*)),
            fw, SLOT(peerTriggered(QAction*)));

    // Seek
    seekToAct = new TAction(this, "seek_to", tr("S&eek to..."), "", QKeySequence("Ctrl+G"));
    connect(seekToAct, SIGNAL(triggered()), parent, SLOT(showSeekToDialog()));

    // Speed submenu
    addSeparator();
    addMenu(new TMenuPlaySpeed(parent, core));

    // A-B submenu
    addMenu(new TMenuInOut(parent, core));

	addActionsTo(parent);
}

void TMenuPlay::onStateChanged(TCoreState state) {

	playAct->setEnabled(state != STATE_PLAYING);
	// playOrPauseAct always enabled
	if (state == STATE_PLAYING) {
		playOrPauseAct->setTextAndTip(tr("&Pause"));
		playOrPauseAct->setIcon(pauseIcon);
	} else {
		playOrPauseAct->setTextAndTip(tr("&Play"));
		playOrPauseAct->setIcon(playIcon);
	}
	pauseAct->setEnabled(state == STATE_PLAYING);
	// Allowed to push stop twice...
	// stopAct->setEnabled(core->state() != STATE_STOPPED);
}

void TMenuPlay::enableActions(bool stopped, bool, bool) {

	playAct->setEnabled(core->state() != STATE_PLAYING);
	// playOrPauseAct always enabled
	pauseAct->setEnabled(core->state() == STATE_PLAYING);
	// Allowed to push stop twice...
	// stopAct->setEnabled(core->state() != STATE_STOPPED);
    seekToAct->setEnabled(!stopped);
}

} // namespace Action
} // namespace Gui


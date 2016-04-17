#include "gui/action/menuplay.h"
#include "images.h"
#include "core.h"
#include "settings/preferences.h"
#include "gui/action/action.h"
#include "gui/action/widgetactions.h"
#include "gui/playlist.h"


using namespace Settings;

namespace Gui {
namespace Action {

class TMenuAB : public TMenu {
public:
	explicit TMenuAB(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	QActionGroup* group;
	TAction* repeatAct;
};

TMenuAB::TMenuAB(QWidget* parent, TCore* c)
    : TMenu(parent, "ab_menu", tr("&A-B section"))
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	TAction* a  = new TAction(this, "set_a_marker", tr("Set &A marker"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(setAMarker()));

	a = new TAction(this, "set_b_marker", tr("Set &B marker"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(setBMarker()));

	a = new TAction(this, "clear_ab_markers", tr("&Clear A-B markers"));
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(clearABMarkers()));

	addSeparator();
	repeatAct = new TAction(this, "repeat", tr("&Repeat"));
	repeatAct->setCheckable(true);
	repeatAct->setChecked(core->mset.loop);
	group->addAction(repeatAct);
	connect(repeatAct, SIGNAL(triggered(bool)), core, SLOT(toggleRepeat(bool)));
	// Currently no one else sets it

	addActionsTo(parent);
}

void TMenuAB::enableActions(bool stopped, bool, bool) {
	// Uses mset, so useless to set if stopped
	group->setEnabled(!stopped);
}

void TMenuAB::onMediaSettingsChanged(TMediaSettings* mset) {
	repeatAct->setChecked(mset->loop);
}

void TMenuAB::onAboutToShow() {
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
	frameBackStepAct = new TAction(this, "frame_back_step", tr("Fra&me back step"), "", Qt::Key_Comma);
	connect(frameBackStepAct, SIGNAL(triggered()), core, SLOT(frameBackStep()));

	frameStepAct = new TAction(this, "frame_step", tr("&Frame step"), "", Qt::Key_Period);
	connect(frameStepAct, SIGNAL(triggered()), core, SLOT(frameStep()));

	addSeparator();
	rewind1Act = new TAction(this, "rewind1", "", "rewind10s", Qt::Key_Left);
	rewind1Act->addShortcut(QKeySequence("Shift+Ctrl+B")); // MCE remote key
	connect(rewind1Act, SIGNAL(triggered()), core, SLOT(srewind()));

	forward1Act = new TAction(this, "forward1", "", "forward10s", Qt::Key_Right);
	forward1Act->addShortcut(QKeySequence("Shift+Ctrl+F")); // MCE remote key
	connect(forward1Act, SIGNAL(triggered()), core, SLOT(sforward()));

	rewind2Act = new TAction(this, "rewind2", "", "rewind1m", Qt::Key_Down);
	connect(rewind2Act, SIGNAL(triggered()), core, SLOT(rewind()));

	forward2Act = new TAction(this, "forward2", "", "forward1m", Qt::Key_Up);
	connect(forward2Act, SIGNAL(triggered()), core, SLOT(forward()));

	rewind3Act = new TAction(this, "rewind3", "", "rewind10m", Qt::Key_PageDown);
	connect(rewind3Act, SIGNAL(triggered()), core, SLOT(fastrewind()));

	forward3Act = new TAction(this, "forward3", "", "forward10m", Qt::Key_PageUp);
	connect(forward3Act, SIGNAL(triggered()), core, SLOT(fastforward()));

	QList<QAction*> rewind_actions;
	rewind_actions << rewind1Act << rewind2Act << rewind3Act;
	rewindbutton_action = new TSeekingButton(rewind_actions, this);
	rewindbutton_action->setObjectName("rewindbutton_action");
	parent->addAction(rewindbutton_action);

	QList<QAction*> forward_actions;
	forward_actions << forward1Act << forward2Act << forward3Act;
	forwardbutton_action = new TSeekingButton(forward_actions, this);
	forwardbutton_action->setObjectName("forwardbutton_action");
	parent->addAction(rewindbutton_action);

	// TODO: doubles playlist next prev action
	addSeparator();
	playNextAct = new TAction(this, "play_next", tr("&Next"), "next", Qt::Key_Greater);
	playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
	connect(playNextAct, SIGNAL(triggered()), playlist, SLOT(playNext()));

	playPrevAct = new TAction(this, "play_prev", tr("Pre&vious"), "previous", Qt::Key_Less);
	playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
	connect(playPrevAct, SIGNAL(triggered()), playlist, SLOT(playPrev()));

	// A-B submenu
	addSeparator();
	addMenu(new TMenuAB(parent, core));
	// Speed submenu
	addMenu(new TMenuPlaySpeed(parent, core));

	addSeparator();
	gotoAct = new TAction(this, "jump_to", tr("&Jump to..."), "jumpto", QKeySequence("Ctrl+J"));
	connect(gotoAct, SIGNAL(triggered()), parent, SLOT(showGotoDialog()));

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
	bool e = !stopped;
	frameStepAct->setEnabled(e);
	frameBackStepAct->setEnabled(e);
	rewind1Act->setEnabled(e);
	rewind2Act->setEnabled(e);
	rewind3Act->setEnabled(e);
	forward1Act->setEnabled(e);
	forward2Act->setEnabled(e);
	forward3Act->setEnabled(e);
	playPrevAct->setEnabled(playlist->count() > 0);
	playNextAct->setEnabled(playlist->count() > 0);
	gotoAct->setEnabled(e);
}

QString TMenuPlay::timeForJumps(int secs) {

	int minutes = (int) secs / 60;
	int seconds = secs % 60;

	if (minutes == 0) {
		return tr("%n second(s)", "", seconds);
	}
	if (seconds == 0) {
		return tr("%n minute(s)", "", minutes);
	}
	QString m = tr("%n minute(s)", "", minutes);
	QString s = tr("%n second(s)", "", seconds);
	return tr("%1 and %2").arg(m).arg(s);
}

void TMenuPlay::setJumpText(TAction* rewindAct, TAction* forwardAct, int secs) {

	QString s = timeForJumps(secs);
	rewindAct->setTextAndTip(tr("-%1").arg(s));
	forwardAct->setTextAndTip(tr("+%1").arg(s));
}

void TMenuPlay::setJumpTexts() {

	setJumpText(rewind1Act, forward1Act, pref->seeking1);
	setJumpText(rewind2Act, forward2Act, pref->seeking2);
	setJumpText(rewind3Act, forward3Act, pref->seeking3);
}

void TMenuPlay::retranslateStrings() {

	rewindbutton_action->setText(tr("3 in 1 rewind"));
	forwardbutton_action->setText(tr("3 in 1 forward"));
	setJumpTexts();
}

} // namespace Action
} // namespace Gui


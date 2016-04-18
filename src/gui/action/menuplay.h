#ifndef GUI_PLAYMENU_H
#define GUI_PLAYMENU_H

#include "gui/action/menu.h"
#include "corestate.h"


class TCore;

namespace Gui {

class TPlaylist;

namespace Action {

class TAction;
class TSeekingButton;


class TMenuPlay : public TMenu {
	Q_OBJECT
public:
	explicit TMenuPlay(QWidget* parent, TCore* c, Gui::TPlaylist* plist);
	void setJumpTexts();
	void retranslateStrings();
protected:
	virtual void enableActions(bool stopped, bool, bool);
private:
	TCore* core;
	Gui::TPlaylist* playlist;

	TAction* playAct;
	TAction* playOrPauseAct;
	QIcon pauseIcon;
	QIcon playIcon;
	TAction* pauseAct;
	TAction* stopAct;
	TAction* frameBackStepAct;
	TAction* frameStepAct;
	TAction* rewind1Act;
	TAction* forward1Act;
	TAction* rewind2Act;
	TAction* forward2Act;
	TAction* rewind3Act;
	TAction* forward3Act;
	TSeekingButton* rewindbutton_action;
	TSeekingButton* forwardbutton_action;
    TAction* gotoAct;

	QString timeForJumps(int secs);
	void setJumpText(TAction* rewindAct, TAction* forwardAct, int secs);

private slots:
	void onStateChanged(TCoreState state);
}; // class TMenuPlay

} // namespace Action
} // namespace Gui

#endif // GUI_PLAYMENU_H

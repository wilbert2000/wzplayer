#ifndef GUI_PLAYMENU_H
#define GUI_PLAYMENU_H

#include "gui/action/menus.h"


namespace Gui {

class TPlayMenu : public TMenu {
	Q_OBJECT
public:
	explicit TPlayMenu(QWidget* parent, TCore* c, Gui::TPlaylist* plist);
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
	TAction* playPrevAct;
	TAction* playNextAct;
	TAction* gotoAct;
private slots:
	void onStateChanged(TCore::State state);
};

} // namespace Gui

#endif // GUI_PLAYMENU_H

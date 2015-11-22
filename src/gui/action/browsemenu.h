#ifndef GUI_BROWSEMENU_H
#define GUI_BROWSEMENU_H

#include "config.h"
#include "gui/action/menus.h"


namespace Gui {

class TBrowseMenu : public TMenu {
	Q_OBJECT

public:
	TBrowseMenu(QWidget* parent, TCore* c);

protected:
	virtual void enableActions(bool stopped, bool, bool);

private:
	TCore* core;

	TMenu* titlesMenu;
	TActionGroup* titleGroup;
	TMenu* chaptersMenu;
	TActionGroup* chapterGroup;
	TMenu* anglesMenu;
	TActionGroup* angleGroup;

#if PROGRAM_SWITCH
	TMenu* programMenu;
	TActionGroup* programGroup;
#endif

	TAction* dvdnavUpAct;
	TAction* dvdnavDownAct;
	TAction* dvdnavLeftAct;
	TAction* dvdnavRightAct;

	TAction* dvdnavMenuAct;
	TAction* dvdnavPrevAct;
	TAction* dvdnavSelectAct;
	TAction* dvdnavMouseAct;

private slots:
	void updateTitles();
	void updateChapters();
	void updateAngles();
};

} // namespace Gui

#endif // GUI_BROWSEMENU_H

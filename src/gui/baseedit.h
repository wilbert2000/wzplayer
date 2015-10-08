#ifndef _GUI_BASEEDIT_H
#define _GUI_BASEEDIT_H

#include "gui/baseplus.h"
#include "gui/editabletoolbar.h"


namespace Gui {

class TBaseEdit : public TBasePlus {
	Q_OBJECT

public:
	TBaseEdit();
	virtual ~TBaseEdit();

	virtual void loadConfig(const QString& gui_group);
	virtual void saveConfig(const QString& gui_group);

protected:
	TAction* editControlWidgetAct;

	virtual void retranslateStrings();
	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

private:
	TEditableToolbar* controlwidget;
}; // class TBaseEdit

} // namespace Gui

#endif // _GUI_BASEEDIT_H

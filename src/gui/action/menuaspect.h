#ifndef GUI_MENUASPECT_H
#define GUI_MENUASPECT_H

#include "gui/action/menu.h"


namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

class TMenuAspect : public TMenu {
	Q_OBJECT
public:
    explicit TMenuAspect(TBase* mw);

protected:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();

private:
	TActionGroup* group;
	TAction* aspectAutoAct;
	TAction* aspectDisabledAct;
	TAction* nextAspectAct;

	void upd();

private slots:
	void onAspectRatioChanged(int);
};

} // namespace Action
} // namespace Gui

#endif // GUI_MENUASPECT_H

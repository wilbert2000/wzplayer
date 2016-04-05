#ifndef GUI_MENUASPECT_H
#define GUI_MENUASPECT_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

class TMenuAspect : public TMenu {
	Q_OBJECT
public:
	explicit TMenuAspect(QWidget* parent, TCore* c);

protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();

private:
	TCore* core;
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

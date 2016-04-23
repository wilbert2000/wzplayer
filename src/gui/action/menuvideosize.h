#ifndef GUI_VIDEOSIZEMENU_H
#define GUI_VIDEOSIZEMENU_H

#include "gui/action/actiongroup.h"
#include "gui/action/menu.h"


class TCore;
class TPlayerWindow;

namespace Gui {

class TBase;

namespace Action {

class TVideoSizeGroup : public TActionGroup {
	Q_OBJECT
public:
	explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
	int size_percentage;

public slots:
	void updateVideoSizeGroup();

private:
	TPlayerWindow* playerWindow;

	void uncheck();
};


class TMenuVideoSize : public TMenu {
	Q_OBJECT
public:
    TMenuVideoSize(TBase* mw, TPlayerWindow* pw);

protected:
    virtual void enableActions();
	virtual void onAboutToShow();

private:
	TPlayerWindow* playerWindow;
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
	TAction* currentSizeAct;

	bool optimizeSizeFactorPreDef(int factor, int predef_factor);
	void upd();

private slots:
	void onVideoSizeFactorChanged();
	void optimizeSizeFactor();
}; // class TMenuVideoSize

} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOSIZEMENU_H

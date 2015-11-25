#ifndef GUI_VIDEOSIZEMENU_H
#define GUI_VIDEOSIZEMENU_H

#include "gui/action/actiongroup.h"
#include "gui/action/menu.h"


class TPlayerWindow;

namespace Gui {

class TBase;

class TVideoSizeGroup : public TActionGroup {
	Q_OBJECT
public:
	explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
	int size_percentage;
	TPlayerWindow* playerWindow;
public slots:
	void enableVideoSizeGroup(bool on);
	void updateVideoSizeGroup();
private:
	void uncheck();
};


class TMenuVideoSize : public TMenu {
	Q_OBJECT
public:
	TMenuVideoSize(TBase* mw, TPlayerWindow* pw);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onAboutToShow();
private:
	TBase* mainWindow;
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
	TAction* currentSizeAct;

	bool optimizeSizeFactorPreDef(int& factor, int predef_factor);
	void upd();

private slots:
	virtual void fullscreenChanged();
	void optimizeSizeFactor();
	void onVideoSizeChanged();
};

} // namespace Gui

#endif // GUI_VIDEOSIZEMENU_H

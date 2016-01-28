#ifndef GUI_VIDEOSIZEMENU_H
#define GUI_VIDEOSIZEMENU_H

#include <QTimer>
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
	void enableVideoSizeGroup(bool on);
	void updateVideoSizeGroup();

private:
	TPlayerWindow* playerWindow;

	void uncheck();
};


class TMenuVideoSize : public TMenu {
	Q_OBJECT
public:
	TMenuVideoSize(TBase* mw, TCore* core, TPlayerWindow* pw);

protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onAboutToShow();

private:
	TBase* mainWindow;
	TPlayerWindow* playerWindow;
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
	TAction* currentSizeAct;

	QTimer update_size_factor_timer;
	int block_update_size_factor;

	bool optimizeSizeFactorPreDef(int factor, int predef_factor);
	void upd();

private slots:
	void updateSizeFactor();
	void unlockSizeFactor();
	void onFullscreenChanged();
	void onFullscreenChangedDone();
	void onVideoSizeFactorChanged();
	void onZoomChanged(double);
	void onMainWindowResizeEvent(QResizeEvent* event);
	void optimizeSizeFactor();
}; // class TMenuVideoSize

} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOSIZEMENU_H

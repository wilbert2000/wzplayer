#ifndef GUI_VIDEOSIZEMENU_H
#define GUI_VIDEOSIZEMENU_H

#include "gui/action/actiongroup.h"
#include "gui/action/menu.h"


class TPlayerWindow;

namespace Gui {

class TVideoSizeGroup : public TActionGroup {
	Q_OBJECT
public:
	explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
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
	TMenuVideoSize(QWidget* parent, TPlayerWindow* pw);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onAboutToShow();
private:
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
private slots:
	virtual void fullscreenChanged();
};

} // namespace Gui

#endif // GUI_VIDEOSIZEMENU_H

#ifndef VIDEOSIZEGROUP_H
#define VIDEOSIZEGROUP_H

#include "gui/action/actiongroup.h"
#include <QMenu>

class TPlayerWindow;

namespace Gui {

class TVideoSizeGroup : public TActionGroup {
	Q_OBJECT

public:
	explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
	virtual ~TVideoSizeGroup();

public slots:
	void enableVideoSizeGroup(bool on);
	void updateVideoSizeGroup();

private:
	TPlayerWindow* playerWindow;

	void uncheck();
};

class TVideoSizeMenu : public QMenu {
	Q_OBJECT

public:
	TVideoSizeMenu(QWidget* parent, TPlayerWindow* pw);
	virtual ~TVideoSizeMenu();

	void enableVideoSize(bool on);
	void retranslateStrings();

private:
	TVideoSizeGroup* sizeGroup;
	TAction* doubleSizeAct;

private slots:
	void onAboutToShow();
};

} // namespace Gui
#endif // VIDEOSIZEGROUP_H

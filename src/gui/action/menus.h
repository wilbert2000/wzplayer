#ifndef VIDEOSIZEGROUP_H
#define VIDEOSIZEGROUP_H

#include "gui/action/actiongroup.h"
#include <QMenu>

class TPlayerWindow;
class TCore;

namespace Gui {

class TCCMenu : public QMenu {
	Q_OBJECT

public:
	explicit TCCMenu(QWidget* parent, TCore* c);
	virtual ~TCCMenu();

	void retranslateStrings();

private:
	TCore* core;

	TActionGroup* ccGroup;
	TActionGroupItem* ccNoneAct;
	TActionGroupItem* ccChannel1Act;
	TActionGroupItem* ccChannel2Act;
	TActionGroupItem* ccChannel3Act;
	TActionGroupItem* ccChannel4Act;

private slots:
	void onAboutToShow();
};

class TOnTopMenu : public QMenu {
	Q_OBJECT

public:
	explicit TOnTopMenu(QWidget* parent);
	virtual ~TOnTopMenu();

	void retranslateStrings();

private:
	TActionGroup* onTopActionGroup;
	TActionGroupItem* onTopAlwaysAct;
	TActionGroupItem* onTopNeverAct;
	TActionGroupItem* onTopWhilePlayingAct;

	TAction* toggleStayOnTopAct;

private slots:
	void onAboutToShow();
};


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

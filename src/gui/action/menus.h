#ifndef VIDEOSIZEGROUP_H
#define VIDEOSIZEGROUP_H

#include "gui/action/actiongroup.h"
#include <QMenu>

class TPlayerWindow;
class TCore;

namespace Gui {

class TAudioChannelMenu : public QMenu {
	Q_OBJECT

public:
	explicit TAudioChannelMenu(QWidget* parent, TCore* c);

	TActionGroup* channelsGroup;
	void retranslateStrings();

private:
	TCore* core;

	TActionGroupItem* channelsStereoAct;
	TActionGroupItem* channelsSurroundAct;
	TActionGroupItem* channelsFull51Act;
	TActionGroupItem* channelsFull61Act;
	TActionGroupItem* channelsFull71Act;

private slots:
	void onAboutToShow();
};

class TCCMenu : public QMenu {
	Q_OBJECT

public:
	explicit TCCMenu(QWidget* parent, TCore* c);

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

class TSubFPSMenu : public QMenu {
	Q_OBJECT

public:
	explicit TSubFPSMenu(QWidget* parent, TCore* c);

	TActionGroup* subFPSGroup;
	void retranslateStrings();

private:
	TCore* core;

	TActionGroupItem* subFPSNoneAct;
	TActionGroupItem* subFPS23976Act;
	TActionGroupItem* subFPS24Act;
	TActionGroupItem* subFPS25Act;
	TActionGroupItem* subFPS29970Act;
	TActionGroupItem* subFPS30Act;

private slots:
	void onAboutToShow();
};

class TOnTopMenu : public QMenu {
	Q_OBJECT

public:
	explicit TOnTopMenu(QWidget* parent);

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

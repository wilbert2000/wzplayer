#ifndef GUI_SIZEGRIP_H
#define GUI_SIZEGRIP_H

#include <QToolBar>

class QStyleOptionToolBar;

namespace Gui {

class TSizeGrip : public QToolBar {

public:
	TSizeGrip(QToolBar* tb);
	virtual ~TSizeGrip();

	void follow();

protected:
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual bool event(QEvent*);

private:
	QToolBar* toolbar;
	bool resizing;
	QPoint p;
	QRect r;

	void followToolbar();
};

} // namespace Gui

#endif // GUI_SIZEGRIP_H

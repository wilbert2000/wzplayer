#ifndef GUI_SIZEGRIP_H
#define GUI_SIZEGRIP_H

#include <QToolBar>

class QStyleOptionToolBar;

namespace Gui {

class TSizeGrip : public QToolBar {
	Q_OBJECT

public:
	TSizeGrip(QToolBar* tb);
	virtual ~TSizeGrip();

	void follow();

signals:
	void saveSizeHint();

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

private slots:
	void onOrientationChanged(Qt::Orientation orientation);

};

} // namespace Gui

#endif // GUI_SIZEGRIP_H

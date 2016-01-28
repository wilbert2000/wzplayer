#ifndef GUI_SIZEGRIP_H
#define GUI_SIZEGRIP_H

#include <QToolBar>

class QStyleOptionToolBar;

namespace Gui {
namespace Action {


class TSizeGrip : public QToolBar {
	Q_OBJECT

public:
	TSizeGrip(QWidget* parent, QToolBar* tb);
	virtual ~TSizeGrip();

	void follow();

public slots:
	void delayedHide();

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

}; // class TSizeGrip

} // namespace Action
} // namespace Gui

#endif // GUI_SIZEGRIP_H

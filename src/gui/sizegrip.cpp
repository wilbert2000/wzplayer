#include "gui/sizegrip.h"
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QDebug>

namespace Gui {

TSizeGrip::TSizeGrip(QToolBar* tb)
	: QToolBar(0)
	, toolbar(tb)
	, resizing(false) {

	setWindowFlags(toolbar->windowFlags());
	setCursor(Qt::SizeHorCursor);
	followToolbar();
}

TSizeGrip::~TSizeGrip() {
}

void TSizeGrip::mousePressEvent(QMouseEvent* event) {
	//qDebug("TSizeGrip::mousePressEvent");

	resizing = true;
	p = event->globalPos();
	r = toolbar->geometry();
	event->accept();
}

void TSizeGrip::followToolbar() {

	int w = style()->pixelMetric(QStyle::PM_ToolBarHandleExtent) + 2;
	int d = 1;
	QRect t = toolbar->geometry();
	if (isLeftToRight())
		setGeometry(t.right() - d, t.top(), w, t.height());
	else
		setGeometry(t.left() - w + d, t.top(), w, t.height());
}

void TSizeGrip::follow() {

	if (!resizing)
		followToolbar();
}

void TSizeGrip::mouseMoveEvent(QMouseEvent* event) {
	//qDebug("TSizeGrip::mouseMoveEvent");

	if (event->buttons() != Qt::LeftButton)
		return;
	if (!resizing)
		return;
	if (toolbar->testAttribute(Qt::WA_WState_ConfigPending))
		return;

	QPoint np(event->globalPos());

	QSize ns;
	ns.rheight() = toolbar->height();
	if (isLeftToRight())
		ns.rwidth() = r.width() + np.x() - p.x();
	else
		ns.rwidth() = r.width() - (np.x() - p.x());

	ns = ns.expandedTo(toolbar->minimumSize()).expandedTo(toolbar->minimumSizeHint()).boundedTo(toolbar->maximumSize());

	QPoint o;
	QRect nr(o, ns);
	if (isLeftToRight())
		nr.moveTopLeft(r.topLeft());
	else
		nr.moveTopRight(r.topRight());

	toolbar->setGeometry(nr);
	followToolbar();

	event->accept();
}

bool TSizeGrip::event(QEvent* e) {

	if (e->type() == QEvent::MouseButtonRelease) {
		resizing = false;
	}

	// Not QToolbar!!
	return QWidget::event(e);
}

} // namespace Gui

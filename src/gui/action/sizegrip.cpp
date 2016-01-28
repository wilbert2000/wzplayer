#include "gui/action/sizegrip.h"
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolBar>
#include <QDebug>

namespace Gui {
namespace Action {


TSizeGrip::TSizeGrip(QWidget* parent, QToolBar* tb)
	: QToolBar(parent)
	, toolbar(tb)
	, resizing(false) {

	// Copy stay on top and floating
	setWindowFlags(toolbar->windowFlags());
	onOrientationChanged(toolbar->orientation());
	connect(toolbar, SIGNAL(orientationChanged(Qt::Orientation)),
			this, SLOT(onOrientationChanged(Qt::Orientation)));
}

void TSizeGrip::onOrientationChanged(Qt::Orientation orientation) {
	//qDebug("TSizeGrip::onOrientationChanged");

	setOrientation(orientation);
	if (orientation == Qt::Horizontal)
		setCursor(Qt::SizeHorCursor);
	else
		setCursor(Qt::SizeVerCursor);
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

	int wh = style()->pixelMetric(QStyle::PM_ToolBarHandleExtent) + 3;
	int d = 1;
	QRect t = toolbar->geometry();
	if (orientation() == Qt::Horizontal)
		if (isLeftToRight())
			setGeometry(t.right() - d, t.top(), wh, t.height());
		else
			setGeometry(t.left() - wh + d, t.top(), wh, t.height());
	else
		if (isLeftToRight())
			setGeometry(t.left(), t.bottom() - d, t.width(), wh);
		else
			setGeometry(t.left(), t.top() - wh + d, t.width(), wh);
}

void TSizeGrip::follow() {

	if (!resizing)
		followToolbar();
}

void TSizeGrip::mouseMoveEvent(QMouseEvent* event) {
	//qDebug("TSizeGrip::mouseMoveEvent");

	if (!resizing || toolbar->testAttribute(Qt::WA_WState_ConfigPending))
		return;

	QPoint np(event->globalPos());

	QSize ns;
	if (orientation() == Qt::Horizontal) {
		ns.rheight() = toolbar->height();
		if (isLeftToRight())
			ns.rwidth() = r.width() + np.x() - p.x();
		else
			ns.rwidth() = r.width() - (np.x() - p.x());
	} else {
		ns.rwidth() = toolbar->width();
		if (isLeftToRight())
			ns.rheight() = r.height() + np.y() - p.y();
		else
			ns.rheight() = r.height() - (np.y() - p.y());
	}

	ns = ns.expandedTo(toolbar->minimumSize()).expandedTo(toolbar->minimumSizeHint()).boundedTo(toolbar->maximumSize());

	QPoint o;
	QRect nr(o, ns);
	if (orientation() == Qt::Horizontal) {
		if (isLeftToRight())
			nr.moveTopLeft(r.topLeft());
		else
			nr.moveTopRight(r.topRight());
	} else {
		if (isLeftToRight())
			nr.moveBottomLeft(r.bottomLeft());
		else
			nr.moveBottomRight(r.bottomRight());
	}

	toolbar->setGeometry(nr);
	followToolbar();
	emit saveSizeHint();

	event->accept();
}

void TSizeGrip::delayedHide() {

	if (!underMouse() && !toolbar->underMouse())
		hide();
}

bool TSizeGrip::event(QEvent* e) {

	switch (e->type()) {
		case QEvent::MouseButtonRelease:
			resizing = false;
			return true;
		case QEvent::Leave:
			hide();
			return true;

		// Events to hide for QToolBar, but passed to QWidget
		case QEvent::MouseMove:
		case QEvent::MouseButtonPress:

		// Events to hide for QToolbar, so it won't change the mouse cursor
		case QEvent::Enter:
		case QEvent::CursorChange:
		case QEvent::HoverEnter:
		case QEvent::HoverMove:
		case QEvent::HoverLeave:
			return QWidget::event(e);

		default: ;
	}

	return QToolBar::event(e);
}

} // namespace Action
} // namespace Gui

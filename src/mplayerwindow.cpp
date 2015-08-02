/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "mplayerwindow.h"
#include "global.h"
#include "desktopinfo.h"
#include "colorutils.h"

#ifndef MINILIB
#include "images.h"
#endif

#include <QLabel>
#include <QTimer>
#include <QCursor>
#include <QEvent>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QTimer>
#include <QDebug>

#if LOGO_ANIMATION
#include <QPropertyAnimation>
#endif

/* ---------------------------------------------------------------------- */

MplayerLayer::MplayerLayer(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
#if REPAINT_BACKGROUND_OPTION
	, repaint_background(false)
#endif
	, normal_background(true)
{
	setObjectName("mplayerlayer");

	// If not set parent mplayerwindow will not get mouse move events
	setMouseTracking(true);
	setFocusPolicy( Qt::NoFocus );

	setAutoFillBackground(true);
	setMinimumSize( QSize(0,0) );

#ifndef Q_OS_WIN
	#if QT_VERSION < 0x050000
	setAttribute(Qt::WA_OpaquePaintEvent);
	#if QT_VERSION >= 0x040400
	setAttribute(Qt::WA_NativeWindow);
	#endif
	setAttribute(Qt::WA_PaintUnclipped);
	//setAttribute(Qt::WA_PaintOnScreen);
	#endif
#endif

}

MplayerLayer::~MplayerLayer() {
}

#if REPAINT_BACKGROUND_OPTION
void MplayerLayer::setRepaintBackground(bool b) {
	qDebug("MplayerLayer::setRepaintBackground: %d", b);
	repaint_background = b;
}

void MplayerLayer::paintEvent( QPaintEvent * e ) {
	//qDebug("MplayerLayer::paintEvent: repaint_background: %d", repaint_background);

	// repaint_background is the option to draw the background or not,
	// preventing flicker and speeding up redraws when set to false.
	// When repaint background is false the background still needs to be
	// repainted when no video is loaded, which is controlled by normal_background.
	// MplayerWindow::playingStarted calls setFastBackground to set it and
	// MplayerWindow::playingStopped calls restoreNormalBackground to clear it,
	// together with the Qt::WA_PaintOnScreen attribute (when not on Windows).

	if (repaint_background || normal_background) {
		//qDebug("MplayerLayer::paintEvent: painting");
		QPainter painter(this);
		painter.eraseRect( e->rect() );
		//painter.fillRect( e->rect(), QColor(255,0,0) );
	}
}
#endif

void MplayerLayer::setFastBackground() {
	qDebug("MplayerLayer::setFastBackground");
	normal_background = false;

#ifndef Q_OS_WIN
	setAttribute(Qt::WA_PaintOnScreen);
#endif
}

void MplayerLayer::restoreNormalBackground() {
	qDebug("MplayerLayer::restoreNormalBackground");

#ifndef Q_OS_WIN
	setAttribute(Qt::WA_PaintOnScreen, false);
#endif

	normal_background = true;
}


/* ---------------------------------------------------------------------- */

MplayerWindow::MplayerWindow(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
	, main_window_moved(false)
	, video_width(0)
	, video_height(0)
	, aspect((double) 4/3)
	, monitoraspect(0)
	, logo(0)
	, offset_x(0)
	, offset_y(0)
	, zoom_factor(1.0)
	, delay_left_click(false)
	, left_click_timer(0)
	, double_clicked(false)
#if LOGO_ANIMATION
	, animated_logo(false)
#endif
#ifdef SHAREWIDGET
	, corner_widget(0)
#endif
	, drag_pos(0, 0)
	, dragging(false)
	, kill_fake_event(false)
	, fullscreen(false)
	, autohide_cursor(false)
	, check_hide_mouse_last_position(0, 0)
	, autohide_interval(1000)
{
	setObjectName("mplayerwindow");

	setFocusPolicy( Qt::StrongFocus );
	setMouseTracking(true);

	setMinimumSize( QSize(0,0) );
	setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Expanding );

	setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( this, QColor(0,0,0) );

	mplayerlayer = new MplayerLayer(this);

	logo = new QLabel( mplayerlayer );
	logo->setObjectName("mplayerwindow logo");
	logo->setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( logo, QColor(0,0,0) );
	logo->setMouseTracking(true);

	QVBoxLayout * mplayerlayerLayout = new QVBoxLayout( mplayerlayer );
	mplayerlayerLayout->addWidget( logo, 0, Qt::AlignHCenter | Qt::AlignVCenter );

	left_click_timer = new QTimer(this);
	left_click_timer->setSingleShot(true);
	left_click_timer->setInterval(qApp->doubleClickInterval()+10);
	connect(left_click_timer, SIGNAL(timeout()), this, SIGNAL(leftClicked()));

	check_hide_mouse_timer = new QTimer(this);
	check_hide_mouse_timer->setSingleShot(true);
	check_hide_mouse_timer->setInterval(autohide_interval);
	connect(check_hide_mouse_timer, SIGNAL(timeout()), this, SLOT(checkHideMouse()) );

	left_button_pressed_time = new QTime();

	retranslateStrings();
}

MplayerWindow::~MplayerWindow() {
}

void MplayerWindow::resizeEvent(QResizeEvent*)
{
	//qDebug("MplayerWindow::resizeEvent: %d, %d", e->size().width(), e->size().height() );
	updateVideoWindow();
}

void MplayerWindow::setResolution( int w, int h, double aspect) {
	video_width = w;
	video_height = h;

	setAspect(aspect);
}

void MplayerWindow::setMonitorAspect(double asp) {
	monitoraspect = asp;
}

void MplayerWindow::setAspect( double asp) {
    aspect = asp;
	if (monitoraspect!=0) {
		aspect = aspect / monitoraspect * DesktopInfo::desktop_aspectRatio(this);
	}
	updateVideoWindow();
}

void MplayerWindow::updateVideoWindow()
{
	//qDebug("MplayerWindow::updateVideoWindow width: %d height: %d aspect: %f zoom: %f ofsx: %d ofsy %d",
	//	width(), height(), aspect, zoom_factor, offset_x, offset_y);

	int w = width();
	int h = height();

	// Select best fit: height adjusted or width adjusted,
	// in case video aspect does not match the window aspect ratio.
	// Height adjusted gives horizontal black borders.
	// Width adjusted gives vertical black borders,
	if (aspect != 0) {
		int height_adjust = w / aspect + 0.5;
		if (height_adjust <= h) {
			// adjust height
			h = height_adjust;
		} else {
			// adjust width
			w = h * aspect + 0.5;
		}
	}

	// Zoom
	w = w * zoom_factor + 0.5;
	h = h * zoom_factor + 0.5;

	// Center
	int x = (width() - w) / 2;
	int y = (height() - h) / 2;

	// Move
	x += offset_x;
	y += offset_y;

	mplayerlayer->setGeometry(x, y, w, h);

	//qDebug("MplayerWindow::updateVideoWindow x: %d y: %d w: %d, h: %d", x, y, w, h);
}

void MplayerWindow::mousePressEvent( QMouseEvent * event) {
	qDebug( "MplayerWindow::mousePressEvent" );

	event->accept();

	// Ignore second press event after a double click.
	if (!double_clicked) {
		showHiddenCursor(false);

		if ((event->button() == Qt::LeftButton)
				&& (event->modifiers() == Qt::NoModifier)) {
			left_button_pressed_time->start();
			drag_pos = event->globalPos();
			dragging = false;
			main_window_moved = false;
			// Catch release_events with button still down
			// Happens only when mouse capture is released?
			kill_fake_event = true;
		}
	}
}

void MplayerWindow::mouseMoveEvent(QMouseEvent * event) {
	//qDebug( "MplayerWindow::mouseMoveEvent" );

	event->accept();

	// No longer kill release event with button down
	kill_fake_event = false;

	if ((event->buttons() == Qt::LeftButton)
			&& (event->modifiers() == Qt::NoModifier)
			&& fullscreen
			&& !double_clicked) {

		QPoint pos = event->globalPos();
		QPoint diff = pos - drag_pos;

		// Don't start dragging before having moved startDragDistance
		// or startDragTime has elapsed. A timer for drag time does not work
		// because Qts capture of the mouse blocks timer events.
		if (!dragging &&
			((diff.manhattanLength() > QApplication::startDragDistance())
			|| (left_button_pressed_time->elapsed() >= QApplication::startDragTime()))) {
			dragging = true;

			// Cancel pending left click
			if (delay_left_click)
				left_click_timer->stop();

			setCursor(QCursor(Qt::DragMoveCursor));
			qDebug("MplayerWindow::mouseMoveEvent started drag");
		}

		if (dragging) {
			drag_pos = pos;
			moveVideo(diff.x(), diff.y());
		}
	} else if (event->buttons() == Qt::NoButton)
		showHiddenCursor(autohide_cursor);

	// For DVD nav
	emit mouseMoved(event->pos());
}

// Return whether this event is accused of dragging.
// Returning false will cancel the event.
bool MplayerWindow::checkDragging(QMouseEvent * event) {

	if (double_clicked)
		return true;

	bool kill = kill_fake_event;
	kill_fake_event = false;

	if (dragging) {
		dragging = false;
		setCursor(QCursor(Qt::ArrowCursor));
		qDebug( "MplayerWindow::mouseReleaseEvent stopped dragging" );
		return false;
	}

	// Move of main window. Moves can be canceled, but that should be handled below.
	// Set by BaseGui::moveEvent
	if (main_window_moved) {
		main_window_moved = false;
		qDebug("MplayerWindow::mouseReleaseEvent canceled release event dragging mainwindow");
		return false;
	}

	// Except for dragging, also nice when event queue is not keeping up with
	// events. After the mouse has been captured, mouse release events sometimes
	// do not come through until the mouse moved (on Qt 4.8 KDE 4.1.14.9).
	if (left_button_pressed_time->elapsed() >= QApplication::startDragTime()) {
		qDebug("MplayerWindow::mouseReleaseEvent canceled left click taking longer as %d ms",
			   QApplication::startDragTime());
		return false;
	}

	// Dragging the mouse more then startDragDistance delivers a mouse release event,
	// before the first mouse move event, while the left mouse is still down.
	// (on Qt 4.8, KDE 4.1.14.9), Like an end-of-drag or what?
	// Only when mouse not captured. Kill it.
	if (kill) {
		QPoint pos = event->globalPos();
		QPoint diff = pos - drag_pos;
		if (diff.manhattanLength() > QApplication::startDragDistance()) {
			qDebug("MplayerWindow::mouseReleaseEvent killed release event");
			return false;
		}
	}

	// No dragging
	return true;
}

void MplayerWindow::mouseReleaseEvent(QMouseEvent * event) {
	qDebug( "MplayerWindow::mouseReleaseEvent");

	event->accept();

	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() != Qt::NoModifier) {
			qDebug("MplayerWindow::mouseReleaseEvent ignoring modified event");
		} else if (checkDragging(event)) {
			if (delay_left_click) {
				if (double_clicked) {
					double_clicked = false;
					qDebug( "MplayerWindow::mouseReleaseEvent ignoring event after double click" );
				} else {
					// Delay left click until double click has a chance to arrive
					left_click_timer->start();
					qDebug( "MplayerWindow::mouseReleaseEvent delaying left click" );
				}
			} else {
				double_clicked = false;
				// Click right away
				qDebug( "MplayerWindow::mouseReleaseEvent emitting left click" );
				emit leftClicked();
			}
		}
	} else if (event->button() == Qt::MidButton) {
		emit middleClicked();
	} else if (event->button() == Qt::XButton1) {
		emit xbutton1Clicked();
	} else if (event->button() == Qt::XButton2) {
		emit xbutton2Clicked();
	} else if (event->button() == Qt::RightButton) {
		emit rightClicked();
	}

	// autoHideCursor will detect if it is not wanted
	autoHideCursorStartTimer();
}

void MplayerWindow::mouseDoubleClickEvent( QMouseEvent * event ) {
	qDebug( "MplayerWindow::mouseDoubleClickEvent" );

	event->accept();

	if ((event->button() == Qt::LeftButton)
			&& (event->modifiers() == Qt::NoModifier)) {
		double_clicked = true;
		if (delay_left_click) {
			left_click_timer->stop();
		}
		emit doubleClicked();
	}
}

void MplayerWindow::wheelEvent( QWheelEvent * event ) {
	qDebug("MplayerWindow::wheelEvent: delta: %d", event->delta());

	event->accept();

	if (event->orientation() == Qt::Vertical) {
		if (event->delta() >= 0)
			emit wheelUp();
		else
			emit wheelDown();
	} else {
		qDebug("MplayerWindow::wheelEvent: horizontal event received, doing nothing");
	}
}

QSize MplayerWindow::sizeHint() const {
	return QSize( video_width, video_height );
}

void MplayerWindow::setZoom(double factor) {
	if (factor < ZOOM_MIN)
		factor = ZOOM_MIN;
	else if (factor > ZOOM_MAX)
		factor = ZOOM_MAX;
	if (qAbs(zoom_factor - factor) > 0.0001) {
		zoom_factor = factor;
		updateVideoWindow();
	}
}

void MplayerWindow::resetZoomAndPan() {
	zoom_factor = 1.0;
	offset_x = 0;
	offset_y = 0;
	updateVideoWindow();
}

void MplayerWindow::aboutToEnterFullscreen() {
	zoom_factor = 1.0;
	offset_x = 0;
	offset_y = 0;
	fullscreen = true;
}

void MplayerWindow::aboutToExitFullscreen() {
	zoom_factor = 1.0;
	offset_x = 0;
	offset_y = 0;
	fullscreen = false;
}

double MplayerWindow::zoom() { return zoom_factor; }

void MplayerWindow::moveVideo(int dx, int dy) {
	offset_x += dx;
	offset_y += dy;
	updateVideoWindow();
}

void MplayerWindow::panLeft() {
	moveVideo( +16, 0 );
}

void MplayerWindow::panRight() {
	moveVideo( -16, 0 );
}

void MplayerWindow::panUp() {
	moveVideo( 0, +16 );
}

void MplayerWindow::panDown() {
	moveVideo( 0, -16 );
}

// Language change stuff
void MplayerWindow::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#ifdef SHAREWIDGET
void MplayerWindow::setMouseTrackingInclChildren(QWidget *w) {
	qDebug() << "MplayerWindow::setMouseTrackingInclChildren: " << w->objectName();

	w->setMouseTracking(true);

	QObjectList children = w->children();
	for (int n = 0; n < children.count(); n++) {
		QObject* child = children[n];
		if (child->isWidgetType()) {
			setMouseTrackingInclChildren(static_cast<QWidget *>(child));
		}
	}
}

void MplayerWindow::setCornerWidget(QWidget * w) {
	corner_widget = w;
	setMouseTrackingInclChildren(corner_widget);

	QHBoxLayout * blayout = new QHBoxLayout;
	blayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));
	blayout->addWidget(corner_widget);

	QVBoxLayout * layout = new QVBoxLayout(this);
	layout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
	layout->addLayout(blayout);
}
#endif

#if USE_COLORKEY
void MplayerWindow::setColorKey( QColor c ) {
	ColorUtils::setBackgroundColor( mplayerlayer, c );
}
#endif

void MplayerWindow::autoHideCursorStartTimer() {
	check_hide_mouse_last_position = QCursor::pos();
	check_hide_mouse_timer->start();
}

void MplayerWindow::showHiddenCursor(bool startTimer) {
	if (cursor().shape() == Qt::BlankCursor) {
		setCursor(QCursor(Qt::ArrowCursor));
	}
	if (startTimer) {
		autoHideCursorStartTimer();
	} else {
		check_hide_mouse_timer->stop();
	}
}

// Called by timer
void MplayerWindow::checkHideMouse() {
	if (!autohide_cursor
		|| ((QCursor::pos() - check_hide_mouse_last_position).manhattanLength()
			> SHOW_MOUSE_TRESHOLD)) {
		showHiddenCursor(true);
	} else {
		if (cursor().shape() == Qt::ArrowCursor) {
			setCursor(QCursor(Qt::BlankCursor));
		}
		autoHideCursorStartTimer();
	}
}

// TODO: disable autoHideCUrsor when paused?
// Currently only start and stop toggle autohide_cursor
void MplayerWindow::setAutoHideCursor(bool enable) {
	autohide_cursor = enable;
	if (autohide_cursor)
		autoHideCursorStartTimer();
	else showHiddenCursor(false);
}

void MplayerWindow::playingStarted() {
	qDebug("MplayerWindow::playingStarted");
	// No longer needed. Now done by Core::initPlaying
	// repaint();
	mplayerlayer->setFastBackground();
	setAutoHideCursor(true);
}

void MplayerWindow::playingStopped() {
	qDebug("MplayerWindow::playingStopped");
	mplayerlayer->restoreNormalBackground();
	// Clear background right away.
	// Pro: no artifacts when things take a little while.
	// Against: more flicker when switching bright videos in playlist.
	// repaint();
	setAutoHideCursor(false);
}

void MplayerWindow::setLogoVisible(bool b) {
	qDebug("MplayerWindow::setLogoVisible %d", b);

#ifdef SHAREWIDGET
	if (corner_widget) {
		corner_widget->setVisible(b);
	}
#endif

#if !LOGO_ANIMATION
	logo->setVisible(b);
#else
	if (!animated_logo) {
		logo->setVisible(b);
	} else {
		if (b) {
			logo->show();
			QPropertyAnimation * animation = new QPropertyAnimation(logo, "pos");
			animation->setDuration(200);
			animation->setEasingCurve(QEasingCurve::OutBounce);
			animation->setStartValue(QPoint(logo->x(), 0 - logo->y()));
			animation->setEndValue(logo->pos());
			animation->start();
		} else {
			QPropertyAnimation * animation = new QPropertyAnimation(logo, "pos");
			animation->setDuration(200);
			animation->setEasingCurve(QEasingCurve::OutBounce);
			animation->setEndValue(QPoint(width(), logo->y()));
			animation->setStartValue(logo->pos());
			animation->start();
			connect(animation, SIGNAL(finished()), logo, SLOT(hide()));
			//logo->hide();
		}
	}
#endif
}

void MplayerWindow::retranslateStrings() {
	//qDebug("MplayerWindow::retranslateStrings");
#ifndef MINILIB
	logo->setPixmap( Images::icon("background") );
#endif
}


#include "moc_mplayerwindow.cpp"

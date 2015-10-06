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

#include "playerwindow.h"

#include <QLabel>
#include <QTimer>
#include <QCursor>
#include <QEvent>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QDebug>

#if LOGO_ANIMATION
#include <QPropertyAnimation>
#endif

#include "desktopinfo.h"
#include "colorutils.h"
#include "images.h"

#include "proc/playerprocess.h"


/* ---------------------------------------------------------------------- */

TPlayerLayer::TPlayerLayer(QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
	, repaint_background(false)
	, normal_background(true)
{
	setObjectName("playerlayer");

	// If not set parent playerwindow will not get mouse move events
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

TPlayerLayer::~TPlayerLayer() {
}

void TPlayerLayer::setRepaintBackground(bool b) {
	qDebug("TPlayerLayer::setRepaintBackground: %d", b);
	repaint_background = b;
}

void TPlayerLayer::paintEvent( QPaintEvent * e ) {
	//qDebug("TPlayerLayer::paintEvent: repaint_background: %d", repaint_background);

	// repaint_background is the option to draw the background or not,
	// preventing flicker and speeding up redraws when set to false.
	// When repaint background is false the background still needs to be
	// repainted when no video is loaded, which is controlled by normal_background.
	// TPlayerWindow::aboutToStartPlaying calls setFastBackground to set it and
	// TPlayerWindow::playingStopped calls restoreNormalBackground to clear it,
	// together with the Qt::WA_PaintOnScreen attribute (when not on Windows).

	if (repaint_background || normal_background) {
		//qDebug("TPlayerLayer::paintEvent: painting");
		QPainter painter(this);
		painter.eraseRect( e->rect() );
		//painter.fillRect( e->rect(), QColor(255,0,0) );
	}
}

void TPlayerLayer::setFastBackground() {
	qDebug("TPlayerLayer::setFastBackground");
	normal_background = false;

#ifndef Q_OS_WIN
	setAttribute(Qt::WA_PaintOnScreen);
#endif
}

void TPlayerLayer::restoreNormalBackground() {
	qDebug("TPlayerLayer::restoreNormalBackground");

#ifndef Q_OS_WIN
	setAttribute(Qt::WA_PaintOnScreen, false);
#endif

	normal_background = true;
}


/* ---------------------------------------------------------------------- */

TPlayerWindow::TPlayerWindow(QWidget* parent)
	: QWidget(parent)
	, aspect(0)
	, monitoraspect(0)
	, zoom_factor(1.0)
	, zoom_factor_fullscreen(1.0)
	, pan_offset()
	, pan_offset_fullscreen()
	, delay_left_click(false)
	, double_clicked(false)
#if LOGO_ANIMATION
	, animated_logo(false)
#endif
#ifdef SHAREWIDGET
	, corner_widget(0)
#endif
	, video_width(0)
	, video_height(0)
	, size_group(0)
	, last_video_size()
	, drag_pos()
	, dragging(false)
	, kill_fake_event(false)
	, fullscreen(false)
	, enable_messages(false)
	, autohide_cursor(false)
	, check_hide_mouse_last_position()
	, autohide_interval(1000)
{
	setObjectName("playerwindow");

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	setMinimumSize(QSize(0, 0));
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);

	setAutoFillBackground(true);
	ColorUtils::setBackgroundColor(this, QColor(0, 0, 0));

	playerlayer = new TPlayerLayer(this);

	logo = new QLabel();
	logo->setObjectName("logo");
	logo->setAutoFillBackground(true);
	logo->setMouseTracking(true);
	ColorUtils::setBackgroundColor(logo, QColor(0, 0, 0));

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(logo, 0, Qt::AlignHCenter | Qt::AlignVCenter);
	playerlayer->setLayout(layout);

	left_click_timer = new QTimer(this);
	left_click_timer->setSingleShot(true);
	left_click_timer->setInterval(qApp->doubleClickInterval() + 10);
	connect(left_click_timer, SIGNAL(timeout()), this, SIGNAL(leftClicked()));

	check_hide_mouse_timer = new QTimer(this);
	check_hide_mouse_timer->setSingleShot(true);
	check_hide_mouse_timer->setInterval(autohide_interval);
	connect(check_hide_mouse_timer, SIGNAL(timeout()), this, SLOT(checkHideMouse()));
}

TPlayerWindow::~TPlayerWindow() {
}

void TPlayerWindow::setMonitorAspect(double asp) {
	monitoraspect = asp;
}

void TPlayerWindow::setAspect(double aspect, bool updateVideoWindow) {
	qDebug("TPlayerWindow::setAspect: %f", aspect);

	// See core::startPlayer. The player is started with --no-keepaspect and
	// monitorpixelaspect=1, so aspect changes don't require a restart of the player,
	// hence monitor aspect needs to be handled here.
	if (monitoraspect != 0) {
		aspect = aspect / monitoraspect * TDesktopInfo::desktop_aspectRatio(this);
	}

	this->aspect = aspect;
	if (updateVideoWindow)
		this->updateVideoWindow();
}

void TPlayerWindow::setResolution(int width, int height) {
	qDebug("TPlayerWindow::setResolution: %d x %d", width, height);

	video_width = width;
	video_height = height;
	last_video_size = QSize(width, height);

	enableSizeGroup();

	// Disable messages and post enable if video
	enable_messages = false;
	if (width > 0) {
		pauseMessages(500);
	}
}

void TPlayerWindow::set(double aspect,
						double zoom_factor,
						double zoom_factor_fullscreen,
						QPoint pan,
						QPoint pan_fullscreen) {

	// Clear video size
	setResolution(0, 0);
	// false = do not update video window
	setAspect(aspect, false);
	setZoom(zoom_factor, zoom_factor_fullscreen, false);
	setPan(pan, pan_fullscreen, false);
}

QSize TPlayerWindow::getAdjustedSize(int w, int h, double desired_zoom) const {
	//qDebug("TPlayerWindow::getAdjustedSize: in: %d x %d zoom %f aspect %f",
	//	   w, h, desired_zoom, aspect);

	// Select best fit: height adjusted or width adjusted,
	// in case video aspect does not match the window aspect ratio.
	// Height adjusted gives horizontal black borders.
	// Width adjusted gives vertical black borders,
	if (aspect != 0) {
		int height_adjust = qRound(w / aspect);
		if (height_adjust <= h) {
			// adjust height
			h = height_adjust;
		} else {
			// adjust width
			w = qRound(h * aspect);
		}
	}

	// Zoom
	QSize size = QSize(w, h) * desired_zoom;

	//qDebug("TPlayerWindow::getAdjustedSize: out: %d x %d", size.width(), size.height());
	return size;
}

void TPlayerWindow::setSizeGroup(Gui::TActionGroup* group) {

	size_group = group;
	size_group->setEnabled(false);
};

void TPlayerWindow::uncheckSizeGroup() {

	QAction* current = size_group->checkedAction();
	if (current)
		current->setChecked(false);
}

void TPlayerWindow::enableSizeGroup() {

	size_group->setEnabled(!fullscreen && video_width > 0 && video_height > 0);
	uncheckSizeGroup();
}

void TPlayerWindow::updateSizeGroup() {
	// qDebug("TPlayerWindow::updateSizegroup");

	if (!fullscreen && video_width > 0 && video_height > 0) {
		// Update size group with new size factor
		QSize video_size = getAdjustedSize(video_width, video_height, 1.0);
		int size_factor_x = qRound((double) width() * 100 / video_size.width());
		int size_factor_y = qRound((double) height() * 100/ video_size.height());

		uncheckSizeGroup();
		// Set when x and y factor agree
		if (size_factor_x == size_factor_y) {
			if (size_group->setChecked(size_factor_x)) {
				//qDebug("TPlayerWindow::updateSizegroup: set size group to %d%%",
				//		size_factor_x);
			} else {
				//qDebug("TPlayerWindow::updateSizegroup: no size group action for %d%%",
				//	   size_factor_x);
			}
		} else {
			//qDebug("TPlayerWindow::updateSizegroup: width %d%% and height %d%% factor mismatch",
			//	   size_factor_x, size_factor_y);
		}
	}
}

void TPlayerWindow::updateVideoWindow() {
	/*
	qDebug() << "TPlayerWindow::updateVideoWindow: in: fullscreen" << fullscreen
			<< " size" << width() << "x" << height()
			<< " aspect" << aspect
			<< " zoom" << zoom()
			<< " pan" << pan();
	*/
	QSize video_size = getAdjustedSize(width(), height(), zoom());

	// Center
	QPoint pos((width() - video_size.width()) / 2, (height() - video_size.height()) / 2);

	// Move
	pos += pan();

	playerlayer->setGeometry(pos.x(), pos.y(), video_size.width(), video_size.height());

	// Keep OSD in sight. Need the offset as seen by player.
	QPoint osd_pos(Proc::default_osd_pos);
	if (pos.x() < 0)
		osd_pos.rx() -= pos.x();
	if (pos.y() < 0)
		osd_pos.ry() -= pos.y();
	emit moveOSD(osd_pos);

	// Update status with new size
	if (enable_messages && !fullscreen && video_size != last_video_size) {
		emit showMessage(tr("Video size %1 x %2").arg(video_size.width()).arg(video_size.height()),
						 2000, 1); // 2 sec, osd_level 1
		last_video_size = video_size;
	}

	updateSizeGroup();

	//qDebug("TPlayerWindow::updateVideoWindow: out: pos (%d, %d)  size %d x %d",
	//	   pos.x(), pos.y(), video_size.width(), video_size.height());
}

void TPlayerWindow::resizeEvent(QResizeEvent *) {
	//qDebug("TPlayerWindow::resizeEvent");
	updateVideoWindow();
}

void TPlayerWindow::startDragging() {

	dragging = true;
	// Cancel pending left click
	if (delay_left_click)
		left_click_timer->stop();
	setCursor(QCursor(Qt::DragMoveCursor));

	// qDebug("TPlayerWindow::startDragging: started drag");
}

void TPlayerWindow::stopDragging() {

	dragging = false;
	setCursor(QCursor(Qt::ArrowCursor));

	// qDebug( "TPlayerWindow::stopDragging: stopped dragging" );
}

void TPlayerWindow::mousePressEvent(QMouseEvent* event) {
	// qDebug( "TPlayerWindow::mousePressEvent" );

	event->accept();

	// Ignore second press event after a double click.
	if (!double_clicked) {
		showHiddenCursor(false);

		if ((event->button() == Qt::LeftButton)
				&& (event->modifiers() == Qt::NoModifier)) {
			left_button_pressed_time.start();
			drag_pos = event->globalPos();
			dragging = false;
			// Catch release_events with button still down
			// Happens only when mouse capture is released?
			kill_fake_event = true;
		}
	}
}

void TPlayerWindow::mouseMoveEvent(QMouseEvent* event) {
	//qDebug( "TPlayerWindow::mouseMoveEvent" );

	event->accept();

	// No longer kill release event with button down
	kill_fake_event = false;

	if ((event->buttons() == Qt::LeftButton)
			&& (event->modifiers() == Qt::NoModifier)
			&& !double_clicked) {

		QPoint pos = event->globalPos();
		QPoint diff = pos - drag_pos;

		// Start dragging after having moved startDragDistance
		// or startDragTime elapsed.
		if (!dragging &&
			((diff.manhattanLength() > QApplication::startDragDistance())
			|| (left_button_pressed_time.elapsed() >= QApplication::startDragTime()))) {
			startDragging();
		}

		if (dragging) {
			drag_pos = pos;
			if (fullscreen) {
				moveVideo(diff);
			} else {
				emit moveWindow(diff);
			}
		}
	} else if (event->buttons() == Qt::NoButton)
		showHiddenCursor(autohide_cursor);

	// For DVDNAV
	if (!dragging && playerlayer->underMouse()) {
		// Make event relative to video layer
		QPoint pos = event->pos() - playerlayer->pos();
		emit mouseMoved(pos);
	}
}

// Return whether this event is accused of dragging.
// Returning false will cancel the event.
bool TPlayerWindow::checkDragging(QMouseEvent* event) {

	if (double_clicked)
		return true;

	bool kill = kill_fake_event;
	kill_fake_event = false;

	if (dragging) {
		stopDragging();
		return false;
	}

	// After the mouse has been captured, mouse release events sometimes
	// do not come through until the mouse moved (on Qt 4.8 KDE 4.1.14.9).
	if (left_button_pressed_time.elapsed() >= QApplication::startDragTime()) {
		//qDebug("TPlayerWindow::mouseReleaseEvent: canceled release event taking longer as %d ms",
		//	   QApplication::startDragTime());
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
			//qDebug("TPlayerWindow::mouseReleaseEvent: killed release event");
			return false;
		}
	}

	// No dragging
	return true;
}

void TPlayerWindow::mouseReleaseEvent(QMouseEvent* event) {
	// qDebug( "TPlayerWindow::mouseReleaseEvent");

	event->accept();

	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() != Qt::NoModifier) {
			qDebug("TPlayerWindow::mouseReleaseEvent: ignoring modified event");
		} else if (checkDragging(event)) {
			if (delay_left_click) {
				if (double_clicked) {
					double_clicked = false;
					// qDebug( "TPlayerWindow::mouseReleaseEvent: ignoring event after double click" );
				} else {
					// Delay left click until double click has a chance to arrive
					left_click_timer->start();
					// qDebug( "TPlayerWindow::mouseReleaseEvent: delaying left click" );
				}
			} else {
				double_clicked = false;
				// Click right away
				// qDebug( "TPlayerWindow::mouseReleaseEvent: emitting left click" );
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

void TPlayerWindow::mouseDoubleClickEvent( QMouseEvent * event ) {
	//qDebug( "TPlayerWindow::mouseDoubleClickEvent" );

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

void TPlayerWindow::wheelEvent( QWheelEvent * event ) {
	//qDebug("TPlayerWindow::wheelEvent: delta: %d", event->delta());

	event->accept();

	if (event->orientation() == Qt::Vertical) {
		if (event->delta() >= 0)
			emit wheelUp();
		else
			emit wheelDown();
	} else {
		qDebug("TPlayerWindow::wheelEvent: horizontal event received, doing nothing");
	}
}

void TPlayerWindow::aboutToEnterFullscreen() {
	//qDebug("TPlayerWindow::aboutToEnterFullscreen");

	fullscreen = true;
	enableSizeGroup();
}

void TPlayerWindow::enableMessages() {
	enable_messages = true;
}

void TPlayerWindow::pauseMessages(int msec) {

	// Disable messages and post enable
	enable_messages = false;
	QTimer::singleShot(msec, this, SLOT(enableMessages()));
}

void TPlayerWindow::aboutToExitFullscreen() {
	//qDebug("TPlayerWindow::aboutToExitFullscreen");

	fullscreen = false;
	enableSizeGroup();

	pauseMessages(500);
}

void TPlayerWindow::setZoom(double factor,
							double factor_fullscreen,
							bool updateVideoWindow) {
	qDebug("TPlayerWindow::setZoom: normal screen %f, full screen %f", factor, factor_fullscreen);

	if (factor < ZOOM_MIN)
		factor = ZOOM_MIN;
	else if (factor > ZOOM_MAX)
		factor = ZOOM_MAX;

	if (factor_fullscreen == 0.0) {
		// Set only current zoom
		if (fullscreen)
			zoom_factor_fullscreen = factor;
		else zoom_factor = factor;
	} else {
		// Set both zooms
		if (factor_fullscreen < ZOOM_MIN)
			factor_fullscreen = ZOOM_MIN;
		else if (factor_fullscreen > ZOOM_MAX)
			factor_fullscreen = ZOOM_MAX;

		zoom_factor = factor;
		zoom_factor_fullscreen = factor_fullscreen;
	}

	if (updateVideoWindow)
		this->updateVideoWindow();
}

double TPlayerWindow::zoom() {
	return fullscreen ? zoom_factor_fullscreen : zoom_factor;
}

void TPlayerWindow::setPan(QPoint pan, QPoint pan_fullscreen, bool updateVideoWindow) {
	qDebug() << "TPlayerWindow::setPan: pan" << pan << "pan full screen" << pan_fullscreen;

	pan_offset = pan;
	pan_offset_fullscreen = pan_fullscreen;
	if (updateVideoWindow)
		this->updateVideoWindow();
}


void TPlayerWindow::moveVideo(QPoint delta) {
	if (fullscreen)
		pan_offset_fullscreen += delta;
	else pan_offset += delta;
	updateVideoWindow();
}

void TPlayerWindow::moveVideo(int dx, int dy) {
	moveVideo(QPoint(dx, dy));
}

QPoint TPlayerWindow::pan() {
	return fullscreen ? pan_offset_fullscreen : pan_offset;
}

void TPlayerWindow::resetZoomAndPan() {

	zoom_factor_fullscreen = 1.0;
	pan_offset_fullscreen = QPoint();
	zoom_factor = 1.0;
	pan_offset = QPoint();
	updateVideoWindow();
}

void TPlayerWindow::setColorKey(QColor c) {
	ColorUtils::setBackgroundColor(playerlayer, c);
}

void TPlayerWindow::autoHideCursorStartTimer() {
	check_hide_mouse_last_position = QCursor::pos();
	check_hide_mouse_timer->start();
}

void TPlayerWindow::showHiddenCursor(bool startTimer) {
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
void TPlayerWindow::checkHideMouse() {
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

// Start and stop toggle autohide_cursor. Pause hides.
void TPlayerWindow::setAutoHideCursor(bool enable) {
	autohide_cursor = enable;
	if (autohide_cursor)
		autoHideCursorStartTimer();
	else showHiddenCursor(false);
}

void TPlayerWindow::aboutToStartPlaying() {
	qDebug("TPlayerWindow::aboutToStartPlaying");

	playerlayer->setFastBackground();
	setAutoHideCursor(true);
}

void TPlayerWindow::playingStopped(bool clear_background) {
	qDebug("TPlayerWindow::playingStopped");

	playerlayer->restoreNormalBackground();
	// Clear background right away.
	// Pro: no artifacts when things take a little while.
	// Against: longer black flicker when restarting or switching bright videos
	if (clear_background)
		repaint();
	else qDebug("TPlayerWindow::playingStopped: not clearing background");

	setAutoHideCursor(false);
	setResolution(0, 0);
}

// Called by TBase::retranslateStrings
void TPlayerWindow::retranslateStrings() {
	//qDebug("TPlayerWindow::retranslateStrings");
	logo->setPixmap( Images::icon("background") );
}

void TPlayerWindow::setLogoVisible(bool b) {
	qDebug("TPlayerWindow::setLogoVisible: %d", b);

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

#ifdef SHAREWIDGET
void TPlayerWindow::setMouseTrackingInclChildren(QWidget *w) {
	//qDebug() << "TPlayerWindow::setMouseTrackingInclChildren: " << w->objectName();

	w->setMouseTracking(true);

	QObjectList children = w->children();
	for (int n = 0; n < children.count(); n++) {
		QObject* child = children[n];
		if (child->isWidgetType()) {
			setMouseTrackingInclChildren(static_cast<QWidget *>(child));
		}
	}
}

void TPlayerWindow::setCornerWidget(QWidget * w) {
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

#include "moc_playerwindow.cpp"

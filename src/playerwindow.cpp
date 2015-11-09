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

#include <QDebug>
#include <QTimer>
#include <QCursor>
#include <QEvent>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QLabel>

#include "desktop.h"
#include "colorutils.h"
#include "images.h"
#include "settings/preferences.h"
#include "proc/playerprocess.h"


using namespace Settings;

TPlayerLayer::TPlayerLayer(QWidget* parent)
	: QWidget(parent)
	, repaint_background(false)
	, normal_background(true) {

#ifndef Q_OS_WIN
#if QT_VERSION < 0x050000
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_NativeWindow);
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

void TPlayerLayer::paintEvent(QPaintEvent* e) {
	//qDebug("TPlayerLayer::paintEvent: repaint_background: %d", repaint_background);

	// repaint_background is the option to draw the background or not,
	// preventing flicker and speeding up redraws when set to false.
	// When repaint background is false the background still needs to be
	// repainted when no video is loaded, which is controlled by normal_background.
	// TPlayerWindow::aboutToStartPlaying calls setFastBackground() to set it
	// and TPlayerWindow::playingStopped() calls restoreNormalBackground() to
	// clear it.
	if (repaint_background || normal_background) {
		QPainter painter(this);
		painter.eraseRect(e->rect());
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


TPlayerWindow::TPlayerWindow(QWidget* parent)
	: QWidget(parent)
	, video_width(0)
	, video_height(0)
	, zoom_factor(1.0)
	, zoom_factor_fullscreen(1.0)
	, aspect(0)
	, monitoraspect(0)
	, double_clicked(false)
	, delay_left_click(true)
	, dragging(false)
	, kill_fake_event(false)
	, enable_messages(false) {

	setMinimumSize(QSize(0, 0));
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
	ColorUtils::setBackgroundColor(this, QColor(0, 0, 0));
	setAutoFillBackground(true);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	playerlayer = new TPlayerLayer(this);
	playerlayer->setObjectName("playerlayer");
	playerlayer->setMinimumSize(QSize(0, 0));
	playerlayer->setAutoFillBackground(false);
	playerlayer->setFocusPolicy(Qt::NoFocus);
	playerlayer->setMouseTracking(true);

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
		aspect = aspect / monitoraspect * TDesktop::aspectRatio(this);
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

QSize TPlayerWindow::getAdjustedSize(int w, int h, double zoom) const {
	//qDebug("TPlayerWindow::getAdjustedSize: in: %d x %d zoom %f aspect %f",
	//	   w, h, zoom, aspect);

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
	QSize size = QSize(w, h) * zoom;

	//qDebug("TPlayerWindow::getAdjustedSize: out: %d x %d", size.width(), size.height());
	return size;
}

void TPlayerWindow::updateSizeFactor() {

	if (!pref->fullscreen && video_width > 0 && video_height > 0) {
		QSize video_size = getAdjustedSize(video_width, video_height, 1.0);
		double factor_x = (double) width() / video_size.width();
		double factor_y = (double) height() / video_size.height();
		// Store smallest factor in pref
		if (factor_y < factor_x) {
			pref->size_factor = factor_y;
		} else {
			pref->size_factor = factor_x;
		}
	}
}

void TPlayerWindow::updateVideoWindow() {
	qDebug() << "TPlayerWindow::updateVideoWindow: video size:"
			 << video_width << "x" << video_height
			 << " window size:" << size()
			 << " desktop size:" << TDesktop::size(this)
			 << " zoom:" << zoom()
			 << " pan:" << pan()
			 << " aspect:" << aspect
			 << " fs:" << pref->fullscreen;

	QSize s = pref->fullscreen ? TDesktop::size(this) : size();
	QSize video_size = getAdjustedSize(s.width(), s.height(), zoom());

	// Center
	s = (s - video_size) / 2;
	QPoint p(s.width(), s.height());

	// Move
	p += pan();

	// Return to local coords in fullscreen
	if (pref->fullscreen)
		p = mapFromGlobal(p);

	// Set geo video layer
	playerlayer->setGeometry(p.x(), p.y(), video_size.width(), video_size.height());

	// Keep OSD in sight. Need the offset as seen by player.
	QPoint osd_pos(Proc::default_osd_pos);
	if (p.x() < 0)
		osd_pos.rx() -= p.x();
	if (p.y() < 0)
		osd_pos.ry() -= p.y();
	emit moveOSD(osd_pos);

	// Update status with new size
	if (enable_messages && !pref->fullscreen && video_size != last_video_size) {
		emit showMessage(tr("Video size %1 x %2").arg(video_size.width()).arg(video_size.height()),
						 2500, 1); // 2.5 sec, osd_level 1
		last_video_size = video_size;
	}

	// Update pref->size_factor
	updateSizeFactor();

	qDebug() << "TPlayerWindow::updateVideoWindow: out:" << p << video_size;
}

void TPlayerWindow::resizeEvent(QResizeEvent *) {
	updateVideoWindow();
}

void TPlayerWindow::startDragging() {

	dragging = true;
	// Cancel pending left click
	if (delay_left_click)
		left_click_timer->stop();
	QApplication::setOverrideCursor(QCursor(Qt::DragMoveCursor));
}

void TPlayerWindow::stopDragging() {

	dragging = false;
	QApplication::restoreOverrideCursor();
}

void TPlayerWindow::mousePressEvent(QMouseEvent* event) {
	// qDebug("TPlayerWindow::mousePressEvent");

	event->accept();

	// Ignore second press event after a double click.
	if (!double_clicked) {

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
	//qDebug("TPlayerWindow::mouseMoveEvent");

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
			if (pref->fullscreen) {
				moveVideo(diff);
			} else {
				emit moveWindow(diff);
			}
		}
	}

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
	// qDebug("TPlayerWindow::mouseReleaseEvent");

	event->accept();

	if (event->button() == Qt::LeftButton) {
		if (event->modifiers() != Qt::NoModifier) {
			qDebug("TPlayerWindow::mouseReleaseEvent: ignoring modified event");
		} else if (checkDragging(event)) {
			if (delay_left_click) {
				if (double_clicked) {
					double_clicked = false;
					// qDebug("TPlayerWindow::mouseReleaseEvent: ignoring event after double click");
				} else {
					// Delay left click until double click has a chance to arrive
					left_click_timer->start();
					// qDebug("TPlayerWindow::mouseReleaseEvent: delaying left click");
				}
			} else {
				double_clicked = false;
				// Click right away
				// qDebug("TPlayerWindow::mouseReleaseEvent: emitting left click");
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
}

void TPlayerWindow::mouseDoubleClickEvent(QMouseEvent* event) {
	//qDebug("TPlayerWindow::mouseDoubleClickEvent");

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

void TPlayerWindow::wheelEvent(QWheelEvent* event) {
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

	pauseMessages(500);
}

void TPlayerWindow::setZoom(double factor,
							double factor_fullscreen,
							bool updateVideoWindow) {
	qDebug("TPlayerWindow::setZoom: normal screen %f, full screen %f", factor, factor_fullscreen);

	const double ZOOM_MIN = 0.05;
	const double ZOOM_MAX = 8.0; // High max can blow up surface

	if (factor < ZOOM_MIN)
		factor = ZOOM_MIN;
	else if (factor > ZOOM_MAX)
		factor = ZOOM_MAX;

	if (factor_fullscreen == 0.0) {
		// Set only current zoom
		if (pref->fullscreen)
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
	return pref->fullscreen ? zoom_factor_fullscreen : zoom_factor;
}

void TPlayerWindow::setPan(QPoint pan, QPoint pan_fullscreen, bool updateVideoWindow) {
	qDebug() << "TPlayerWindow::setPan: pan" << pan << "pan full screen" << pan_fullscreen;

	pan_offset = pan;
	pan_offset_fullscreen = pan_fullscreen;
	if (updateVideoWindow)
		this->updateVideoWindow();
}

void TPlayerWindow::moveVideo(QPoint delta) {

	if (pref->fullscreen)
		pan_offset_fullscreen += delta;
	else pan_offset += delta;
	updateVideoWindow();
}

void TPlayerWindow::moveVideo(int dx, int dy) {
	moveVideo(QPoint(dx, dy));
}

QPoint TPlayerWindow::pan() {
	return pref->fullscreen ? pan_offset_fullscreen : pan_offset;
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

void TPlayerWindow::aboutToStartPlaying() {
	//qDebug("TPlayerWindow::aboutToStartPlaying");

	playerlayer->setFastBackground();
}

void TPlayerWindow::playingStopped(bool clear_background) {
	//qDebug("TPlayerWindow::playingStopped");

	playerlayer->restoreNormalBackground();
	// Clear background right away.
	// Pro: no artifacts when things take a little while.
	// Against: longer black flicker when restarting or switching bright videos
	if (clear_background)
		repaint();
	else qDebug("TPlayerWindow::playingStopped: not clearing background");

	setResolution(0, 0);
}

// Called by TBase::retranslateStrings
void TPlayerWindow::retranslateStrings() {
	//qDebug("TPlayerWindow::retranslateStrings");
	logo->setPixmap(Images::icon("background"));
}

void TPlayerWindow::setLogoVisible(bool b) {
	qDebug("TPlayerWindow::setLogoVisible: %d", b);
	logo->setVisible(b);
}

#include "moc_playerwindow.cpp"

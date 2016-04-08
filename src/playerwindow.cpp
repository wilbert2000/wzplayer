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

// Window containing the video player
TVideoWindow::TVideoWindow(QWidget* parent) :
	QWidget(parent),
	normal_background(true) {

	setAutoFillBackground(true);

#ifndef Q_OS_WIN
#if QT_VERSION < 0x050000
	setAttribute(Qt::WA_NativeWindow);
	//setAttribute(Qt::WA_DontCreateNativeAncestors);
#endif
#endif
}

TVideoWindow::~TVideoWindow() {
}

void TVideoWindow::paintEvent(QPaintEvent*) {
	// qDebug() << "TVideoWindow::paintEvent:" << e->rect();
	// QPainter painter(this);
	// painter.eraseRect(e->rect());
}

void TVideoWindow::setFastBackground() {
	qDebug("TVideoWindow::setFastBackground");

	normal_background = false;
	setAutoFillBackground(false);
	// Don't erase background before paint
	setAttribute(Qt::WA_OpaquePaintEvent);
	// No restore background by system
	setAttribute(Qt::WA_NoSystemBackground);

#ifndef Q_OS_WIN
	// Disable composition and double buffering on X11
	// Needed for mplayer
	setAttribute(Qt::WA_PaintOnScreen);
#endif
}

void TVideoWindow::restoreNormalBackground() {
	qDebug("TVideoWindow::restoreNormalBackground");

	normal_background = true;
	setAutoFillBackground(true);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
	setAttribute(Qt::WA_NoSystemBackground, false);

#ifndef Q_OS_WIN
	setAttribute(Qt::WA_PaintOnScreen, false);
#endif
}


TPlayerWindow::TPlayerWindow(QWidget* parent) :
	QWidget(parent),
	video_size(0, 0),
	last_video_out_size(0, 0),
	aspect(0),
	zoom_factor(1.0),
	zoom_factor_fullscreen(1.0),
	double_clicked(false),
	delay_left_click(true),
	dragging(false),
	kill_fake_event(false) {

	setMinimumSize(QSize(0, 0));
	setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
	ColorUtils::setBackgroundColor(this, QColor(0, 0, 0));
	setAutoFillBackground(true);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	video_window = new TVideoWindow(this);
	video_window->setObjectName("video_window");
	setColorKey();
	video_window->setMinimumSize(QSize(0, 0));
	video_window->setFocusPolicy(Qt::NoFocus);
	video_window->setMouseTracking(true);

	left_click_timer = new QTimer(this);
	left_click_timer->setSingleShot(true);
	left_click_timer->setInterval(qApp->doubleClickInterval() + 10);
	connect(left_click_timer, SIGNAL(timeout()), this, SIGNAL(leftClicked()));
	setDelayLeftClick(pref->delay_left_click);
}

TPlayerWindow::~TPlayerWindow() {
}

void TPlayerWindow::setResolution(int width, int height) {
	qDebug("TPlayerWindow::setResolution: %d x %d", width, height);

	video_size = QSize(width, height);
	if (height == 0) {
		aspect = 0;
	} else {
		aspect = (double) width / height;
	}
	if (!video_size.isEmpty() && video_window->normal_background) {
		setFastWindow();
	}
}

void TPlayerWindow::getSizeFactors(double& factorX, double& factorY) {

	if (video_size.isEmpty()) {
		factorX = 0;
		factorY = 0;
	} else {
		if (pref->fullscreen) {
			// Should both be the same...
			factorX = (double) last_video_out_size.width() / video_size.width();
			factorY = (double) last_video_out_size.height() / video_size.height();
		} else {
			factorX = (double) width() / video_size.width();
			factorY = (double) height() /  video_size.height();
		}
		return;
	}
}

double TPlayerWindow::getSizeFactor() {

	double factorX, factorY;
	getSizeFactors(factorX, factorY);
	if (factorY < factorX) {
		return factorY;
	}
	return factorX;
}

void TPlayerWindow::updateSizeFactor() {

	if (!video_size.isEmpty()) {
		double old_factor = pref->size_factor;
		pref->size_factor = getSizeFactor();
		qDebug("TPlayerWindow::updateSizeFactor: updating size factor from %f to %f",
			   old_factor, pref->size_factor);
		emit videoSizeFactorChanged();
	}
}

void TPlayerWindow::updateVideoWindow() {
	qDebug() << "TPlayerWindow::updateVideoWindow: video size:" << video_size
			 << " window size:" << size()
			 << " desktop size:" << TDesktop::size(this)
			 << " zoom:" << zoom()
			 << " pan:" << pan()
			 << " fs:" << pref->fullscreen;

	// Note: Can give MPV the whole window, it uses it for OSD and Subs.
	// Mplayer too, but it does misbehave with some VOs, like XV, not properly
	// clearing the background not covered by the video. MPlayer with GL or
	// VDPAU work ok too, but on my setup clear the whole background creating
	// a nasty flicker when changing video dimensions.

	// On fullscreen ignore the toolbars
	QSize s = pref->fullscreen ? TDesktop::size(this) : size();
	QSize vsize = s;

	// Select best fit: height adjusted or width adjusted,
	// in case video aspect does not match the window aspect ratio.
	// Height adjusted gives horizontal black borders.
	// Width adjusted gives vertical black borders,
	if (aspect != 0) {
		int height_adjusted = qRound(vsize.width() / aspect);
		if (height_adjusted <= vsize.height()) {
			// adjust the height
			vsize.rheight() = height_adjusted;
		} else {
			// adjust the width
			vsize.rwidth() = qRound(vsize.height() * aspect);
		}
	}

	// Zoom
	vsize *= zoom();

	// Calc window pos and size
	QPoint wpos;
	QSize wsize;
	if (pref->isMPlayer()) {
		// MPlayer does not support zoom, so we blow up the video surface
		// when zoom is too large
		wsize = vsize;
		// Center
		s = (s - wsize) / 2;
		wpos = QPoint(s.width(), s.height());
		// Move
		wpos += pan();
	} else {
		// Give MPV the whole window, passing zoom and pan to player
		wsize = s;
	}

	// Return to local coords in fullscreen
	if (pref->fullscreen)
		wpos = mapFromGlobal(wpos);

	// Set geometry video layer
	playerlayer->setGeometry(wpos.x(), wpos.y(), wsize.width(), wsize.height());

	// Pass zoom and pan to player.
	if (pref->isMPV()) {
		// Pan wants factor relative to whole scaled video
		QPoint pan = this->pan();
		double pan_x = (double) pan.x() / vsize.width();
		double pan_y = (double) pan.y() / vsize.height();
		emit setZoomAndPan(zoom(), pan_x, pan_y);
	}

	// Update status with new video out size
	if (vsize != last_video_out_size) {
		last_video_out_size = vsize;
		emit videoOutChanged(vsize);
	}

	qDebug() << "TPlayerWindow::updateVideoWindow: out: win pos" << wpos
			 << "win size" << wsize << "video size" << vsize;
}

void TPlayerWindow::resizeEvent(QResizeEvent*) {
	qDebug("TPlayerWindow::resizeEvent");

	updateVideoWindow();
	updateSizeFactor();
}

void TPlayerWindow::startDragging() {

	dragging = true;
	// Cancel pending left click
	if (delay_left_click)
		left_click_timer->stop();
	QApplication::setOverrideCursor(QCursor(Qt::DragMoveCursor));
	emit draggingChanged(true);
}

void TPlayerWindow::stopDragging() {
	//qDebug("TPlayerWindow::stopDragging");

	dragging = false;
	QApplication::restoreOverrideCursor();
	emit draggingChanged(false);
}

void TPlayerWindow::mousePressEvent(QMouseEvent* event) {
	// qDebug("TPlayerWindow::mousePressEvent");

	event->accept();

	if (event->button() == Qt::LeftButton && !double_clicked) {
		left_button_pressed_time.start();
		drag_pos = event->globalPos();
		dragging = false;
		// Catch release_events with button still down
		// Happens only when mouse capture is released?
		kill_fake_event = true;
	}
}

void TPlayerWindow::mouseMoveEvent(QMouseEvent* event) {
	//qDebug("TPlayerWindow::mouseMoveEvent");

	event->accept();

	// No longer kill release event with button down
	kill_fake_event = false;

	if (event->buttons() == Qt::LeftButton && !double_clicked) {

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
			// Move video in fullscreen or with modifier, otherwise move window
			drag_pos = pos;
			if (pref->fullscreen || event->modifiers() != Qt::NoModifier) {
				if (!video_size.isEmpty()) {
					moveVideo(diff);
				}
			} else {
				emit moveWindow(diff);
			}
		}
	}

	// For DVDNAV
	if (!dragging && video_window->underMouse()) {
		// Make event relative to video layer
		QPoint pos = event->pos() - video_window->pos();
		emit mouseMoved(pos);
	}
}

// Return whether this event is accused of dragging.
// Returning false will cancel the event.
bool TPlayerWindow::checkDragging(QMouseEvent* event) {

	// Don't kill double click
	if (double_clicked)
		return true;

	// Clear kill_fake_event
	bool kill = kill_fake_event;
	kill_fake_event = false;

	if (dragging) {
		stopDragging();
		return false;
	}

	// After the mouse has been captured, mouse release events sometimes
	// do not come through until the mouse moved (on Qt 4.8 KDE 4.1.14.9).
	if (left_button_pressed_time.elapsed() >= QApplication::startDragTime()) {
		qDebug("TPlayerWindow::mouseReleaseEvent: canceled release event taking longer as %d ms",
			   QApplication::startDragTime());
		return false;
	}

	// Dragging the mouse more then startDragDistance delivers a mouse release event,
	// before the first mouse move event, while the left mouse is still down
	// (on Qt 4.8, KDE 4.1.14.9). Like an end-of-capture or what? Kill it.
	if (kill) {
		QPoint pos = event->globalPos();
		QPoint diff = pos - drag_pos;
		if (diff.manhattanLength() > QApplication::startDragDistance()) {
			qDebug("TPlayerWindow::mouseReleaseEvent: canceled release event with drag distance larger than %d",
				   QApplication::startDragDistance());
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
		if (checkDragging(event)) {
			if (event->modifiers() != Qt::NoModifier) {
				qDebug("TPlayerWindow::mouseReleaseEvent: ignoring modified event");
			} else if (delay_left_click) {
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

	if (event->button() == Qt::LeftButton
		&& event->modifiers() == Qt::NoModifier) {
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

void TPlayerWindow::setZoom(double factor,
							double factor_fullscreen,
							bool updateVideoWindow) {
	qDebug("TPlayerWindow::setZoom: normal screen %f, full screen %f", factor, factor_fullscreen);

	if (factor_fullscreen == 0) {
		// Set only current zoom
		if (pref->fullscreen)
			zoom_factor_fullscreen = factor;
		else zoom_factor = factor;
	} else {
		// Set both zooms
		zoom_factor = factor;
		zoom_factor_fullscreen = factor_fullscreen;
	}

	if (updateVideoWindow) {
		this->updateVideoWindow();
		if (pref->fullscreen) {
			updateSizeFactor();
		}
	}
}

double TPlayerWindow::zoom() {
	return pref->fullscreen ? zoom_factor_fullscreen : zoom_factor;
}

void TPlayerWindow::setPan(QPoint pan, QPoint pan_fullscreen) {
	qDebug() << "TPlayerWindow::setPan: pan" << pan << "pan full screen" << pan_fullscreen;

	pan_offset = pan;
	pan_offset_fullscreen = pan_fullscreen;
}

void TPlayerWindow::moveVideo(QPoint delta) {

	if (pref->fullscreen) {
		pan_offset_fullscreen += delta;
	} else {
		pan_offset += delta;
	}
	updateVideoWindow();
	QPoint p = pan();
	emit displayMessage(tr("Pan (%1, %2)")
		.arg(QString::number(p.x())).arg(QString::number(p.y())));
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

void TPlayerWindow::setColorKey() {

	if (pref->useColorKey()) {
		ColorUtils::setBackgroundColor(video_window, pref->color_key);
	} else {
		ColorUtils::setBackgroundColor(video_window, QColor(0, 0, 0));
	}
}

void TPlayerWindow::setFastWindow() {
	qDebug("TPlayerWindow::setFastWindow");
	video_window->setFastBackground();
}

void TPlayerWindow::restoreNormalWindow() {
	qDebug("TPlayerWindow::restoreNormalWindow");

	video_window->restoreNormalBackground();
	// Clear video size
	video_size = QSize(0, 0);
}

#include "moc_playerwindow.cpp"

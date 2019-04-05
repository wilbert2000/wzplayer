/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/playerwindow.h"

#include "gui/msg.h"
#include "desktop.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "images.h"

#include <QTimer>
#include <QCursor>
#include <QEvent>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QLabel>


using namespace Settings;

namespace Gui {

// Window containing the video player
TVideoWindow::TVideoWindow(QWidget* parent) :
    QWidget(parent),
    normal_background(true) {

    setAutoFillBackground(false);
    // Don't erase background before paint
    setAttribute(Qt::WA_OpaquePaintEvent);
    // setAttribute(Qt::WA_NativeWindow);
}

void TVideoWindow::paintEvent(QPaintEvent* e) {

    if (normal_background || pref->isMPV()) {
        QPainter painter(this);
        painter.eraseRect(e->rect());
    }
}

void TVideoWindow::setFastBackground() {

    normal_background = false;
    // Disable restore background by system
    setAttribute(Qt::WA_NoSystemBackground);

    // For mplayer disable composition and double buffering on X11.
    // Fills up the log since Qt 5.x. with:
    // WARN Qt.QWidget::paintEngine: Should no longer be called
    // If Qt::WA_PaintOnScreen is not set the window will have a bad flicker
    // during resizes due to the background clearing.
    // TODO: Hence, for now, fixed by supressing the warning in
    // LogManager::qtMessageHandler()...
    // Makes the dock flash on screen if enabled with MPV, but MPV does not
    // need it since its video lives in a child of the player window.
#ifndef Q_OS_WIN
    if (pref->isMPlayer()) {
        setAttribute(Qt::WA_PaintOnScreen);
    }
#endif
}

void TVideoWindow::restoreNormalBackground() {

    normal_background = true;
    // Enable restore background by system
    setAttribute(Qt::WA_NoSystemBackground, false);

    // For mplayer restore ccomposition and double buffering on X11
#ifndef Q_OS_WIN
    if (pref->isMPlayer()) {
        setAttribute(Qt::WA_PaintOnScreen, false);
    }
#endif
}


TPlayerWindow::TPlayerWindow(QWidget* parent) :
    QWidget(parent),
    debug(logger()),
    video_size(0, 0),
    last_video_out_size(0, 0),
    fps(0),
    last_fps(0),
    aspect(0),
    zoom_factor(1.0),
    zoom_factor_fullscreen(1.0),
    double_clicked(false),
    delay_left_click(true),
    dragging(false) {

    setMinimumSize(QSize(0, 0));
    setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
    TColorUtils::setBackgroundColor(this, QColor(0, 0, 0));
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    video_window = new TVideoWindow(this);
    video_window->setObjectName("video_window");
    video_window->setMinimumSize(QSize(0, 0));
    video_window->setMouseTracking(true);
    setColorKey();

    left_click_timer = new QTimer(this);
    left_click_timer->setSingleShot(true);
    left_click_timer->setInterval(qApp->doubleClickInterval() + 10);
    connect(left_click_timer, &QTimer::timeout,
            this, &TPlayerWindow::onLeftClicked);
    setDelayLeftClick(pref->delay_left_click);
}

void TPlayerWindow::setResolution(int width, int height, const double fps) {
    WZDEBUG(QString("%1 x %2 %3 fps").arg(width).arg(height).arg(fps));

    video_size = QSize(width, height);
    if (height == 0) {
        aspect = 0;
    } else {
        aspect = (double) width / height;
    }
    this->fps = fps;
    if (!video_size.isEmpty() && video_window->normal_background) {
        setFastWindow();
    }
}

void TPlayerWindow::getSizeFactors(double& factorX, double& factorY) const {

    if (video_size.isEmpty()) {
        factorX = 0;
        factorY = 0;
    } else {
        if (pref->fullscreen) {
            // Should both be the same...
            factorX = (double) last_video_out_size.width() / video_size.width();
            factorY = (double) last_video_out_size.height()
                      / video_size.height();
        } else {
            factorX = (double) width() / video_size.width();
            factorY = (double) height() /  video_size.height();
        }
    }
}

double TPlayerWindow::getSizeFactor() const {

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
        // WZTRACE(QString("updating size from %1 to %2")
        //         .arg(old_factor).arg(pref->size_factor));
        // Need to emit if old == new to detect changes by user
        emit videoSizeFactorChanged(old_factor, pref->size_factor);
    }
}

void TPlayerWindow::updateVideoWindow() {
    /*
    debug << "updateVideoWindow in vsize" << video_size
          << "wsize" << size()
          << "dsize" << TDesktop::size(this)
          << "zoom" << zoom()
          << "pan" << pan()
          << "fs" << pref->fullscreen
          << debug;
    */

    // Note: can give MPV the whole window, it uses it for OSD and Subs.
    // Mplayer too, with most VOs, but with some, like XV, it misbehaves,
    // not clearing the background not covered by the video.
    // MPV supports video-zoom and pan, but pan has really bad performance,
    // though it enables using black borders for OSD and subs.
    // MPlayer does not support pan in slave mode. It does support zoom through
    // the pansan slave command.

    // On fullscreen ignore the toolbars
    QSize wsize = pref->fullscreen ? TDesktop::size(this) : size();
    // Set video size to window size
    QSize vsize = wsize;

    // Select best fit: height adjusted or width adjusted,
    // in case video aspect does not match the window aspect ratio.
    // Height adjusted gives horizontal black borders.
    // Width adjusted gives vertical black borders.
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

    // Zoom video size. A large size can blow up the video surface.
    vsize *= zoom();

    // Center video window
    QSize c = (wsize - vsize) / 2;
    QRect vwin(c.width(), c.height(), vsize.width(), vsize.height());

    // Pan video window
    vwin.translate(pan());

    // Return to local coords in fullscreen
    if (pref->fullscreen) {
        vwin.moveTo(vwin.topLeft()
                    - pos() // Could assume pos() player window always (0,0)
                    // Put the docks on top of the video area:
                    - ((QWidget*)parent())->pos() // pos() panel
                    );
    }

    // Set geometry video window
    video_window->setGeometry(vwin);

    // Update status bar with new video out size
    if (vsize != last_video_out_size
            || qAbs(fps - last_fps) > 0.001) {
        last_video_out_size = vsize;
        last_fps = fps;
        emit videoOutChanged();
    }

    //debug << "updateVideoWindow out window" << vwin
    //      << "video size" << vsize;
    //debug << debug;
}

void TPlayerWindow::resizeEvent(QResizeEvent*) {

    updateVideoWindow();
    updateSizeFactor();
}

void TPlayerWindow::startDragging() {

    dragging = true;
    // Cancel pending left click
    if (delay_left_click) {
        left_click_timer->stop();
    }
    setCursor(QCursor(Qt::DragMoveCursor));
    emit draggingChanged(true);
}

void TPlayerWindow::stopDragging() {

    dragging = false;
    unsetCursor();
    emit draggingChanged(false);
}

void TPlayerWindow::mousePressEvent(QMouseEvent* event) {

    // Let parent handle dragging of main window, cancel menus etc.
    event->ignore();

    if (event->button() == Qt::LeftButton && !double_clicked) {
        left_button_pressed_time.start();
        drag_pos = event->globalPos();
        dragging = false;

        if (pref->fullscreen) {
            // Accept event to prevent dragging by OS, which might try to
            // apply nice effects to the dragging, like KDE translucency etc.
            event->accept();
        }
    }
}

void TPlayerWindow::mouseMoveEvent(QMouseEvent* event) {

    // Drag video
    if (event->buttons() == Qt::LeftButton
        && !double_clicked
        && (pref->fullscreen || event->modifiers() != Qt::NoModifier)) {

        QPoint pos = event->globalPos();
        QPoint diff = pos - drag_pos;

        // Start dragging after having moved startDragDistance
        // or startDragTime elapsed.
        if (!dragging &&
            ((diff.manhattanLength() > QApplication::startDragDistance())
             || (left_button_pressed_time.elapsed()
                 >= QApplication::startDragTime()))) {
            startDragging();
        }

        if (dragging) {
            drag_pos = pos;
            if (!video_size.isEmpty()) {
                moveVideo(diff);
                event->accept();
                return;
            }
        }
    }

    // For DVDNAV pass event to player.
    // Don't pass when button down, assuming the video is being dragged.
    if (event->buttons() == Qt::NoButton && video_window->underMouse()) {
        // Make event relative to video layer
        emit dvdnavMousePos(event->pos() - video_window->pos());
    }

    // Pass event to parent
    event->ignore();
}

void TPlayerWindow::onLeftClicked() {
    emit leftClicked();
}

void TPlayerWindow::mouseReleaseEvent(QMouseEvent* event) {

    // Default: show event to parent
    event->ignore();

    if (event->button() == Qt::LeftButton) {
        if (dragging) {
            stopDragging();
            event->accept();
        } else if (event->modifiers() != Qt::NoModifier) {
        } else if (left_button_pressed_time.elapsed()
                   >= QApplication::startDragTime()) {
        } else if (delay_left_click) {
            if (double_clicked) {
                event->accept();
            } else {
                // Delay left click until double click has chance to arrive
                left_click_timer->start();
            }
        } else {
            // Click right away
            onLeftClicked();
        }
        double_clicked = false;
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

    if (event->button() == Qt::LeftButton
        && event->modifiers() == Qt::NoModifier) {
        double_clicked = true;
        left_click_timer->stop();
        emit doubleClicked();
        event->accept();
    } else {
        event->ignore();
    }
}

void TPlayerWindow::setZoom(double factor,
                            double factor_fullscreen,
                            bool updateVideoWindow) {

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

double TPlayerWindow::zoom() const {
    return pref->fullscreen ? zoom_factor_fullscreen : zoom_factor;
}

void TPlayerWindow::setPan(const QPoint& pan, const QPoint& pan_fullscreen) {

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
    Gui::msg2(tr("Pan (%1, %2)").arg(p.x()).arg(p.y()));
}

void TPlayerWindow::moveVideo(int dx, int dy) {
    moveVideo(QPoint(dx, dy));
}

QPoint TPlayerWindow::pan() const {
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
        TColorUtils::setBackgroundColor(video_window, pref->color_key);
    } else {
        TColorUtils::setBackgroundColor(video_window, QColor(0, 0, 0));
    }
}

void TPlayerWindow::setFastWindow() {
    WZTRACE("");
    video_window->setFastBackground();
}

void TPlayerWindow::restoreNormalWindow() {
    WZTRACE("");

    video_window->restoreNormalBackground();
    // Clear video size and fps
    video_size = QSize(0, 0);
    fps = 0;
}

} // namespace Gui

#include "moc_playerwindow.cpp"

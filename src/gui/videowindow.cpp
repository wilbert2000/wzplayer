#include "gui/videowindow.h"
#include "wzdebug.h"

#include <QPaintEvent>
#include <QPainter>


//LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::TVideoWindow)

namespace Gui {

// Window containing the video player
TVideoWindow::TVideoWindow(QWidget* parent) :
    QWidget(parent),
    normalBackground(true) {

    // Don't want background filled
    setAutoFillBackground(false);
    // Don't erase background before paint
    setAttribute(Qt::WA_OpaquePaintEvent);
    // TPlayers.startPlayer()'s call to winId() will provide the native handle
    // setAttribute(Qt::WA_NativeWindow);
    // Disable input
    setEnabled(false);
}

void TVideoWindow::paintEvent(QPaintEvent* e) {

    if (normalBackground) {
        QPainter painter(this);
        painter.eraseRect(e->rect());
    }
}

void TVideoWindow::setFastBackground() {

    // Disable paintEvent()
    normalBackground = false;
    // Disable restore background by system
    setAttribute(Qt::WA_NoSystemBackground);

    // Disable composition and double buffering on X11.
    // Fills up the log since Qt 5.x. with:
    // "WARN Qt.QWidget::paintEngine: Should no longer be called".
    // If Qt::WA_PaintOnScreen is not set the window will have a bad flicker
    // during resizes due to the background clearing.
    // Fixed by supressing the warning in LogManager::qtMessageHandler().
#ifndef Q_OS_WIN
    setAttribute(Qt::WA_PaintOnScreen);
#endif
}

void TVideoWindow::restoreNormalBackground() {

    // Enable paintEvent()
    normalBackground = true;

    // Enable restore background by system
    setAttribute(Qt::WA_NoSystemBackground, false);

    // Restore ccomposition and double buffering on X11
#ifndef Q_OS_WIN
    setAttribute(Qt::WA_PaintOnScreen, false);
#endif
}


} // namespace Gui

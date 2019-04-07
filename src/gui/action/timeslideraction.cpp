#include "gui/action/timeslideraction.h"
#include "gui/action/timeslider.h"
#include "gui/action/widgetactions.h"
#include "gui/mainwindow.h"
#include "gui/playerwindow.h"
#include "settings/preferences.h"
#include "player/player.h"
#include "wztime.h"
#include "wztimer.h"

#include <QToolTip>


namespace Gui {
namespace Action {


TTimeSliderAction::TTimeSliderAction(TMainWindow* mw,
                                     QWidget* aPanel,
                                     Player::TPlayer* player) :
    TWidgetAction(mw),
    wzdebug(logger()),
    panel(aPanel),
    previewPlayer(player),
    lastPreviewTime(-1),
    pos(0),
    maxPos(1000),
    duration(0) {

    setObjectName("timeslider_action");
    setText(tr("Time slider"));

    previewTimer = new TWZTimer(this, "previewtimer", false);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(100);
    connect(previewTimer, &TWZTimer::timeout,
            this, &TTimeSliderAction::onPreviewTimerTimeout);
}

QWidget* TTimeSliderAction::createWidget(QWidget* parent) {

    TTimeSlider* slider = new TTimeSlider(parent, pos, maxPos, duration,
        Settings::pref->time_slider_drag_delay);
    slider->setEnabled(isEnabled());

    connect(slider, &TTimeSlider::posChanged,
            this, &TTimeSliderAction::onPosChanged);
    connect(slider, &TTimeSlider::draggingPosChanged,
            this, &TTimeSliderAction::onDraggingPosChanged);
    connect(slider, &TTimeSlider::delayedDraggingPos,
            this, &TTimeSliderAction::onDelayedDraggingPos);

    connect(slider, &TTimeSlider::wheelUp,
            this, &TTimeSliderAction::wheelUp);
    connect(slider, &TTimeSlider::wheelDown,
            this, &TTimeSliderAction::wheelDown);

    connect(slider, &TTimeSlider::toolTipEvent,
            this, &TTimeSliderAction::onToolTipEvent);

    return slider;
}

void TTimeSliderAction::setPos() {

    QList<QWidget*> widgets = createdWidgets();
    for(int i = 0; i < widgets.count(); i++) {
        TTimeSlider* widget = static_cast<TTimeSlider*>(widgets.at(i));
        bool was_blocked = widget->blockSignals(true);
        widget->setPos(pos);
        widget->blockSignals(was_blocked);
    }
}

void TTimeSliderAction::setPosition(double sec) {

    int p = 0;
    if (sec > 0 && duration > 0.1) {
        p = qRound((sec * maxPos) / duration);
        if (p > maxPos) {
            p = maxPos;
        }
    }

    if (p != pos) {
        pos = p;
        setPos();
    }
}

void TTimeSliderAction::setDuration(double t) {

    duration = t;
    QList<QWidget*> l = createdWidgets();
    for (int n = 0; n < l.count(); n++) {
        TTimeSlider* s = (TTimeSlider*) l[n];
        s->setDuration(t);
    }
}

// Slider pos changed
void TTimeSliderAction::onPosChanged(int value) {

    pos = value;
    if (Settings::pref->seek_relative || duration <= 0) {
        emit percentageChanged((double) (pos * 100) / maxPos);
    } else {
        emit positionChanged(duration * pos / maxPos);
    }
}

// Slider pos changed while dragging
void TTimeSliderAction::onDraggingPosChanged(int value) {

    pos = value;
    emit dragPositionChanged(duration * pos / maxPos);
}

// Delayed slider pos while dragging
void TTimeSliderAction::onDelayedDraggingPos(int value) {

    pos = value;
    if (Settings::pref->update_while_seeking) {
        onPosChanged(pos);
    }
}

void TTimeSliderAction::onPreviewTimerTimeout() {

    if (previewSlider->underMouse()) {
        preview();
    } else {
        previewPlayer->playerWindow->hide();
        lastPreviewTime = -1;
    }
}

void TTimeSliderAction::preview() {

    QPoint pos = QCursor::pos();
    int secs = previewSlider->getTime(previewSlider->mapFromGlobal(pos));
    if (secs != lastPreviewTime) {
        lastPreviewTime = secs;
        pos = panel->mapFromGlobal(pos);

        QWidget* parent = previewSlider->parentWidget();
        QRect r = parent->frameGeometry();
        parent = parent->parentWidget();
        if (parent) {
            r.moveTo(parent->mapToGlobal(r.topLeft()));
        }
        r.moveTo(panel->mapFromGlobal(r.topLeft()));

        const int d = 6;
        TPlayerWindow* playerWindow = previewPlayer->playerWindow;
        if (previewSlider->orientation() == Qt::Horizontal) {
            pos.rx() = pos.x() - playerWindow->width() / 2;
            if (pos.y() > panel->height() / 2) {
                pos.ry() = r.top() - playerWindow->height() - d;
            } else {
                pos.ry() = r.top() + r.height() + d;
            }
        } else {
            pos.ry() = pos.y() - playerWindow->height() / 2;
            if (pos.x() > panel->width() / 2) {
                pos.rx() = r.x() - playerWindow->width() - d;
            } else {
                pos.rx() = r.x() + r.width() + d;
            }
        }
        playerWindow->move(pos);
        if (!playerWindow->isVisible()) {
            playerWindow->setVisible(true);
        }

        previewPlayer->pause();
        previewPlayer->seekTime(secs);
    }

    previewTimer->start();
}

void TTimeSliderAction::onToolTipEvent(TTimeSlider* slider,
                                       QPoint pos,
                                       int secs) {

    if (previewPlayer->statePOP()) {
        previewSlider = slider;
        previewTimer->start();
    } else {
        QToolTip::showText(pos, TWZTime::formatTime(secs), slider);
    }
}

} // namespace Action
} // namespace Gui

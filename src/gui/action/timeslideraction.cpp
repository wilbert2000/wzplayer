#include "gui/action/timeslideraction.h"
#include "gui/action/timeslider.h"
#include "gui/action/widgetactions.h"
#include "gui/mainwindow.h"
#include "gui/playerwindow.h"
#include "settings/preferences.h"
#include "player/player.h"
#include "wztime.h"
#include "wztimer.h"

#include <QApplication>
#include <QToolTip>


namespace Gui {
namespace Action {

const int POS_RES = 10;
const int NO_POS = -POS_RES - 1;

TTimeSliderAction::TTimeSliderAction(TMainWindow* mw, Player::TPlayer* player) :
    TWidgetAction(mw),
    posMS(0),
    durationMS(0),
    requestedPos(0),
    previewPlayer(player->previewPlayer),
    lastPreviewTime(NO_POS) {

    setObjectName("timeslider_action");
    setText(tr("Time slider"));

    connect(player, &Player::TPlayer::positionChanged,
            this, &TTimeSliderAction::setPosition);
    connect(player, &Player::TPlayer::durationChanged,
            this, &TTimeSliderAction::setDuration);

    connect(this, &TTimeSliderAction::positionChanged,
            player, &Player::TPlayer::seekMS);
    connect(this, &TTimeSliderAction::percentageChanged,
            player, &Player::TPlayer::seekPercentage);

    connect(this, &TTimeSliderAction::wheelUp,
            player, &Player::TPlayer::wheelUpSeeking);
    connect(this, &TTimeSliderAction::wheelDown,
            player, &Player::TPlayer::wheelDownSeeking);

    updatePosTimer = new TWZTimer(this, "updatepostimer", false);
    updatePosTimer->setSingleShot(true);
    updatePosTimer->setInterval(Settings::pref->seek_rate);
    connect(updatePosTimer, &TWZTimer::timeout,
            this, &TTimeSliderAction::onUpdatePosTimerTimeout);

    previewTimer = new TWZTimer(this, "previewtimer", false);
    previewTimer->setSingleShot(true);
    previewTimer->setInterval(Settings::pref->seek_rate);
    connect(previewTimer, &TWZTimer::timeout,
            this, &TTimeSliderAction::onPreviewTimerTimeout);
}

QWidget* TTimeSliderAction::createWidget(QWidget* parent) {

    TTimeSlider* slider = new TTimeSlider(parent, posMS, durationMS);
    slider->setEnabled(isEnabled());

    connect(slider, &TTimeSlider::posChanged,
            this, &TTimeSliderAction::onPosChanged);
    connect(slider, &TTimeSlider::draggingPosChanged,
            this, &TTimeSliderAction::onPosChanged);

    connect(slider, &TTimeSlider::wheelUp,
            this, &TTimeSliderAction::wheelUp);
    connect(slider, &TTimeSlider::wheelDown,
            this, &TTimeSliderAction::wheelDown);

    connect(slider, &TTimeSlider::toolTipEvent,
            this, &TTimeSliderAction::onToolTipEvent);

    return slider;
}

// Update this pos and widgets pos
void TTimeSliderAction::setPos(int ms) {

    posMS = ms;

    QList<QWidget*> widgets = createdWidgets();
    for(int i = 0; i < widgets.count(); i++) {
        TTimeSlider* widget = static_cast<TTimeSlider*>(widgets.at(i));
        bool wasBlocked = widget->blockSignals(true);
        widget->setPosMS(posMS);
        widget->blockSignals(wasBlocked);
    }
}

// Slot triggered by player when play position updated
void TTimeSliderAction::setPosition(int ms) {

    if (ms < 0) {
        WZWARN(QString("Received negative position %1").arg(ms));
        ms = 0;
    } else if (ms > durationMS) {
        WZWARN(QString("Received position %1 larger than duration %2")
               .arg(ms).arg(durationMS));
        ms = durationMS;
    }

    if (ms != posMS) {
        setPos(ms);
    }
}

// Slot triggered by player when duration updated
void TTimeSliderAction::setDuration(int ms) {
    WZDEBUG(QString("Received duration %1 ms").arg(ms));

    durationMS = ms;

    // Probably changed video
    lastPreviewTime = NO_POS;
    requestedPos = NO_POS;

    QList<QWidget*> widgets = createdWidgets();
    for (int i = 0; i < widgets.count(); i++) {
        TTimeSlider* slider = static_cast<TTimeSlider*>(widgets.at(i));
        bool wasBlocked = slider->blockSignals(true);
        slider->setDurationMS(durationMS);
        slider->blockSignals(wasBlocked);
    }
}

void TTimeSliderAction::onUpdatePosTimerTimeout() {

    if (previewPlayer->isBuffering()) {
        WZDEBUG("Waiting for player to catch up");
        updatePosTimer->start();
    } else if (qAbs(requestedPos - posMS) > POS_RES) {
        if (Settings::pref->seek_relative) {
            emit percentageChanged(double(requestedPos * 100) / durationMS);
        } else {
            emit positionChanged(requestedPos);
        }
    }
}

// Slider pos changed
void TTimeSliderAction::onPosChanged(int ms) {

    requestedPos = ms;
    if (durationMS <= 0) {
        WZWARN(QString("Ignoring posChanged() while duration %1 <= 0")
               .arg(durationMS));
    } else if (!updatePosTimer->isActive()) {
        updatePosTimer->start();
    }
}

void TTimeSliderAction::preview() {

    TPlayerWindow* playerWindow = previewPlayer->playerWindow;
    QPoint pos = QCursor::pos();
    int ms = previewSlider->getTimeMS(previewSlider->mapFromGlobal(pos));
    if (qAbs(ms - lastPreviewTime) > POS_RES) {
        // 10 ms -> resolution 100 fps
        lastPreviewTime = ms;

        // Map geometry parent widget slider to global
        QWidget* parentWidget = previewSlider->parentWidget();
        QRect r = parentWidget->frameGeometry();
        if (parentWidget->parentWidget()) {
            r.moveTo(parentWidget->parentWidget()->mapToGlobal(r.topLeft()));
        }

        // Map to local parent player window
        parentWidget = playerWindow->parentWidget();
        r.moveTo(parentWidget->mapFromGlobal(r.topLeft()));
        pos = parentWidget->mapFromGlobal(pos);

        // Set pos
        const int d = 6;
        if (previewSlider->orientation() == Qt::Horizontal) {
            pos.rx() = pos.x() - playerWindow->width() / 2;
            if (pos.y() > parentWidget->height() / 2) {
                pos.ry() = r.top() - playerWindow->height() - d;
            } else {
                pos.ry() = r.top() + r.height() + d;
            }
        } else {
            pos.ry() = pos.y() - playerWindow->height() / 2;
            if (pos.x() > parentWidget->width() / 2) {
                pos.rx() = r.x() - playerWindow->width() - d;
            } else {
                pos.rx() = r.x() + r.width() + d;
            }
        }

        // Move player window
        playerWindow->move(pos);
        // Show player window
        if (!playerWindow->isVisible()) {
            playerWindow->setVisible(true);
            playerWindow->raise();
        }

        // Seek requested time
        previewPlayer->pause();
        previewPlayer->seekMS(ms);
    } else if (!playerWindow->isVisible()) {
        playerWindow->setVisible(true);
        playerWindow->raise();
    }

    previewTimer->start();
}

void TTimeSliderAction::onPreviewTimerTimeout() {

    if (previewSlider->underMouse() && !QApplication::mouseButtons()) {
        preview();
    } else {
        previewPlayer->playerWindow->hide();
    }
}

void TTimeSliderAction::onToolTipEvent(TTimeSlider* slider,
                                       QPoint pos,
                                       int ms) {

    if (previewPlayer->statePOP() && durationMS > 0) {
        previewSlider = slider;
        if (!previewTimer->isActive()) {
            previewTimer->start();
        }
    } else {
        QToolTip::showText(pos, TWZTime::formatTimeSec(ms / 1000), slider);
    }
}

} // namespace Action
} // namespace Gui

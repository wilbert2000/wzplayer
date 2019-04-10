#ifndef GUI_ACTION_TIMESLIDERACTION_H
#define GUI_ACTION_TIMESLIDERACTION_H

#include "gui/action/widgetactions.h"
#include "wzdebug.h"


class TWZTimer;

namespace Player {
class TPlayer;
}

namespace Gui {

class TMainWindow;

namespace Action {

class TTimeSlider;

class TTimeSliderAction : public TWidgetAction {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER
public:
    TTimeSliderAction(TMainWindow* mw, Player::TPlayer* player);

signals:
    void positionChanged(int ms);
    void percentageChanged(double percentage);

    void wheelUp();
    void wheelDown();

protected:
    virtual QWidget* createWidget(QWidget* parent) override;

private:
    TWZTimer* updatePosTimer;
    int posMS;
    int durationMS;
    int requestedPos;

    Player::TPlayer* previewPlayer;
    TWZTimer* previewTimer;
    TTimeSlider* previewSlider;
    int lastPreviewTime;

    void setPos(int ms);
    void preview();

private slots:
    void setPosition(int ms);
    void setDuration(int ms);

    void onPosChanged(int ms);
    void onToolTipEvent(TTimeSlider* slider, QPoint pos, int ms);

    void onUpdatePosTimerTimeout();
    void onPreviewTimerTimeout();
};

} // namespace Action
} // namespace Gui
#endif // GUI_ACTION_TIMESLIDERACTION_H

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
    TTimeSliderAction(TMainWindow* mw,
                      QWidget* aPanel,
                      Player::TPlayer* player);

public slots:
    void setPosition(double sec);
    void setDuration(double);

signals:
    void positionChanged(double sec);
    void percentageChanged(double percentage);
    void dragPositionChanged(double sec);

    void wheelUp();
    void wheelDown();

protected:
    virtual QWidget* createWidget(QWidget* parent) override;

private:
    QWidget* panel;
    Player::TPlayer* previewPlayer;
    TWZTimer* previewTimer;
    TTimeSlider* previewSlider;
    int lastPreviewTime;

    int pos;
    int maxPos;
    double duration;

    void setPos();
    void preview();

private slots:
    void onPosChanged(int value);
    void onDraggingPosChanged(int value);
    void onDelayedDraggingPos(int value);
    void onToolTipEvent(TTimeSlider* slider, QPoint pos, int secs);
    void onPreviewTimerTimeout();
};

} // namespace Action
} // namespace Gui
#endif // GUI_ACTION_TIMESLIDERACTION_H

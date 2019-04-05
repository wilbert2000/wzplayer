#ifndef GUI_ACTION_TIMESLIDERACTION_H
#define GUI_ACTION_TIMESLIDERACTION_H

#include "gui/action/widgetactions.h"
#include "wzdebug.h"


namespace Gui {
namespace Action {

class TTimeSliderAction : public TWidgetAction {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TTimeSliderAction(QWidget* parent);

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
    int pos;
    int maxPos;
    double duration;

    void setPos();

private slots:
    void onPosChanged(int value);
    void onDraggingPosChanged(int value);
    void onDelayedDraggingPos(int value);
};

} // namespace Action
} // namespace Gui
#endif // GUI_ACTION_TIMESLIDERACTION_H

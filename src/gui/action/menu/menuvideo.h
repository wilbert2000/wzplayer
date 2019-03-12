#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu/menu.h"
#include <QActionGroup>


namespace Gui {

class TPlayerWindow;
class TMainWindow;
class TVideoEqualizer;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuAspect : public TMenu {
    Q_OBJECT
public:
    explicit TMenuAspect(QWidget* parent, TMainWindow* mw);

protected:
    virtual void onAboutToShow() override;

private slots:
    void setAspectToolTip(QString tip);
};

class TZoomAndPanGroup : public QActionGroup {
    Q_OBJECT
public:
    explicit TZoomAndPanGroup(TMainWindow* mw);
};

class TMenuVideo : public TMenu {
    Q_OBJECT
public:
    TMenuVideo(TMainWindow* mw);

protected:
    virtual void enableActions();

private:
    TAction* stereo3DAct;

    TAction* screenshotAct;
    TAction* screenshotsAct;
    TAction* capturingAct;

private slots:
    void startStopScreenshots();
    void startStopCapture();
}; // class TMenuVideo

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOMENU_H

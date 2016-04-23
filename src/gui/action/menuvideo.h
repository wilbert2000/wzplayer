#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu.h"


class TCore;
class TPlayerWindow;

namespace Gui {

class TBase;
class TVideoEqualizer;

namespace Action {

class TAction;
class TActionGroup;


class TMenuVideo : public TMenu {
	Q_OBJECT
public:
    TMenuVideo(TBase* mw,
               TCore* c,
               TPlayerWindow* playerwindow,
               TVideoEqualizer* videoEqualizer);

protected:
    virtual void enableActions();

private:
	TCore* core;

	TAction* fullscreenAct;
	TAction* exitFullscreenAct;

	TAction* equalizerAct;
	TAction* resetVideoEqualizerAct;

	TAction* decContrastAct;
	TAction* incContrastAct;
	TAction* decBrightnessAct;
	TAction* incBrightnessAct;
	TAction* decHueAct;
	TAction* incHueAct;
	TAction* decSaturationAct;
	TAction* incSaturationAct;
	TAction* decGammaAct;
	TAction* incGammaAct;

	TAction* stereo3DAct;

	TAction* screenshotAct;
	TAction* screenshotsAct;
	TAction * capturingAct;

private slots:
	void onFullscreenChanged();
}; // class TMenuVideo

} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOMENU_H

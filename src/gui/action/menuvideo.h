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
	TMenuVideo(TBase* parent,
			   TCore* c,
			   TPlayerWindow* playerwindow,
			   TVideoEqualizer* videoEqualizer);

protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	TCore* core;

	TAction* fullscreenAct;
	TAction* exitFullscreenAct;

#if USE_ADAPTER
	TMenu* screenMenu;
	TActionGroup* screenGroup;
#endif

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
	TAction* flipAct;
	TAction* mirrorAct;

	TAction* screenshotAct;
	TAction* screenshotsAct;

#ifdef CAPTURE_STREAM
	TAction * capturingAct;
#endif

private slots:
	void onFullscreenChanged();
}; // class TMenuVideo

} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOMENU_H

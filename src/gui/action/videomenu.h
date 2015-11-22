#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu.h"


class TCore;
class TPlayerWindow;

namespace Gui {

class TAction;
class TActionGroup;
class TBase;
class TVideoEqualizer;

class TVideoMenu : public TMenu {
	Q_OBJECT
public:
	TVideoMenu(TBase* parent,
			   TCore* c,
			   TPlayerWindow* playerwindow,
			   TVideoEqualizer* videoEqualizer);
	TAction* fullscreenAct;

protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	TCore* core;

#if USE_ADAPTER
	TMenu* screenMenu;
	TActionGroup* screenGroup;
#endif

	TAction* videoEqualizerAct;
	TAction* stereo3dAct;
	TAction* flipAct;
	TAction* mirrorAct;

	TAction* nextVideoTrackAct;
	TActionGroup* videoTrackGroup;
	TMenu* videoTrackMenu;

	TAction* screenshotAct;
	TAction* screenshotsAct;

#ifdef CAPTURE_STREAM
	TAction * capturingAct;
#endif
#ifdef VIDEOPREVIEW
	TAction* videoPreviewAct;
#endif

private slots:
	void updateVideoTracks();
};

} // namespace Gui

#endif // GUI_VIDEOMENU_H

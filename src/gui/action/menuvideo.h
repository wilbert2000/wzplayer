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

class TMenuVideo : public TMenu {
	Q_OBJECT
public:
	TMenuVideo(TBase* parent,
			   TCore* c,
			   TPlayerWindow* playerwindow,
			   TVideoEqualizer* videoEqualizer);
	void fullscreenChanged(bool fullscreen);

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

#ifndef GUI_AUDIOMENU_H
#define GUI_AUDIOMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TAudioEqualizer;

class TMenuAudio : public TMenu {
public:
	explicit TMenuAudio(QWidget* parent, TCore* c, TAudioEqualizer* audioEqualizer);

protected:
	virtual void enableActions(bool stopped, bool, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	TCore* core;

	TAction* muteAct;
	TAction* decVolumeAct;
	TAction* incVolumeAct;

	TAction* decAudioDelayAct;
	TAction* incAudioDelayAct;
	TAction* audioDelayAct;

	TAction* audioEqualizerAct;
	TAction* resetAudioEqualizerAct;

	QMenu* audioFilterMenu;
	TAction* volnormAct;
#ifdef MPLAYER_SUPPORT
	TAction* extrastereoAct;
	TAction* karaokeAct;
#endif

	TAction* loadAudioAct;
	TAction* unloadAudioAct;
};

} // namespace Gui

#endif // GUI_AUDIOMENU_H

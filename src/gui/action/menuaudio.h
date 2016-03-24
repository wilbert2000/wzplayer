#ifndef GUI_AUDIOMENU_H
#define GUI_AUDIOMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAudioEqualizer;

namespace Action {

class TAction;


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
	TAction* extrastereoAct;
	TAction* karaokeAct;

	TAction* loadAudioAct;
	TAction* unloadAudioAct;
}; // class TMenuAudio

} // namespace Action
} // namespace Gui

#endif // GUI_AUDIOMENU_H

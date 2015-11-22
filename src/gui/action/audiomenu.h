#ifndef GUI_AUDIOMENU_H
#define GUI_AUDIOMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TActionGroup;
class TAudioEqualizer;

class TAudioMenu : public TMenu {
	Q_OBJECT
public:
	explicit TAudioMenu(QWidget* parent, TCore* c, TAudioEqualizer* audioEqualizer);

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
	QMenu* audioFilterMenu;
	TAction* volnormAct;
#ifdef MPLAYER_SUPPORT
	TAction* extrastereoAct;
	TAction* karaokeAct;
#endif

	TAction* nextAudioTrackAct;
	TActionGroup* audioTrackGroup;
	TMenu* audioTrackMenu;

	TAction* loadAudioAct;
	TAction* unloadAudioAct;

private slots:
	void updateAudioTracks();
};

} // namespace Gui

#endif // GUI_AUDIOMENU_H

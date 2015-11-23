#ifndef GUI_MENUAUDIOTRACKS_H
#define GUI_MENUAUDIOTRACKS_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TActionGroup;

class TMenuAudioTracks : public TMenu {
	Q_OBJECT
public:
	explicit TMenuAudioTracks(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool audio);
private:
	TCore* core;
	TAction* nextAudioTrackAct;
	TActionGroup* audioTrackGroup;
private slots:
	void updateAudioTracks();
};

} // namespace Gui

#endif // GUI_MENUAUDIOTRACKS_H

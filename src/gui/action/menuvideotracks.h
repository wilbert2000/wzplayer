#ifndef GUI_MENUVIDEOTRACKS_H
#define GUI_MENUVIDEOTRACKS_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TActionGroup;

class TMenuVideoTracks : public TMenu {
	Q_OBJECT
public:
	explicit TMenuVideoTracks(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
private:
	TCore* core;
	TAction* nextVideoTrackAct;
	TActionGroup* videoTrackGroup;
private slots:
	void updateVideoTracks();
};

} // namespace Gui

#endif // GUI_MENUVIDEOTRACKS_H

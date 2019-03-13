#ifndef GUI_ACTION_MENU_AUDIOMENU_H
#define GUI_ACTION_MENU_AUDIOMENU_H

#include "gui/action/menu/menu.h"
#include "gui/action/actiongroup.h"


namespace Gui {

class TMainWindow;

namespace Action {

class TAction;

namespace Menu {

class TAudioChannelGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TAudioChannelGroup(TMainWindow* mw);
};

class TStereoGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TStereoGroup(TMainWindow* mw);
};

class TMenuAudioTracks : public TMenu {
    Q_OBJECT
public:
    explicit TMenuAudioTracks(QWidget* parent, TMainWindow* mw);
private slots:
    void updateAudioTracks(TAction* next, TActionGroup* group);
};

class TMenuAudio : public TMenu {
public:
    explicit TMenuAudio(QWidget* parent, TMainWindow* mw);
}; // class TMenuAudio

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_AUDIOMENU_H

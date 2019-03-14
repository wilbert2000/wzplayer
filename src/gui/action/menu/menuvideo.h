#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu/menu.h"
#include "gui/action/actiongroup.h"
#include <QActionGroup>


namespace Gui {

class TPlayerWindow;
class TMainWindow;
class TVideoEqualizer;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuAspect : public TMenu {
    Q_OBJECT
public:
    explicit TMenuAspect(QWidget* parent, TMainWindow* mw);
private slots:
    void setAspectToolTip(QString tip);
};

class TZoomAndPanGroup : public QActionGroup {
    Q_OBJECT
public:
    explicit TZoomAndPanGroup(TMainWindow* mw);
};

class TDeinterlaceGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TDeinterlaceGroup(TMainWindow* mw);
};

class TRotateGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TRotateGroup(TMainWindow* mw);
};

class TMenuVideoTracks : public TMenu {
    Q_OBJECT
public:
    explicit TMenuVideoTracks(QWidget* parent, TMainWindow* mw);
private slots:
    void updateVideoTracks(TActionGroup* group);
};

class TMenuVideo : public TMenu {
    Q_OBJECT
public:
    TMenuVideo(QWidget* parent, TMainWindow* mw);
}; // class TMenuVideo

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOMENU_H

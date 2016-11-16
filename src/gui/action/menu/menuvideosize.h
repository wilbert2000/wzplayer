#ifndef GUI_ACTION_MENU_MENUVIDEOSIZE_H
#define GUI_ACTION_MENU_MENUVIDEOSIZE_H

#include "log4qt/logger.h"
#include "gui/action/actiongroup.h"
#include "gui/action/menu/menu.h"


namespace Gui {

class TMainWindow;
class TPlayerWindow;

namespace Action {
namespace Menu {

class TVideoSizeGroup : public TActionGroup {
    Q_OBJECT

public:
    explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
    int size_percentage;

public slots:
    void updateVideoSizeGroup();

private:
    TPlayerWindow* playerWindow;

    void uncheck();
};


class TMenuVideoSize : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TMenuVideoSize(TMainWindow* mw, TPlayerWindow* pw);

protected:
    virtual void enableActions();
    virtual void onAboutToShow();

private:
    TPlayerWindow* playerWindow;
    TVideoSizeGroup* group;
    TAction* doubleSizeAct;
    TAction* resizeOnLoadAct;
    TAction* currentSizeAct;

    void upd();

private slots:
    void onVideoSizeFactorChanged();
    void onResizeOnLoadTriggered(bool);
}; // class TMenuVideoSize

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIDEOSIZE_H

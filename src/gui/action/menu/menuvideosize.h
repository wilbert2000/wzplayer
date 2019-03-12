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

class TWindowSizeGroup : public TActionGroup {
    Q_OBJECT
public:
    explicit TWindowSizeGroup(TMainWindow* mw, TPlayerWindow* pw);
    int size_percentage;

public slots:
    void updateWindowSizeGroup();

private:
    TPlayerWindow* playerWindow;

    void uncheck();
};


class TMenuVideoSize : public TMenu {
    Q_OBJECT
public:
    TMenuVideoSize(QWidget* parent, TMainWindow* mw);

protected:
    virtual void onAboutToShow() override;

private slots:
    void setWindowSizeToolTip(QString tip);

}; // class TMenuVideoSize

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUVIDEOSIZE_H

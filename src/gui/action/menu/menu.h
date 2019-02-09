#ifndef GUI_ACTION_MENU_MENU_H
#define GUI_ACTION_MENU_MENU_H

#include "gui/action/menu/menuexec.h"


namespace Settings {
class TMediaSettings;
}

namespace Gui {

class TMainWindow;

namespace Action {
namespace Menu {


class TMenu : public TMenuExec {
    Q_OBJECT
public:
    explicit TMenu(QWidget* parent,
                   TMainWindow* w,
                   const QString& name,
                   const QString& text,
                   const QString& icon = QString());
    virtual ~TMenu();

    void addActionsTo(QWidget* w);

protected:
    TMainWindow* main_window;
    virtual void onAboutToShow();
    virtual void setVisible(bool visible);

protected slots:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENU_H

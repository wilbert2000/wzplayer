#ifndef GUI_ACTION_MENU_MENUEXEC_H
#define GUI_ACTION_MENU_MENUEXEC_H

#include <QMenu>


namespace Gui {
namespace Action {
namespace Menu {


class TMenuExec : public QMenu {
    Q_OBJECT
public:
    explicit TMenuExec(QWidget* parent);

public slots:
    void execSlot();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUEXEC_H

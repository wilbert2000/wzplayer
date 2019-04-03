#ifndef GUI_ACTION_MENU_MENU_H
#define GUI_ACTION_MENU_MENU_H

#include <QMenu>


namespace Gui {
namespace Action {
namespace Menu {

class TMenu : public QMenu {
    Q_OBJECT
public:
    explicit TMenu(QWidget* parent,
                   const QString& name = "",
                   const QString& text = "",
                   const QString& icon = "");
public slots:
    void execSlot();
};

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENU_H

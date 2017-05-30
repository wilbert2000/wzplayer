#ifndef GUI_DOCKWIDGET_H
#define GUI_DOCKWIDGET_H

#include <QDockWidget>
#include "wzdebug.h"


namespace Gui {

class TDockWidget : public QDockWidget {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TDockWidget(const QString& title, QWidget* parent);
    virtual ~TDockWidget();

    void loadConfig();
    void onShowMainWindow();
    void onHideMainWindow();

private:
    bool restore;
};

} // namespace Gui

#endif // GUI_DOCKWIDGET_H

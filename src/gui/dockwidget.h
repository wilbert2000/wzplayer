#ifndef GUI_DOCKWIDGET_H
#define GUI_DOCKWIDGET_H

#include <QDockWidget>
#include "wzdebug.h"


namespace Gui {

class TDockWidget : public QDockWidget {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TDockWidget(QWidget* parent,
                const QString& objectName,
                const QString& title);

private slots:
    void onFocusChanged(QWidget *old, QWidget *now);
    void onDockVisibilityChanged(bool visible);
};

} // namespace Gui

#endif // GUI_DOCKWIDGET_H

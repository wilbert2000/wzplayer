#ifndef GUI_DOCKWIDGET_H
#define GUI_DOCKWIDGET_H

#include <QDockWidget>
#include "wzdebug.h"


namespace Gui {

class TPlayerWindow;

class TDockWidget : public QDockWidget {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TDockWidget(QWidget* parent,
                TPlayerWindow* aPlayerWindow,
                const QString& objectName,
                const QString& title);

    Qt::DockWidgetArea getArea() const;

protected:
    virtual void closeEvent(QCloseEvent* e) override;

private:
    TPlayerWindow* playerWindow;
    Qt::DockWidgetArea lastArea;

    void resizeMainWindow(bool visible);
    void triggerResize(bool visible);

private slots:
    void onToggleViewTriggered(bool visible);
    void onFocusChanged(QWidget *old, QWidget *now);
    void onDockVisibilityChanged(bool visible);
    void onDockLocationChanged(Qt::DockWidgetArea area);
};

} // namespace Gui

#endif // GUI_DOCKWIDGET_H

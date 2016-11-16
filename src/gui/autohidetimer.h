#ifndef AUTOHIDETIMER_H
#define AUTOHIDETIMER_H

#include <QTimer>
#include <QWidget>
#include <QAction>
#include <QList>
#include "wzdebug.h"


namespace Gui {

class TAutoHideItem {
public:
    QAction* action;
    QWidget* widget;

    TAutoHideItem() : action(0), widget(0) {}
    TAutoHideItem(QAction* a, QWidget* w) : action(a), widget(w) {}
    virtual ~TAutoHideItem();
};

class TAutoHideTimer : public QTimer {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    explicit TAutoHideTimer(QObject *parent, QWidget* playerwin);
    virtual ~TAutoHideTimer();

    void add(QAction* action, QWidget* w);

public slots:
    void start();
    void stop();

    void enable();
    void disable();

    void setAutoHideMouse(bool on);
    void startAutoHideMouse() { setAutoHideMouse(true); }
    void stopAutoHideMouse() { setAutoHideMouse(false); }
    void setDraggingPlayerWindow(bool dragging);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    bool autoHide;
    bool enabled;
    bool settingVisible;

    bool autoHideMouse;
    bool mouseHidden;
    QPoint autoHideMouseLastPosition;
    bool draggingPlayerWindow;

    QWidget* playerWindow;

    QList<QAction*> actions;
    QList<QWidget*> widgets;

    typedef QMap<QString, TAutoHideItem> TItemMap;
    TItemMap items;

    bool hiddenWidget() const;
    bool visibleWidget() const;

    bool mouseInsideShowArea() const;
    void setVisible(bool visible);

    void showHiddenMouse();
    void hideMouse();

private slots:
    void onActionToggled(bool visible);
    void onTimeOut();
};

} // namespace Gui

#endif // AUTOHIDETIMER_H

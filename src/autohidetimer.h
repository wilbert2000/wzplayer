#ifndef AUTOHIDETIMER_H
#define AUTOHIDETIMER_H

#include <QTimer>
#include <QWidget>
#include <QAction>
#include <QList>

class TAutoHideItem {
public:
	QAction* action;
	QWidget* widget;

	TAutoHideItem() : action(0), widget(0) {}
	TAutoHideItem(QAction* a, QWidget* w) : action(a), widget(w) {}
	virtual ~TAutoHideItem() {}
};

class TAutoHideTimer : public QTimer {
	Q_OBJECT
public:
	explicit TAutoHideTimer(QObject *parent, QWidget* playerwin);
	virtual ~TAutoHideTimer();

	void add(QAction* action, QWidget* w);

public slots:
	void start();
	void stop();

	void enable();
	void disable();

protected:
	bool eventFilter(QObject* obj, QEvent* event);

private:
	bool autoHide;
	bool enabled;
	bool settingVisible;

	QWidget* playerWindow;

	QList<QAction*> actions;
	QList<QWidget*> widgets;

	typedef QMap<QString, TAutoHideItem> TItemMap;
	TItemMap items;

	bool hiddenWidget() const;
	bool visibleWidget() const;

	bool insideShowArea(const QPoint& p) const;
	void setVisible(bool visible);

private slots:
	void onActionToggled(bool visible);
	void onTimeOut();
};

#endif // AUTOHIDETIMER_H

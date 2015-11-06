#include "autohidetimer.h"
#include <QDebug>
#include <QApplication>
#include <QMouseEvent>
#include "desktop.h"
#include "settings/preferences.h"

const int MOUSE_MOVED_TRESHOLD = 4;

using namespace Settings;

TAutoHideTimer::TAutoHideTimer(QObject *parent, QWidget* playerwin)
	: QTimer(parent)
	, autoHide(false)
	, settingVisible(false)
	, playerWindow(playerwin) {

	setSingleShot(true);
	setInterval(pref->floating_hide_delay);
	connect(this, SIGNAL(timeout()), this, SLOT(onTimeOut()));

	playerWindow->installEventFilter(this);
}

TAutoHideTimer::~TAutoHideTimer() {
}

void TAutoHideTimer::start() {

	autoHide = true;
	QTimer::start();
}

void TAutoHideTimer::stop() {

	autoHide = false;
	QTimer::stop();
	// Show the widgets to save their visible state
	setVisible(true);
}

void TAutoHideTimer::add(QAction* action, QWidget* w) {

	TAutoHideItem item(action, w);
	items[action->objectName()] = item;
	if (action->isChecked()) {
		actions.append(action);
		widgets.append(w);
	}
	connect(action, SIGNAL(toggled(bool)), this, SLOT(onActionToggled(bool)));
}

void TAutoHideTimer::setVisible(bool visible) {

	settingVisible = true;
	for(int i = 0; i < actions.size(); i++) {
		QAction* action = actions[i];
		if (action->isChecked() != visible) {
			action->trigger();
		}
	}
	settingVisible = false;
}

bool TAutoHideTimer::visibleWidget() const {

	for(int i = 0; i < widgets.size(); i++) {
		if (widgets[i]->isVisible())
			return true;
	}
	return false;
}

bool TAutoHideTimer::hiddenWidget() const {

	for(int i = 0; i < widgets.size(); i++) {
		if (widgets[i]->isHidden())
			return true;
	}
	return false;
}

bool TAutoHideTimer::insideShowArea(const QPoint& p) const {

	const int margin = 100;

	// Check bottom of screen
	if (pref->fullscreen && p.y() > TDesktop::size(playerWindow).height() - margin) {
		return true;
	}

	// Check around widgets
	for (int i = 0; i < widgets.size(); i++) {
		QWidget* w = widgets[i];
		QRect sa(w->mapToGlobal(QPoint(0, 0)) - QPoint(margin, margin),
				 w->size() + QSize(margin, margin));
		if (sa.contains(p)) {
			return true;
		}
	}

	return false;
}

void TAutoHideTimer::onActionToggled(bool visible) {

	if (settingVisible)
		return;

	QString name = QObject::sender()->objectName();
	TItemMap::const_iterator i = items.find(name);
	if (i == items.end()) {
		qWarning() << "TAutoHideTimer::onActionToggled: action" << name << "not found";
	} else {
		TAutoHideItem item = i.value();
		if (visible) {
			actions.append(item.action);
			widgets.append(item.widget);
			// Show the other widgets too
			setVisible(true);
			start();
		} else {
			actions.removeOne(item.action);
			widgets.removeOne(item.widget);
		}
	}
}

void TAutoHideTimer::onTimeOut() {

	if (autoHide && visibleWidget()) {
		if (QApplication::mouseButtons() || insideShowArea(QCursor::pos())) {
			start();
		} else {
			setVisible(false);
		}
	}
}

bool TAutoHideTimer::eventFilter(QObject* obj, QEvent* event) {

	if (autoHide
		&& (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress)
		&& hiddenWidget()) {
		if (event->type() == QEvent::MouseButtonPress
			|| pref->floating_activation_area == Settings::TPreferences::Anywhere) {
			setVisible(true);
			start();
		} else {
			QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
			if (insideShowArea(mouse_event->globalPos())) {
				setVisible(true);
				start();
			}
		}
	}

	return QTimer::eventFilter(obj, event);
}


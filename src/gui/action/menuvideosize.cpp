#include "gui/action/videosizemenu.h"
#include "settings/preferences.h"
#include "playerwindow.h"


using namespace Settings;

namespace Gui {
TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup(parent, "size")
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
	new TActionGroupItem(this, this, "size_50", QT_TR_NOOP("5&0%"), 50, false);
	new TActionGroupItem(this, this, "size_75", QT_TR_NOOP("7&5%"), 75, false);
	a = new TActionGroupItem(this, this, "size_100", QT_TR_NOOP("&100%"), 100, false);
	a->setShortcut(Qt::CTRL | Qt::Key_1);
	new TActionGroupItem(this, this, "size_125", QT_TR_NOOP("1&25%"), 125, false);
	new TActionGroupItem(this, this, "size_150", QT_TR_NOOP("15&0%"), 150, false);
	new TActionGroupItem(this, this, "size_175", QT_TR_NOOP("1&75%"), 175, false);
	a = new TActionGroupItem(this, this, "size_200", QT_TR_NOOP("&200%"), 200, false);
	a->setShortcut(Qt::CTRL | Qt::Key_2);
	new TActionGroupItem(this, this, "size_300", QT_TR_NOOP("&300%"), 300, false);
	new TActionGroupItem(this, this, "size_400", QT_TR_NOOP("&400%"), 400, false);

	setChecked(pref->size_factor * 100);
}

void TVideoSizeGroup::uncheck() {

	QAction* current = checkedAction();
	if (current)
		current->setChecked(false);
}

void TVideoSizeGroup::enableVideoSizeGroup(bool on) {

	QSize s = playerWindow->resolution();
	setEnabled(on && !pref->fullscreen && s.width() > 0 && s.height() > 0);
}

void TVideoSizeGroup::updateVideoSizeGroup() {
	// qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup");

	uncheck();
	QSize s = playerWindow->resolution();
	if (pref->fullscreen || s.width() <= 0 || s.height() <= 0) {
		setEnabled(false);
	} else {
		setEnabled(true);

		// Update size factor
		QSize video_size = playerWindow->getAdjustedSize(s.width(), s.height(), 1.0);
		int factor_x = qRound((double) playerWindow->width() * 100 / video_size.width());
		int factor_y = qRound((double) playerWindow->height() * 100/ video_size.height());

		// Set when x and y factor agree
		if (factor_x == factor_y) {
			setChecked(factor_x);
		}
	}
}


TVideoSizeMenu::TVideoSizeMenu(QWidget* parent, TPlayerWindow* pw)
	: TMenu(parent, this, "videosize_menu", QT_TR_NOOP("&Size"), "video_size") {

	group = new TVideoSizeGroup(this, pw);
	addActions(group->actions());
	connect(group, SIGNAL(activated(int)), parent, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(this, "toggle_double_size", QT_TR_NOOP("&Toggle double size"), "", Qt::CTRL | Qt::Key_D);
	connect(doubleSizeAct, SIGNAL(triggered()), parent, SLOT(toggleDoubleSize()));

	connect(parent, SIGNAL(aboutToEnterFullscreenSignal()), this, SLOT(fullscreenChanged()));
	connect(parent, SIGNAL(didExitFullscreenSignal()), this, SLOT(fullscreenChanged()));

	addActionsTo(parent);
}

void TVideoSizeMenu::enableActions(bool stopped, bool video, bool) {

	group->enableVideoSizeGroup(!stopped && video);
	doubleSizeAct->setEnabled(group->isEnabled());
}

void TVideoSizeMenu::fullscreenChanged() {

	group->enableVideoSizeGroup(true);
	doubleSizeAct->setEnabled(group->isEnabled());
}

void TVideoSizeMenu::onAboutToShow() {

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
}


} // namespace Gui

#include "gui/action/videosize.h"
#include <QDebug>
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "playerwindow.h"
#include "images.h"


using namespace Settings;

namespace Gui {

TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup("size", parent)
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
	a = new TActionGroupItem(this, this, "5&0%", "size_50", 50);
	a = new TActionGroupItem(this, this, "7&5%", "size_75", 75);
	a = new TActionGroupItem(this, this, "&100%", "size_100", 100);
	a->setShortcut(Qt::CTRL | Qt::Key_1);
	a = new TActionGroupItem(this, this, "1&25%", "size_125", 125);
	a = new TActionGroupItem(this, this, "15&0%", "size_150", 150);
	a = new TActionGroupItem(this, this, "1&75%", "size_175", 175);
	a = new TActionGroupItem(this, this, "&200%", "size_200", 200);
	a->setShortcut(Qt::CTRL | Qt::Key_2);
	a = new TActionGroupItem(this, this, "&300%", "size_300", 300);
	a = new TActionGroupItem(this, this, "&400%", "size_400", 400);
}

TVideoSizeGroup::~TVideoSizeGroup() {
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
	: QMenu(parent) {

	menuAction()->setObjectName("videosize_menu");
	sizeGroup = new TVideoSizeGroup(this, pw);
	addActions(sizeGroup->actions());
	connect(sizeGroup, SIGNAL(activated(int)), parent, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(Qt::CTRL | Qt::Key_D, this, "toggle_double_size");
	addAction(doubleSizeAct);
	connect(doubleSizeAct, SIGNAL(triggered()), parent, SLOT(toggleDoubleSize()));

	connect(this, SIGNAL(aboutToShow()), this, SLOT(onAboutToShow()));
}

TVideoSizeMenu::~TVideoSizeMenu() {
}

void TVideoSizeMenu::retranslateStrings() {

	menuAction()->setText(tr("Si&ze"));
	menuAction()->setIcon(Images::icon("video_size"));
	doubleSizeAct->change(tr("&Toggle double size"));
}

void TVideoSizeMenu::enableVideoSize(bool on) {

	sizeGroup->enableVideoSizeGroup(on);
	doubleSizeAct->setEnabled(sizeGroup->isEnabled());
}

void TVideoSizeMenu::onAboutToShow() {
	//qDebug("TVideoSizeMenu::onAboutToShow");

	sizeGroup->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(sizeGroup->isEnabled());
}

} // namespace Gui



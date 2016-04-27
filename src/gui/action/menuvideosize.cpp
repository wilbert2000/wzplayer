#include "gui/action/menuvideosize.h"
#include <QDebug>
#include "desktop.h"
#include "settings/preferences.h"
#include "playerwindow.h"
#include "gui/base.h"
#include "core.h"


using namespace Settings;

namespace Gui {
namespace Action {


TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup(parent, "size")
	, size_percentage(qRound(pref->size_factor * 100))
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
    a = new TActionGroupItem(this, this, "size_25", tr("25%"), 25, false);
    a->setShortcut(Qt::CTRL | Qt::Key_1);
    a = new TActionGroupItem(this, this, "size_50", tr("5&0%"), 50, false);
    a->setShortcut(Qt::CTRL | Qt::Key_2);
    a = new TActionGroupItem(this, this, "size_75", tr("7&5%"), 75, false);
    a->setShortcut(Qt::CTRL | Qt::Key_3);
    a = new TActionGroupItem(this, this, "size_100", tr("&100%"), 100, false);
    a->setShortcut(Qt::CTRL | Qt::Key_4);
    a = new TActionGroupItem(this, this, "size_125", tr("125%"), 125, false);
    a->setShortcut(Qt::CTRL | Qt::Key_5);
    a = new TActionGroupItem(this, this, "size_150", tr("15&0%"), 150, false);
    a->setShortcut(Qt::CTRL | Qt::Key_6);
    a = new TActionGroupItem(this, this, "size_175", tr("1&75%"), 175, false);
    a->setShortcut(Qt::CTRL | Qt::Key_7);
    a = new TActionGroupItem(this, this, "size_200", tr("&200%"), 200, false);
    a->setShortcut(Qt::CTRL | Qt::Key_8);
    a = new TActionGroupItem(this, this, "size_300", tr("&300%"), 300, false);
    a->setShortcut(Qt::CTRL | Qt::Key_9);
    a = new TActionGroupItem(this, this, "size_400", tr("&400%"), 400, false);
    a->setShortcut(Qt::CTRL | Qt::Key_0);

	setChecked(size_percentage);
}

void TVideoSizeGroup::uncheck() {

	QAction* current = checkedAction();
	if (current)
		current->setChecked(false);
}

void TVideoSizeGroup::updateVideoSizeGroup() {
	//qDebug("Gui::Action::TVideoSizeGroup::updateVideoSizeGroup");

	uncheck();
	QSize s = playerWindow->resolution();
	if (s.isEmpty()) {
		setEnabled(false);
		size_percentage = 0;
	} else {
		setEnabled(true);
		double factorX, factorY;
		playerWindow->getSizeFactors(factorX, factorY);

		// Set size factor for menu
		if (factorX < factorY) {
			size_percentage = qRound(factorX * 100);
		} else {
			size_percentage = qRound(factorY * 100);
		}

		// Only set check menu when x and y factor agree on +/- half a pixel
		// TODO: fuzzy...
		double diffX = 0.5 / s.width() / factorX;
		double diffY = 0.5 / s.height() / factorY;
		if (diffY < diffX) {
			diffX = diffY;
		}
		// Multiply allowed diff with zoom...
		diffX *= playerWindow->zoom();
		diffY = qAbs(factorX - factorY);
		if (diffY < diffX) {
			setChecked(size_percentage);
			//qDebug("Gui::Action::TVideoSizeGroup::updateVideoSizeGroup: match on %d fx %f fy %f diff %f allowed diff %f",
			//	   size_percentage, factorX, factorY, diffY, diffX);
		} else {
			//qDebug("Gui::Action::TVideoSizeGroup::updateVideoSizeGroup: no match on %d fx %f fy %f diff %f allowed diff %f",
			//	   size_percentage, factorX, factorY, diffY, diffX);
		}
	}
}


TMenuVideoSize::TMenuVideoSize(TBase* mw, TPlayerWindow* pw) :
    TMenu(mw, mw, "videosize_menu", tr("&Size"), "video_size"),
    playerWindow(pw) {

	group = new TVideoSizeGroup(this, pw);
	addActions(group->actions());
    connect(group, SIGNAL(activated(int)), main_window, SLOT(changeSize(int)));

	addSeparator();
    doubleSizeAct = new TAction(this, "toggle_double_size", tr("&Toggle double size"), "", Qt::Key_D);
    connect(doubleSizeAct, SIGNAL(triggered()), main_window, SLOT(toggleDoubleSize()));

    currentSizeAct = new TAction(this, "video_size", "", "", QKeySequence("`"));
	connect(currentSizeAct, SIGNAL(triggered()), this, SLOT(optimizeSizeFactor()));
    //setDefaultAction(currentSizeAct);

    connect(playerWindow, SIGNAL(videoSizeFactorChanged(double, double)),
			this, SLOT(onVideoSizeFactorChanged()), Qt::QueuedConnection);

    addActionsTo(main_window);
	upd();
}

void TMenuVideoSize::enableActions() {

    TCore* core = main_window->getCore();
    bool enable = core->statePOP() && core->hasVideo();
	group->setEnabled(enable);
	doubleSizeAct->setEnabled(enable);
	currentSizeAct->setEnabled(enable);
}

void TMenuVideoSize::upd() {
	//qDebug("Gui::Action::TMenuVideoSize:upd: size factor %f", pref->size_factor);

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
	currentSizeAct->setEnabled(group->isEnabled());

	// Update text and tips
    QString txt = tr("&Optimize size %1%").arg(QString::number(group->size_percentage));
	currentSizeAct->setTextAndTip(txt);

    txt = tr("Size %1%").arg(QString::number(group->size_percentage));
	QString scut = menuAction()->shortcut().toString();
	if (!scut.isEmpty()) {
		txt += " (" + scut + ")";
	}
	menuAction()->setToolTip(txt);
}

void TMenuVideoSize::onAboutToShow() {
	upd();
}

void TMenuVideoSize::onVideoSizeFactorChanged() {
	upd();
}

bool TMenuVideoSize::optimizeSizeFactorPreDef(int factor, int predef_factor) {

	int d = predef_factor / 10;
	if (d < 10)
		d = 10;
	if (qAbs(factor - predef_factor) < d) {
		qDebug("Gui::Action::TMenuVideoSize::optimizeSizeFactorPreDef: optimizing size from %d%% to predefined value %d%%",
			   factor, predef_factor);
        main_window->changeSize(predef_factor);
		return true;
	}
	return false;
}

void TMenuVideoSize::optimizeSizeFactor() {
	qDebug("Gui::Action::TMenuVideoSize::optimizeSizeFactor");

	double factor;

	if (pref->fullscreen) {
		// TODO: use core to update mset.zoom_factor_fullscreen
		playerWindow->setZoom(1.0);
		return;
	}

	double size_factor = pref->size_factor;

	// Limit size to 0.6 of available desktop
	const double f = 0.6;
	QSize available_size = TDesktop::availableSize(playerWindow);
	QSize res = playerWindow->resolution();
	QSize video_size = res * size_factor;

	double max = f * available_size.height();
	// Adjust height first
	if (video_size.height() > max) {
		factor = max / res.height();
		qDebug("Gui::Action::TMenuVideoSize::optimizeSizeFactor: height larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = res * size_factor;
	}
	// Adjust width
	max = f * available_size.width();
	if (video_size.width() > max) {
		factor = max / res.width();
		qDebug("Gui::Action::TMenuVideoSize::optimizeSizeFactor: width larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = res * size_factor;
	}

	// Round to predefined values
	int factor_int = qRound(size_factor * 100);
	const int factors[] = {25, 50, 75, 100, 125, 150, 175, 200, 300, 400 };
	for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); i++) {
		if (optimizeSizeFactorPreDef(factor_int, factors[i])) {
			return;
		}
	}

	// Make width multiple of 16
	int new_w = ((video_size.width() + 8) / 16) * 16;
	factor = (double) new_w / res.width();
	qDebug("Gui::Action::TMenuVideoSize::optimizeSizeFactor: optimizing width %d factor %f to multiple of 16 %d factor %f",
		   video_size.width(), size_factor, new_w, factor);
    main_window->changeSize(factor);
}


} // namespace Action
} // namespace Gui


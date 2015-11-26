#include "gui/action/menuvideosize.h"
#include <QDebug>
#include "desktop.h"
#include "settings/preferences.h"
#include "playerwindow.h"
#include "gui/base.h"


using namespace Settings;

namespace Gui {

TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup(parent, "size")
	, size_percentage(qRound(pref->size_factor * 100))
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

	setChecked(size_percentage);
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
	qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup");

	uncheck();
	QSize s = playerWindow->resolution();
	if (pref->fullscreen || s.width() <= 0 || s.height() <= 0) {
		setEnabled(false);
	} else {
		setEnabled(true);

		// Update size factor
		QSize video_size = playerWindow->getAdjustedSize(s.width(), s.height(), 1.0);
		int factor_x = qRound(((double) (playerWindow->width() * 100)) / video_size.width());
		int factor_y = qRound(((double) (playerWindow->height() * 100)) / video_size.height());

		// Set when x and y factor agree
		if (factor_x == factor_y) {
			setChecked(factor_x);
		}

		// Store smallest for use by TMenuVideoSize::currentSizeAct
		if (factor_y < factor_x) {
			factor_x = factor_y;
		}
		qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup: updating size factor from %d to %d",
			   size_percentage, factor_x);
		size_percentage = factor_x;
	}
}


TMenuVideoSize::TMenuVideoSize(TBase* mw, TPlayerWindow* pw)
	: TMenu(mw, this, "videosize_menu", QT_TR_NOOP("&Size"), "video_size")
	, mainWindow(mw) {

	group = new TVideoSizeGroup(this, pw);
	addActions(group->actions());
	connect(group, SIGNAL(activated(int)), mainWindow, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(this, "toggle_double_size", QT_TR_NOOP("&Toggle double size"), "", Qt::CTRL | Qt::Key_D);
	connect(doubleSizeAct, SIGNAL(triggered()), mainWindow, SLOT(toggleDoubleSize()));

	currentSizeAct = new TAction(this, "video_size", "");
	connect(currentSizeAct, SIGNAL(triggered()), this, SLOT(optimizeSizeFactor()));
	connect(mainWindow, SIGNAL(videoSizeFactorChanged()), this, SLOT(onVideoSizeFactorChanged()));

	connect(mainWindow, SIGNAL(fullscreenChanged()), this, SLOT(onFullscreenChanged()));

	addActionsTo(mainWindow);
	upd();
}

void TMenuVideoSize::enableActions(bool stopped, bool video, bool) {

	group->enableVideoSizeGroup(!stopped && video);
	doubleSizeAct->setEnabled(group->isEnabled());
	currentSizeAct->setEnabled(group->isEnabled());
}

void TMenuVideoSize::upd() {
	qDebug("Gui::TMenuVideoSize:upd: %f", pref->size_factor);

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
	currentSizeAct->setEnabled(group->isEnabled());

	// Update text and tips
	int s = pref->fullscreen ? 100 : group->size_percentage;
	QString txt = translator->tr("&Round size %1%").arg(QString::number(s));
	currentSizeAct->setTextAndTip(txt);

	txt = translator->tr("Size %1%").arg(QString::number(s));
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

void TMenuVideoSize::onFullscreenChanged() {
	upd();
}

bool TMenuVideoSize::optimizeSizeFactorPreDef(int factor, int predef_factor) {

	if (qAbs(factor - predef_factor) < 10) {
		qDebug("Gui::TMenuVideoSize::optimizeSizeFactorPreDef: optimizing size factor from %f to predefined value %f",
			   pref->size_factor, (double) predef_factor / 100);
		mainWindow->changeSize(predef_factor);
		return true;
	}
	return false;
}

void TMenuVideoSize::optimizeSizeFactor() {
	qDebug("TMenuVideoSize::optimizeSizeFactor()");

	double size_factor = pref->size_factor;

	// Limit size to 0.5 of available desktop
	const double f = 0.5;
	TPlayerWindow* playerWindow = group->playerWindow;
	QSize available_size = TDesktop::availableSize(playerWindow);
	QSize res = playerWindow->resolution();
	QSize video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);

	double max = f * available_size.height();
	double factor;
	// Adjust height first
	if (video_size.height() > max) {
		factor = max / res.height();
		qDebug("Gui::TBase::optimizeSizeFactor: height larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);
	}
	// Adjust width
	max = f * available_size.width();
	if (video_size.width() > max) {
		factor = max / res.width();
		qDebug("Gui::TBase::optimizeSizeFactor: width larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);
	}

	// Round to predefined values
	int factor_int = qRound(size_factor * 100);
	const int factors[] = {50, 75, 100, 125, 150, 175, 200, 300, 400 };
	for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); i++) {
		if (optimizeSizeFactorPreDef(factor_int, factors[i])) {
			return;
		}
	}

	// Make width multiple of 16
	int new_w = ((video_size.width() + 8) / 16) * 16;
	if (new_w != video_size.width()) {
		int new_factor = qRound(((double) (new_w * 100)) / res.width());
		qDebug("Gui::TBase::optimizeSizeFactor: optimizing width %d factor %d to multiple of 16 %d factor %d",
			   video_size.width(), factor_int, new_w, new_factor);
		mainWindow->changeSize(new_factor);
	} else {
		qDebug("Gui::TBase::optimizeSizeFactor: video size already multiple of 16");
	}
}

} // namespace Gui

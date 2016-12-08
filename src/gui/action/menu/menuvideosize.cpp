#include "gui/action/menu/menuvideosize.h"
#include "gui/desktop.h"
#include "settings/preferences.h"
#include "gui/playerwindow.h"
#include "gui/mainwindow.h"
#include "player/player.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


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
        }
	}
}


TMenuVideoSize::TMenuVideoSize(TMainWindow* mw, TPlayerWindow* pw) :
    TMenu(mw, mw, "videosize_menu", tr("Window &size"), "video_size"),
    playerWindow(pw) {

    group = new TVideoSizeGroup(this, pw);
    addActions(group->actions());
    connect(group, SIGNAL(activated(int)), main_window, SLOT(setSize(int)));

    addSeparator();
    doubleSizeAct = new TAction(this, "toggle_double_size",
                                tr("&Toggle double size"), "", Qt::Key_D);
    connect(doubleSizeAct, SIGNAL(triggered()),
            main_window, SLOT(toggleDoubleSize()));

    currentSizeAct = new TAction(this, "video_size", "", "", QKeySequence("`"));
    connect(currentSizeAct, SIGNAL(triggered()),
            main_window, SLOT(optimizeSizeFactor()));
    //setDefaultAction(currentSizeAct);
    connect(playerWindow, SIGNAL(videoSizeFactorChanged(double, double)),
            this, SLOT(onVideoSizeFactorChanged()), Qt::QueuedConnection);


    resizeOnLoadAct = new TAction(this, "resize_on_load",
                                  tr("&Resize on load"), "",
                                  Qt::ALT | Qt::Key_R);
    resizeOnLoadAct->setCheckable(true);
    connect(resizeOnLoadAct, SIGNAL(triggered(bool)),
            this, SLOT(onResizeOnLoadTriggered(bool)));

    addActionsTo(main_window);
    upd();
}

void TMenuVideoSize::enableActions() {

    bool enable = player->statePOP() && player->hasVideo();
	group->setEnabled(enable);
	doubleSizeAct->setEnabled(enable);
	currentSizeAct->setEnabled(enable);
    // Resize on load always enabled
}

void TMenuVideoSize::upd() {

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
    resizeOnLoadAct->setChecked(pref->resize_on_load);
	currentSizeAct->setEnabled(group->isEnabled());

	// Update text and tips
    QString txt = tr("&Optimize (current size %1%)").arg(group->size_percentage);
	currentSizeAct->setTextAndTip(txt);

    txt = tr("Size %1%").arg(group->size_percentage);
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

void TMenuVideoSize::onResizeOnLoadTriggered(bool b) {
    logger()->info("onResizeOnLoadTriggered: setting resize on load to %1", b);

    pref->resize_on_load = b;
}

} // namespace Menu
} // namespace Action
} // namespace Gui


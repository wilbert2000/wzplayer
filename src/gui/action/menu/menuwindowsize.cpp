#include "gui/action/menu/menuwindowsize.h"
#include "desktop.h"
#include "settings/preferences.h"
#include "gui/playerwindow.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "iconprovider.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TWindowSizeGroup::TWindowSizeGroup(TMainWindow* mw, TPlayerWindow* pw)
    : TActionGroup(mw, "window_size_group")
    , size_percentage(qRound(pref->size_factor * 100))
    , playerWindow(pw) {

    setEnabled(false);

    TActionGroupItem* a;
    a = new TActionGroupItem(mw, this, "size_25", tr("25%"), 25);
    a->setShortcut(Qt::CTRL | Qt::Key_1);
    a = new TActionGroupItem(mw, this, "size_33", tr("33%"), 33);
    a->setShortcut(Qt::CTRL | Qt::Key_2);
    a = new TActionGroupItem(mw, this, "size_50", tr("50%"), 50);
    a->setShortcut(Qt::CTRL | Qt::Key_3);
    a = new TActionGroupItem(mw, this, "size_75", tr("75%"), 75);
    a->setShortcut(Qt::CTRL | Qt::Key_4);
    a = new TActionGroupItem(mw, this, "size_100", tr("100%"), 100);
    a->setShortcut(Qt::CTRL | Qt::Key_5);
    a = new TActionGroupItem(mw, this, "size_125", tr("125%"), 125);
    a->setShortcut(Qt::CTRL | Qt::Key_6);
    a = new TActionGroupItem(mw, this, "size_150", tr("150%"), 150);
    a->setShortcut(Qt::CTRL | Qt::Key_7);
    a = new TActionGroupItem(mw, this, "size_175", tr("175%"), 175);
    a->setShortcut(Qt::CTRL | Qt::Key_8);
    a = new TActionGroupItem(mw, this, "size_200", tr("200%"), 200);
    a->setShortcut(Qt::CTRL | Qt::Key_9);
    a = new TActionGroupItem(mw, this, "size_300", tr("300%"), 300);
    a->setShortcut(Qt::CTRL | Qt::Key_0);
    a = new TActionGroupItem(mw, this, "size_400", tr("400%"), 400);

    setChecked(size_percentage);

    connect(this, &TWindowSizeGroup::activated,
            mw, &TMainWindow::setSizePercentage);
}

void TWindowSizeGroup::uncheck() {

    QAction* current = checkedAction();
    if (current) {
        current->setChecked(false);
    }
}

void TWindowSizeGroup::update() {

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
        if (size_percentage == 33) {
            setChecked(size_percentage);
        } else {
            // Only set check menu when x and y factor agree on +/- half a pixel
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
}


TMenuWindowSize::TMenuWindowSize(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "window_size_menu", tr("Window size"), "noicon") {

    menuAction()->setIcon(iconProvider.windowSizeIcon);

    TWindowSizeGroup* group = mw->findChild<TWindowSizeGroup*>();
    addActions(group->actions());

    addSeparator();
    addAction(mw->requireAction("size_toggle_double"));
    addAction(mw->requireAction("size_optimize"));
    addAction(mw->requireAction("resize_on_load"));

    connect(mw, &TMainWindow::setWindowSizeToolTip,
            this, &TMenuWindowSize::setWindowSizeToolTip);
    connect(this, &TMenuWindowSize::aboutToShow,
            mw, &TMainWindow::updateWindowSizeMenu);

    mw->updateWindowSizeMenu();
}

void TMenuWindowSize::setWindowSizeToolTip(QString tip) {

    QString s = menuAction()->shortcut().toString();
    if (!s.isEmpty()) {
        tip += " (" + s + ")";
    }
    menuAction()->setToolTip(tip);
}


} // namespace Menu
} // namespace Action
} // namespace Gui


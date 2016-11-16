#include "gui/action/menu/menuaspect.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "settings/aspectratio.h"
#include "gui/action/actiongroup.h"
#include "gui/action/menu/menu.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TMenuAspect::TMenuAspect(TMainWindow* mw) :
    TMenu(mw, mw, "aspect_menu", tr("&Aspect ratio"), "aspect") {

    group = new TActionGroup(this, "aspect");
    group->setEnabled(false);
    aspectAutoAct = new TActionGroupItem(this, group, "aspect_detect",
                                         tr("&Auto"), TAspectRatio::AspectAuto);
    addSeparator();
    new TActionGroupItem(this, group, "aspect_1_1",
        TAspectRatio::aspectIDToString(0), TAspectRatio::Aspect11);
    new TActionGroupItem(this, group, "aspect_5_4",
        TAspectRatio::aspectIDToString(1), TAspectRatio::Aspect54);
    new TActionGroupItem(this, group, "aspect_4_3",
        TAspectRatio::aspectIDToString(2), TAspectRatio::Aspect43);
    new TActionGroupItem(this, group, "aspect_11_8",
        TAspectRatio::aspectIDToString(3), TAspectRatio::Aspect118);
    new TActionGroupItem(this, group, "aspect_14_10",
        TAspectRatio::aspectIDToString(4), TAspectRatio::Aspect1410);
    new TActionGroupItem(this, group, "aspect_3_2",
        TAspectRatio::aspectIDToString(5), TAspectRatio::Aspect32);
    new TActionGroupItem(this, group, "aspect_14_9",
        TAspectRatio::aspectIDToString(6), TAspectRatio::Aspect149);
    new TActionGroupItem(this, group, "aspect_16_10",
        TAspectRatio::aspectIDToString(7), TAspectRatio::Aspect1610);
    new TActionGroupItem(this, group, "aspect_16_9",
        TAspectRatio::aspectIDToString(8), TAspectRatio::Aspect169);
    new TActionGroupItem(this, group, "aspect_2_1",
        TAspectRatio::aspectIDToString(9), TAspectRatio::Aspect2);
    new TActionGroupItem(this, group, "aspect_2.35_1",
        TAspectRatio::aspectIDToString(10), TAspectRatio::Aspect235);
    addSeparator();
    aspectDisabledAct = new TActionGroupItem(this, group, "aspect_none",
        tr("&Disabled"), TAspectRatio::AspectNone);

    connect(group, SIGNAL(activated(int)), player, SLOT(setAspectRatio(int)));
    connect(player, SIGNAL(aspectRatioChanged(int)),
            this, SLOT(onAspectRatioChanged(int)),
            Qt::QueuedConnection);

    addSeparator();
    nextAspectAct = new TAction(this, "next_aspect", tr("&Next aspect ratio"),
                                "", Qt::Key_A);
    connect(nextAspectAct, SIGNAL(triggered()), player, SLOT(nextAspectRatio()));

    addActionsTo(main_window);
    upd();
}

void TMenuAspect::upd() {

    group->setChecked(player->mset.aspect_ratio.ID());

    double aspect = player->mset.aspectToDouble();
    QString s = TAspectRatio::doubleToString(aspect);
    menuAction()->setToolTip(tr("Aspect ratio %1").arg(s));

    s = tr("&Auto") + "\t"
        + TAspectRatio::doubleToString(player->mdat.video_aspect_original);
    aspectAutoAct->setTextAndTip(s);

    s = tr("&Disabled") + "\t" + TAspectRatio::doubleToString(
            (double) player->mdat.video_width / player->mdat.video_height);
    aspectDisabledAct->setTextAndTip(s);
}

void TMenuAspect::enableActions() {

    // Uses mset, so useless to set if stopped or no video
    bool enabled = player->statePOP() && player->hasVideo();
    group->setEnabled(enabled);
    nextAspectAct->setEnabled(enabled);
    upd();
}

void TMenuAspect::onMediaSettingsChanged(TMediaSettings*) {
    upd();
}

void TMenuAspect::onAboutToShow() {
    upd();
}

void TMenuAspect::onAspectRatioChanged(int) {
    upd();
}

} // namespace Menu
} // namespace Action
} // namespace Gui

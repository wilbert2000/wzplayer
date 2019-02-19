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
    TMenu(mw, mw, "aspect_menu", tr("Aspect ratio"), "aspect") {

    group = new TActionGroup(mw, "aspectgroup");
    group->setEnabled(false);
    aspectAutoAct = new TActionGroupItem(mw, group, "aspect_detect",
                                         tr("Auto"), TAspectRatio::AspectAuto);
    addAction(aspectAutoAct);
    addSeparator();
    addAction(new TActionGroupItem(mw, group, "aspect_1_1",
        TAspectRatio::aspectIDToString(0), TAspectRatio::Aspect11));
    addAction(new TActionGroupItem(mw, group, "aspect_5_4",
        TAspectRatio::aspectIDToString(1), TAspectRatio::Aspect54));
    addAction(new TActionGroupItem(mw, group, "aspect_4_3",
        TAspectRatio::aspectIDToString(2), TAspectRatio::Aspect43));
    addAction(new TActionGroupItem(mw, group, "aspect_11_8",
        TAspectRatio::aspectIDToString(3), TAspectRatio::Aspect118));
    addAction(new TActionGroupItem(mw, group, "aspect_14_10",
        TAspectRatio::aspectIDToString(4), TAspectRatio::Aspect1410));
    addAction(new TActionGroupItem(mw, group, "aspect_3_2",
        TAspectRatio::aspectIDToString(5), TAspectRatio::Aspect32));
    addAction(new TActionGroupItem(mw, group, "aspect_14_9",
        TAspectRatio::aspectIDToString(6), TAspectRatio::Aspect149));
    addAction(new TActionGroupItem(mw, group, "aspect_16_10",
        TAspectRatio::aspectIDToString(7), TAspectRatio::Aspect1610));
    addAction(new TActionGroupItem(mw, group, "aspect_16_9",
        TAspectRatio::aspectIDToString(8), TAspectRatio::Aspect169));
    addAction(new TActionGroupItem(mw, group, "aspect_2_1",
        TAspectRatio::aspectIDToString(9), TAspectRatio::Aspect2));
    addAction(new TActionGroupItem(mw, group, "aspect_2.35_1",
        TAspectRatio::aspectIDToString(10), TAspectRatio::Aspect235));
    addSeparator();
    aspectDisabledAct = new TActionGroupItem(mw, group, "aspect_none",
        tr("Disabled"), TAspectRatio::AspectNone);
    addAction(aspectDisabledAct);

    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setAspectRatio);
    connect(player, &Player::TPlayer::aspectRatioChanged,
            this, &TMenuAspect::onAspectRatioChanged,
            Qt::QueuedConnection);

    addSeparator();
    nextAspectAct = new TAction(mw, "next_aspect", tr("Next aspect ratio"), "",
                                Qt::Key_A);
    addAction(nextAspectAct);
    connect(nextAspectAct, &TAction::triggered,
            player, &Player::TPlayer::nextAspectRatio);

    upd();
}

void TMenuAspect::upd() {

    group->setChecked(player->mset.aspect_ratio.ID());

    double aspect = player->mset.aspectToDouble();
    QString s = TAspectRatio::doubleToString(aspect);
    menuAction()->setToolTip(tr("Aspect ratio %1").arg(s));

    s = tr("Auto") + "\t"
        + TAspectRatio::doubleToString(player->mdat.video_aspect_original);
    aspectAutoAct->setTextAndTip(s);

    s = tr("Disabled") + "\t" + TAspectRatio::doubleToString(
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

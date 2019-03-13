#include "gui/action/menu/menuvideofilter.h"
#include "settings/mediasettings.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TFilterGroup::TFilterGroup(TMainWindow* mw) : QActionGroup(mw) {

    setObjectName("filtergroup");
    setExclusive(false);
    setEnabled(false);

    postProcessingAct = new TAction(this, "postprocessing",
                                    tr("Postprocessing"));
    postProcessingAct->setCheckable(true);
    connect(postProcessingAct, &TAction::triggered,
            player, &Player::TPlayer::setPostprocessing);

    deblockAct = new TAction(this, "deblock", tr("Deblock"));
    deblockAct->setCheckable(true);
    connect(deblockAct, &TAction::triggered,
            player, &Player::TPlayer::setDeblock);

    deringAct = new TAction(this, "dering", tr("Dering"));
    deringAct->setCheckable(true);
    connect(deringAct, &TAction::triggered,
            player, &Player::TPlayer::setDering);

    gradfunAct = new TAction(this, "gradfun", tr("Debanding (gradfun)"));
    gradfunAct->setCheckable(true);
    connect(gradfunAct, &TAction::triggered,
            player, &Player::TPlayer::setGradfun);

    addNoiseAct = new TAction(this, "add_noise", tr("Add noise"));
    addNoiseAct->setCheckable(true);
    connect(addNoiseAct, &TAction::triggered,
            player, &Player::TPlayer::setNoise);

    addLetterboxAct = new TAction(this, "add_letterbox",
                                  tr("Add black borders"), "letterbox");
    addLetterboxAct->setCheckable(true);
    connect(addLetterboxAct, &TAction::triggered,
            player, &Player::TPlayer::setetterbox);

    softwareScalingAct = new TAction(this, "software_scaling",
                                     tr("Software scaling"));
    softwareScalingAct->setCheckable(true);
    connect(softwareScalingAct, &TAction::triggered,
            player, &Player::TPlayer::setSoftwareScaling);

    phaseAct = new TAction(this, "autodetect_phase", tr("Autodetect phase"));
    phaseAct->setCheckable(true);
    connect(phaseAct, &TAction::triggered,
            player, &Player::TPlayer::setAutophase);
}

void TFilterGroup::updateFilters() {

    postProcessingAct->setChecked(player->mset.postprocessing_filter);
    deblockAct->setChecked(player->mset.deblock_filter);
    deringAct->setChecked(player->mset.dering_filter);
    gradfunAct->setChecked(player->mset.gradfun_filter);
    addNoiseAct->setChecked(player->mset.noise_filter);
    addLetterboxAct->setChecked(player->mset.add_letterbox);
    softwareScalingAct->setChecked(player->mset.upscaling_filter);
    phaseAct->setChecked(player->mset.phase_filter);
}


TMenuVideoFilter::TMenuVideoFilter(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "videofilter_menu", tr("Video filters"),
            "video_filters") {

    QActionGroup* group = mw->findChild<TFilterGroup*>("filtergroup");
    addActions(group->actions());

    // Denoise
    TMenu* menu = new TMenu(this, mw, "denoise_menu", tr("Denoise"), "denoise");
    group = mw->findChild<TActionGroup*>("denoisegroup");
    menu->addActions(group->actions());
    addMenu(menu);
    connect(menu, &TMenu::aboutToShow, mw, &TMainWindow::updateFilters);

    // Unsharp
    menu = new TMenu(this, mw, "sharpen_menu", tr("Sharpen"), "sharpen");
    group = mw->findChild<TActionGroup*>("sharpengroup");
    menu->addActions(group->actions());
    addMenu(menu);
    connect(menu, &TMenu::aboutToShow, mw, &TMainWindow::updateFilters);

    connect(this, &TMenu::aboutToShow, mw, &TMainWindow::updateFilters);
}

} // namespace Menu
} // namespace Action
} // namespace Gui

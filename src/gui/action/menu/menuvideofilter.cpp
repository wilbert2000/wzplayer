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


TMenuVideoFilter::TMenuVideoFilter(TMainWindow* mw)
    : TMenu(mw, mw, "videofilter_menu", tr("Filters"), "video_filters") {

    group = new QActionGroup(mw);
    group->setExclusive(false);
    group->setEnabled(false);

    postProcessingAct = new TAction(group, "postprocessing",
                                    tr("Postprocessing"));
    postProcessingAct->setCheckable(true);
    connect(postProcessingAct, &TAction::triggered,
            player, &Player::TPlayer::setPostprocessing);

    deblockAct = new TAction(group, "deblock", tr("Deblock"));
    deblockAct->setCheckable(true);
    connect(deblockAct, &TAction::triggered,
            player, &Player::TPlayer::setDeblock);

    deringAct = new TAction(group, "dering", tr("Dering"));
    deringAct->setCheckable(true);
    connect(deringAct, &TAction::triggered,
            player, &Player::TPlayer::setDering);

    gradfunAct = new TAction(group, "gradfun", tr("Debanding (gradfun)"));
    gradfunAct->setCheckable(true);
    connect(gradfunAct, &TAction::triggered,
            player, &Player::TPlayer::setGradfun);

    addNoiseAct = new TAction(group, "add_noise", tr("Add noise"));
    addNoiseAct->setCheckable(true);
    connect(addNoiseAct, &TAction::triggered,
            player, &Player::TPlayer::setNoise);

    addLetterboxAct = new TAction(group, "add_letterbox",
                                  tr("Add black borders"), "letterbox");
    addLetterboxAct->setCheckable(true);
    connect(addLetterboxAct, &TAction::triggered,
            player, &Player::TPlayer::setetterbox);

    softwareScalingAct = new TAction(group, "software_scaling",
                                     tr("Software scaling"));
    softwareScalingAct->setCheckable(true);
    connect(softwareScalingAct, &TAction::triggered,
            player, &Player::TPlayer::setSoftwareScaling);

    phaseAct = new TAction(group, "autodetect_phase", tr("Autodetect phase"));
    phaseAct->setCheckable(true);
    connect(phaseAct, &TAction::triggered,
            player, &Player::TPlayer::setAutophase);

    addActions(group->actions());

    // Denoise
    TMenu* menu = new TMenu(mw, mw, "denoise_menu", tr("Denoise"), "denoise");
    denoiseGroup = new TActionGroup(mw, "denoisegroup");
    denoiseGroup->setEnabled(false);
    denoiseNoneAct = new TActionGroupItem(mw, denoiseGroup, "denoise_none",
                                         tr("Off"), TMediaSettings::NoDenoise);
    denoiseNormalAct = new TActionGroupItem(mw, denoiseGroup,
                                            "denoise_normal", tr("Normal"),
                                            TMediaSettings::DenoiseNormal);
    denoiseSoftAct = new TActionGroupItem(mw, denoiseGroup, "denoise_soft",
                                          tr("Soft"),
                                          TMediaSettings::DenoiseSoft);
    menu->addActions(denoiseGroup->actions());
    addMenu(menu);
    connect(denoiseGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setDenoiser);
    connect(menu, &TMenu::aboutToShow,
            this, &TMenuVideoFilter::onAboutToShowDenoise);

    // Unsharp group
    menu = new TMenu(mw, mw, "sharpen_menu", tr("Sharpen"), "sharpen");
    sharpenGroup = new TActionGroup(mw, "sharpen");
    sharpenGroup->setEnabled(false);
    sharpenNoneAct = new TActionGroupItem(mw, sharpenGroup, "sharpen_off",
                                          tr("None"), 0);
    blurAct = new TActionGroupItem(mw, sharpenGroup, "blur", tr("Blur"), 1);
    sharpenAct = new TActionGroupItem(mw, sharpenGroup, "sharpen",
                                      tr("Sharpen"), 2);
    menu->addActions(sharpenGroup->actions());
    addMenu(menu);
    connect(sharpenGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setSharpen);
    connect(menu, &TMenu::aboutToShow,
            this, &TMenuVideoFilter::onAboutToShowUnSharp);

    updateFilters();
}

void TMenuVideoFilter::updateFilters() {

    postProcessingAct->setChecked(player->mset.postprocessing_filter);
    deblockAct->setChecked(player->mset.deblock_filter);
    deringAct->setChecked(player->mset.dering_filter);
    gradfunAct->setChecked(player->mset.gradfun_filter);
    addNoiseAct->setChecked(player->mset.noise_filter);
    addLetterboxAct->setChecked(player->mset.add_letterbox);
    softwareScalingAct->setChecked(player->mset.upscaling_filter);
    phaseAct->setChecked(player->mset.phase_filter);

    denoiseGroup->setChecked(player->mset.current_denoiser);
    sharpenGroup->setChecked(player->mset.current_unsharp);
}

void TMenuVideoFilter::enableActions() {

    bool enable = player->statePOP() && player->hasVideo()
                  && player->videoFiltersEnabled();
    group->setEnabled(enable);
    addLetterboxAct->setEnabled(enable && pref->isMPlayer());
    denoiseGroup->setEnabled(enable);
    sharpenGroup->setEnabled(enable);
}

void TMenuVideoFilter::onMediaSettingsChanged(Settings::TMediaSettings*) {
    updateFilters();
}

void TMenuVideoFilter::onAboutToShow() {
    updateFilters();
}

void TMenuVideoFilter::onAboutToShowDenoise() {
    denoiseGroup->setChecked(player->mset.current_denoiser);
}

void TMenuVideoFilter::onAboutToShowUnSharp() {
    sharpenGroup->setChecked(player->mset.current_unsharp);
}

} // namespace Menu
} // namespace Action
} // namespace Gui

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
    : TMenu(mw, mw, "videofilter_menu", tr("F&ilters"), "video_filters") {

    group = new QActionGroup(this);
    group->setExclusive(false);
    group->setEnabled(false);

    postProcessingAct = new TAction(this, "postprocessing",
                                    tr("&Postprocessing"));
    postProcessingAct->setCheckable(true);
    group->addAction(postProcessingAct);
    connect(postProcessingAct, &TAction::triggered,
            player, &Player::TPlayer::setPostprocessing);

    deblockAct = new TAction(this, "deblock", tr("&Deblock"));
    deblockAct->setCheckable(true);
    group->addAction(deblockAct);
    connect(deblockAct, &TAction::triggered,
            player, &Player::TPlayer::setDeblock);

    deringAct = new TAction(this, "dering", tr("De&ring"));
    deringAct->setCheckable(true);
    group->addAction(deringAct);
    connect(deringAct, &TAction::triggered,
            player, &Player::TPlayer::setDering);

    gradfunAct = new TAction(this, "gradfun", tr("Debanding (&gradfun)"));
    gradfunAct->setCheckable(true);
    group->addAction(gradfunAct);
    connect(gradfunAct, &TAction::triggered,
            player, &Player::TPlayer::setGradfun);

    addNoiseAct = new TAction(this, "add_noise", tr("Add n&oise"));
    addNoiseAct->setCheckable(true);
    group->addAction(addNoiseAct);
    connect(addNoiseAct, &TAction::triggered,
            player, &Player::TPlayer::setNoise);

    addLetterboxAct = new TAction(this, "add_letterbox",
                                  tr("Add &black borders"), "letterbox");
    addLetterboxAct->setCheckable(true);
    group->addAction(addLetterboxAct);
    connect(addLetterboxAct, &TAction::triggered,
            player, &Player::TPlayer::setetterbox);

    softwareScalingAct = new TAction(this, "software_scaling",
                                     tr("Soft&ware scaling"));
    softwareScalingAct->setCheckable(true);
    group->addAction(softwareScalingAct);
    connect(softwareScalingAct, &TAction::triggered,
            player, &Player::TPlayer::setSoftwareScaling);

    phaseAct = new TAction(this, "autodetect_phase", tr("&Autodetect phase"));
    phaseAct->setCheckable(true);
    group->addAction(phaseAct);
    connect(phaseAct, &TAction::triggered,
            player, &Player::TPlayer::setAutophase);

    // Denoise
    TMenu* menu = new TMenu(this, main_window, "denoise_menu", tr("De&noise"),
                            "denoise");
    denoiseGroup = new TActionGroup(this, "denoise");
    denoiseGroup->setEnabled(false);
    denoiseNoneAct = new TActionGroupItem(this, denoiseGroup, "denoise_none",
                                          tr("&Off"), TMediaSettings::NoDenoise,
                                          false);
    denoiseNormalAct = new TActionGroupItem(this, denoiseGroup,
                                            "denoise_normal", tr("&Normal"),
                                            TMediaSettings::DenoiseNormal,
                                            false);
    denoiseSoftAct = new TActionGroupItem(this, denoiseGroup, "denoise_soft",
                                          tr("&Soft"),
                                          TMediaSettings::DenoiseSoft, false);
    menu->addActions(denoiseGroup->actions());
    addMenu(menu);
    connect(denoiseGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setDenoiser);
    connect(menu, &TMenu::aboutToShow,
            this, &TMenuVideoFilter::onAboutToShowDenoise);

    // Unsharp group
    menu = new TMenu(this, main_window, "sharpen_menu", tr("S&harpen"),
                     "sharpen");
    sharpenGroup = new TActionGroup(this, "sharpen");
    sharpenGroup->setEnabled(false);
    sharpenNoneAct = new TActionGroupItem(this, sharpenGroup, "sharpen_off",
                                          tr("&None"), 0, false);
    blurAct = new TActionGroupItem(this, sharpenGroup, "blur", tr("&Blur"), 1,
                                   false);
    sharpenAct = new TActionGroupItem(this, sharpenGroup, "sharpen",
                                      tr("&Sharpen"), 2, false);
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

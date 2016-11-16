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

	postProcessingAct = new TAction(this, "postprocessing", tr("&Postprocessing"));
	postProcessingAct->setCheckable(true);
	group->addAction(postProcessingAct);
    connect(postProcessingAct, SIGNAL(triggered(bool)), player, SLOT(togglePostprocessing(bool)));

	deblockAct = new TAction(this, "deblock", tr("&Deblock"));
	deblockAct->setCheckable(true);
	group->addAction(deblockAct);
    connect(deblockAct, SIGNAL(triggered(bool)), player, SLOT(toggleDeblock(bool)));

	deringAct = new TAction(this, "dering", tr("De&ring"));
	deringAct->setCheckable(true);
	group->addAction(deringAct);
    connect(deringAct, SIGNAL(triggered(bool)), player, SLOT(toggleDering(bool)));

	gradfunAct = new TAction(this, "gradfun", tr("Debanding (&gradfun)"));
	gradfunAct->setCheckable(true);
	group->addAction(gradfunAct);
    connect(gradfunAct, SIGNAL(triggered(bool)), player, SLOT(toggleGradfun(bool)));

	addNoiseAct = new TAction(this, "add_noise", tr("Add n&oise"));
	addNoiseAct->setCheckable(true);
	group->addAction(addNoiseAct);
    connect(addNoiseAct, SIGNAL(triggered(bool)), player, SLOT(toggleNoise(bool)));

	addLetterboxAct = new TAction(this, "add_letterbox", tr("Add &black borders"), "letterbox");
	addLetterboxAct->setCheckable(true);
	group->addAction(addLetterboxAct);
    connect(addLetterboxAct, SIGNAL(triggered(bool)), player, SLOT(changeLetterbox(bool)));

	upscaleAct = new TAction(this, "upscaling", tr("Soft&ware scaling"));
	upscaleAct->setCheckable(true);
	group->addAction(upscaleAct);
    connect(upscaleAct, SIGNAL(triggered(bool)), player, SLOT(changeUpscale(bool)));

	phaseAct = new TAction(this, "autodetect_phase", tr("&Autodetect phase"));
	phaseAct->setCheckable(true);
	group->addAction(phaseAct);
    connect(phaseAct, SIGNAL(triggered(bool)), player, SLOT(toggleAutophase(bool)));

	// Denoise
    TMenu* menu = new TMenu(this, main_window, "denoise_menu", tr("De&noise"), "denoise");
	denoiseGroup = new TActionGroup(this, "denoise");
	denoiseGroup->setEnabled(false);
	denoiseNoneAct = new TActionGroupItem(this, denoiseGroup, "denoise_none", tr("&Off"), TMediaSettings::NoDenoise, false);
	denoiseNormalAct = new TActionGroupItem(this, denoiseGroup, "denoise_normal", tr("&Normal"), TMediaSettings::DenoiseNormal, false);
	denoiseSoftAct = new TActionGroupItem(this, denoiseGroup, "denoise_soft", tr("&Soft"), TMediaSettings::DenoiseSoft, false);
	menu->addActions(denoiseGroup->actions());
    menu->addActionsTo(main_window);
	addMenu(menu);
    connect(denoiseGroup, SIGNAL(activated(int)), player, SLOT(changeDenoise(int)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowDenoise()));

	// Unsharp group
    menu = new TMenu(this, main_window, "unsharp_menu", tr("Blur/S&harp"), "unsharp");
	unsharpGroup = new TActionGroup(this, "unsharp");
	unsharpGroup->setEnabled(false);
	unsharpNoneAct = new TActionGroupItem(this, unsharpGroup, "unsharp_off", tr("&None"), 0, false);
	blurAct = new TActionGroupItem(this, unsharpGroup, "blur", tr("&Blur"), 1, false);
	sharpenAct = new TActionGroupItem(this, unsharpGroup, "sharpen", tr("&Sharpen"), 2, false);
	menu->addActions(unsharpGroup->actions());
    menu->addActionsTo(main_window);
	addMenu(menu);
    connect(unsharpGroup, SIGNAL(activated(int)), player, SLOT(changeUnsharp(int)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(onAboutToShowUnSharp()));

	updateFilters();
}

void TMenuVideoFilter::updateFilters() {

    postProcessingAct->setChecked(player->mset.postprocessing_filter);
    deblockAct->setChecked(player->mset.deblock_filter);
    deringAct->setChecked(player->mset.dering_filter);
    gradfunAct->setChecked(player->mset.gradfun_filter);
    addNoiseAct->setChecked(player->mset.noise_filter);
    addLetterboxAct->setChecked(player->mset.add_letterbox);
    upscaleAct->setChecked(player->mset.upscaling_filter);
    phaseAct->setChecked(player->mset.phase_filter);

    denoiseGroup->setChecked(player->mset.current_denoiser);
    unsharpGroup->setChecked(player->mset.current_unsharp);
}

void TMenuVideoFilter::enableActions() {

    bool enable = player->statePOP() && player->hasVideo() && player->videoFiltersEnabled();
	group->setEnabled(enable);
    addLetterboxAct->setEnabled(enable && pref->isMPlayer());
	denoiseGroup->setEnabled(enable);
	unsharpGroup->setEnabled(enable);
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
    unsharpGroup->setChecked(player->mset.current_unsharp);
}

} // namespace Menu
} // namespace Action
} // namespace Gui

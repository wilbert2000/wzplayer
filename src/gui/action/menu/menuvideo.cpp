#include "gui/action/menu/menuvideo.h"
#include <QDebug>
#include "player/player.h"
#include "settings/mediasettings.h"
#include "gui/action/menu/menuaspect.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuvideosize.h"
#include "gui/action/menu/menuvideotracks.h"
#include "gui/videoequalizer.h"
#include "gui/mainwindow.h"

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


class TMenuDeinterlace : public TMenu {
public:
    explicit TMenuDeinterlace(TMainWindow* mw);
protected:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
    TActionGroup* group;
	TAction* toggleDeinterlaceAct;
};


TMenuDeinterlace::TMenuDeinterlace(TMainWindow* mw)
    : TMenu(mw, mw, "deinterlace_menu", tr("&Deinterlace"), "deinterlace") {

	group = new TActionGroup(this, "deinterlace");
	group->setEnabled(false);
    new TActionGroupItem(this, group, "deinterlace_none", tr("&None"), TMediaSettings::NoDeinterlace);
    new TActionGroupItem(this, group, "deinterlace_l5", tr("&Lowpass5"), TMediaSettings::L5);
    new TActionGroupItem(this, group, "deinterlace_yadif0", tr("&Yadif (normal)"), TMediaSettings::Yadif);
    new TActionGroupItem(this, group, "deinterlace_yadif1", tr("Y&adif (double framerate)"), TMediaSettings::Yadif_1);
    new TActionGroupItem(this, group, "deinterlace_lb", tr("Linear &Blend"), TMediaSettings::LB);
    new TActionGroupItem(this, group, "deinterlace_kern", tr("&Kerndeint"), TMediaSettings::Kerndeint);
    group->setChecked(player->mset.current_deinterlacer);
    connect(group, SIGNAL(activated(int)), player, SLOT(changeDeinterlace(int)));
	// No one else sets it

	addSeparator();
    toggleDeinterlaceAct = new TAction(this, "toggle_deinterlacing", tr("Toggle deinterlacing"), "deinterlace", Qt::Key_I);
    toggleDeinterlaceAct->setCheckable(true);
    connect(toggleDeinterlaceAct, SIGNAL(triggered()), player, SLOT(toggleDeinterlace()));

    addActionsTo(main_window);
}

void TMenuDeinterlace::enableActions() {

	// Using mset, so useless to set if stopped or no video
    bool enabled = player->statePOP() && player->hasVideo()
                   && player->videoFiltersEnabled();
	group->setEnabled(enabled);
	toggleDeinterlaceAct->setEnabled(enabled);
}

void TMenuDeinterlace::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->current_deinterlacer);
}

void TMenuDeinterlace::onAboutToShow() {
    group->setChecked(player->mset.current_deinterlacer);
}


class TMenuTransform : public TMenu {
public:
    explicit TMenuTransform(TMainWindow* mw);
protected:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
    TAction* flipAct;
    TAction* mirrorAct;
	TActionGroup* group;
};


TMenuTransform::TMenuTransform(TMainWindow* mw)
    : TMenu(mw, mw, "transform_menu", tr("&Transform"), "transform") {

    flipAct = new TAction(this, "flip", tr("Fli&p image"));
    flipAct->setCheckable(true);
    connect(flipAct, SIGNAL(triggered(bool)), player, SLOT(toggleFlip(bool)));

    mirrorAct = new TAction(this, "mirror", tr("&Mirror image"));
    mirrorAct->setCheckable(true);
    connect(mirrorAct, SIGNAL(triggered(bool)), player, SLOT(toggleMirror(bool)));

    addSeparator();
	group = new TActionGroup(this, "rotate");
	group->setEnabled(false);
    new TActionGroupItem(this, group, "rotate_none", tr("&No rotation"), 0);
    new TActionGroupItem(this, group, "rotate_90", trUtf8("&Rotate 90° clockwise"), 90, true, true);
    new TActionGroupItem(this, group, "rotate_270", trUtf8("Rotate 90° &counter-clockwise"), 270, true, true);
    group->setChecked(player->mset.rotate);
    connect(group, SIGNAL(activated(int)), player, SLOT(changeRotate(int)));
    // No one changes it

    addActionsTo(main_window);
}

void TMenuTransform::enableActions() {

    // Using mset, so useless to set if stopped or no video
    bool enable = player->statePOP() && player->hasVideo()
                  && player->videoFiltersEnabled();
    flipAct->setEnabled(enable);
    mirrorAct->setEnabled(enable);
    group->setEnabled(enable);
}


void TMenuTransform::onMediaSettingsChanged(Settings::TMediaSettings* mset) {

    flipAct->setChecked(mset->flip);
    mirrorAct->setChecked(mset->mirror);
    group->setChecked(mset->rotate);
}

void TMenuTransform::onAboutToShow() {

    flipAct->setChecked(player->mset.flip);
    mirrorAct->setChecked(player->mset.mirror);
    group->setChecked(player->mset.rotate);
}


class TMenuZoomAndPan : public TMenu {
public:
    explicit TMenuZoomAndPan(TMainWindow* mw);
protected:
    virtual void enableActions();
private:
	QActionGroup* group;
};


TMenuZoomAndPan::TMenuZoomAndPan(TMainWindow* mw)
    : TMenu(mw, mw, "zoom_and_pan_menu", tr("&Zoom and pan"), "zoom_and_pan") {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	// Zoom
    TAction* a = new TAction(this, "reset_zoom_and_pan",
                             tr("&Reset zoom and pan"), "", Qt::Key_5);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(resetZoomAndPan()));

    // Zoom
	addSeparator();
    a = new TAction(this, "dec_zoom", tr("Zoom &-"), "", Qt::Key_1);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(decZoom()));
    a = new TAction(this, "inc_zoom", tr("Zoom &+"), "", Qt::Key_9);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(incZoom()));

	// Pan
	addSeparator();
    a = new TAction(this, "move_left", tr("Move &left"), "", Qt::Key_4);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(panRight()));
    a = new TAction(this, "move_right", tr("Move &right"), "", Qt::Key_6);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(panLeft()));
    a = new TAction(this, "move_up", tr("Move &up"), "", Qt::Key_8);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(panDown()));
    a = new TAction(this, "move_down", tr("Move &down"), "", Qt::Key_2);
	group->addAction(a);
    connect(a, SIGNAL(triggered()), player, SLOT(panUp()));

    addActionsTo(main_window);
}

void TMenuZoomAndPan::enableActions() {
    group->setEnabled(player->statePOP() && player->hasVideo());
}


TMenuVideo::TMenuVideo(TMainWindow* mw,
                       TPlayerWindow* playerwindow,
                       TVideoEqualizer* videoEqualizer) :
        TMenu(mw, mw, "video_menu", tr("&Video"), "noicon") {

    fullscreenAct = new TAction(this, "fullscreen", tr("&Fullscreen"), "",
                                Qt::Key_F);
    //fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable(true);
    connect(fullscreenAct, SIGNAL(triggered(bool)),
            main_window, SLOT(toggleFullscreen(bool)));

    exitFullscreenAct = new TAction(this, "exit_fullscreen",
                                    tr("Exit fullscreen"), "", Qt::Key_Escape,
                                    false);
	exitFullscreenAct->setEnabled(false);
    main_window->addAction(exitFullscreenAct);
    connect(exitFullscreenAct, SIGNAL(triggered()),
            main_window, SLOT(exitFullscreen()));

    connect(main_window, SIGNAL(fullscreenChanged()),
            this, SLOT(onFullscreenChanged()),
            Qt::QueuedConnection);

	// Aspect submenu
	addSeparator();
    addMenu(new TMenuAspect(main_window));
	// Size submenu
    addMenu(new TMenuVideoSize(main_window, playerwindow));
	// Zoom and pan submenu
    addMenu(new TMenuZoomAndPan(main_window));

	// Equalizer
	addSeparator();
    equalizerAct = new TAction(this, "video_equalizer", tr("&Equalizer"), "", QKeySequence("E"));
	equalizerAct->setCheckable(true);
	equalizerAct->setChecked(videoEqualizer->isVisible());
	connect(equalizerAct, SIGNAL(triggered(bool)), videoEqualizer, SLOT(setVisible(bool)));
	connect(videoEqualizer, SIGNAL(visibilityChanged(bool)), equalizerAct, SLOT(setChecked(bool)));

    resetVideoEqualizerAct = new TAction(this, "reset_video_equalizer", tr("Reset video equalizer"), "", QKeySequence("Shift+E"));
	connect(resetVideoEqualizerAct, SIGNAL(triggered()), videoEqualizer, SLOT(reset()));

	// Short cuts equalizer (not in menu)
    decContrastAct = new TAction(this, "dec_contrast", tr("Dec contrast"), "", Qt::ALT | Qt::Key_1, false);
    main_window->addAction(decContrastAct);
    connect(decContrastAct, SIGNAL(triggered()), player, SLOT(decContrast()));

    incContrastAct = new TAction(this, "inc_contrast", tr("Inc contrast"), "", Qt::ALT | Qt::Key_2, false);
    main_window->addAction(incContrastAct);
    connect(incContrastAct, SIGNAL(triggered()), player, SLOT(incContrast()));

    decBrightnessAct = new TAction(this, "dec_brightness", tr("Dec brightness"), "", Qt::ALT | Qt::Key_3, false);
    main_window->addAction(decBrightnessAct);
    connect(decBrightnessAct, SIGNAL(triggered()), player, SLOT(decBrightness()));

    incBrightnessAct = new TAction(this, "inc_brightness", tr("Inc brightness"), "", Qt::ALT | Qt::Key_4, false);
    main_window->addAction(incBrightnessAct);
    connect(incBrightnessAct, SIGNAL(triggered()), player, SLOT(incBrightness()));

    decHueAct = new TAction(this, "dec_hue", tr("Dec hue"), "", Qt::ALT | Qt::Key_5, false);
    main_window->addAction(decHueAct);
    connect(decHueAct, SIGNAL(triggered()), player, SLOT(decHue()));

    incHueAct = new TAction(this, "inc_hue", tr("Inc hue"), "", Qt::ALT | Qt::Key_6, false);
    main_window->addAction(incHueAct);
    connect(incHueAct, SIGNAL(triggered()), player, SLOT(incHue()));

    decSaturationAct = new TAction(this, "dec_saturation", tr("Dec saturation"), "", Qt::ALT | Qt::Key_7, false);
    main_window->addAction(decSaturationAct);
    connect(decSaturationAct, SIGNAL(triggered()), player, SLOT(decSaturation()));

    incSaturationAct = new TAction(this, "inc_saturation", tr("Inc saturation"), "", Qt::ALT | Qt::Key_8, false);
    main_window->addAction(incSaturationAct);
    connect(incSaturationAct, SIGNAL(triggered()), player, SLOT(incSaturation()));

    decGammaAct = new TAction(this, "dec_gamma", tr("Dec gamma"), "", Qt::ALT | Qt::Key_9, false);
    main_window->addAction(decGammaAct);
    connect(decGammaAct, SIGNAL(triggered()), player, SLOT(decGamma()));

    incGammaAct = new TAction(this, "inc_gamma", tr("Inc gamma"), "", Qt::ALT | Qt::Key_0, false);
    main_window->addAction(incGammaAct);
    connect(incGammaAct, SIGNAL(triggered()), player, SLOT(incGamma()));

	// Deinterlace submenu
	addSeparator();
    addMenu(new TMenuDeinterlace(main_window));
    // Transform submenu
    addMenu(new TMenuTransform(main_window));
    // Video filter submenu
    addMenu(new TMenuVideoFilter(main_window));

	// Stereo 3D
    stereo3DAct = new TAction(this, "stereo_3d_filter", tr("Stereo &3D filter..."), "stereo3d");
    connect(stereo3DAct, SIGNAL(triggered()), main_window, SLOT(showStereo3dDialog()));

	// Video tracks
	addSeparator();
    addMenu(new TMenuVideoTracks(main_window));

	// Screenshots
	addSeparator();
	// Single
    screenshotAct = new TAction(this, "screenshot", tr("S&creenshot"), "", Qt::Key_R);
    connect(screenshotAct, SIGNAL(triggered()), player, SLOT(screenshot()));

    // Multiple
    screenshotsAct = new TAction(this, "multiple_screenshots",
        tr("Start/stop takin&g screenshots"), "screenshots", Qt::SHIFT | Qt::Key_R);
    connect(screenshotsAct, SIGNAL(triggered()), player, SLOT(screenshots()));

    // Capture
    capturingAct = new TAction(this, "capture_stream", tr("Start/stop capturing stream"),
        "record", Qt::CTRL | Qt::Key_R);
    connect(capturingAct, SIGNAL(triggered()), player, SLOT(switchCapturing()) );

    addActionsTo(main_window);
}

void TMenuVideo::enableActions() {

    // Depending on mset, so useless to set if no video
    bool enable = player->statePOP() && player->hasVideo();

    equalizerAct->setEnabled(enable);
    resetVideoEqualizerAct->setEnabled(enable);

    decContrastAct->setEnabled(enable);
    incContrastAct->setEnabled(enable);
    decBrightnessAct->setEnabled(enable);
    incBrightnessAct->setEnabled(enable);
    decHueAct->setEnabled(enable);
    incHueAct->setEnabled(enable);
    decSaturationAct->setEnabled(enable);
    incSaturationAct->setEnabled(enable);
    decGammaAct->setEnabled(enable);
    incGammaAct->setEnabled(enable);

    bool enableFilters = enable && player->videoFiltersEnabled();
    stereo3DAct->setEnabled(enableFilters);

    bool enableScreenShots = enable
                             && pref->use_screenshot
                             && !pref->screenshot_directory.isEmpty();
    screenshotAct->setEnabled(enableScreenShots);
    screenshotsAct->setEnabled(enableScreenShots);

    capturingAct->setEnabled(enable && !pref->screenshot_directory.isEmpty());
}

void TMenuVideo::onFullscreenChanged() {

	fullscreenAct->setChecked(pref->fullscreen);
	exitFullscreenAct->setEnabled(pref->fullscreen);
}

} // namespace Menu
} // namespace Action
} // namespace Gui

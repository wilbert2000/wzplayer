#include "gui/action/menuvideo.h"
#include <QDebug>
#include "core.h"
#include "settings/mediasettings.h"
#include "gui/action/menuaspect.h"
#include "gui/action/menuvideofilter.h"
#include "gui/action/menuvideosize.h"
#include "gui/action/menuvideotracks.h"
#include "gui/videoequalizer.h"
#include "gui/base.h"

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {
namespace Action {


class TMenuDeinterlace : public TMenu {
public:
	explicit TMenuDeinterlace(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
	TAction* toggleDeinterlaceAct;
};


TMenuDeinterlace::TMenuDeinterlace(QWidget* parent, TCore* c)
    : TMenu(parent, "deinterlace_menu", tr("&Deinterlace"), "deinterlace")
	, core(c) {

	group = new TActionGroup(this, "deinterlace");
	group->setEnabled(false);
    new TActionGroupItem(this, group, "deinterlace_none", tr("&None"), TMediaSettings::NoDeinterlace);
    new TActionGroupItem(this, group, "deinterlace_l5", tr("&Lowpass5"), TMediaSettings::L5);
    new TActionGroupItem(this, group, "deinterlace_yadif0", tr("&Yadif (normal)"), TMediaSettings::Yadif);
    new TActionGroupItem(this, group, "deinterlace_yadif1", tr("Y&adif (double framerate)"), TMediaSettings::Yadif_1);
    new TActionGroupItem(this, group, "deinterlace_lb", tr("Linear &Blend"), TMediaSettings::LB);
    new TActionGroupItem(this, group, "deinterlace_kern", tr("&Kerndeint"), TMediaSettings::Kerndeint);
	group->setChecked(core->mset.current_deinterlacer);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeDeinterlace(int)));
	// No one else sets it

	addSeparator();
    toggleDeinterlaceAct = new TAction(this, "toggle_deinterlacing", tr("Toggle deinterlacing"), "deinterlace", Qt::Key_D);
    toggleDeinterlaceAct->setCheckable(true);
	connect(toggleDeinterlaceAct, SIGNAL(triggered()), core, SLOT(toggleDeinterlace()));

	addActionsTo(parent);
}

void TMenuDeinterlace::enableActions(bool stopped, bool video, bool) {

	// Using mset, so useless to set if stopped or no video
	bool enabled = !stopped && video && core->videoFiltersEnabled();
	group->setEnabled(enabled);
	toggleDeinterlaceAct->setEnabled(enabled);
}

void TMenuDeinterlace::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->current_deinterlacer);
}

void TMenuDeinterlace::onAboutToShow() {
	group->setChecked(core->mset.current_deinterlacer);
}


class TMenuRotate : public TMenu {
public:
	explicit TMenuRotate(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TMenuRotate::TMenuRotate(QWidget* parent, TCore* c)
    : TMenu(parent, "rotate_menu", tr("&Rotate"), "rotate")
	, core(c) {

	group = new TActionGroup(this, "rotate");
	group->setEnabled(false);
    new TActionGroupItem(this, group, "rotate_none", tr("&Off"), TMediaSettings::NoRotate);
    new TActionGroupItem(this, group, "rotate_cw_flip", tr("&Rotate 90 degrees clockwise and flip"), TMediaSettings::Clockwise_flip);
    new TActionGroupItem(this, group, "rotate_cw", tr("Rotate 90 degrees &clockwise"), TMediaSettings::Clockwise);
    new TActionGroupItem(this, group, "rotate_cc", tr("Rotate 90 degrees counterclock&wise"), TMediaSettings::Counterclockwise);
    new TActionGroupItem(this, group, "rotate_cc_flip", tr("Rotate 90 degrees counterclockwise and &flip"), TMediaSettings::Counterclockwise_flip);
	group->setChecked(core->mset.rotate);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeRotate(int)));
	// No one else changes it
	addActionsTo(parent);
}

void TMenuRotate::enableActions(bool stopped, bool video, bool) {
	// Using mset, so useless to set if stopped or no video
	group->setEnabled(!stopped && video && core->videoFiltersEnabled());
}

void TMenuRotate::onMediaSettingsChanged(Settings::TMediaSettings* mset) {
	group->setChecked(mset->rotate);
}

void TMenuRotate::onAboutToShow() {
	group->setChecked(core->mset.rotate);
}


class TMenuZoomAndPan : public TMenu {
public:
	explicit TMenuZoomAndPan(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
private:
	TCore* core;
	QActionGroup* group;
};


TMenuZoomAndPan::TMenuZoomAndPan(QWidget* parent, TCore* c)
    : TMenu(parent, "zoom_and_pan_menu", tr("&Zoom and pan"), "zoom_and_pan")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	// Zoom
    TAction* a = new TAction(this, "reset_zoom", tr("&Reset"), "zoom_reset", Qt::SHIFT | Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(resetZoomAndPan()));
	addSeparator();
    a = new TAction(this, "auto_zoom", tr("&Auto zoom"), "", Qt::SHIFT | Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoom()));
    a = new TAction(this, "zoom_169", tr("Zoom for &16:9"), "", Qt::SHIFT | Qt::Key_A);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor169()));
    a = new TAction(this, "zoom_235", tr("Zoom for &2.35:1"), "", Qt::SHIFT | Qt::Key_S);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor235()));
	addSeparator();
    a = new TAction(this, "dec_zoom", tr("Zoom &-"), "", Qt::Key_Q);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decZoom()));
    a = new TAction(this, "inc_zoom", tr("Zoom &+"), "", Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incZoom()));

	// Pan
	addSeparator();
    a = new TAction(this, "move_left", tr("Move &left"), "", Qt::ALT | Qt::Key_Left);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panRight()));
    a = new TAction(this, "move_right", tr("Move &right"), "", Qt::ALT | Qt::Key_Right);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panLeft()));
    a = new TAction(this, "move_up", tr("Move &up"), "", Qt::ALT | Qt::Key_Up);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panDown()));
    a = new TAction(this, "move_down", tr("Move &down"), "", Qt::ALT | Qt::Key_Down);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panUp()));

	addActionsTo(parent);
}

void TMenuZoomAndPan::enableActions(bool stopped, bool video, bool) {
	group->setEnabled(!stopped && video);
}


TMenuVideo::TMenuVideo(TBase* parent, TCore* c, TPlayerWindow* playerwindow, TVideoEqualizer* videoEqualizer)
    : TMenu(parent, "video_menu", tr("&Video"), "noicon")
	, core(c) {

    fullscreenAct = new TAction(this, "fullscreen", tr("&Fullscreen"), "", Qt::Key_F);
	fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable(true);
	connect(fullscreenAct, SIGNAL(triggered(bool)), parent, SLOT(toggleFullscreen(bool)));

    exitFullscreenAct = new TAction(this, "exit_fullscreen", tr("Exit fullscreen"), "", Qt::Key_Escape, false);
	exitFullscreenAct->setEnabled(false);
	parent->addAction(exitFullscreenAct);
	connect(exitFullscreenAct, SIGNAL(triggered()), parent, SLOT(exitFullscreen()));

    connect(parent, SIGNAL(fullscreenChanged()),
            this, SLOT(onFullscreenChanged()),
            Qt::QueuedConnection);

	// Aspect submenu
	addSeparator();
	addMenu(new TMenuAspect(parent, core));
	// Size submenu
	addMenu(new TMenuVideoSize(parent, playerwindow));
	// Zoom and pan submenu
	addMenu(new TMenuZoomAndPan(parent, core));

	// Equalizer
	addSeparator();
    equalizerAct = new TAction(this, "video_equalizer", tr("&Equalizer"), "", QKeySequence("E"));
	equalizerAct->setCheckable(true);
	equalizerAct->setChecked(videoEqualizer->isVisible());
	connect(equalizerAct, SIGNAL(triggered(bool)), videoEqualizer, SLOT(setVisible(bool)));
	connect(videoEqualizer, SIGNAL(visibilityChanged(bool)), equalizerAct, SLOT(setChecked(bool)));

    resetVideoEqualizerAct = new TAction(this, "reset_video_equalizer", tr("Reset video equalizer"));
	connect(resetVideoEqualizerAct, SIGNAL(triggered()), videoEqualizer, SLOT(reset()));

	// Short cuts equalizer (not in menu)
    decContrastAct = new TAction(this, "dec_contrast", tr("Dec contrast"), "", Qt::Key_1, false);
	parent->addAction(decContrastAct);
	connect(decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()));

    incContrastAct = new TAction(this, "inc_contrast", tr("Inc contrast"), "", Qt::Key_2, false);
	parent->addAction(incContrastAct);
	connect(incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()));

    decBrightnessAct = new TAction(this, "dec_brightness", tr("Dec brightness"), "", Qt::Key_3, false);
	parent->addAction(decBrightnessAct);
	connect(decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()));

    incBrightnessAct = new TAction(this, "inc_brightness", tr("Inc brightness"), "", Qt::Key_4, false);
	parent->addAction(incBrightnessAct);
	connect(incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()));

    decHueAct = new TAction(this, "dec_hue", tr("Dec hue"), "", Qt::Key_5, false);
	parent->addAction(decHueAct);
	connect(decHueAct, SIGNAL(triggered()), core, SLOT(decHue()));

    incHueAct = new TAction(this, "inc_hue", tr("Inc hue"), "", Qt::Key_6, false);
	parent->addAction(incHueAct);
	connect(incHueAct, SIGNAL(triggered()), core, SLOT(incHue()));

    decSaturationAct = new TAction(this, "dec_saturation", tr("Dec saturation"), "", Qt::Key_7, false);
	parent->addAction(decSaturationAct);
	connect(decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()));

    incSaturationAct = new TAction(this, "inc_saturation", tr("Inc saturation"), "", Qt::Key_8, false);
	parent->addAction(incSaturationAct);
	connect(incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()));

    decGammaAct = new TAction(this, "dec_gamma", tr("Dec gamma"), "", Qt::Key_9, false);
	parent->addAction(decGammaAct);
	connect(decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()));

    incGammaAct = new TAction(this, "inc_gamma", tr("Inc gamma"), "", Qt::Key_0, false);
	parent->addAction(incGammaAct);
	connect(incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()));

	// Deinterlace submenu
	addSeparator();
	addMenu(new TMenuDeinterlace(parent, core));
	// Video filter submenu
	addMenu(new TMenuVideoFilter(parent, core));

	// Stereo 3D
    stereo3DAct = new TAction(this, "stereo_3d_filter", tr("Stereo &3D filter..."), "stereo3d");
	connect(stereo3DAct, SIGNAL(triggered()), parent, SLOT(showStereo3dDialog()));

	// Rotate submenu
	addSeparator();
	addMenu(new TMenuRotate(parent, core));

    flipAct = new TAction(this, "flip", tr("Fli&p image"));
	flipAct->setCheckable(true);
	connect(flipAct, SIGNAL(triggered(bool)), core, SLOT(toggleFlip(bool)));

    mirrorAct = new TAction(this, "mirror", tr("&Mirror image"));
	mirrorAct->setCheckable(true);
	connect(mirrorAct, SIGNAL(triggered(bool)), core, SLOT(toggleMirror(bool)));

	// Video tracks
	addSeparator();
	addMenu(new TMenuVideoTracks(parent, core));

	// Screenshots
	addSeparator();
	// Single
    screenshotAct = new TAction(this, "screenshot", tr("S&creenshot"), "", Qt::Key_S);
	connect(screenshotAct, SIGNAL(triggered()), core, SLOT(screenshot()));
	// Multiple
    screenshotsAct = new TAction(this, "multiple_screenshots", tr("Start/stop takin&g screenshots"), "screenshots", QKeySequence("Shift+D"));
	connect(screenshotsAct, SIGNAL(triggered()), core, SLOT(screenshots()));
	// Capture
    capturingAct = new TAction(this, "capture_stream", tr("Start/stop capturing stream"), "record");
	connect(capturingAct, SIGNAL(triggered()), core, SLOT(switchCapturing()) );

	addActionsTo(parent);
}

void TMenuVideo::enableActions(bool stopped, bool video, bool) {

	// Depending on mset, so useless to set if no video
	bool enableVideo = !stopped && video;

	equalizerAct->setEnabled(enableVideo);
	resetVideoEqualizerAct->setEnabled(enableVideo);

	decContrastAct->setEnabled(enableVideo);
	incContrastAct->setEnabled(enableVideo);
	decBrightnessAct->setEnabled(enableVideo);
	incBrightnessAct->setEnabled(enableVideo);
	decHueAct->setEnabled(enableVideo);
	incHueAct->setEnabled(enableVideo);
	decSaturationAct->setEnabled(enableVideo);
	incSaturationAct->setEnabled(enableVideo);
	decGammaAct->setEnabled(enableVideo);
	incGammaAct->setEnabled(enableVideo);

	bool enableFilters = enableVideo && core->videoFiltersEnabled();
	stereo3DAct->setEnabled(enableFilters);
	flipAct->setEnabled(enableFilters);
	mirrorAct->setEnabled(enableFilters);

	bool enableScreenShots = enableFilters
							 && pref->use_screenshot
							 && !pref->screenshot_directory.isEmpty();
	screenshotAct->setEnabled(enableScreenShots);
	screenshotsAct->setEnabled(enableScreenShots);

	capturingAct->setEnabled(enableVideo
							 && !pref->screenshot_directory.isEmpty());
}

void TMenuVideo::onFullscreenChanged() {

	fullscreenAct->setChecked(pref->fullscreen);
	exitFullscreenAct->setEnabled(pref->fullscreen);
}

void TMenuVideo::onMediaSettingsChanged(Settings::TMediaSettings* mset) {

	flipAct->setChecked(mset->flip);
	mirrorAct->setChecked(mset->mirror);
}

} // namespace Action
} // namespace Gui

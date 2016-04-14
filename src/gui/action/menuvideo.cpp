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
	: TMenu(parent, this, "deinterlace_menu", QT_TR_NOOP("&Deinterlace"), "deinterlace")
	, core(c) {

	group = new TActionGroup(this, "deinterlace");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "deinterlace_none", QT_TR_NOOP("&None"), TMediaSettings::NoDeinterlace);
	new TActionGroupItem(this, group, "deinterlace_l5", QT_TR_NOOP("&Lowpass5"), TMediaSettings::L5);
	new TActionGroupItem(this, group, "deinterlace_yadif0", QT_TR_NOOP("&Yadif (normal)"), TMediaSettings::Yadif);
	new TActionGroupItem(this, group, "deinterlace_yadif1", QT_TR_NOOP("Y&adif (double framerate)"), TMediaSettings::Yadif_1);
	new TActionGroupItem(this, group, "deinterlace_lb", QT_TR_NOOP("Linear &Blend"), TMediaSettings::LB);
	new TActionGroupItem(this, group, "deinterlace_kern", QT_TR_NOOP("&Kerndeint"), TMediaSettings::Kerndeint);
	group->setChecked(core->mset.current_deinterlacer);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeDeinterlace(int)));
	// No one else sets it

	addSeparator();
	toggleDeinterlaceAct = new TAction(this, "toggle_deinterlacing", QT_TR_NOOP("Toggle deinterlacing"), "", Qt::Key_D);
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
	: TMenu(parent, this, "rotate_menu", QT_TR_NOOP("&Rotate"), "rotate")
	, core(c) {

	group = new TActionGroup(this, "rotate");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "rotate_none", QT_TR_NOOP("&Off"), TMediaSettings::NoRotate);
	new TActionGroupItem(this, group, "rotate_clockwise_flip", QT_TR_NOOP("&Rotate 90 degrees clockwise and flip"), TMediaSettings::Clockwise_flip);
	new TActionGroupItem(this, group, "rotate_clockwise", QT_TR_NOOP("Rotate 90 degrees &clockwise"), TMediaSettings::Clockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise", QT_TR_NOOP("Rotate 90 degrees counterclock&wise"), TMediaSettings::Counterclockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise_flip", QT_TR_NOOP("Rotate 90 degrees counterclockwise and &flip"), TMediaSettings::Counterclockwise_flip);
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
	: TMenu(parent, this, "zoom_and_pan_menu", QT_TR_NOOP("&Zoom and pan"), "zoom_and_pan")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	// Zoom
	TAction* a = new TAction(this, "reset_zoom", QT_TR_NOOP("&Reset"), "zoom_reset", Qt::SHIFT | Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(resetZoomAndPan()));
	addSeparator();
	a = new TAction(this, "auto_zoom", QT_TR_NOOP("&Auto zoom"), "", Qt::SHIFT | Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoom()));
	a = new TAction(this, "zoom_169", QT_TR_NOOP("Zoom for &16:9"), "", Qt::SHIFT | Qt::Key_A);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor169()));
	a = new TAction(this, "zoom_235", QT_TR_NOOP("Zoom for &2.35:1"), "", Qt::SHIFT | Qt::Key_S);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor235()));
	addSeparator();
	a = new TAction(this, "dec_zoom", QT_TR_NOOP("Zoom &-"), "", Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decZoom()));
	a = new TAction(this, "inc_zoom", QT_TR_NOOP("Zoom &+"), "", Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incZoom()));

	// Pan
	addSeparator();
	a = new TAction(this, "move_left", QT_TR_NOOP("Move &left"), "", Qt::ALT | Qt::Key_Left);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panRight()));
	a = new TAction(this, "move_right", QT_TR_NOOP("Move &right"), "", Qt::ALT | Qt::Key_Right);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panLeft()));
	a = new TAction(this, "move_up", QT_TR_NOOP("Move &up"), "", Qt::ALT | Qt::Key_Up);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panDown()));
	a = new TAction(this, "move_down", QT_TR_NOOP("Move &down"), "", Qt::ALT | Qt::Key_Down);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panUp()));

	addActionsTo(parent);
}

void TMenuZoomAndPan::enableActions(bool stopped, bool video, bool) {
	group->setEnabled(!stopped && video);
}


TMenuVideo::TMenuVideo(TBase* parent, TCore* c, TPlayerWindow* playerwindow, TVideoEqualizer* videoEqualizer)
	: TMenu(parent, this, "video_menu", QT_TR_NOOP("&Video"), "noicon")
	, core(c) {

	fullscreenAct = new TAction(this, "fullscreen", QT_TR_NOOP("&Fullscreen"), "", Qt::Key_F);
	fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable(true);
	connect(fullscreenAct, SIGNAL(triggered(bool)), parent, SLOT(toggleFullscreen(bool)));

	exitFullscreenAct = new TAction(this, "exit_fullscreen", QT_TR_NOOP("Exit fullscreen"), "", Qt::Key_Escape, false);
	exitFullscreenAct->setEnabled(false);
	parent->addAction(exitFullscreenAct);
	connect(exitFullscreenAct, SIGNAL(triggered()), parent, SLOT(exitFullscreen()));

	connect(parent, SIGNAL(fullscreenChanged()), this, SLOT(onFullscreenChanged()));

	// Aspect submenu
	addSeparator();
	addMenu(new TMenuAspect(parent, core));
	// Size submenu
	addMenu(new TMenuVideoSize(parent, playerwindow));
	// Zoom and pan submenu
	addMenu(new TMenuZoomAndPan(parent, core));

	// Equalizer
	addSeparator();
	equalizerAct = new TAction(this, "video_equalizer", QT_TR_NOOP("&Equalizer"), "equalizer", QKeySequence("Ctrl+E"));
	equalizerAct->setCheckable(true);
	equalizerAct->setChecked(videoEqualizer->isVisible());
	connect(equalizerAct, SIGNAL(triggered(bool)), videoEqualizer, SLOT(setVisible(bool)));
	connect(videoEqualizer, SIGNAL(visibilityChanged(bool)), equalizerAct, SLOT(setChecked(bool)));

	resetVideoEqualizerAct = new TAction(this, "reset_video_equalizer", QT_TR_NOOP("Reset video equalizer"));
	connect(resetVideoEqualizerAct, SIGNAL(triggered()), videoEqualizer, SLOT(reset()));

	// Short cuts equalizer (not in menu)
	decContrastAct = new TAction(this, "dec_contrast", QT_TR_NOOP("Dec contrast"), "", Qt::Key_1, false);
	parent->addAction(decContrastAct);
	connect(decContrastAct, SIGNAL(triggered()), core, SLOT(decContrast()));

	incContrastAct = new TAction(this, "inc_contrast", QT_TR_NOOP("Inc contrast"), "", Qt::Key_2, false);
	parent->addAction(incContrastAct);
	connect(incContrastAct, SIGNAL(triggered()), core, SLOT(incContrast()));

	decBrightnessAct = new TAction(this, "dec_brightness", QT_TR_NOOP("Dec brightness"), "", Qt::Key_3, false);
	parent->addAction(decBrightnessAct);
	connect(decBrightnessAct, SIGNAL(triggered()), core, SLOT(decBrightness()));

	incBrightnessAct = new TAction(this, "inc_brightness", QT_TR_NOOP("Inc brightness"), "", Qt::Key_4, false);
	parent->addAction(incBrightnessAct);
	connect(incBrightnessAct, SIGNAL(triggered()), core, SLOT(incBrightness()));

	decHueAct = new TAction(this, "dec_hue", QT_TR_NOOP("Dec hue"), "", Qt::Key_5, false);
	parent->addAction(decHueAct);
	connect(decHueAct, SIGNAL(triggered()), core, SLOT(decHue()));

	incHueAct = new TAction(this, "inc_hue", QT_TR_NOOP("Inc hue"), "", Qt::Key_6, false);
	parent->addAction(incHueAct);
	connect(incHueAct, SIGNAL(triggered()), core, SLOT(incHue()));

	decSaturationAct = new TAction(this, "dec_saturation", QT_TR_NOOP("Dec saturation"), "", Qt::Key_7, false);
	parent->addAction(decSaturationAct);
	connect(decSaturationAct, SIGNAL(triggered()), core, SLOT(decSaturation()));

	incSaturationAct = new TAction(this, "inc_saturation", QT_TR_NOOP("Inc saturation"), "", Qt::Key_8, false);
	parent->addAction(incSaturationAct);
	connect(incSaturationAct, SIGNAL(triggered()), core, SLOT(incSaturation()));

	decGammaAct = new TAction(this, "dec_gamma", QT_TR_NOOP("Dec gamma"), "", Qt::Key_9, false);
	parent->addAction(decGammaAct);
	connect(decGammaAct, SIGNAL(triggered()), core, SLOT(decGamma()));

	incGammaAct = new TAction(this, "inc_gamma", QT_TR_NOOP("Inc gamma"), "", Qt::Key_0, false);
	parent->addAction(incGammaAct);
	connect(incGammaAct, SIGNAL(triggered()), core, SLOT(incGamma()));

	// Deinterlace submenu
	addSeparator();
	addMenu(new TMenuDeinterlace(parent, core));
	// Video filter submenu
	addMenu(new TMenuVideoFilter(parent, core));

	// Stereo 3D
	stereo3DAct = new TAction(this, "stereo_3d_filter", QT_TR_NOOP("Stereo &3D filter..."), "stereo3d");
	connect(stereo3DAct, SIGNAL(triggered()), parent, SLOT(showStereo3dDialog()));

	// Rotate submenu
	addSeparator();
	addMenu(new TMenuRotate(parent, core));

	flipAct = new TAction(this, "flip", QT_TR_NOOP("Fli&p image"));
	flipAct->setCheckable(true);
	connect(flipAct, SIGNAL(triggered(bool)), core, SLOT(toggleFlip(bool)));

	mirrorAct = new TAction(this, "mirror", QT_TR_NOOP("&Mirror image"));
	mirrorAct->setCheckable(true);
	connect(mirrorAct, SIGNAL(triggered(bool)), core, SLOT(toggleMirror(bool)));

	// Video tracks
	addSeparator();
	addMenu(new TMenuVideoTracks(parent, core));

	// Screenshots
	addSeparator();
	// Single
	screenshotAct = new TAction(this, "screenshot", QT_TR_NOOP("S&creenshot"), "", Qt::Key_S);
	connect(screenshotAct, SIGNAL(triggered()), core, SLOT(screenshot()));
	// Multiple
	screenshotsAct = new TAction(this, "multiple_screenshots", QT_TR_NOOP("Start/stop takin&g screenshots"), "screenshots", QKeySequence("Shift+D"));
	connect(screenshotsAct, SIGNAL(triggered()), core, SLOT(screenshots()));
	// Capture
	capturingAct = new TAction(this, "capture_stream", QT_TR_NOOP("Start/stop capturing stream"), "record");
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

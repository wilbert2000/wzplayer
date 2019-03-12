#include "gui/action/menu/menuvideo.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuvideosize.h"
#include "gui/action/menu/menuvideotracks.h"
#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/videoequalizer.h"
#include "gui/mainwindow.h"

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TMenuAspect::TMenuAspect(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, mw, "aspect_menu", tr("Aspect ratio")) {

    TActionGroup* group = mw->findChild<TActionGroup*>("aspectgroup");
    addActions(group->actions());
    insertSeparator(mw->findAction("aspect_1_1"));
    insertSeparator(mw->findAction("aspect_none"));
    addSeparator();
    addAction(mw->findAction("aspect_next"));

    connect(mw, &TMainWindow::setAspectMenuToolTip,
            this, &TMenuAspect::setAspectMenuToolTip);
}

void TMenuAspect::onAboutToShow() {
    main_window->updateAspectMenu();
}

void TMenuAspect::setAspectMenuToolTip(const QString &tip) {
    menuAction()->setToolTip(tip);
}


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
    : TMenu(mw, mw, "deinterlace_menu", tr("Deinterlace"), "deinterlace") {

    group = new TActionGroup(mw, "deinterlace");
    group->setEnabled(false);
    new TActionGroupItem(mw, group, "deinterlace_none", tr("None"),
                         TMediaSettings::NoDeinterlace);
    new TActionGroupItem(mw, group, "deinterlace_l5", tr("Lowpass5"),
                         TMediaSettings::L5);
    new TActionGroupItem(mw, group, "deinterlace_yadif0",
                         tr("Yadif (normal)"), TMediaSettings::Yadif);
    new TActionGroupItem(mw, group, "deinterlace_yadif1",
                         tr("Yadif (double framerate)"),
                         TMediaSettings::Yadif_1);
    new TActionGroupItem(mw, group, "deinterlace_lb", tr("Linear Blend"),
                         TMediaSettings::LB);
    new TActionGroupItem(mw, group, "deinterlace_kern", tr("Kerndeint"),
                         TMediaSettings::Kerndeint);
    group->setChecked(player->mset.current_deinterlacer);
    addActions(group->actions());
    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setDeinterlace);
    // No one else sets it

    addSeparator();
    toggleDeinterlaceAct = new TAction(mw, "toggle_deinterlacing",
                                       tr("Toggle deinterlacing"),
                                       "deinterlace", Qt::Key_I);
    toggleDeinterlaceAct->setCheckable(true);
    addAction(toggleDeinterlaceAct);
    connect(toggleDeinterlaceAct, &TAction::triggered,
            player, &Player::TPlayer::toggleDeinterlace);
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
    : TMenu(mw, mw, "transform_menu", tr("Transform"), "transform") {

    flipAct = new TAction(mw, "flip", tr("Flip image"));
    flipAct->setCheckable(true);
    connect(flipAct, &TAction::triggered,
            player, &Player::TPlayer::setFlip);
    addAction(flipAct);

    mirrorAct = new TAction(mw, "mirror", tr("Mirror image"));
    mirrorAct->setCheckable(true);
    connect(mirrorAct, &TAction::triggered,
            player, &Player::TPlayer::setMirror);
    addAction(mirrorAct);

    addSeparator();
    group = new TActionGroup(mw, "rotate");
    group->setEnabled(false);
    new TActionGroupItem(mw, group, "rotate_none", tr("No rotation"), 0);
    new TActionGroupItem(mw, group, "rotate_90",
                         trUtf8("Rotate 90° clockwise"), 90, true, true);
    new TActionGroupItem(mw, group, "rotate_270",
                         trUtf8("Rotate 90° counter-clockwise"), 270, true,
                         true);
    group->setChecked(player->mset.rotate);
    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setRotate);
    addActions(group->actions());
    // No one changes it
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
    : TMenu(mw, mw, "zoom_and_pan_menu", tr("Zoom and pan"), "zoom_and_pan") {

    group = new QActionGroup(this);
    group->setExclusive(false);
    group->setEnabled(false);

    // Zoom
    TAction* a = new TAction(mw, "reset_zoom_pan", tr("Reset zoom and pan"),
                             "", Qt::Key_5);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::resetZoomAndPan);

    // Zoom
    addSeparator();
    a = new TAction(mw, "dec_zoom", tr("Zoom -"), "", Qt::Key_1);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decZoom);
    a = new TAction(mw, "inc_zoom", tr("Zoom +"), "", Qt::Key_9);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incZoom);

    // Pan
    addSeparator();
    a = new TAction(mw, "move_left", tr("Move left"), "", Qt::Key_4);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panRight);
    a = new TAction(mw, "move_right", tr("Move right"), "", Qt::Key_6);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panLeft);
    a = new TAction(mw, "move_up", tr("Move up"), "", Qt::Key_8);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panDown);
    a = new TAction(mw, "move_down", tr("Move down"), "", Qt::Key_2);
    addAction(a);
    group->addAction(a);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panUp);
}

void TMenuZoomAndPan::enableActions() {
    group->setEnabled(player->statePOP() && player->hasVideo());
}


TMenuVideo::TMenuVideo(TMainWindow* mw,
                       TPlayerWindow* playerwindow,
                       TVideoEqualizer* videoEqualizer) :
        TMenu(mw, mw, "video_menu", tr("Video"), "noicon") {

    addAction(mw->findAction("fullscreen"));

    // Aspect submenu
    addSeparator();
    addMenu(new TMenuAspect(this, mw));
    // Size submenu
    addMenu(new TMenuVideoSize(mw, playerwindow));
    // Zoom and pan submenu
    addMenu(new TMenuZoomAndPan(mw));

    // Equalizer
    addSeparator();
    equalizerAct = new TAction(mw, "video_equalizer", tr("Equalizer"), "",
                               QKeySequence("E"));
    equalizerAct->setCheckable(true);
    equalizerAct->setChecked(videoEqualizer->isVisible());
    addAction(equalizerAct);
    connect(equalizerAct, &TAction::triggered,
            videoEqualizer, &TVideoEqualizer::setVisible);
    connect(videoEqualizer, &TVideoEqualizer::visibilityChanged,
            equalizerAct, &TAction::setChecked);

    resetVideoEqualizerAct = new TAction(mw, "reset_video_equalizer",
                                         tr("Reset video equalizer"), "",
                                         QKeySequence("Shift+E"));
    addAction(resetVideoEqualizerAct);
    connect(resetVideoEqualizerAct, &TAction::triggered,
            videoEqualizer, &TVideoEqualizer::reset);

    // Short cuts equalizer (not in menu)
    decContrastAct = new TAction(mw, "dec_contrast", tr("Dec contrast"), "",
                                 Qt::ALT | Qt::Key_1);
    connect(decContrastAct, &TAction::triggered,
            player, &Player::TPlayer::decContrast);

    incContrastAct = new TAction(mw, "inc_contrast", tr("Inc contrast"), "",
                                 Qt::ALT | Qt::Key_2);
    connect(incContrastAct, &TAction::triggered,
            player, &Player::TPlayer::incContrast);

    decBrightnessAct = new TAction(mw, "dec_brightness", tr("Dec brightness"),
                                   "", Qt::ALT | Qt::Key_3);
    connect(decBrightnessAct, &TAction::triggered,
            player, &Player::TPlayer::decBrightness);

    incBrightnessAct = new TAction(mw, "inc_brightness", tr("Inc brightness"),
                                   "", Qt::ALT | Qt::Key_4);
    connect(incBrightnessAct, &TAction::triggered,
            player, &Player::TPlayer::incBrightness);

    decHueAct = new TAction(mw, "dec_hue", tr("Dec hue"), "",
                            Qt::ALT | Qt::Key_5);
    connect(decHueAct, &TAction::triggered, player, &Player::TPlayer::decHue);

    incHueAct = new TAction(mw, "inc_hue", tr("Inc hue"), "",
                            Qt::ALT | Qt::Key_6);
    connect(incHueAct, &TAction::triggered, player, &Player::TPlayer::incHue);

    decSaturationAct = new TAction(mw, "dec_saturation", tr("Dec saturation"),
                                   "", Qt::ALT | Qt::Key_7);
    connect(decSaturationAct, &TAction::triggered,
            player, &Player::TPlayer::decSaturation);

    incSaturationAct = new TAction(mw, "inc_saturation", tr("Inc saturation"),
                                   "", Qt::ALT | Qt::Key_8);
    connect(incSaturationAct, &TAction::triggered,
            player, &Player::TPlayer::incSaturation);

    decGammaAct = new TAction(mw, "dec_gamma", tr("Dec gamma"), "",
                              Qt::ALT | Qt::Key_9);
    connect(decGammaAct, &TAction::triggered,
            player, &Player::TPlayer::decGamma);

    incGammaAct = new TAction(mw, "inc_gamma", tr("Inc gamma"), "",
                              Qt::ALT | Qt::Key_0);
    connect(incGammaAct, &TAction::triggered,
            player, &Player::TPlayer::incGamma);

    // Color space
    addMenu(new TMenuVideoColorSpace(mw));

    // Deinterlace submenu
    addSeparator();
    addMenu(new TMenuDeinterlace(mw));
    // Transform submenu
    addMenu(new TMenuTransform(mw));
    // Video filter submenu
    addMenu(new TMenuVideoFilter(mw));

    // Stereo 3D
    stereo3DAct = new TAction(mw, "stereo_3d_filter", tr("Stereo 3D filter..."),
                              "stereo3d");
    addAction(stereo3DAct);
    connect(stereo3DAct, &TAction::triggered,
            mw, &TMainWindow::showStereo3dDialog);

    // Video tracks
    addSeparator();
    addMenu(new TMenuVideoTracks(mw));

    // Screenshots
    addSeparator();
    // Single
    screenshotAct = new TAction(mw, "screenshot", tr("Screenshot"), "",
                                Qt::Key_R);
    addAction(screenshotAct);
    connect(screenshotAct, &TAction::triggered,
            player, &Player::TPlayer::screenshot);

    // Multiple
    screenshotsAct = new TAction(mw, "multiple_screenshots",
        tr("Start screenshots"), "screenshots", Qt::SHIFT | Qt::Key_R);
    screenshotsAct->setCheckable(true);
    screenshotsAct->setChecked(false);
    addAction(screenshotsAct);
    connect(screenshotsAct, &TAction::triggered,
            this, &TMenuVideo::startStopScreenshots);

    // Capture
    capturingAct = new TAction(mw, "capture_stream",
                               tr("Start capture"),
                               "record", Qt::CTRL | Qt::Key_R);
    capturingAct->setCheckable(true);
    capturingAct->setChecked(false);
    addAction(capturingAct);
    connect(capturingAct, &TAction::triggered,
            this, &TMenuVideo::startStopCapture);
}

void TMenuVideo::startStopScreenshots() {

    if (screenshotAct->isChecked()) {
        screenshotsAct->setText(tr("Start screenshots"));
    } else {
        screenshotsAct->setText(tr("Stop screenshots"));
    }
    player->screenshots();
}

void TMenuVideo::startStopCapture() {

    if (capturingAct->isChecked()) {
        capturingAct->setText(tr("Start capture"));
    } else {
        capturingAct->setText(tr("Stop capture"));
    }
    player->switchCapturing();
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

} // namespace Menu
} // namespace Action
} // namespace Gui

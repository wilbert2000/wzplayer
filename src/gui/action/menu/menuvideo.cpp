#include "gui/action/menu/menuvideo.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuwindowsize.h"
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

    connect(mw, &TMainWindow::setAspectToolTip,
            this, &TMenuAspect::setAspectToolTip);
}

void TMenuAspect::onAboutToShow() {
    main_window->updateAspectMenu();
}

void TMenuAspect::setAspectToolTip(QString tip) {

    QString s = menuAction()->shortcut().toString();
    if (!s.isEmpty()) {
        tip += " (" + s + ")";
    }
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


TZoomAndPanGroup::TZoomAndPanGroup(TMainWindow* mw)
    : QActionGroup(mw) {

    setObjectName("zoomandpangroup");
    setExclusive(false);
    setEnabled(false);

    // Zoom
    TAction* a = new TAction(mw, "reset_zoom_pan", tr("Reset zoom and pan"),
                             "", Qt::Key_5);
    connect(a, &TAction::triggered, player, &Player::TPlayer::resetZoomAndPan);
    addAction(a);

    // Zoom
    a = new TAction(mw, "dec_zoom", tr("Zoom -"), "", Qt::Key_1);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decZoom);
    addAction(a);

    a = new TAction(mw, "inc_zoom", tr("Zoom +"), "", Qt::Key_9);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incZoom);
    addAction(a);

    // Pan
    a = new TAction(mw, "move_left", tr("Move left"), "", Qt::Key_4);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panRight);
    addAction(a);

    a = new TAction(mw, "move_right", tr("Move right"), "", Qt::Key_6);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panLeft);
    addAction(a);

    a = new TAction(mw, "move_up", tr("Move up"), "", Qt::Key_8);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panDown);
    addAction(a);

    a = new TAction(mw, "move_down", tr("Move down"), "", Qt::Key_2);
    connect(a, &TAction::triggered, player, &Player::TPlayer::panUp);
    addAction(a);
}


class TMenuZoomAndPan : public TMenu {
public:
    explicit TMenuZoomAndPan(QWidget* parent, TMainWindow* mw);
};

TMenuZoomAndPan::TMenuZoomAndPan(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "zoom_and_pan_menu", tr("Zoom and pan")) {

    TZoomAndPanGroup* group = mw->findChild<TZoomAndPanGroup*>("zoomandpangroup");
    addActions(group->actions());
    insertSeparator(mw->findAction("dec_zoom"));
    insertSeparator(mw->findAction("move_left"));
}


TMenuVideo::TMenuVideo(TMainWindow* mw) :
        TMenu(mw, mw, "video_menu", tr("Video"), "noicon") {

    addAction(mw->findAction("fullscreen"));

    addSeparator();
    // Aspect submenu
    addMenu(new TMenuAspect(this, mw));
    // Size submenu
    addMenu(new TMenuWindowSize(this, mw));
    // Zoom and pan submenu
    addMenu(new TMenuZoomAndPan(this, mw));

    addSeparator();
    // Equalizer
    addAction(mw->findAction("video_equalizer"));
    addAction(mw->findAction("reset_video_equalizer"));
    // Color space
    addMenu(new TMenuVideoColorSpace(this, mw));

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

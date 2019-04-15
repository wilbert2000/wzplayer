#include "gui/action/menu/menuvideo.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuwindowsize.h"
#include "gui/action/menu/menuvideocolorspace.h"
#include "gui/videoequalizer.h"
#include "gui/mainwindow.h"
#include "iconprovider.h"

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

TAspectGroup::TAspectGroup(TMainWindow *mw) :
    TActionGroup(mw, "aspect_group") {

    setEnabled(false);
    aspectAutoAct = new TActionGroupItem(mw, this, "aspect_detect",
                                         tr("Auto"), TAspectRatio::AspectAuto);

    new TActionGroupItem(mw, this, "aspect_1_1",
        TAspectRatio::aspectIDToString(0), TAspectRatio::Aspect11);
    new TActionGroupItem(mw, this, "aspect_5_4",
        TAspectRatio::aspectIDToString(1), TAspectRatio::Aspect54);
    new TActionGroupItem(mw, this, "aspect_4_3",
        TAspectRatio::aspectIDToString(2), TAspectRatio::Aspect43);
    new TActionGroupItem(mw, this, "aspect_11_8",
        TAspectRatio::aspectIDToString(3), TAspectRatio::Aspect118);
    new TActionGroupItem(mw, this, "aspect_14_10",
        TAspectRatio::aspectIDToString(4), TAspectRatio::Aspect1410);
    new TActionGroupItem(mw, this, "aspect_3_2",
        TAspectRatio::aspectIDToString(5), TAspectRatio::Aspect32);
    new TActionGroupItem(mw, this, "aspect_14_9",
        TAspectRatio::aspectIDToString(6), TAspectRatio::Aspect149);
    new TActionGroupItem(mw, this, "aspect_16_10",
        TAspectRatio::aspectIDToString(7), TAspectRatio::Aspect1610);
    new TActionGroupItem(mw, this, "aspect_16_9",
        TAspectRatio::aspectIDToString(8), TAspectRatio::Aspect169);
    new TActionGroupItem(mw, this, "aspect_2_1",
        TAspectRatio::aspectIDToString(9), TAspectRatio::Aspect2);
    new TActionGroupItem(mw, this, "aspect_2.35_1",
        TAspectRatio::aspectIDToString(10), TAspectRatio::Aspect235);

    aspectDisabledAct = new TActionGroupItem(mw, this, "aspect_none",
        tr("Disabled"), TAspectRatio::AspectNone);

    nextAspectAct = new TAction(mw, "aspect_next", tr("Next aspect ratio"), "",
                                Qt::Key_A);
    nextAspectAct->setEnabled(false);
    connect(nextAspectAct, &TAction::triggered,
            player, &Player::TPlayer::nextAspectRatio);

    connect(this, &TActionGroup::activated,
            player, &Player::TPlayer::setAspectRatio);
    connect(player, &Player::TPlayer::aspectRatioChanged,
            this, &TAspectGroup::update,
            Qt::QueuedConnection);
    connect(player, &Player::TPlayer::mediaSettingsChanged,
            this, &TAspectGroup::update);
    connect(player, &Player::TPlayer::stateChanged,
            this, &TAspectGroup::onPlayerStateChanged);
}

void TAspectGroup::update() {

    setChecked(player->mset.aspect_ratio.ID());

    // Update menu action tip
    double aspect = player->mset.aspectToDouble();
    QString s = TAspectRatio::doubleToString(aspect);
    emit setAspectToolTip(tr("Aspect ratio %1").arg(s));

    // Set aspect auto text
    s = tr("Auto") + "\t"
        + TAspectRatio::doubleToString(player->mdat.video_aspect_original);
    aspectAutoAct->setTextAndTip(s);

    // Set aspect disabled text
    s = tr("Disabled") + "\t" + TAspectRatio::doubleToString(
            (double) player->mdat.video_width / player->mdat.video_height);
    aspectDisabledAct->setTextAndTip(s);
}

void TAspectGroup::onPlayerStateChanged() {

    bool enable = player->statePOP() && player->hasVideo();
    setEnabled(enable);
    nextAspectAct->setEnabled(enable);
    update();
}

TMenuAspect::TMenuAspect(QWidget* parent, TMainWindow* mw) :
    TMenu(parent, "aspect_menu", tr("Aspect ratio")) {

    TAspectGroup* group = mw->findChild<TAspectGroup*>();
    addActions(group->actions());
    insertSeparator(mw->requireAction("aspect_1_1"));
    insertSeparator(mw->requireAction("aspect_none"));
    addSeparator();
    addAction(mw->requireAction("aspect_next"));

    connect(group, &TAspectGroup::setAspectToolTip,
            this, &TMenuAspect::setAspectToolTip);
    connect(this, &TMenuAspect::aboutToShow,
            group, &TAspectGroup::update);
}

void TMenuAspect::setAspectToolTip(QString tip) {

    QString s = menuAction()->shortcut().toString();
    if (!s.isEmpty()) {
        tip += " (" + s + ")";
    }
    menuAction()->setToolTip(tip);
}


TDeinterlaceGroup::TDeinterlaceGroup(TMainWindow* mw)
    : TActionGroup(mw, "deinterlace_group") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "deinterlace_none", tr("None"),
                         TMediaSettings::NoDeinterlace);
    new TActionGroupItem(mw, this, "deinterlace_l5", tr("Lowpass5"),
                         TMediaSettings::L5);
    new TActionGroupItem(mw, this, "deinterlace_yadif0",
                         tr("Yadif (normal)"), TMediaSettings::Yadif);
    new TActionGroupItem(mw, this, "deinterlace_yadif1",
                         tr("Yadif (double framerate)"),
                         TMediaSettings::Yadif_1);
    new TActionGroupItem(mw, this, "deinterlace_lb", tr("Linear Blend"),
                         TMediaSettings::LB);
    new TActionGroupItem(mw, this, "deinterlace_kern", tr("Kerndeint"),
                         TMediaSettings::Kerndeint);
}

class TMenuDeinterlace : public TMenu {
public:
    explicit TMenuDeinterlace(QWidget* parent, TMainWindow* mw);
};


TMenuDeinterlace::TMenuDeinterlace(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "deinterlace_menu", tr("Deinterlace"), "deinterlace") {

    TDeinterlaceGroup* group = mw->findChild<TDeinterlaceGroup*>(
                "deinterlace_group");
    addActions(group->actions());
    addSeparator();
    addAction(mw->requireAction("toggle_deinterlacing"));
}

TRotateGroup::TRotateGroup(TMainWindow* mw) :
    TActionGroup(mw, "rotate_group") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "rotate_none", tr("No rotation"), 0);
    new TActionGroupItem(mw, this, "rotate_90",
                         trUtf8("Rotate 90° clockwise"), 90, true, true);
    new TActionGroupItem(mw, this, "rotate_270",
                         trUtf8("Rotate 90° counter-clockwise"), 270, true,
                         true);
}

class TMenuTransform : public TMenu {
public:
    explicit TMenuTransform(QWidget* parent, TMainWindow* mw);
};


TMenuTransform::TMenuTransform(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "transform_menu", tr("Transform"), "transform") {

    addAction(mw->requireAction("flip"));
    addAction(mw->requireAction("mirror"));
    addSeparator();
    TRotateGroup* group = mw->findChild<TRotateGroup*>("rotate_group");
    addActions(group->actions());

    connect(this, &TMenuTransform::aboutToShow,
            mw, &TMainWindow::updateTransformMenu);
}


TZoomAndPanGroup::TZoomAndPanGroup(TMainWindow* mw)
    : QActionGroup(mw) {

    setObjectName("zoom_and_pan_group");
    setExclusive(false);
    setEnabled(false);

    // Reset zoom
    TAction* a = new TAction(mw, "reset_zoom_pan", tr("Reset zoom and pan"),
                             "noicon", Qt::Key_5);
    a->setIcon(iconProvider.zoomResetIcon);
    connect(a, &TAction::triggered, player, &Player::TPlayer::resetZoomAndPan);
    addAction(a);

    // Zoom in
    a = new TAction(mw, "inc_zoom", tr("Zoom in"), "noicon", Qt::Key_9);
    a->setIcon(iconProvider.zoomInIcon);
    connect(a, &TAction::triggered, player, &Player::TPlayer::incZoom);
    addAction(a);

    // Zoom out
    a = new TAction(mw, "dec_zoom", tr("Zoom out"), "noicon", Qt::Key_1);
    a->setIcon(iconProvider.zoomOutIcon);
    connect(a, &TAction::triggered, player, &Player::TPlayer::decZoom);
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
    : TMenu(parent, "zoom_and_pan_menu", tr("Zoom and pan")) {

    TZoomAndPanGroup* group = mw->findChild<TZoomAndPanGroup*>(
                "zoom_and_pan_group");
    addActions(group->actions());
    insertSeparator(mw->requireAction("inc_zoom"));
    insertSeparator(mw->requireAction("move_left"));
}


TMenuVideoTracks::TMenuVideoTracks(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, "video_track_menu", tr("Video track")) {

    addAction(mw->requireAction("next_video_track"));
    addSeparator();
    connect(mw, &TMainWindow::videoTrackGroupChanged,
            this, &TMenuVideoTracks::updateVideoTracks);
}

void TMenuVideoTracks::updateVideoTracks(TActionGroup* group) {
    addActions(group->actions());
}


TMenuVideo::TMenuVideo(QWidget* parent, TMainWindow* mw) :
        TMenu(parent, "video_menu", tr("Video"), "noicon") {

    addAction(mw->requireAction("fullscreen"));

    addSeparator();
    // Aspect submenu
    addMenu(new TMenuAspect(this, mw));
    // Size submenu
    addMenu(new TMenuWindowSize(this, mw));
    // Zoom and pan submenu
    addMenu(new TMenuZoomAndPan(this, mw));

    addSeparator();
    // Equalizer
    addAction(mw->requireAction("video_equalizer"));
    addAction(mw->requireAction("reset_video_equalizer"));
    // Color space
    addMenu(new TMenuVideoColorSpace(this, mw));

    addSeparator();
    // Deinterlace submenu
    addMenu(new TMenuDeinterlace(this, mw));
    // Transform submenu
    addMenu(new TMenuTransform(this, mw));
    // Video filter submenu
    addMenu(new TMenuVideoFilter(this, mw));
    // Stereo 3D
    addAction(mw->requireAction("stereo_3d_filter"));

    addSeparator();
    // Video tracks
    addMenu(new TMenuVideoTracks(this, mw));

    addSeparator();
    // Screenshots
    addAction(mw->requireAction("screenshot"));
    addAction(mw->requireAction("screenshots"));
    addAction(mw->requireAction("capture_stream"));
}

} // namespace Menu
} // namespace Action
} // namespace Gui

#include "gui/action/menu/menuvideo.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "gui/action/menu/menuvideofilter.h"
#include "gui/action/menu/menuwindowsize.h"
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
    connect(this, &TMenuAspect::aboutToShow,
            mw, &TMainWindow::updateAspectMenu);
}

void TMenuAspect::setAspectToolTip(QString tip) {

    QString s = menuAction()->shortcut().toString();
    if (!s.isEmpty()) {
        tip += " (" + s + ")";
    }
    menuAction()->setToolTip(tip);
}


TDeinterlaceGroup::TDeinterlaceGroup(TMainWindow* mw)
    : TActionGroup(mw, "deinterlacegroup") {

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
    : TMenu(parent, mw, "deinterlace_menu", tr("Deinterlace"), "deinterlace") {

    TDeinterlaceGroup* group = mw->findChild<TDeinterlaceGroup*>(
                "deinterlacegroup");
    addActions(group->actions());
    addSeparator();
    addAction(mw->findAction("toggle_deinterlacing"));
}

TRotateGroup::TRotateGroup(TMainWindow* mw) :
    TActionGroup(mw, "rotategroup") {

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
    : TMenu(parent, mw, "transform_menu", tr("Transform"), "transform") {

    addAction(mw->findAction("flip"));
    addAction(mw->findAction("mirror"));
    addSeparator();
    TRotateGroup* group = mw->findChild<TRotateGroup*>("rotategroup");
    addActions(group->actions());

    connect(this, &TMenuTransform::aboutToShow,
            mw, &TMainWindow::updateTransformMenu);
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


TMenuVideoTracks::TMenuVideoTracks(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "videotrack_menu", tr("Video track")) {

    addAction(mw->findAction("next_video_track"));
    connect(mw, &TMainWindow::videoTrackGroupChanged,
            this, &TMenuVideoTracks::updateVideoTracks);
}

void TMenuVideoTracks::updateVideoTracks(TAction* next, TActionGroup* group) {

    clear();
    addAction(next);
    addSeparator();
    addActions(group->actions());
}


TMenuVideo::TMenuVideo(QWidget* parent, TMainWindow* mw) :
        TMenu(parent, mw, "video_menu", tr("Video"), "noicon") {

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

    addSeparator();
    // Deinterlace submenu
    addMenu(new TMenuDeinterlace(this, mw));
    // Transform submenu
    addMenu(new TMenuTransform(this, mw));
    // Video filter submenu
    addMenu(new TMenuVideoFilter(this, mw));
    // Stereo 3D
    addAction(mw->findAction("stereo_3d_filter"));

    addSeparator();
    // Video tracks
    addMenu(new TMenuVideoTracks(this, mw));

    addSeparator();
    // Screenshots
    addAction(mw->findAction("screenshot"));
    addAction(mw->findAction("screenshots"));
    addAction(mw->findAction("capture_stream"));
}

} // namespace Menu
} // namespace Action
} // namespace Gui

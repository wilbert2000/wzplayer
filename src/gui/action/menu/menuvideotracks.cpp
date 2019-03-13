#include "gui/action/menu/menuvideotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Action {
namespace Menu {


TMenuVideoTracks::TMenuVideoTracks(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "videotrack_menu", tr("Video track"), "video_track") {

    // Next video track
    addAction(mw->findAction("next_video_track"));

    addSeparator();
    videoTrackGroup = new TActionGroup(mw, "videotrackgroup");
    connect(videoTrackGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setVideoTrack);
    connect(player, &Player::TPlayer::videoTracksChanged,
            this, &TMenuVideoTracks::updateVideoTracks);
    connect(player, &Player::TPlayer::videoTrackChanged,
            videoTrackGroup, &TActionGroup::setChecked);
}

void TMenuVideoTracks::updateVideoTracks() {
    WZDEBUG("");

    videoTrackGroup->clear();

    Maps::TTracks* videos = &player->mdat.videos;
    if (videos->count() == 0) {
        QAction* a = videoTrackGroup->addAction(tr("<empty>"));
        a->setEnabled(false);
    } else {
        Maps::TTracks::TTrackIterator i = videos->getIterator();
        while (i.hasNext()) {
            i.next();
            const Maps::TTrackData& track = i.value();
            QAction* action = new QAction(videoTrackGroup);
            action->setCheckable(true);
            action->setText(track.getDisplayName());
            action->setData(track.getID());
            if (track.getID() == videos->getSelectedID())
                action->setChecked(true);
        }
    }

    addActions(videoTrackGroup->actions());
}

} // namespace Menu
} // namespace Action
} // namespace Gui

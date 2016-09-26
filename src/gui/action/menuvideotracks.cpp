#include "gui/action/menuvideotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Action {


TMenuVideoTracks::TMenuVideoTracks(TMainWindow* mw)
    : TMenu(mw, mw, "videotrack_menu", tr("&Video track"), "video_track") {

	// Next video track
	nextVideoTrackAct = new TAction(this, "next_video_track", tr("Next video track"));
    main_window->addAction(nextVideoTrackAct);
    connect(nextVideoTrackAct, SIGNAL(triggered()), player, SLOT(nextVideoTrack()));

	addSeparator();
	videoTrackGroup = new TActionGroup(this, "videotrack");
    connect(videoTrackGroup, SIGNAL(activated(int)), player, SLOT(changeVideoTrack(int)));
    connect(player, SIGNAL(videoTracksChanged()), this, SLOT(updateVideoTracks()));
    connect(player, SIGNAL(videoTrackChanged(int)), videoTrackGroup, SLOT(setChecked(int)));
}

void TMenuVideoTracks::enableActions() {

    nextVideoTrackAct->setEnabled(player->statePOP() && player->mdat.videos.count() > 1);
}

void TMenuVideoTracks::updateVideoTracks() {
    logger()->debug("updateVideoTracks");

	videoTrackGroup->clear();

    Maps::TTracks* videos = &player->mdat.videos;
	if (videos->count() == 0) {
		QAction* a = videoTrackGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		Maps::TTracks::TTrackIterator i = videos->getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTrackData track = i.value();
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

} // namespace Action
} // namespace Gui

#include "gui/action/menuvideotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "core.h"
#include "gui/base.h"


namespace Gui {
namespace Action {


TMenuVideoTracks::TMenuVideoTracks(TBase* mw, TCore* c)
    : TMenu(mw, mw, "videotrack_menu", tr("&Video track"), "video_track")
	, core(c) {

	// Next video track
	nextVideoTrackAct = new TAction(this, "next_video_track", tr("Next video track"));
    main_window->addAction(nextVideoTrackAct);
	connect(nextVideoTrackAct, SIGNAL(triggered()), core, SLOT(nextVideoTrack()));

	addSeparator();
	videoTrackGroup = new TActionGroup(this, "videotrack");
	connect(videoTrackGroup, SIGNAL(activated(int)), core, SLOT(changeVideoTrack(int)));
	connect(core, SIGNAL(videoTracksChanged()), this, SLOT(updateVideoTracks()));
	connect(core, SIGNAL(videoTrackChanged(int)), videoTrackGroup, SLOT(setChecked(int)));
}

void TMenuVideoTracks::enableActions() {

    nextVideoTrackAct->setEnabled(core->statePOP() && core->mdat.videos.count() > 1);
}

void TMenuVideoTracks::updateVideoTracks() {
    logger()->debug("updateVideoTracks");

	videoTrackGroup->clear();

	Maps::TTracks* videos = &core->mdat.videos;
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

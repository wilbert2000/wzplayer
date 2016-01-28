#include "gui/action/menuvideotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "core.h"


namespace Gui {
namespace Action {


TMenuVideoTracks::TMenuVideoTracks(QWidget *parent, TCore* c)
	: TMenu(parent, this, "videotrack_menu", QT_TR_NOOP("&Video track"), "video_track")
	, core(c) {

	// Next video track
	nextVideoTrackAct = new TAction(this, "next_video_track", QT_TR_NOOP("Next video track"));
	parent->addAction(nextVideoTrackAct);
	connect(nextVideoTrackAct, SIGNAL(triggered()), core, SLOT(nextVideoTrack()));

	addSeparator();
	videoTrackGroup = new TActionGroup(this, "videotrack");
	connect(videoTrackGroup, SIGNAL(activated(int)), core, SLOT(changeVideoTrack(int)));
	connect(core, SIGNAL(videoTrackInfoChanged()), this, SLOT(updateVideoTracks()));
	connect(core, SIGNAL(videoTrackChanged(int)), videoTrackGroup, SLOT(setChecked(int)));
}

void TMenuVideoTracks::enableActions(bool stopped, bool video, bool) {

	nextVideoTrackAct->setEnabled(!stopped && video && core->mdat.videos.count() > 1);
}

void TMenuVideoTracks::updateVideoTracks() {
	qDebug("Gui::Action::TMenuVideoTracks::updateVideoTracks");

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

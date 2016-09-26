#include "gui/action/menuaudiotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"


namespace Gui {
namespace Action {


TMenuAudioTracks::TMenuAudioTracks(TMainWindow* mw)
    : TMenu(mw, mw, "audiotrack_menu", tr("Audio &track"), "audio_track") {

	// Next audio track
    nextAudioTrackAct = new TAction(this, "next_audio_track", tr("Next audio track"), "", QKeySequence("*"));
    main_window->addAction(nextAudioTrackAct);
    connect(nextAudioTrackAct, SIGNAL(triggered()), player, SLOT(nextAudioTrack()));

	addSeparator();
	audioTrackGroup = new TActionGroup(this, "audiotrack");
    connect(audioTrackGroup, SIGNAL(activated(int)), player, SLOT(changeAudioTrack(int)));
    connect(player, SIGNAL(audioTracksChanged()), this, SLOT(updateAudioTracks()));
    connect(player, SIGNAL(audioTrackChanged(int)), audioTrackGroup, SLOT(setChecked(int)));
}

void TMenuAudioTracks::enableActions() {

    nextAudioTrackAct->setEnabled(player->statePOP() && player->mdat.audios.count() > 1);
}

void TMenuAudioTracks::updateAudioTracks() {
		logger()->debug("updateAudioTracks");

		audioTrackGroup->clear();

        Maps::TTracks* audios = &player->mdat.audios;
		if (audios->count() == 0) {
			QAction* a = audioTrackGroup->addAction(tr("<empty>"));
			a->setEnabled(false);
		} else {
			Maps::TTracks::TTrackIterator i = audios->getIterator();
			while (i.hasNext()) {
				i.next();
				Maps::TTrackData track = i.value();
				QAction* action = new QAction(audioTrackGroup);
				action->setCheckable(true);
				action->setText(track.getDisplayName());
				action->setData(track.getID());
				if (track.getID() == audios->getSelectedID())
					action->setChecked(true);
			}
		}

		addActions(audioTrackGroup->actions());
	}

} // namespace Action
} // namespace Gui

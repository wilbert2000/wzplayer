#include "gui/action/menuaudiotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "core.h"
#include "gui/base.h"


namespace Gui {
namespace Action {


TMenuAudioTracks::TMenuAudioTracks(TBase* mw, TCore* c)
    : TMenu(mw, mw, "audiotrack_menu", tr("Audio &track"), "audio_track")
	, core(c) {

	// Next audio track
    nextAudioTrackAct = new TAction(this, "next_audio_track", tr("Next audio track"), "", QKeySequence("*"));
    main_window->addAction(nextAudioTrackAct);
	connect(nextAudioTrackAct, SIGNAL(triggered()), core, SLOT(nextAudioTrack()));

	addSeparator();
	audioTrackGroup = new TActionGroup(this, "audiotrack");
	connect(audioTrackGroup, SIGNAL(activated(int)), core, SLOT(changeAudioTrack(int)));
	connect(core, SIGNAL(audioTracksChanged()), this, SLOT(updateAudioTracks()));
	connect(core, SIGNAL(audioTrackChanged(int)), audioTrackGroup, SLOT(setChecked(int)));
}

void TMenuAudioTracks::enableActions(bool stopped, bool, bool audio) {

	nextAudioTrackAct->setEnabled(!stopped && audio && core->mdat.audios.count() > 1);
}

void TMenuAudioTracks::updateAudioTracks() {
		qDebug("Gui::Action::TMenuAudio::updateAudioTracks");

		audioTrackGroup->clear();

		Maps::TTracks* audios = &core->mdat.audios;
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

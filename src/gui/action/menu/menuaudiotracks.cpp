#include "gui/action/menu/menuaudiotracks.h"
#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "gui/mainwindow.h"
#include "player/player.h"


namespace Gui {
namespace Action {
namespace Menu {

TMenuAudioTracks::TMenuAudioTracks(TMainWindow* mw)
    : TMenu(mw, mw, "audiotrack_menu", tr("Audio &track"), "audio_track") {

    // Next audio track
    nextAudioTrackAct = new TAction(this, "next_audio_track",
                                    tr("Next audio track"), "",
                                    QKeySequence("*"));
    connect(nextAudioTrackAct, &TAction::triggered,
            player, &Player::TPlayer::nextAudioTrack);

    addSeparator();
    audioTrackGroup = new TActionGroup(this, "audiotrack");
    connect(audioTrackGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setAudioTrack);
    connect(player, &Player::TPlayer::audioTracksChanged,
            this, &TMenuAudioTracks::updateAudioTracks);
    connect(player, &Player::TPlayer::audioTrackChanged,
            audioTrackGroup, &TActionGroup::setChecked);
}

void TMenuAudioTracks::enableActions() {

    nextAudioTrackAct->setEnabled(player->statePOP() && player->mdat.audios.count() > 1);
}

void TMenuAudioTracks::updateAudioTracks() {
    WZDEBUG("");

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

} // namespace Menu
} // namespace Action
} // namespace Gui

#include "gui/action/menu/menuaudio.h"
#include "gui/mainwindow.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "images.h"

#include <QWidget>

using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


TAudioChannelGroup::TAudioChannelGroup(TMainWindow* mw)
    : TActionGroup(mw, "audiochannelgroup") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "channels_stereo", tr("Stereo"),
                         TMediaSettings::ChStereo);
    new TActionGroupItem(mw, this, "channels_surround", tr("4.0 Surround"),
                         TMediaSettings::ChSurround);
    new TActionGroupItem(mw, this, "channels_ful51", tr("5.1 Surround"),
                         TMediaSettings::ChFull51);
    new TActionGroupItem(mw, this, "channels_ful61", tr("6.1 Surround"),
                         TMediaSettings::ChFull61);
    new TActionGroupItem(mw, this, "channels_ful71", tr("7.1 Surround"),
                         TMediaSettings::ChFull71);
    setChecked(player->mset.audio_use_channels);
    connect(this, &TActionGroup::activated,
            player, &Player::TPlayer::setAudioChannels);
    // No one else sets it
}


class TMenuAudioChannel : public TMenu {
public:
    explicit TMenuAudioChannel(QWidget* parent, TMainWindow* mw);
};

TMenuAudioChannel::TMenuAudioChannel(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "audiochannels_menu", tr("Audio channels"),
            "audio_channels") {

    addActions(mw->findChild<TAudioChannelGroup*>("audiochannelgroup")
               ->actions());
}


TStereoGroup::TStereoGroup(TMainWindow* mw)
    : TActionGroup(mw, "stereogroup") {

    setEnabled(false);
    new TActionGroupItem(mw, this, "stereo", tr("Stereo"),
                         TMediaSettings::Stereo);
    new TActionGroupItem(mw, this, "left_channel", tr("Left channel"),
                         TMediaSettings::Left);
    new TActionGroupItem(mw, this, "right_channel", tr("Right channel"),
                         TMediaSettings::Right);
    new TActionGroupItem(mw, this, "mono", tr("Mono"),
                         TMediaSettings::Mono);
    new TActionGroupItem(mw, this, "reverse_channels", tr("Reverse"),
                         TMediaSettings::Reverse);
    setChecked(player->mset.stereo_mode);
    connect(this, &TStereoGroup::activated,
            player, &Player::TPlayer::setStereoMode);
    // No one changes it
}

class TMenuStereo : public TMenu {
public:
    explicit TMenuStereo(QWidget* parent, TMainWindow* mw);
};

TMenuStereo::TMenuStereo(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "stereomode_menu", tr("Stereo mode")) {

    addActions(mw->findChild<TStereoGroup*>("stereogroup")->actions());
}

TMenuAudioTracks::TMenuAudioTracks(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "audiotrack_menu", tr("Audio track")) {

    addAction(mw->findAction("next_audio_track"));
    addSeparator();
    connect(mw, &TMainWindow::audioTrackGroupChanged,
            this, &TMenuAudioTracks::updateAudioTracks);
}

void TMenuAudioTracks::updateAudioTracks(TActionGroup* group) {
    addActions(group->actions());
}


TMenuAudio::TMenuAudio(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "audio_menu", tr("Audio"), "noicon") {

    // Mute
    addAction(mw->findAction("mute"));
    addAction(mw->findAction("decrease_volume"));
    addAction(mw->findAction("increase_volume"));

    // Delay
    addSeparator();
    addAction(mw->findAction("dec_audio_delay"));
    addAction(mw->findAction("inc_audio_delay"));
    addAction(mw->findAction("audio_delay"));

    // Equalizer
    addSeparator();
    addAction(mw->findAction("audio_equalizer"));
    addAction(mw->findAction("reset_audio_equalizer"));

    // Stereo and channel subs
    addSeparator();
    addMenu(new TMenuStereo(this, mw));
    addMenu(new TMenuAudioChannel(this, mw));

    // Filter sub
    TMenu* menu = new TMenu(this, mw, "audiofilter_menu",
                            tr("Audio filters"), "audio_filters");
    menu->addAction(mw->findAction("volnorm_filter"));
    menu->addAction(mw->findAction("extrastereo_filter"));
    menu->addAction(mw->findAction("karaoke_filter"));
    addMenu(menu);

    // Audio tracks
    addSeparator();
    addMenu(new TMenuAudioTracks(this, mw));

    // Load/unload
    addSeparator();
    addAction(mw->findAction("load_audio_file"));
    addAction(mw->findAction("unload_audio_file"));
}

} // namespace Menu
} // namespace Action
} // namesapce Gui

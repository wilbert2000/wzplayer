#include "gui/action/menu/menuaudio.h"
#include "gui/action/menu/menuaudiotracks.h"
#include "gui/action/actionseditor.h"
#include "gui/action/actiongroup.h"
#include "gui/mainwindow.h"
#include "gui/audioequalizer.h"
#include "player/player.h"
#include "settings/mediasettings.h"
#include "images.h"

#include <QWidget>

using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {


class TMenuAudioChannel : public TMenu {
public:
    explicit TMenuAudioChannel(TMainWindow* mw);
protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();
private:
    TActionGroup* group;
};


TMenuAudioChannel::TMenuAudioChannel(TMainWindow* mw)
    : TMenu(mw, mw, "audiochannels_menu", tr("Channels"), "audio_channels") {

    group = new TActionGroup(this, "channels");
    group->setEnabled(false);
    new TActionGroupItem(mw, group, "channels_stereo", tr("Stereo"),
                         TMediaSettings::ChStereo);
    new TActionGroupItem(mw, group, "channels_surround", tr("4.0 Surround"),
                         TMediaSettings::ChSurround);
    new TActionGroupItem(mw, group, "channels_ful51", tr("5.1 Surround"),
                         TMediaSettings::ChFull51);
    new TActionGroupItem(mw, group, "channels_ful61", tr("6.1 Surround"),
                         TMediaSettings::ChFull61);
    new TActionGroupItem(mw, group, "channels_ful71", tr("7.1 Surround"),
                         TMediaSettings::ChFull71);
    group->setChecked(player->mset.audio_use_channels);
    addActions(group->actions());
    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setAudioChannels);
    // No one else sets it
}

void TMenuAudioChannel::enableActions() {
    // Uses mset, so useless to set if stopped or no audio
    group->setEnabled(player->statePOP() && player->hasAudio());
}

void TMenuAudioChannel::onMediaSettingsChanged(TMediaSettings* mset) {
    group->setChecked(mset->audio_use_channels);
}

void TMenuAudioChannel::onAboutToShow() {
    group->setChecked(player->mset.audio_use_channels);
}


class TMenuStereo : public TMenu {
public:
    explicit TMenuStereo(TMainWindow* mw);
protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
    virtual void onAboutToShow();
private:
    TActionGroup* group;
};

TMenuStereo::TMenuStereo(TMainWindow* mw)
    : TMenu(mw, mw, "stereomode_menu", tr("Stereo mode"), "stereo_mode") {

    group = new TActionGroup(this, "stereo");
    group->setEnabled(false);
    new TActionGroupItem(mw, group, "stereo", tr("Stereo"),
                         TMediaSettings::Stereo);
    new TActionGroupItem(mw, group, "left_channel", tr("Left channel"),
                         TMediaSettings::Left);
    new TActionGroupItem(mw, group, "right_channel", tr("Right channel"),
                         TMediaSettings::Right);
    new TActionGroupItem(mw, group, "mono", tr("Mono"),
                         TMediaSettings::Mono);
    new TActionGroupItem(mw, group, "reverse_channels", tr("Reverse"),
                         TMediaSettings::Reverse);
    group->setChecked(player->mset.stereo_mode);
    addActions(group->actions());
    connect(group, &TActionGroup::activated,
            player, &Player::TPlayer::setStereoMode);
    // No one else changes it
}

void TMenuStereo::enableActions() {
    group->setEnabled(player->statePOP() && player->hasAudio());
}

void TMenuStereo::onMediaSettingsChanged(TMediaSettings* mset) {
    group->setChecked(mset->stereo_mode);
}

void TMenuStereo::onAboutToShow() {
    group->setChecked(player->mset.stereo_mode);
}


TMenuAudio::TMenuAudio(TMainWindow* mw, TAudioEqualizer* audioEqualizer)
    : TMenu(mw, mw, "audio_menu", tr("Audio"), "noicon") {

    // Mute
    muteAct = new TAction(mw, "mute", tr("Mute"), "noicon", Qt::Key_M);
    muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
    muteAct->setCheckable(true);
    muteAct->setChecked(player->getMute());
    QIcon icset(Images::icon("volume"));
    icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
    muteAct->setIcon(icset);
    addAction(muteAct);
    connect(muteAct, &TAction::triggered, player, &Player::TPlayer::mute);
    connect(player, &Player::TPlayer::muteChanged,
            muteAct, &TAction::setChecked);

    decVolumeAct = new TAction(mw, "decrease_volume", tr("Volume -"),
                               "audio_down", Qt::Key_Minus);
    decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
    connect(decVolumeAct, &TAction::triggered,
            player, &Player::TPlayer::decVolume);
    addAction(decVolumeAct);

    incVolumeAct = new TAction(mw, "increase_volume", tr("Volume +"),
                               "audio_up", Qt::Key_Plus);
    incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
    connect(incVolumeAct, &TAction::triggered,
            player, &Player::TPlayer::incVolume);
    addAction(incVolumeAct);

    addSeparator();
    decAudioDelayAct = new TAction(mw, "dec_audio_delay", tr("Delay -"),
                                   "delay_down", Qt::CTRL | Qt::Key_Minus);
    connect(decAudioDelayAct, &TAction::triggered,
            player, &Player::TPlayer::decAudioDelay);
    addAction(decAudioDelayAct);

    incAudioDelayAct = new TAction(mw, "inc_audio_delay", tr("Delay +"),
                                   "delay_up", Qt::CTRL | Qt::Key_Plus);
    connect(incAudioDelayAct, &TAction::triggered,
            player, &Player::TPlayer::incAudioDelay);
    addAction(incAudioDelayAct);

    audioDelayAct = new TAction(mw, "audio_delay", tr("Set delay..."), "",
                                Qt::META | Qt::Key_Plus);
    connect(audioDelayAct, &TAction::triggered,
            mw, &TMainWindow::showAudioDelayDialog);
    addAction(audioDelayAct);

    // Equalizer
    addSeparator();
    audioEqualizerAct = new TAction(mw, "audio_equalizer", tr("Equalizer"),
                                    "", Qt::ALT | Qt::Key_Plus);
    audioEqualizerAct->setCheckable(true);
    audioEqualizerAct->setChecked(audioEqualizer->isVisible());
    connect(audioEqualizerAct, &TAction::triggered,
            audioEqualizer, &TAudioEqualizer::setVisible);
    connect(audioEqualizer, &TAudioEqualizer::visibilityChanged,
            audioEqualizerAct, &TAction::setChecked);
    addAction(audioEqualizerAct);

    resetAudioEqualizerAct = new TAction(mw, "reset_audio_equalizer",
                                         tr("Reset audio equalizer"), "",
                                         Qt::ALT | Qt::Key_Minus);
    connect(resetAudioEqualizerAct, &TAction::triggered,
            audioEqualizer, &TAudioEqualizer::reset);
    addAction(resetAudioEqualizerAct);

    // Stereo and channel subs
    addSeparator();
    addMenu(new TMenuStereo(mw));
    addMenu(new TMenuAudioChannel(mw));

    // Filter sub
    audioFilterMenu = new TMenu(mw, mw, "audiofilter_menu", tr("Filters"),
                                "audio_filters");
    volnormAct = new TAction(mw, "volnorm_filter", tr("Volume normalization"));
    volnormAct->setCheckable(true);
    connect(volnormAct, &TAction::triggered,
            player, &Player::TPlayer::setVolnorm);
    audioFilterMenu->addAction(volnormAct);

    extrastereoAct = new TAction(mw, "extrastereo_filter", tr("Extrastereo"));
    extrastereoAct->setCheckable(true);
    connect(extrastereoAct, &TAction::triggered,
            player, &Player::TPlayer::setExtrastereo);
    audioFilterMenu->addAction(extrastereoAct);

    karaokeAct = new TAction(mw, "karaoke_filter", tr("Karaoke"));
    karaokeAct->setCheckable(true);
    connect(karaokeAct, &TAction::triggered,
            player, &Player::TPlayer::toggleKaraoke);
    audioFilterMenu->addAction(karaokeAct);

    addMenu(audioFilterMenu);

    // Audio tracks
    addSeparator();
    addMenu(new TMenuAudioTracks(mw));

    // Load/unload
    addSeparator();
    loadAudioAct = new TAction(mw, "load_audio_file",
                               tr("Load external file..."), "open");
    connect(loadAudioAct, &TAction::triggered,
            mw, &TMainWindow::loadAudioFile);
    addAction(loadAudioAct);

    unloadAudioAct = new TAction(mw, "unload_audio_file", tr("Unload"),
                                 "unload");
    connect(unloadAudioAct, &TAction::triggered,
            player, &Player::TPlayer::unloadAudioFile);
    addAction(unloadAudioAct);
}

void TMenuAudio::enableActions() {

    // Maybe global settings
    bool enable = pref->global_volume
            || (player->statePOP() && player->hasAudio());
    muteAct->setEnabled(enable);
    decVolumeAct->setEnabled(enable);
    incVolumeAct->setEnabled(enable);

    // Settings only stored in mset
    enable = player->statePOP() && player->hasAudio();

    decAudioDelayAct->setEnabled(enable);
    incAudioDelayAct->setEnabled(enable);
    audioDelayAct->setEnabled(enable);

    // Equalizer can be global
    bool enableEqualizer = pref->use_audio_equalizer
                           && (pref->global_audio_equalizer || enable);
    audioEqualizerAct->setEnabled(enableEqualizer);
    resetAudioEqualizerAct->setEnabled(enableEqualizer);

    // Filters, mset only
    volnormAct->setEnabled(enable);
    extrastereoAct->setEnabled(enable);
    karaokeAct->setEnabled(enable && pref->isMPlayer());

    loadAudioAct->setEnabled(player->statePOP());
    unloadAudioAct->setEnabled(enable && player->mset.external_audio.count());
}

void TMenuAudio::onMediaSettingsChanged(TMediaSettings* mset) {

    // Filters
    volnormAct->setChecked(mset->volnorm_filter);

    // Karaoke
    karaokeAct->setChecked(mset->karaoke_filter);
    // Extra stereo
    extrastereoAct->setChecked(mset->extrastereo_filter);
}

} // namespace Menu
} // namespace Action
} // namesapce Gui

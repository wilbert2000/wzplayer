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
    : TMenu(mw, mw, "audiochannels_menu", tr("&Channels"), "audio_channels") {

	group = new TActionGroup(this, "channels");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "channels_stereo", tr("&Stereo"), TMediaSettings::ChStereo);
	new TActionGroupItem(this, group, "channels_surround", tr("&4.0 Surround"), TMediaSettings::ChSurround);
	new TActionGroupItem(this, group, "channels_ful51", tr("&5.1 Surround"), TMediaSettings::ChFull51);
	new TActionGroupItem(this, group, "channels_ful61", tr("&6.1 Surround"), TMediaSettings::ChFull61);
	new TActionGroupItem(this, group, "channels_ful71", tr("&7.1 Surround"), TMediaSettings::ChFull71);
    group->setChecked(player->mset.audio_use_channels);
    connect(group, SIGNAL(activated(int)), player, SLOT(setAudioChannels(int)));
	// No one else sets it
    addActionsTo(main_window);
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
    : TMenu(mw, mw, "stereomode_menu", tr("&Stereo mode"), "stereo_mode") {

	group = new TActionGroup(this, "stereo");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "stereo", tr("&Stereo"), TMediaSettings::Stereo);
	new TActionGroupItem(this, group, "left_channel", tr("&Left channel"), TMediaSettings::Left);
	new TActionGroupItem(this, group, "right_channel", tr("&Right channel"), TMediaSettings::Right);
	new TActionGroupItem(this, group, "mono", tr("&Mono"), TMediaSettings::Mono);
	new TActionGroupItem(this, group, "reverse_channels", tr("Re&verse"), TMediaSettings::Reverse);
    group->setChecked(player->mset.stereo_mode);
    connect(group, SIGNAL(activated(int)), player, SLOT(setStereoMode(int)));
	// No one else changes it
    addActionsTo(main_window);
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
    : TMenu(mw, mw, "audio_menu", tr("&Audio"), "noicon") {

	// Mute
	muteAct = new TAction(this, "mute", tr("&Mute"), "noicon", Qt::Key_M);
	muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
	muteAct->setCheckable(true);
    muteAct->setChecked(player->getMute());

	QIcon icset(Images::icon("volume"));
	icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
	muteAct->setIcon(icset);

    connect(muteAct, SIGNAL(triggered(bool)), player, SLOT(mute(bool)));
    connect(player, SIGNAL(muteChanged(bool)), muteAct, SLOT(setChecked(bool)));

    decVolumeAct = new TAction(this, "decrease_volume", tr("Volume &-"), "audio_down", Qt::Key_Minus);
	decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
    connect(decVolumeAct, SIGNAL(triggered()), player, SLOT(decVolume()));

    incVolumeAct = new TAction(this, "increase_volume", tr("Volume &+"), "audio_up", Qt::Key_Plus);
	incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
    connect(incVolumeAct, SIGNAL(triggered()), player, SLOT(incVolume()));

	addSeparator();
    decAudioDelayAct = new TAction(this, "dec_audio_delay", tr("&Delay -"), "delay_down", Qt::CTRL | Qt::Key_Minus);
    connect(decAudioDelayAct, SIGNAL(triggered()), player, SLOT(decAudioDelay()));

    incAudioDelayAct = new TAction(this, "inc_audio_delay", tr("Del&ay +"), "delay_up", Qt::CTRL | Qt::Key_Plus);
    connect(incAudioDelayAct, SIGNAL(triggered()), player, SLOT(incAudioDelay()));

    audioDelayAct = new TAction(this, "audio_delay", tr("Set dela&y..."), "", Qt::META | Qt::Key_Plus);
    connect(audioDelayAct, SIGNAL(triggered()), main_window, SLOT(showAudioDelayDialog()));

	// Equalizer
	addSeparator();
    audioEqualizerAct = new TAction(this, "audio_equalizer", tr("&Equalizer"), "", Qt::ALT | Qt::Key_Plus);
	audioEqualizerAct->setCheckable(true);
	audioEqualizerAct->setChecked(audioEqualizer->isVisible());
	connect(audioEqualizerAct, SIGNAL(triggered(bool)), audioEqualizer, SLOT(setVisible(bool)));
	connect(audioEqualizer, SIGNAL(visibilityChanged(bool)), audioEqualizerAct, SLOT(setChecked(bool)));

    resetAudioEqualizerAct = new TAction(this, "reset_audio_equalizer", tr("Reset audio equalizer"), "", Qt::ALT | Qt::Key_Minus);
	connect(resetAudioEqualizerAct, SIGNAL(triggered()), audioEqualizer, SLOT(reset()));

	// Stereo and channel subs
	addSeparator();
    addMenu(new TMenuStereo(main_window));
    addMenu(new TMenuAudioChannel(main_window));

    // Filter sub
    audioFilterMenu = new TMenu(main_window, main_window, "audiofilter_menu", tr("&Filters"), "audio_filters");
	volnormAct = new TAction(this, "volnorm_filter", tr("Volume &normalization"), "", 0, false);
	volnormAct->setCheckable(true);
	audioFilterMenu->addAction(volnormAct);
    connect(volnormAct, SIGNAL(triggered(bool)), player, SLOT(toggleVolnorm(bool)));

	extrastereoAct = new TAction(this, "extrastereo_filter", tr("&Extrastereo"), "", 0, false);
	extrastereoAct->setCheckable(true);
	audioFilterMenu->addAction(extrastereoAct);
    connect(extrastereoAct, SIGNAL(triggered(bool)), player, SLOT(toggleExtrastereo(bool)));

	karaokeAct = new TAction(this, "karaoke_filter", tr("&Karaoke"), "", 0, false);
	karaokeAct->setCheckable(true);
	audioFilterMenu->addAction(karaokeAct);
    connect(karaokeAct, SIGNAL(triggered(bool)), player, SLOT(toggleKaraoke(bool)));

	addMenu(audioFilterMenu);

	// Audio tracks
	addSeparator();
    addMenu(new TMenuAudioTracks(main_window));

	// Load/unload
	addSeparator();
	loadAudioAct = new TAction(this, "load_audio_file", tr("&Load external file..."), "open");
    connect(loadAudioAct, SIGNAL(triggered()), main_window, SLOT(loadAudioFile()));
	unloadAudioAct = new TAction(this, "unload_audio_file", tr("&Unload"), "unload");
    connect(unloadAudioAct, SIGNAL(triggered()), player, SLOT(unloadAudioFile()));

    addActionsTo(main_window);
}

void TMenuAudio::enableActions() {

    // Maybe global settings
    bool enable = pref->global_volume || (player->statePOP() && player->hasAudio());
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

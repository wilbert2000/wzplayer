#include "gui/action/menuaudio.h"
#include <QWidget>
#include "images.h"
#include "settings/mediasettings.h"
#include "core.h"
#include "gui/action/actiongroup.h"
#include "gui/action/actionseditor.h"
#include "gui/action/menuaudiotracks.h"
#include "gui/audioequalizer.h"


using namespace Settings;

namespace Gui {
namespace Action {


class TMenuAudioChannel : public TMenu {
public:
	explicit TMenuAudioChannel(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TMenuAudioChannel::TMenuAudioChannel(QWidget *parent, TCore* c)
    : TMenu(parent, "audiochannels_menu", tr("&Channels"), "audio_channels")
	, core(c) {

	group = new TActionGroup(this, "channels");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "channels_stereo", tr("&Stereo"), TMediaSettings::ChStereo);
	new TActionGroupItem(this, group, "channels_surround", tr("&4.0 Surround"), TMediaSettings::ChSurround);
	new TActionGroupItem(this, group, "channels_ful51", tr("&5.1 Surround"), TMediaSettings::ChFull51);
	new TActionGroupItem(this, group, "channels_ful61", tr("&6.1 Surround"), TMediaSettings::ChFull61);
	new TActionGroupItem(this, group, "channels_ful71", tr("&7.1 Surround"), TMediaSettings::ChFull71);
	group->setChecked(core->mset.audio_use_channels);
	connect(group, SIGNAL(activated(int)), core, SLOT(setAudioChannels(int)));
	// No one else sets it
	addActionsTo(parent);
}

void TMenuAudioChannel::enableActions(bool stopped, bool, bool audio) {
	// Uses mset, so useless to set if stopped or no audio
	group->setEnabled(!stopped && audio);
}

void TMenuAudioChannel::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->audio_use_channels);
}

void TMenuAudioChannel::onAboutToShow() {
	group->setChecked(core->mset.audio_use_channels);
}


class TMenuStereo : public TMenu {
public:
	explicit TMenuStereo(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};

TMenuStereo::TMenuStereo(QWidget *parent, TCore* c)
    : TMenu(parent, "stereomode_menu", tr("&Stereo mode"), "stereo_mode")
	, core(c) {

	group = new TActionGroup(this, "stereo");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "stereo", tr("&Stereo"), TMediaSettings::Stereo);
	new TActionGroupItem(this, group, "left_channel", tr("&Left channel"), TMediaSettings::Left);
	new TActionGroupItem(this, group, "right_channel", tr("&Right channel"), TMediaSettings::Right);
	new TActionGroupItem(this, group, "mono", tr("&Mono"), TMediaSettings::Mono);
	new TActionGroupItem(this, group, "reverse_channels", tr("Re&verse"), TMediaSettings::Reverse);
	group->setChecked(core->mset.stereo_mode);
	connect(group, SIGNAL(activated(int)), core, SLOT(setStereoMode(int)));
	// No one else changes it
	addActionsTo(parent);
}

void TMenuStereo::enableActions(bool stopped, bool, bool audio) {
	group->setEnabled(!stopped && audio);
}

void TMenuStereo::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->stereo_mode);
}

void TMenuStereo::onAboutToShow() {
	group->setChecked(core->mset.stereo_mode);
}


TMenuAudio::TMenuAudio(QWidget* parent, TCore* c, TAudioEqualizer* audioEqualizer)
    : TMenu(parent, "audio_menu", tr("&Audio"), "noicon")
	, core(c) {

	// Mute
	muteAct = new TAction(this, "mute", tr("&Mute"), "noicon", Qt::Key_M);
	muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
	muteAct->setCheckable(true);
	muteAct->setChecked(core->getMute());

	QIcon icset(Images::icon("volume"));
	icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
	muteAct->setIcon(icset);

	connect(muteAct, SIGNAL(triggered(bool)), core, SLOT(mute(bool)));
	connect(core, SIGNAL(muteChanged(bool)), muteAct, SLOT(setChecked(bool)));

    decVolumeAct = new TAction(this, "decrease_volume", tr("Volume &-"), "audio_down", Qt::Key_Minus);
	decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
	connect(decVolumeAct, SIGNAL(triggered()), core, SLOT(decVolume()));

    incVolumeAct = new TAction(this, "increase_volume", tr("Volume &+"), "audio_up", Qt::Key_Plus);
	incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
	connect(incVolumeAct, SIGNAL(triggered()), core, SLOT(incVolume()));

	addSeparator();
    decAudioDelayAct = new TAction(this, "dec_audio_delay", tr("&Delay -"), "delay_down", Qt::CTRL | Qt::Key_Minus);
	connect(decAudioDelayAct, SIGNAL(triggered()), core, SLOT(decAudioDelay()));

    incAudioDelayAct = new TAction(this, "inc_audio_delay", tr("Del&ay +"), "delay_up", Qt::CTRL | Qt::Key_Plus);
	connect(incAudioDelayAct, SIGNAL(triggered()), core, SLOT(incAudioDelay()));

	audioDelayAct = new TAction(this, "audio_delay", tr("Set dela&y..."));
	connect(audioDelayAct, SIGNAL(triggered()), parent, SLOT(showAudioDelayDialog()));

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
	addMenu(new TMenuStereo(parent, core));
	addMenu(new TMenuAudioChannel(parent, core));
	// Filter sub
    audioFilterMenu = new TMenu(parent, "audiofilter_menu", tr("&Filters"), "audio_filters");
	volnormAct = new TAction(this, "volnorm_filter", tr("Volume &normalization"), "", 0, false);
	volnormAct->setCheckable(true);
	audioFilterMenu->addAction(volnormAct);
	connect(volnormAct, SIGNAL(triggered(bool)), core, SLOT(toggleVolnorm(bool)));

	extrastereoAct = new TAction(this, "extrastereo_filter", tr("&Extrastereo"), "", 0, false);
	extrastereoAct->setCheckable(true);
	audioFilterMenu->addAction(extrastereoAct);
	connect(extrastereoAct, SIGNAL(triggered(bool)), core, SLOT(toggleExtrastereo(bool)));

	karaokeAct = new TAction(this, "karaoke_filter", tr("&Karaoke"), "", 0, false);
	karaokeAct->setCheckable(true);
	audioFilterMenu->addAction(karaokeAct);
	connect(karaokeAct, SIGNAL(triggered(bool)), core, SLOT(toggleKaraoke(bool)));

	addMenu(audioFilterMenu);

	// Audio tracks
	addSeparator();
	addMenu(new TMenuAudioTracks(parent, core));

	// Load/unload
	addSeparator();
	loadAudioAct = new TAction(this, "load_audio_file", tr("&Load external file..."), "open");
	connect(loadAudioAct, SIGNAL(triggered()), parent, SLOT(loadAudioFile()));
	unloadAudioAct = new TAction(this, "unload_audio_file", tr("&Unload"), "unload");
	connect(unloadAudioAct, SIGNAL(triggered()), core, SLOT(unloadAudioFile()));

	addActionsTo(parent);
}

void TMenuAudio::enableActions(bool stopped, bool, bool audio) {

	bool enableAudio = !stopped && audio;

	muteAct->setEnabled(enableAudio);
	decVolumeAct->setEnabled(enableAudio);
	incVolumeAct->setEnabled(enableAudio);
	decAudioDelayAct->setEnabled(enableAudio);
	incAudioDelayAct->setEnabled(enableAudio);
	audioDelayAct->setEnabled(enableAudio);

	bool enableEqualizer = enableAudio && pref->use_audio_equalizer;
	audioEqualizerAct->setEnabled(enableEqualizer);
	resetAudioEqualizerAct->setEnabled(enableEqualizer);

	// Filters
	volnormAct->setEnabled(enableAudio);
	extrastereoAct->setEnabled(enableAudio);
	karaokeAct->setEnabled(enableAudio && pref->isMPlayer());

	loadAudioAct->setEnabled(!stopped);
	unloadAudioAct->setEnabled(enableAudio && !core->mset.external_audio.isEmpty());
}

void TMenuAudio::onMediaSettingsChanged(TMediaSettings* mset) {

	// Filters
	volnormAct->setChecked(mset->volnorm_filter);

	// Karaoke
	karaokeAct->setChecked(mset->karaoke_filter);
	// Extra stereo
	extrastereoAct->setChecked(mset->extrastereo_filter);
}

} // namespace Action
} // namesapce Gui

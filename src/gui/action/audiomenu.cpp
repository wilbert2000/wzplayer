#include "gui/action/audiomenu.h"
#include <QWidget>
#include "images.h"
#include "settings/mediasettings.h"
#include "core.h"
#include "gui/action/actionseditor.h"
#include "gui/audioequalizer.h"


using namespace Settings;

namespace Gui {

class TAudioChannelMenu : public TMenu {
public:
	explicit TAudioChannelMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TAudioChannelMenu::TAudioChannelMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "audiochannels_menu", QT_TR_NOOP("&Channels"), "audio_channels")
	, core(c) {

	group = new TActionGroup(this, "channels");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "channels_stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::ChStereo);
	new TActionGroupItem(this, group, "channels_surround", QT_TR_NOOP("&4.0 Surround"), TMediaSettings::ChSurround);
	new TActionGroupItem(this, group, "channels_ful51", QT_TR_NOOP("&5.1 Surround"), TMediaSettings::ChFull51);
	new TActionGroupItem(this, group, "channels_ful61", QT_TR_NOOP("&6.1 Surround"), TMediaSettings::ChFull61);
	new TActionGroupItem(this, group, "channels_ful71", QT_TR_NOOP("&7.1 Surround"), TMediaSettings::ChFull71);
	group->setChecked(core->mset.audio_use_channels);
	connect(group, SIGNAL(activated(int)), core, SLOT(setAudioChannels(int)));
	// No one else sets it
	addActionsTo(parent);
}

void TAudioChannelMenu::enableActions(bool stopped, bool, bool audio) {
	// Uses mset, so useless to set if stopped or no audio
	group->setEnabled(!stopped && audio);
}

void TAudioChannelMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->audio_use_channels);
}

void TAudioChannelMenu::onAboutToShow() {
	group->setChecked(core->mset.audio_use_channels);
}


class TStereoMenu : public TMenu {
public:
	explicit TStereoMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};

TStereoMenu::TStereoMenu(QWidget *parent, TCore* c)
	: TMenu(parent, this, "stereomode_menu", QT_TR_NOOP("&Stereo mode"), "stereo_mode")
	, core(c) {

	group = new TActionGroup(this, "stereo");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "stereo", QT_TR_NOOP("&Stereo"), TMediaSettings::Stereo);
	new TActionGroupItem(this, group, "left_channel", QT_TR_NOOP("&Left channel"), TMediaSettings::Left);
	new TActionGroupItem(this, group, "right_channel", QT_TR_NOOP("&Right channel"), TMediaSettings::Right);
	new TActionGroupItem(this, group, "mono", QT_TR_NOOP("&Mono"), TMediaSettings::Mono);
	new TActionGroupItem(this, group, "reverse_channels", QT_TR_NOOP("Re&verse"), TMediaSettings::Reverse);
	group->setChecked(core->mset.stereo_mode);
	connect(group, SIGNAL(activated(int)), core, SLOT(setStereoMode(int)));
	// No one else changes it
	addActionsTo(parent);
}

void TStereoMenu::enableActions(bool stopped, bool, bool audio) {
	group->setEnabled(!stopped && audio);
}

void TStereoMenu::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->stereo_mode);
}

void TStereoMenu::onAboutToShow() {
	group->setChecked(core->mset.stereo_mode);
}


TAudioMenu::TAudioMenu(QWidget* parent, TCore* c, TAudioEqualizer* audioEqualizer)
	: TMenu(parent, this, "audio_menu", QT_TR_NOOP("&Audio"), "noicon")
	, core(c) {

	// Mute
	muteAct = new TAction(this, "mute", QT_TR_NOOP("&Mute"), "noicon", Qt::Key_M);
	muteAct->addShortcut(Qt::Key_VolumeMute); // MCE remote key
	muteAct->setCheckable(true);
	muteAct->setChecked(core->getMute());

	QIcon icset(Images::icon("volume"));
	icset.addPixmap(Images::icon("mute"), QIcon::Normal, QIcon::On);
	muteAct->setIcon(icset);

	connect(muteAct, SIGNAL(triggered(bool)), core, SLOT(mute(bool)));
	connect(core, SIGNAL(muteChanged(bool)), muteAct, SLOT(setChecked(bool)));

	decVolumeAct = new TAction(this, "decrease_volume", QT_TR_NOOP("Volume &-"), "audio_down");
	decVolumeAct->setShortcuts(TActionsEditor::stringToShortcuts("9,/"));
	decVolumeAct->addShortcut(Qt::Key_VolumeDown); // MCE remote key
	connect(decVolumeAct, SIGNAL(triggered()), core, SLOT(decVolume()));

	incVolumeAct = new TAction(this, "increase_volume", QT_TR_NOOP("Volume &+"), "audio_up");
	incVolumeAct->setShortcuts(TActionsEditor::stringToShortcuts("0,*"));
	incVolumeAct->addShortcut(Qt::Key_VolumeUp); // MCE remote key
	connect(incVolumeAct, SIGNAL(triggered()), core, SLOT(incVolume()));

	addSeparator();
	decAudioDelayAct = new TAction(this, "dec_audio_delay", QT_TR_NOOP("&Delay -"), "delay_down", Qt::Key_Minus);
	connect(decAudioDelayAct, SIGNAL(triggered()), core, SLOT(decAudioDelay()));

	incAudioDelayAct = new TAction(this, "inc_audio_delay", QT_TR_NOOP("Del&ay +"), "delay_up", Qt::Key_Plus);
	connect(incAudioDelayAct, SIGNAL(triggered()), core, SLOT(incAudioDelay()));

	audioDelayAct = new TAction(this, "audio_delay", QT_TR_NOOP("Set dela&y..."));
	connect(audioDelayAct, SIGNAL(triggered()), parent, SLOT(showAudioDelayDialog()));

	// Equalizer
	addSeparator();
	audioEqualizerAct = new TAction(this, "audio_equalizer", QT_TR_NOOP("&Equalizer"));
	audioEqualizerAct->setCheckable(true);
	audioEqualizerAct->setChecked(audioEqualizer->isVisible());
	connect(audioEqualizerAct, SIGNAL(triggered(bool)), audioEqualizer, SLOT(setVisible(bool)));
	connect(audioEqualizer, SIGNAL(visibilityChanged(bool)), audioEqualizerAct, SLOT(setChecked(bool)));
	// Stereo and channel subs
	addMenu(new TStereoMenu(parent, core));
	addMenu(new TAudioChannelMenu(parent, core));
	// Filter sub
	audioFilterMenu = new TMenu(parent, this, "audiofilter_menu", QT_TR_NOOP("&Filters"), "audio_filters");
	volnormAct = new TAction(this, "volnorm_filter", QT_TR_NOOP("Volume &normalization"), "", false);
	volnormAct->setCheckable(true);
	audioFilterMenu->addAction(volnormAct);
	connect(volnormAct, SIGNAL(triggered(bool)), core, SLOT(toggleVolnorm(bool)));

#ifdef MPLAYER_SUPPORT
	extrastereoAct = new TAction(this, "extrastereo_filter", QT_TR_NOOP("&Extrastereo"), "", false);
	extrastereoAct->setCheckable(true);
	audioFilterMenu->addAction(extrastereoAct);
	connect(extrastereoAct, SIGNAL(triggered(bool)), core, SLOT(toggleExtrastereo(bool)));

	karaokeAct = new TAction(this, "karaoke_filter", QT_TR_NOOP("&Karaoke"), "", false);
	karaokeAct->setCheckable(true);
	audioFilterMenu->addAction(karaokeAct);
	connect(karaokeAct, SIGNAL(triggered(bool)), core, SLOT(toggleKaraoke(bool)));
#endif

	addMenu(audioFilterMenu);

	// Audio track
	addSeparator();
	// Next audio track
	nextAudioTrackAct = new TAction(this, "next_audio_track", QT_TR_NOOP("Next audio track"), "", Qt::Key_K);
	connect(nextAudioTrackAct, SIGNAL(triggered()), core, SLOT(nextAudioTrack()));

	audioTrackGroup = new TActionGroup(this, "audiotrack");
	connect(audioTrackGroup, SIGNAL(activated(int)), core, SLOT(changeAudioTrack(int)));
	connect(core, SIGNAL(audioTrackInfoChanged()), this, SLOT(updateAudioTracks()));
	connect(core, SIGNAL(audioTrackChanged(int)), audioTrackGroup, SLOT(setChecked(int)));

	audioTrackMenu = new TMenu(parent, this, "audiotrack_menu", QT_TR_NOOP("&Track"), "audio_track");
	addMenu(audioTrackMenu);

	// Load/unload
	addSeparator();
	loadAudioAct = new TAction(this, "load_audio_file", QT_TR_NOOP("&Load external file..."), "open");
	connect(loadAudioAct, SIGNAL(triggered()), parent, SLOT(loadAudioFile()));
	unloadAudioAct = new TAction(this, "unload_audio_file", QT_TR_NOOP("&Unload"), "unload");
	connect(unloadAudioAct, SIGNAL(triggered()), core, SLOT(unloadAudioFile()));

	addActionsTo(parent);
}

void TAudioMenu::enableActions(bool stopped, bool, bool audio) {

	bool enableAudio = !stopped && audio;
	nextAudioTrackAct->setEnabled(enableAudio && core->mdat.audios.count() > 1);

	loadAudioAct->setEnabled(!stopped);

	unloadAudioAct->setEnabled(enableAudio && !core->mset.external_audio.isEmpty());

	// Filters
	volnormAct->setEnabled(enableAudio);

#ifdef MPLAYER_SUPPORT
	extrastereoAct->setEnabled(enableAudio && pref->isMPlayer());
	karaokeAct->setEnabled(enableAudio && pref->isMPlayer());
#endif

	audioEqualizerAct->setEnabled(enableAudio && pref->use_audio_equalizer);

	muteAct->setEnabled(enableAudio);
	decVolumeAct->setEnabled(enableAudio);
	incVolumeAct->setEnabled(enableAudio);
	decAudioDelayAct->setEnabled(enableAudio);
	incAudioDelayAct->setEnabled(enableAudio);
	audioDelayAct->setEnabled(enableAudio);
}

void TAudioMenu::onMediaSettingsChanged(TMediaSettings* mset) {

	// Filters
	volnormAct->setChecked(mset->volnorm_filter);

#ifdef MPLAYER_SUPPORT
	// Karaoke
	karaokeAct->setChecked(mset->karaoke_filter);
	// Extra stereo
	extrastereoAct->setChecked(mset->extrastereo_filter);
#endif
}

void TAudioMenu::updateAudioTracks() {
	qDebug("Gui::TAudioMenu::updateAudioTracks");

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

	audioTrackMenu->addActions(audioTrackGroup->actions());
}


} // namesapce Gui

/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "gui/pref/audio.h"
#include "images.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"
#include "settings/paths.h"

#if USE_ALSA_DEVICES || USE_DSOUND_DEVICES
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui { namespace Pref {

TAudio::TAudio(QWidget* parent, InfoList aol)
	: TWidget(parent, 0)
	, ao_list(aol) {

	setupUi(this);

#if USE_DSOUND_DEVICES
	dsound_devices = TDeviceInfo::dsoundDevices();
#endif

#if USE_ALSA_DEVICES
	alsa_devices = TDeviceInfo::alsaDevices();
#endif

	// Channels combo
	channels_combo->addItem("2", TMediaSettings::ChStereo);
	channels_combo->addItem("4", TMediaSettings::ChSurround);
	channels_combo->addItem("6", TMediaSettings::ChFull51);
	channels_combo->addItem("7", TMediaSettings::ChFull61);
	channels_combo->addItem("8", TMediaSettings::ChFull71);

	connect(ao_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(ao_combo_changed(int)));

	retranslateStrings();
}

TAudio::~TAudio() {
}

QString TAudio::sectionName() {
	return tr("Audio");
}

QPixmap TAudio::sectionIcon() {
	return Images::icon("speaker", icon_size);
}

void TAudio::retranslateStrings() {

	retranslateUi(this);

	icon_label->setPixmap(Images::icon("speaker"));

	updateDriverCombo(true);

	channels_combo->setItemText(0, tr("2 (Stereo)"));
	channels_combo->setItemText(1, tr("4 (4.0 Surround)"));
	channels_combo->setItemText(2, tr("6 (5.1 Surround)"));
	channels_combo->setItemText(3, tr("7 (6.1 Surround)"));
	channels_combo->setItemText(4, tr("8 (7.1 Surround)"));

	createHelp();
}

void TAudio::setData(TPreferences* pref) {

	setAO(pref->ao, true);
	setAudioChannels(pref->initial_audio_channels);
	setUseAudioEqualizer(pref->use_audio_equalizer);
	setAc3DTSPassthrough(pref->use_hwac3);
	setScaleTempoFilter(pref->use_scaletempo);

	// Volume
	setSoftVol(pref->use_soft_vol);
	setAmplification(pref->softvol_max);
	setInitialVolNorm(pref->initial_volnorm);

	// Synchronization
	setAutoSyncActivated(pref->autosync);
	setAutoSyncFactor(pref->autosync_factor);

	setMcActivated(pref->use_mc);
	setMc(pref->mc_value);

	// Language
	setAudioLang(pref->audio_lang);
}

void TAudio::getData(TPreferences* pref) {

	requires_restart = false;

	restartIfStringChanged(pref->ao, AO());
	restartIfBoolChanged(pref->use_audio_equalizer, useAudioEqualizer());
	restartIfBoolChanged(pref->use_hwac3, Ac3DTSPassthrough());
	pref->initial_audio_channels = audioChannels();
	TPreferences::TOptionState scale = scaleTempoFilter();
	if (scale != pref->use_scaletempo) {
		pref->use_scaletempo = scale;
		requires_restart = true;
	}

	restartIfBoolChanged(pref->use_soft_vol, softVol());
	pref->initial_volnorm = initialVolNorm();
	restartIfIntChanged(pref->softvol_max, amplification());

	restartIfBoolChanged(pref->autosync, autoSyncActivated());
	restartIfIntChanged(pref->autosync_factor, autoSyncFactor());

	restartIfBoolChanged(pref->use_mc, mcActivated());
	restartIfDoubleChanged(pref->mc_value, mc());

	pref->audio_lang = audioLang();
}

void TAudio::updateDriverCombo(bool allow_user_defined_ao) {

	QString current_ao = AO();
	ao_combo->clear();
	ao_combo->addItem(tr("Default"), "");

	QString ao;
	for (int n = 0; n < ao_list.count(); n++) {
		ao = ao_list[n].name();
		ao_combo->addItem(ao, ao);
#ifdef Q_OS_OS2
		if (ao == "kai") {
			ao_combo->addItem("kai (" + tr("uniaud mode") + ")", "kai:uniaud");
			ao_combo->addItem("kai (" + tr("dart mode") + ")", "kai:dart");
		}
#endif
#if USE_ALSA_DEVICES
		if ((ao == "alsa") && (!alsa_devices.isEmpty())) {
			for (int n=0; n < alsa_devices.count(); n++) {
				ao_combo->addItem("alsa (" + alsa_devices[n].ID().toString() + " - " + alsa_devices[n].desc() + ")", 
                                   "alsa:device=hw=" + alsa_devices[n].ID().toString());
			}
		}
#endif
#if USE_DSOUND_DEVICES
		if ((ao == "dsound") && (!dsound_devices.isEmpty())) {
			for (int n=0; n < dsound_devices.count(); n++) {
				ao_combo->addItem("dsound (" + dsound_devices[n].ID().toString() + " - " + dsound_devices[n].desc() + ")", 
                                   "dsound:device=" + dsound_devices[n].ID().toString());
			}
		}
#endif
	}
	ao_combo->addItem(tr("User defined..."), "user_defined");

	setAO(current_ao, allow_user_defined_ao);
}

void TAudio::setAO(const QString& ao_driver, bool allow_user_defined) {

	int idx = ao_combo->findData(ao_driver);
	if (idx >= 0) {
		ao_combo->setCurrentIndex(idx);
	} else if (allow_user_defined && !ao_driver.isEmpty()) {
		ao_combo->setCurrentIndex(ao_combo->findData("user_defined"));
		ao_user_defined_edit->setText(ao_driver);
	} else {
		ao_combo->setCurrentIndex(0);
	}
	ao_combo_changed(ao_combo->currentIndex());
}

QString TAudio::AO() {
	
	QString ao = ao_combo->itemData(ao_combo->currentIndex()).toString();
	if (ao == "user_defined") {
		ao = ao_user_defined_edit->text();
	}
	return ao;
}

void TAudio::setSoftVol(bool b) {
	softvol_check->setChecked(b);
}

bool TAudio::softVol() {
	return softvol_check->isChecked();
}

void TAudio::setAutoSyncFactor(int factor) {
	autosync_spin->setValue(factor);
}

int TAudio::autoSyncFactor() {
	return autosync_spin->value();
}

void TAudio::setAutoSyncActivated(bool b) {
	autosync_check->setChecked(b);
}

bool TAudio::autoSyncActivated() {
	return autosync_check->isChecked();
}

void TAudio::setMc(double value) {
	mc_spin->setValue(value);
}

double TAudio::mc() {
	return mc_spin->value();
}

void TAudio::setMcActivated(bool b) {
	use_mc_check->setChecked(b);
}

bool TAudio::mcActivated() {
	return use_mc_check->isChecked();
}

void TAudio::setUseAudioEqualizer(bool b) {
	audio_equalizer_check->setChecked(b);
}

bool TAudio::useAudioEqualizer() {
	return audio_equalizer_check->isChecked();
}

void TAudio::setAc3DTSPassthrough(bool b) {
	hwac3_check->setChecked(b);
}

bool TAudio::Ac3DTSPassthrough() {
	return hwac3_check->isChecked();
}

void TAudio::setInitialVolNorm(bool b) {
	volnorm_check->setChecked(b);
}

bool TAudio::initialVolNorm() {
	return volnorm_check->isChecked();
}


void TAudio::setAmplification(int n) {
	softvol_amp_spin->setValue(n);
}

int TAudio::amplification() {
	return softvol_amp_spin->value();
}

void TAudio::setAudioChannels(int ID) {

	int pos = channels_combo->findData(ID);
	if (pos >= 0) {
		channels_combo->setCurrentIndex(pos);
	} else {
		qWarning("Gui::Pref::TAudio::setAudioChannels: ID: %d not found in combo", ID);
	}
}

int TAudio::audioChannels() {
	
	if (channels_combo->currentIndex() >= 0) {
		return channels_combo->itemData(channels_combo->currentIndex()).toInt();
	}
	qWarning("Gui::Pref::TAudio::audioChannels: no item selected");
	return 0;
}

void TAudio::setScaleTempoFilter(TPreferences::TOptionState value) {
	scaletempo_combo->setState(value);
}

TPreferences::TOptionState TAudio::scaleTempoFilter() {
	return scaletempo_combo->state();
}

void TAudio::setAudioLang(const QString& lang) {
	language_edit->setText(lang);
}

QString TAudio::audioLang() {
	return language_edit->text();
}


void TAudio::ao_combo_changed(int idx) {
	//qDebug("Gui::Pref::TAudio::ao_combo_changed: %d", idx);

	bool visible = (ao_combo->itemData(idx).toString() == "user_defined");
	ao_user_defined_edit->setVisible(visible);
	ao_user_defined_edit->setFocus();
}

void TAudio::createHelp() {

	clearHelp();

	// Audio tab
	addSectionTitle(tr("Audio"));

	setWhatsThis(ao_combo, tr("Audio output driver"),
		tr("Select the audio output driver.") 
#ifndef Q_OS_WIN
#ifdef Q_OS_OS2
        + " " +
		tr("%1 is the recommended one. %2 is only available on older MPlayer (before version %3)")
           .arg("<b><i>kai</i></b>")
           .arg("<b><i>dart</i></b>")
           .arg(MPLAYER_KAI_VERSION)
#else
        + " " + 
		tr("%1 is the recommended one. Try to avoid %2 and %3, they are slow "
           "and can have an impact on performance.")
           .arg("<b><i>alsa</i></b>")
           .arg("<b><i>esd</i></b>")
           .arg("<b><i>arts</i></b>")
#endif
#endif
		);

	setWhatsThis(channels_combo, tr("Channels"),
		tr("Requests the number of playback channels. MPlayer "
		   "asks the decoder to decode the audio into as many channels as "
		   "specified. Then it is up to the decoder to fulfill the "
		   "requirement. This is usually only important when playing "
		   "videos with AC3 audio (like DVDs). In that case liba52 does "
		   "the decoding by default and correctly downmixes the audio "
		   "into the requested number of channels. "
		   "<b>Note</b>: This option is honored by codecs (AC3 only), "
		   "filters (surround) and audio output drivers (OSS at least)."));

	setWhatsThis(hwac3_check, tr("AC3/DTS pass-through S/PDIF"),
		tr("Uses hardware AC3 passthrough.") + "<br>" +
		tr("<b>Note:</b> none of the audio filters will be used when this "
		   "option is enabled."));

	setWhatsThis(audio_equalizer_check, tr("Enable the audio equalizer"),
		tr("Check this option if you want to use the audio equalizer."));

	setWhatsThis(scaletempo_combo, tr("High speed playback without altering pitch"),
		tr("Allows to change the playback speed without altering pitch. "
           "Requires at least MPlayer dev-SVN-r24924."));

	setWhatsThis(softvol_check, tr("Software volume control"),
		tr("Force using the software mixer instead of the sound card mixer."));

	setWhatsThis(softvol_amp_spin, tr("Amplification"),
		tr("Sets the amplification level in percent.<br>"
		   "100 is no amplification.<br>"
		   "For MPlayer a value of 200 doubles the volume (linear scale).<br>"
		   "For MPV a value of 130 doubles the volume (cubic scale)."));

	setWhatsThis(volnorm_check, tr("Volume normalization by default"),
		tr("Maximizes the volume without distorting the sound."));

	setWhatsThis(autosync_check, tr("Audio/video auto synchronization"),
		tr("Gradually adjusts the A/V sync based on audio delay "
           "measurements."));

	setWhatsThis(mc_spin, tr("A-V sync correction"),
		tr("Maximum A-V sync correction per frame (in seconds)"));

	setWhatsThis(language_edit, tr("Language override"),
		tr("Here you can type your preferred language for the audio streams. "
		   "When a media with multiple audio streams is found, SMPlayer will "
		   "try to use your preferred language.<br>"
		   "This only will work with media that offer info about the language "
		   "of the audio streams, like DVDs or mkv files.<br>"
		   "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
		   "will select the audio track if it matches with <i>es</i>, "
		   "<i>esp</i> or <i>spa</i>."));
}

}} // namespace Gui::Pref

#include "moc_audio.cpp"

/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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
#include <QDebug>
#include "images.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"
#include "settings/paths.h"

#if USE_ALSA_DEVICES
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui { namespace Pref {

TAudio::TAudio(QWidget* parent, InfoList aol)
	: TWidget(parent, 0)
	, ao_list(aol),
	  player_id(pref->player_id),
	  mplayer_ao(pref->mplayer_ao),
	  mpv_ao(pref->mpv_ao)  {

	setupUi(this);

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
			this, SLOT(onAOComboChanged(int)));

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

	updateDriverCombo(player_id, false);

	channels_combo->setItemText(0, tr("2 (Stereo)"));
	channels_combo->setItemText(1, tr("4 (4.0 Surround)"));
	channels_combo->setItemText(2, tr("6 (5.1 Surround)"));
	channels_combo->setItemText(3, tr("7 (6.1 Surround)"));
	channels_combo->setItemText(4, tr("8 (7.1 Surround)"));

	createHelp();
}

void TAudio::setData(TPreferences* pref) {

	player_id = pref->player_id;
	mplayer_ao = pref->mplayer_ao;
	mpv_ao = pref->mpv_ao;
	setAO(pref->ao);

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

    restartIfStringChanged(pref->ao, AO(), "ao");
	if (pref->isMPlayer()) {
		pref->mplayer_ao = pref->ao;
		pref->mpv_ao = mpv_ao;
	} else {
		pref->mplayer_ao = mplayer_ao;
		pref->mpv_ao = pref->ao;
	}

    restartIfBoolChanged(pref->use_audio_equalizer, useAudioEqualizer(),
                         "use_audio_equalizer");
    restartIfBoolChanged(pref->use_hwac3, Ac3DTSPassthrough(), "use_hwac3");
	pref->initial_audio_channels = audioChannels();
	TPreferences::TOptionState scale = scaleTempoFilter();
	if (scale != pref->use_scaletempo) {
        logger()->debug("getData: need restart, use_scaletemp changed from %1"
                        " to %2", QString::number(scale),
                        QString::number(pref->use_scaletempo));
        pref->use_scaletempo = scale;
		requires_restart = true;
	}

    restartIfBoolChanged(pref->use_soft_vol, softVol(), "use_soft_vol");
	pref->initial_volnorm = initialVolNorm();
    restartIfIntChanged(pref->softvol_max, amplification(), "softvol_max");

    restartIfBoolChanged(pref->autosync, autoSyncActivated(), "autosync");
    restartIfIntChanged(pref->autosync_factor, autoSyncFactor(),
                        "autosync_factor");

    restartIfBoolChanged(pref->use_mc, mcActivated(), "use_mc");
    restartIfDoubleChanged(pref->mc_value, mc(), "mc_value");

	pref->audio_lang = audioLang();
}

void TAudio::updateDriverCombo(Settings::TPreferences::TPlayerID player_id,
							   bool keep_current_drivers) {


	this->player_id = player_id;
	QString wanted_ao;
	if (keep_current_drivers) {
		wanted_ao = AO();
	} else if (player_id == TPreferences::ID_MPLAYER) {
		wanted_ao = mplayer_ao;
	} else {
		wanted_ao = mpv_ao;
	}
	ao_combo->clear();
	ao_combo->addItem(tr("players default"), "");

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
	}

	ao_combo->addItem(tr("User defined..."), "user_defined");
	// Set selected AO
	setAO(wanted_ao);
}

void TAudio::setAO(const QString& ao_driver) {

	int idx = ao_combo->findData(ao_driver);
	if (idx >= 0) {
		ao_combo->setCurrentIndex(idx);
	} else {
		ao_combo->setCurrentIndex(ao_combo->findData("user_defined"));
		ao_user_defined_edit->setText(ao_driver);
	}
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

	int i = channels_combo->findData(ID);
	if (i < 0)
		i = 0;
	channels_combo->setCurrentIndex(i);
}

int TAudio::audioChannels() {
	
	int i = channels_combo->currentIndex();
	if (i < 0)
		i = 0;
	return channels_combo->itemData(i).toInt();
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


void TAudio::onAOComboChanged(int idx) {

	// Update VOs
	if (idx >= 0) {
		if (player_id == TPreferences::ID_MPLAYER) {
			mplayer_ao = AO();
		} else {
			mpv_ao = AO();
		}
	}

	// Handle user defined VO
	bool visible = ao_combo->itemData(idx).toString() == "user_defined";
	ao_user_defined_edit->setVisible(visible);
	ao_user_defined_edit->setFocus();
}

void TAudio::createHelp() {

	clearHelp();

	// Audio tab
	addSectionTitle(tr("Audio"));

	setWhatsThis(ao_combo, tr("Audio output driver"),
		tr("Select the audio output driver.") 
#ifdef Q_OS_OS2
        + " " +
		tr("%1 is the recommended one. %2 is only available on older MPlayer (before version %3)")
           .arg("<b><i>kai</i></b>")
           .arg("<b><i>dart</i></b>")
           .arg(MPLAYER_KAI_VERSION)
#else
#ifndef Q_OS_WIN
		+ " " +
		tr("%1 and %2 are the most commonly used drivers.")
		   .arg("<b><i>pulse</i></b>")
		   .arg("<b><i>alsa</i></b>")
#endif
		+ " " +
		tr("Select <b><i>players default</i></b> to let the player select the audio driver.")
#endif
		);

	setWhatsThis(channels_combo, tr("Channels"),
		tr("Requests the number of playback channels. The player will "
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
		tr("<b>Note:</b> audio filters will be disabled when this "
		   "option is enabled."));

	setWhatsThis(audio_equalizer_check, tr("Enable the audio equalizer"),
		tr("Check this option if you want to use the audio equalizer."));

	setWhatsThis(scaletempo_combo, tr("High speed playback without altering pitch"),
		tr("Allows to change the playback speed without altering pitch. "
           "Requires at least MPlayer dev-SVN-r24924."));

	setWhatsThis(softvol_check, tr("Software volume control"),
		tr("Use the software mixer instead of the sound card mixer and amplify"
		   " the volume by the factor given by <b>Amplification</b>"));

	setWhatsThis(softvol_amp_spin, tr("Amplification"),
		tr("Sets the amplification level.<br>"
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

	setWhatsThis(language_edit, tr("Language"),
		tr("Here you can select your preferred language for the audio streams. "
		   "When media has multiple audio streams, WZPlayer will "
		   "try to use your preferred language.<br>"
		   "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
		   "will select the audio track if it matches with <i>es</i>, "
		   "<i>esp</i> or <i>spa</i>.") + "<br><br>"
		+ tr("<b>Note:</b> WZPlayer will select the first matching track,"
			 " which might not be the best track for your setup."
			 " Normally the player will already select tracks in the language"
			 " of your system and overriding it should not be needed."));
}

}} // namespace Gui::Pref

#include "moc_audio.cpp"

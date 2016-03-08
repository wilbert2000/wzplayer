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


#include "gui/pref/general.h"
#include "gui/pref/vdpauproperties.h"
#include "settings/preferences.h"
#include "filedialog.h"
#include "images.h"
#include "settings/mediasettings.h"
#include "settings/paths.h"

#if USE_ALSA_DEVICES || USE_DSOUND_DEVICES
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui { namespace Pref {

TGeneral::TGeneral(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);

	// General tab
	playerbin_edit->setDialogType(FileChooser::GetFileName);
	screenshot_edit->setDialogType(FileChooser::GetDirectory);

	// Get VO and AO driver lists from InfoReader:
	InfoReader* i = InfoReader::obj();
	i->getInfo();
	vo_list = i->voList();
	ao_list = i->aoList();

	// Screenshots
#ifdef MPV_SUPPORT
	screenshot_format_combo->addItems(QStringList() << "png" << "ppm" << "pgm" << "pgmyuv" << "tga" << "jpg" << "jpeg");
#else
	screenshot_template_label->hide();
	screenshot_template_edit->hide();
	screenshot_format_label->hide();
	screenshot_format_combo->hide();
#endif


	// Video tab
#if USE_XV_ADAPTORS
	xv_adaptors = TDeviceInfo::xvAdaptors();
#endif

	// Hardware decoding combo
	hwdec_combo->addItem(tr("None"), "no");
	hwdec_combo->addItem(tr("Auto"), "auto");

#ifdef Q_OS_LINUX
	hwdec_combo->addItem("vdpau", "vdpau");
	hwdec_combo->addItem("vaapi", "vaapi");
	hwdec_combo->addItem("vaapi-copy", "vaapi-copy");
#endif

#ifdef Q_OS_OSX
	hwdec_combo->addItem("vda", "vda");
#endif

#ifdef Q_OS_WIN
	hwdec_combo->addItem("dxva2-copy", "dxva2-copy");
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	vdpau_button->hide();
#endif


	// Audio tab
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

	connect(vo_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(vo_combo_changed(int)));
	connect(ao_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(ao_combo_changed(int)));

	retranslateStrings();
}

TGeneral::~TGeneral() {
}

QString TGeneral::sectionName() {
	return tr("General");
}

QPixmap TGeneral::sectionIcon() {
	return Images::icon("pref_general", 22);
}

void TGeneral::retranslateStrings() {

	retranslateUi(this);

	playerbin_edit->setCaption(tr("Select the player executable"));

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	playerbin_edit->setFilter(tr("Executables") +" (*.exe)");
#else
	playerbin_edit->setFilter(tr("All files") +" (*)");
#endif

	int filesettings_method_item = filesettings_method_combo->currentIndex();
	filesettings_method_combo->clear();
	filesettings_method_combo->addItem(tr("one ini file"), "normal");
	filesettings_method_combo->addItem(tr("multiple ini files"), "hash");
	filesettings_method_combo->setCurrentIndex(filesettings_method_item);

	screenshot_edit->setCaption(tr("Select a directory"));

	updateDriverCombos();

	int deinterlace_item = deinterlace_combo->currentIndex();
	deinterlace_combo->clear();
	deinterlace_combo->addItem(tr("None"), TMediaSettings::NoDeinterlace);
	deinterlace_combo->addItem(tr("Lowpass5"), TMediaSettings::L5);
	deinterlace_combo->addItem(tr("Yadif (normal)"), TMediaSettings::Yadif);
	deinterlace_combo->addItem(tr("Yadif (double framerate)"), TMediaSettings::Yadif_1);
	deinterlace_combo->addItem(tr("Linear Blend"), TMediaSettings::LB);
	deinterlace_combo->addItem(tr("Kerndeint"), TMediaSettings::Kerndeint);
	deinterlace_combo->setCurrentIndex(deinterlace_item);

	channels_combo->setItemText(0, tr("2 (Stereo)"));
	channels_combo->setItemText(1, tr("4 (4.0 Surround)"));
	channels_combo->setItemText(2, tr("6 (5.1 Surround)"));
	channels_combo->setItemText(3, tr("7 (6.1 Surround)"));
	channels_combo->setItemText(4, tr("8 (7.1 Surround)"));

	preferred_desc->setText(
		tr("Here you can type your preferred language for the audio "
		   "and subtitle streams. When multiple audio or subtitle streams "
		   "are found, SMPlayer will try to use your preferred language. "
		   "This only will work with media that offer info about the language "
		   "of audio and subtitle streams, like DVDs or mkv files.<br>"
		   "These fields accept regular expressions. "
           "Example: <b>es|esp|spa</b> will select the track if it matches with "
            "<i>es</i>, <i>esp</i> or <i>spa</i>."));

	createHelp();
}

void TGeneral::setData(TPreferences* pref) {

	// General tab
	setPlayerPath(pref->player_bin);

	// Media settings group
	setRememberSettings(pref->remember_media_settings);
	setRememberTimePos(!pref->remember_time_pos);
	setFileSettingsMethod(pref->file_settings_method);

	// Screenshots group
	setUseScreenshots(pref->use_screenshot);
	setScreenshotDir(pref->screenshot_directory);

#ifdef MPV_SUPPORT
	screenshot_template_edit->setText(pref->screenshot_template);
	setScreenshotFormat(pref->screenshot_format);
#endif

	setPauseWhenHidden(pref->pause_when_hidden);
	setCloseOnFinish(pref->close_on_finish);


	// Video tab
	// Video out driver
	QString vo = pref->vo;
	if (vo.isEmpty()) {

#ifdef Q_OS_WIN
		if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA) {
			vo = "direct3d,";
		} else {
			vo = "directx,";
		}
#else
#ifdef Q_OS_OS2
		vo = "kva";
#else
		vo = "xv,";
#endif
#endif

	} // if (vo.isEmpty())
	setVO(vo);

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	vdpau = pref->vdpau;
#endif

	setHwdec(pref->hwdec);
	setFrameDrop(pref->frame_drop);
	setHardFrameDrop(pref->hard_frame_drop);
	setSoftwareVideoEqualizer(pref->use_soft_video_eq);
	setInitialPostprocessing(pref->initial_postprocessing);
	setPostprocessingQuality(pref->postprocessing_quality);
	setInitialDeinterlace(pref->initial_deinterlace);
	setInitialZoom(pref->initial_zoom_factor);

	setStartInFullscreen(pref->start_in_fullscreen);
	setBlackbordersOnFullscreen(pref->add_blackborders_on_fullscreen);

	// Audio tab
	QString ao = pref->ao;

#ifdef Q_OS_OS2
	if (ao.isEmpty()) {
		ao = "kai";
	}
#endif

	setAO(ao);
	setUseAudioEqualizer(pref->use_audio_equalizer);
	global_audio_equalizer_check->setChecked(pref->global_audio_equalizer);
	setAc3DTSPassthrough(pref->use_hwac3);
	setAudioChannels(pref->initial_audio_channels);
	setScaleTempoFilter(pref->use_scaletempo);

	// Volume
	setGlobalVolume(pref->global_volume);
	setSoftVol(pref->use_soft_vol);
	setAmplification(pref->softvol_max);
	setInitialVolNorm(pref->initial_volnorm);

	// Synchronization
	setAutoSyncActivated(pref->autosync);
	setAutoSyncFactor(pref->autosync_factor);

	setMcActivated(pref->use_mc);
	setMc(pref->mc_value);


	// Preferred tab
	setAudioLang(pref->audio_lang);
	setSubtitleLang(pref->subtitle_lang);

	setAudioTrack(pref->initial_audio_track);
	setSubtitleTrack(pref->initial_subtitle_track);
}

void TGeneral::getData(TPreferences* pref) {

	requires_restart = false;
	filesettings_method_changed = false;


	// General tab
	if (pref->player_bin != playerPath()) {
		requires_restart = true;
		pref->player_bin = playerPath();
		pref->setPlayerBin();

		qDebug("Gui::Pref::TGeneral::getData: mplayer binary has changed, getting version number");
		// Forces to get info from mplayer to update version number
		InfoReader* i = InfoReader::obj();
		i->getInfo();
		// Update the drivers list at the same time
		vo_list = i->voList();
		ao_list = i->aoList();
		updateDriverCombos();
	}

	bool remember_ms = rememberSettings();
	TEST_AND_SET(pref->remember_media_settings, remember_ms);
	bool remember_time = rememberTimePos();
	TEST_AND_SET(pref->remember_time_pos, remember_time);
	if (pref->file_settings_method != fileSettingsMethod()) {
		pref->file_settings_method = fileSettingsMethod();
		filesettings_method_changed = true;
	}

	// Screenshots
	TEST_AND_SET(pref->use_screenshot, useScreenshots());
	TEST_AND_SET(pref->screenshot_directory, screenshotDir());
#ifdef MPV_SUPPORT
	TEST_AND_SET(pref->screenshot_template, screenshot_template_edit->text());
	TEST_AND_SET(pref->screenshot_format, screenshotFormat());
#endif

	pref->close_on_finish = closeOnFinish();
	pref->pause_when_hidden = pauseWhenHidden();


	// Video tab
	TEST_AND_SET(pref->vo, VO());

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	pref->vdpau = vdpau;
#endif

	TEST_AND_SET(pref->hwdec, hwdec());
	TEST_AND_SET(pref->frame_drop, frameDrop());
	TEST_AND_SET(pref->hard_frame_drop, hardFrameDrop());
	TEST_AND_SET(pref->use_soft_video_eq, softwareVideoEqualizer());
	pref->initial_postprocessing = initialPostprocessing();
	TEST_AND_SET(pref->postprocessing_quality, postprocessingQuality());
	pref->initial_deinterlace = initialDeinterlace();
	pref->initial_zoom_factor = initialZoom();

	pref->start_in_fullscreen = startInFullscreen();
	if (pref->add_blackborders_on_fullscreen != blackbordersOnFullscreen()) {
		pref->add_blackborders_on_fullscreen = blackbordersOnFullscreen();
		if (pref->fullscreen) requires_restart = true;
	}


	// Audio tab
	TEST_AND_SET(pref->ao, AO());
	TEST_AND_SET(pref->use_audio_equalizer, useAudioEqualizer());
	pref->global_audio_equalizer = global_audio_equalizer_check->isChecked();
	TEST_AND_SET(pref->use_hwac3, Ac3DTSPassthrough());
	pref->initial_audio_channels = audioChannels();
	TEST_AND_SET(pref->use_scaletempo, scaleTempoFilter());

	pref->global_volume = globalVolume();
	TEST_AND_SET(pref->use_soft_vol, softVol());
	pref->initial_volnorm = initialVolNorm();
	TEST_AND_SET(pref->softvol_max, amplification());


	TEST_AND_SET(pref->autosync, autoSyncActivated());
	TEST_AND_SET(pref->autosync_factor, autoSyncFactor());

	TEST_AND_SET(pref->use_mc, mcActivated());
	TEST_AND_SET(pref->mc_value, mc());


	// Preferred tab
	pref->audio_lang = audioLang();
	pref->subtitle_lang = subtitleLang();

	pref->initial_audio_track = audioTrack();
	pref->initial_subtitle_track = subtitleTrack();
}

void TGeneral::updateDriverCombos() {

	QString current_vo = VO();
	QString current_ao = AO();

	vo_combo->clear();
	ao_combo->clear();

	vo_combo->addItem(tr("Default"), "player_default");
	ao_combo->addItem(tr("Default"), "player_default");

	QString vo;
	for (int n = 0; n < vo_list.count(); n++) {
		vo = vo_list[n].name();
#ifdef Q_OS_WIN
		if (vo == "directx") {
			vo_combo->addItem("directx (" + tr("fast") + ")", "directx");
			vo_combo->addItem("directx (" + tr("slow") + ")", "directx:noaccel");
		}
		else
#else
#ifdef Q_OS_OS2
		if (vo == "kva") {
			vo_combo->addItem("kva (" + tr("fast") + ")", "kva");
			vo_combo->addItem("kva (" + tr("snap mode") + ")", "kva:snap");
			vo_combo->addItem("kva (" + tr("slower dive mode") + ")", "kva:dive");
		}
		else
#else
		/*
		if (vo == "xv") vo_combo->addItem("xv (" + tr("fastest") + ")", vo);
		else
		*/
#if USE_XV_ADAPTORS
		if ((vo == "xv") && (!xv_adaptors.isEmpty())) {
			vo_combo->addItem(vo, vo);
			for (int n=0; n < xv_adaptors.count(); n++) {
				vo_combo->addItem("xv (" + xv_adaptors[n].ID().toString() + " - " + xv_adaptors[n].desc() + ")", 
                                   "xv:adaptor=" + xv_adaptors[n].ID().toString());
			}
		}
		else
#endif // USE_XV_ADAPTORS
#endif
#endif
		if (vo == "x11") vo_combo->addItem("x11 (" + tr("slow") + ")", vo);
		else
		if (vo == "gl") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl (" + tr("fast") + ")", "gl:yuv=2:force-pbo");
			vo_combo->addItem("gl (" + tr("fast - ATI cards") + ")", "gl:yuv=2:force-pbo:ati-hack");
			vo_combo->addItem("gl (yuv)", "gl:yuv=3");
		}
		else
		if (vo == "gl2") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl2 (yuv)", "gl2:yuv=3");
		}
		else
		if (vo == "gl_tiled") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl_tiled (yuv)", "gl_tiled:yuv=3");
		}
		else
		if (vo == "null" || vo == "png" || vo == "jpeg" || vo == "gif89a" || 
            vo == "tga" || vo == "pnm" || vo == "md5sum") 
		{
			; // Nothing to do
		}
		else
		vo_combo->addItem(vo, vo);
	}
	vo_combo->addItem(tr("User defined..."), "user_defined");

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

	setVO(current_vo);
	setAO(current_ao);
}

void TGeneral::setPlayerPath(const QString& path) {
	playerbin_edit->setText(path);
}

QString TGeneral::playerPath() {
	return playerbin_edit->text();
}

void TGeneral::setUseScreenshots(bool b) {
	use_screenshots_check->setChecked(b);
}

bool TGeneral::useScreenshots() {
	return use_screenshots_check->isChecked();
}

void TGeneral::setScreenshotDir(const QString& path) {
	screenshot_edit->setText(path);
}

QString TGeneral::screenshotDir() {
	return screenshot_edit->text();
}

#ifdef MPV_SUPPORT
void TGeneral::setScreenshotFormat(const QString& format) {

	int i = screenshot_format_combo->findText(format);
	if (i < 0)
		i = 0;
	screenshot_format_combo->setCurrentIndex(i);
}

QString TGeneral::screenshotFormat() {
	return screenshot_format_combo->currentText();
}
#endif

void TGeneral::setVO(const QString& vo_driver) {

	int idx = vo_combo->findData(vo_driver);
	if (idx != -1) {
		vo_combo->setCurrentIndex(idx);
	} else {
		vo_combo->setCurrentIndex(vo_combo->findData("user_defined"));
		vo_user_defined_edit->setText(vo_driver);
	}
	vo_combo_changed(vo_combo->currentIndex());
}

void TGeneral::setAO(const QString& ao_driver) {

	int idx = ao_combo->findData(ao_driver);
	if (idx != -1) {
		ao_combo->setCurrentIndex(idx);
	} else {
		ao_combo->setCurrentIndex(ao_combo->findData("user_defined"));
		ao_user_defined_edit->setText(ao_driver);
	}
	ao_combo_changed(ao_combo->currentIndex());
}

QString TGeneral::VO() {
	QString vo = vo_combo->itemData(vo_combo->currentIndex()).toString();
	if (vo == "user_defined") {
		vo = vo_user_defined_edit->text();
		/*
		if (vo.isEmpty()) {
			vo = vo_combo->itemData(0).toString();
			qDebug("Gui::Pref::TGeneral::VO: user defined vo is empty, using %s", vo.toUtf8().constData());
		}
		*/
	}
	return vo;
}

QString TGeneral::AO() {
	QString ao = ao_combo->itemData(ao_combo->currentIndex()).toString();
	if (ao == "user_defined") {
		ao = ao_user_defined_edit->text();
		/*
		if (ao.isEmpty()) {
			ao = ao_combo->itemData(0).toString();
			qDebug("Gui::Pref::TGeneral::AO: user defined ao is empty, using %s", ao.toUtf8().constData());
		}
		*/
	}
	return ao;
}

void TGeneral::setHwdec(const QString & v) {
	int idx = hwdec_combo->findData(v);
	if (idx < 0) idx = 0;
	hwdec_combo->setCurrentIndex(idx);
}

QString TGeneral::hwdec() {
	int idx = hwdec_combo->currentIndex();
	return hwdec_combo->itemData(idx).toString();
}

void TGeneral::setFrameDrop(bool b) {
	framedrop_check->setChecked(b);
}

bool TGeneral::frameDrop() {
	return framedrop_check->isChecked();
}

void TGeneral::setHardFrameDrop(bool b) {
	hardframedrop_check->setChecked(b);
}

bool TGeneral::hardFrameDrop() {
	return hardframedrop_check->isChecked();
}

void TGeneral::setRememberSettings(bool b) {
	remember_all_check->setChecked(b);
	//rememberAllButtonToggled(b);
}

bool TGeneral::rememberSettings() {
	return remember_all_check->isChecked();
}

void TGeneral::setRememberTimePos(bool b) {
	remember_time_check->setChecked(b);
}

bool TGeneral::rememberTimePos() {
	return remember_time_check->isChecked();
}

void TGeneral::setFileSettingsMethod(const QString& method) {

	int index = filesettings_method_combo->findData(method);
	if (index < 0)
		index = 0;
	filesettings_method_combo->setCurrentIndex(index);
}

QString TGeneral::fileSettingsMethod() {
	return filesettings_method_combo->itemData(filesettings_method_combo->currentIndex()).toString();
}

void TGeneral::setAudioLang(const QString& lang) {
	audio_lang_edit->setText(lang);
}

QString TGeneral::audioLang() {
	return audio_lang_edit->text();
}

void TGeneral::setSubtitleLang(const QString& lang) {
	subtitle_lang_edit->setText(lang);
}

QString TGeneral::subtitleLang() {
	return subtitle_lang_edit->text();
}

void TGeneral::setAudioTrack(int track) {
	audio_track_spin->setValue(track);
}

int TGeneral::audioTrack() {
	return audio_track_spin->value();
}

void TGeneral::setSubtitleTrack(int track) {
	subtitle_track_spin->setValue(track);
}

int TGeneral::subtitleTrack() {
	return subtitle_track_spin->value();
}

void TGeneral::setCloseOnFinish(bool b) {
	close_on_finish_check->setChecked(b);
}

bool TGeneral::closeOnFinish() {
	return close_on_finish_check->isChecked();
}

void TGeneral::setPauseWhenHidden(bool b) {
	pause_if_hidden_check->setChecked(b);
}

bool TGeneral::pauseWhenHidden() {
	return pause_if_hidden_check->isChecked();
}

void TGeneral::setSoftwareVideoEqualizer(bool b) {
	software_video_equalizer_check->setChecked(b);
}

bool TGeneral::softwareVideoEqualizer() {
	return software_video_equalizer_check->isChecked();
}

void TGeneral::setSoftVol(bool b) {
	softvol_check->setChecked(b);
}

void TGeneral::setGlobalVolume(bool b) {
	global_volume_check->setChecked(b);
}

bool TGeneral::globalVolume() {
	return global_volume_check->isChecked();
}

bool TGeneral::softVol() {
	return softvol_check->isChecked();
}

void TGeneral::setAutoSyncFactor(int factor) {
	autosync_spin->setValue(factor);
}

int TGeneral::autoSyncFactor() {
	return autosync_spin->value();
}

void TGeneral::setAutoSyncActivated(bool b) {
	autosync_check->setChecked(b);
}

bool TGeneral::autoSyncActivated() {
	return autosync_check->isChecked();
}

void TGeneral::setMc(double value) {
	mc_spin->setValue(value);
}

double TGeneral::mc() {
	return mc_spin->value();
}

void TGeneral::setMcActivated(bool b) {
	use_mc_check->setChecked(b);
}

bool TGeneral::mcActivated() {
	return use_mc_check->isChecked();
}

void TGeneral::setUseAudioEqualizer(bool b) {
	audio_equalizer_check->setChecked(b);
}

bool TGeneral::useAudioEqualizer() {
	return audio_equalizer_check->isChecked();
}

void TGeneral::setAc3DTSPassthrough(bool b) {
	hwac3_check->setChecked(b);
}

bool TGeneral::Ac3DTSPassthrough() {
	return hwac3_check->isChecked();
}

void TGeneral::setInitialVolNorm(bool b) {
	volnorm_check->setChecked(b);
}

bool TGeneral::initialVolNorm() {
	return volnorm_check->isChecked();
}

void TGeneral::setInitialPostprocessing(bool b) {
	postprocessing_check->setChecked(b);
}

bool TGeneral::initialPostprocessing() {
	return postprocessing_check->isChecked();
}

void TGeneral::setInitialDeinterlace(int ID) {
	int pos = deinterlace_combo->findData(ID);
	if (pos != -1) {
		deinterlace_combo->setCurrentIndex(pos);
	} else {
		qWarning("Gui::Pref::TGeneral::setInitialDeinterlace: ID: %d not found in combo", ID);
	}
}

int TGeneral::initialDeinterlace() {
	if (deinterlace_combo->currentIndex() != -1) {
		return deinterlace_combo->itemData(deinterlace_combo->currentIndex()).toInt();
	} else {
		qWarning("Gui::Pref::TGeneral::initialDeinterlace: no item selected");
		return 0;
	}
}

void TGeneral::setInitialZoom(double v) {
	zoom_spin->setValue(v);
}

double TGeneral::initialZoom() {
	return zoom_spin->value();
}

void TGeneral::setAmplification(int n) {
	softvol_amp_spin->setValue(n);
}

int TGeneral::amplification() {
	return softvol_amp_spin->value();
}

void TGeneral::setAudioChannels(int ID) {
	int pos = channels_combo->findData(ID);
	if (pos != -1) {
		channels_combo->setCurrentIndex(pos);
	} else {
		qWarning("Gui::Pref::TGeneral::setAudioChannels: ID: %d not found in combo", ID);
	}
}

int TGeneral::audioChannels() {
	if (channels_combo->currentIndex() != -1) {
		return channels_combo->itemData(channels_combo->currentIndex()).toInt();
	} else {
		qWarning("Gui::Pref::TGeneral::audioChannels: no item selected");
		return 0;
	}
}

void TGeneral::setStartInFullscreen(bool b) {
	start_fullscreen_check->setChecked(b);
}

bool TGeneral::startInFullscreen() {
	return start_fullscreen_check->isChecked();
}


void TGeneral::setBlackbordersOnFullscreen(bool b) {
	blackborders_on_fs_check->setChecked(b);
}

bool TGeneral::blackbordersOnFullscreen() {
	return blackborders_on_fs_check->isChecked();
}

void TGeneral::setPostprocessingQuality(int n) {
	postprocessing_quality_spin->setValue(n);
}

int TGeneral::postprocessingQuality() {
	return postprocessing_quality_spin->value();
}

void TGeneral::setScaleTempoFilter(TPreferences::TOptionState value) {
	scaletempo_combo->setState(value);
}

TPreferences::TOptionState TGeneral::scaleTempoFilter() {
	return scaletempo_combo->state();
}

void TGeneral::vo_combo_changed(int idx) {
	//qDebug("Gui::Pref::TGeneral::vo_combo_changed: %d", idx);

	bool visible = (vo_combo->itemData(idx).toString() == "user_defined");
	vo_user_defined_edit->setVisible(visible);
	vo_user_defined_edit->setFocus();

#ifndef Q_OS_WIN
	bool vdpau_button_visible = (vo_combo->itemData(idx).toString() == "vdpau");
	vdpau_button->setVisible(vdpau_button_visible);
#endif
}

void TGeneral::ao_combo_changed(int idx) {
	//qDebug("Gui::Pref::TGeneral::ao_combo_changed: %d", idx);

	bool visible = (ao_combo->itemData(idx).toString() == "user_defined");
	ao_user_defined_edit->setVisible(visible);
	ao_user_defined_edit->setFocus();
}

#ifndef Q_OS_WIN
void TGeneral::on_vdpau_button_clicked() {
	qDebug("Gui::Pref::TGeneral::on_vdpau_button_clicked");

	TVDPAUProperties d(this);

	d.setffh264vdpau(vdpau.ffh264vdpau);
	d.setffmpeg12vdpau(vdpau.ffmpeg12vdpau);
	d.setffwmv3vdpau(vdpau.ffwmv3vdpau);
	d.setffvc1vdpau(vdpau.ffvc1vdpau);
	d.setffodivxvdpau(vdpau.ffodivxvdpau);

	d.setDisableFilters(vdpau.disable_video_filters);

	if (d.exec() == QDialog::Accepted) {
		vdpau.ffh264vdpau = d.ffh264vdpau();
		vdpau.ffmpeg12vdpau = d.ffmpeg12vdpau();
		vdpau.ffwmv3vdpau = d.ffwmv3vdpau();
		vdpau.ffvc1vdpau = d.ffvc1vdpau();
		vdpau.ffodivxvdpau = d.ffodivxvdpau();

		vdpau.disable_video_filters = d.disableFilters();
	}
}
#endif

void TGeneral::createHelp() {
	clearHelp();

	addSectionTitle(tr("General"));

	setWhatsThis(playerbin_edit, tr("%1 executable").arg(pref->playerName()),
		tr("Here you must specify the %1 "
           "executable that SMPlayer will use.").arg(pref->playerName()) + "<br><b>" +
        tr("If this setting is wrong, SMPlayer won't be able to play "
           "anything!") + "</b>");

	setWhatsThis(remember_all_check, tr("Remember settings"),
		tr("Usually SMPlayer will remember the settings for each file you "
           "play (audio track selected, volume, filters...). Disable this "
           "option if you don't like this feature."));

	setWhatsThis(remember_time_check, tr("Remember time position"),
		tr("If you check this option, SMPlayer will remember the last position "
           "of the file when you open it again. This option works only with "
           "regular files (not with DVDs, CDs, URLs...)."));

	setWhatsThis(filesettings_method_combo, tr("Method to store the file settings"),
		tr("This option allows to change the way the file settings would be "
           "stored. The following options are available:") +"<ul><li>" + 
		tr("<b>one ini file</b>: the settings for all played files will be "
           "saved in a single ini file (%1)").arg(QString("<i>"+TPaths::iniPath()+"/smplayer.ini</i>")) + "</li><li>" +
		tr("<b>multiple ini files</b>: one ini file will be used for each played file. "
           "Those ini files will be saved in the folder %1").arg(QString("<i>"+TPaths::iniPath()+"/file_settings</i>")) + "</li></ul>" +
		tr("The latter method could be faster if there is info for a lot of files."));

	setWhatsThis(use_screenshots_check, tr("Enable screenshots"),
		tr("You can use this option to enable or disable the possibility to "
           "take screenshots."));

	setWhatsThis(screenshot_edit, tr("Screenshots folder"),
		tr("Here you can specify a folder where the screenshots taken by "
           "SMPlayer will be stored. If the folder is not valid the "
           "screenshot feature will be disabled."));

#ifdef MPV_SUPPORT
	setWhatsThis(screenshot_template_edit, tr("Template for screenshots"),
		tr("This option specifies the filename template used to save screenshots.") + " " +
		tr("For example %1 would save the screenshot as 'moviename_0001.png'.").arg("%F_%04n") + "<br>" +
		tr("%1 specifies the filename of the video without the extension, "
		   "%2 adds a 4 digit number padded with zeros.").arg("%F").arg("%04n") + " " +
		tr("For a full list of the template specifiers visit this link:") + 
		" <a href=\"http://mpv.io/manual/stable/#options-screenshot-template\">"
		"http://mpv.io/manual/stable/#options-screenshot-template</a>" + "<br>" +
		tr("This option only works with mpv."));

	setWhatsThis(screenshot_format_combo, tr("Format for screenshots"),
		tr("This option allows to choose the image file type used for saving screenshots.") + " " +
		tr("This option only works with mpv.") );
#endif

	setWhatsThis(pause_if_hidden_check, tr("Pause when minimized"),
		tr("If this option is enabled, the file will be paused when the "
           "main window is hidden. When the window is restored, playback "
           "will be resumed."));

	setWhatsThis(close_on_finish_check, tr("Close when finished"),
		tr("If this option is checked, the main window will be automatically "
		   "closed when the current file/playlist finishes."));

	// Video tab
	addSectionTitle(tr("Video"));

	setWhatsThis(vo_combo, tr("Video output driver"),
		tr("Select the video output driver. %1 provides the best performance.")
#ifdef Q_OS_WIN
		  .arg("<b><i>directx</i></b>")
#else
#ifdef Q_OS_OS2
		  .arg("<b><i>kva</i></b>")
#else
		  .arg("<b><i>xv</i></b>")
#endif
#endif
		);

	setWhatsThis(hwdec_combo, tr("Hardware decoding"),
		tr("Sets the hardware video decoding API."
		   "If hardware decoding is not possible, software decoding will be used instead.")
			+ " "
			+ tr("Available options:")
			+ "<ul>"
			"<li>" + tr("None: only software decoding will be used.") + "</li>"
			"<li>" + tr("Auto: tries to automatically enable hardware decoding.") + "</li>"

#ifdef Q_OS_LINUX
			"<li>" + tr("vdpau: for the vdpau and opengl video outputs.") + "</li>"
			"<li>" + tr("vaapi: for the opengl and vaapi video outputs. For Intel GPUs only.") + "</li>"
			"<li>" + tr("vaapi-copy: it copies video back into system RAM. For Intel GPUs only.") + "</li>"
#endif

#ifdef Q_OS_WIN
			"<li>" + tr("dxva2-copy: copies video back to system RAM.") + "</li>"
#endif

			"</ul>"
			+ tr("This option only works with mpv."));

	setWhatsThis(framedrop_check, tr("Allow frame drop"),
		tr("Skip displaying some frames to maintain A/V sync on slow systems."));

	setWhatsThis(hardframedrop_check, tr("Allow hard frame drop"),
		tr("More intense frame dropping (breaks decoding). "
		   "Leads to image distortion!"));

	setWhatsThis(postprocessing_check, tr("Enable postprocessing by default"),
		tr("Postprocessing will be used by default on new opened files."));

	setWhatsThis(postprocessing_quality_spin, tr("Postprocessing quality"),
		tr("Dynamically changes the level of postprocessing depending on the "
           "available spare CPU time. The number you specify will be the "
           "maximum level used. Usually you can use some big number."));

	setWhatsThis(deinterlace_combo, tr("Deinterlace by default"),
        tr("Select the deinterlace filter that you want to be used for new "
           "videos opened.") +" "+ 
        tr("<b>Note:</b> This option won't be used for TV channels."));

	setWhatsThis(zoom_spin, tr("Default zoom"),
		tr("This option sets the default zoom which will be used for "
           "new videos."));

	setWhatsThis(software_video_equalizer_check, tr("Software video equalizer"),
		tr("You can check this option if video equalizing is not supported by "
           "your graphic card or the selected video output driver.<br>"
           "<b>Note:</b> this option can be incompatible with some video "
           "output drivers."));

	setWhatsThis(start_fullscreen_check, tr("Start videos in fullscreen"),
		tr("If this option is checked, all videos will start to play in "
           "fullscreen mode."));

	setWhatsThis(blackborders_on_fs_check, tr("Add black borders on fullscreen"),
		tr("If this option is enabled, black borders will be added to the "
           "image in fullscreen mode. This allows subtitles to be displayed "
           "on the black borders.") /* + "<br>" +
 		tr("This option will be ignored if MPlayer uses its own window, as "
           "some video drivers (like gl) are already able to display the "
           "subtitles automatically in the black borders.") */);


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

	setWhatsThis(audio_equalizer_check, tr("Enable the audio equalizer"),
		tr("Check this option if you want to use the audio equalizer."));

	setWhatsThis(global_audio_equalizer_check, tr("Global audio equalizer"),
		tr("If this option is checked, all media files share the audio equalizer.") +" "+
		tr("If it's not checked, the audio equalizer values are saved along each file "
           "and loaded back when the file is played later."));

	setWhatsThis(hwac3_check, tr("AC3/DTS pass-through S/PDIF"),
		tr("Uses hardware AC3 passthrough.") + "<br>" +
        tr("<b>Note:</b> none of the audio filters will be used when this "
           "option is enabled."));

	setWhatsThis(channels_combo, tr("Channels by default"),
		tr("Requests the number of playback channels. MPlayer "
           "asks the decoder to decode the audio into as many channels as "
           "specified. Then it is up to the decoder to fulfill the "
           "requirement. This is usually only important when playing "
           "videos with AC3 audio (like DVDs). In that case liba52 does "
           "the decoding by default and correctly downmixes the audio "
           "into the requested number of channels. "
           "<b>Note</b>: This option is honored by codecs (AC3 only), "
           "filters (surround) and audio output drivers (OSS at least)."));

	setWhatsThis(scaletempo_combo, tr("High speed playback without altering pitch"),
		tr("Allows to change the playback speed without altering pitch. "
           "Requires at least MPlayer dev-SVN-r24924."));

	setWhatsThis(global_volume_check, tr("Global volume"),
		tr("If this option is checked, the same volume will be used for "
           "all files you play. If the option is not checked each "
           "file uses its own volume.") + "<br>" +
        tr("This option also applies for the mute control."));

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

	addSectionTitle(tr("Preferred audio and subtitles"));

	setWhatsThis(audio_lang_edit, tr("Preferred audio language"),
		tr("Here you can type your preferred language for the audio streams. "
           "When a media with multiple audio streams is found, SMPlayer will "
           "try to use your preferred language.<br>"
           "This only will work with media that offer info about the language "
           "of the audio streams, like DVDs or mkv files.<br>"
           "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
           "will select the audio track if it matches with <i>es</i>, "
           "<i>esp</i> or <i>spa</i>."));

	setWhatsThis(subtitle_lang_edit, tr("Preferred subtitle language"),
		tr("Here you can type your preferred language for the subtitle stream. "
           "When a media with multiple subtitle streams is found, SMPlayer will "
           "try to use your preferred language.<br>"
           "This only will work with media that offer info about the language "
           "of the subtitle streams, like DVDs or mkv files.<br>"
           "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
           "will select the subtitle stream if it matches with <i>es</i>, "
           "<i>esp</i> or <i>spa</i>."));

	setWhatsThis(audio_track_spin, tr("Audio track"),
		tr("Specifies the default audio track which will be used when playing "
           "new files. If the track doesn't exist, the first one will be used. "
           "<br><b>Note:</b> the <i>\"preferred audio language\"</i> has "
           "preference over this option."));

	setWhatsThis(subtitle_track_spin, tr("Subtitle track"),
		tr("Specifies the default subtitle track which will be used when "
           "playing new files. If the track doesn't exist, the first one "
           "will be used. <br><b>Note:</b> the <i>\"preferred subtitle "
           "language\"</i> has preference over this option."));

}

}} // namespace Gui::Pref

#include "moc_general.cpp"

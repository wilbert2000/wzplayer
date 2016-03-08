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

#ifndef GUI_PREF_GENERAL_H
#define GUI_PREF_GENERAL_H

#include "ui_general.h"
#include "gui/pref/widget.h"
#include "inforeader.h"
#include "settings/preferences.h"
#include "gui/deviceinfo.h"

#ifdef Q_OS_WIN
#define USE_DSOUND_DEVICES 1
#else
#define USE_ALSA_DEVICES 1
#define USE_XV_ADAPTORS 1
#endif

#ifdef Q_OS_OS2
#define MPLAYER_KAI_VERSION 30994
#endif


namespace Gui {
namespace Pref {

class TGeneral : public TWidget, public Ui::TGeneral {
	Q_OBJECT

public:
	TGeneral(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TGeneral();

	// Return the name of the section
	virtual QString sectionName();
	// Return the icon of the section
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

	bool fileSettingsMethodChanged() { return filesettings_method_changed; }

protected:
	virtual void createHelp();

	// Tab General
	void setPlayerPath(const QString& path);
	QString playerPath();

	// Media settings
	void setRememberSettings(bool b);
	bool rememberSettings();

	void setRememberTimePos(bool b);
	bool rememberTimePos();

	void setFileSettingsMethod(const QString& method);
	QString fileSettingsMethod();


	// Screenshots
	void setUseScreenshots(bool b);
	bool useScreenshots();

	void setScreenshotDir(const QString& path);
	QString screenshotDir();

#ifdef MPV_SUPPORT
	void setScreenshotFormat(const QString& format);
	QString screenshotFormat();
#endif

	void setPauseWhenHidden(bool b);
	bool pauseWhenHidden();

	void setCloseOnFinish(bool b);
	bool closeOnFinish();


	// Video tab
	void setVO(const QString& vo_driver);
	QString VO();

	void setHwdec(const QString& v);
	QString hwdec();

	void setFrameDrop(bool b);
	bool frameDrop();

	void setHardFrameDrop(bool b);
	bool hardFrameDrop();

	void setSoftwareVideoEqualizer(bool b);
	bool softwareVideoEqualizer();

	void setPostprocessingQuality(int n);
	int postprocessingQuality();

	void setInitialPostprocessing(bool b);
	bool initialPostprocessing();

	void setInitialDeinterlace(int ID);
	int initialDeinterlace();

	void setInitialZoom(double v);
	double initialZoom();

	void setStartInFullscreen(bool b);
	bool startInFullscreen();

	void setBlackbordersOnFullscreen(bool b);
	bool blackbordersOnFullscreen();


	// Audio tab
	void setAO(const QString& ao_driver);
	QString AO();

	void setUseAudioEqualizer(bool b);
	bool useAudioEqualizer();

	void setAc3DTSPassthrough(bool b);
	bool Ac3DTSPassthrough();

	void setGlobalVolume(bool b);
	bool globalVolume();

	void setSoftVol(bool b);
	bool softVol();

	void setAmplification(int n);
	int amplification();

	void setAudioChannels(int ID);
	int audioChannels();

	void setInitialVolNorm(bool b);
	bool initialVolNorm();


	// Preferred tab
	void setAudioLang(const QString& lang);
	QString audioLang();

	void setSubtitleLang(const QString& lang);
	QString subtitleLang();

	void setAudioTrack(int track);
	int audioTrack();

	void setSubtitleTrack(int track);
	int subtitleTrack();

	void setAutoSyncFactor(int factor);
	int autoSyncFactor();

	void setAutoSyncActivated(bool b);
	bool autoSyncActivated();

	void setMc(double value);
	double mc();

	void setMcActivated(bool b);
	bool mcActivated();

	void setScaleTempoFilter(Settings::TPreferences::TOptionState value);
	Settings::TPreferences::TOptionState scaleTempoFilter();

protected slots:
	void vo_combo_changed(int);
	void ao_combo_changed(int);

#ifndef Q_OS_WIN
	void on_vdpau_button_clicked();
#endif

protected:
	virtual void retranslateStrings();
	void updateDriverCombos();

	InfoList vo_list;
	InfoList ao_list;
	
#if USE_DSOUND_DEVICES
	TDeviceList dsound_devices;
#endif

#if USE_ALSA_DEVICES
	TDeviceList alsa_devices;
#endif
#if USE_XV_ADAPTORS
	TDeviceList xv_adaptors;
#endif

private:
	bool filesettings_method_changed;

#ifndef Q_OS_WIN
	struct Settings::TPreferences::VDPAU_settings vdpau;
#endif

};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_GENERAL_H

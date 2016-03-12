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

#ifndef GUI_PREF_AUDIO_H
#define GUI_PREF_AUDIO_H

#include "ui_audio.h"
#include "gui/pref/widget.h"
#include "inforeader.h"
#include "settings/preferences.h"
#include "gui/deviceinfo.h"

#ifdef Q_OS_WIN
#define USE_DSOUND_DEVICES 1
#else
#define USE_ALSA_DEVICES 1
#endif

#ifdef Q_OS_OS2
#define MPLAYER_KAI_VERSION 30994
#endif


namespace Gui {
namespace Pref {

class TAudio : public TWidget, public Ui::TAudio {
	Q_OBJECT

public:
	InfoList ao_list;

	TAudio(QWidget* parent, InfoList aol);
	virtual ~TAudio();

	// Return the name of the section
	virtual QString sectionName();
	// Return the icon of the section
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

	void updateDriverCombo(bool allow_user_defined_ao);

protected:
	virtual void retranslateStrings();

private:

#if USE_DSOUND_DEVICES
	TDeviceList dsound_devices;
#endif

#if USE_ALSA_DEVICES
	TDeviceList alsa_devices;
#endif

	void createHelp();

	void setAO(const QString& ao_driver, bool allow_user_defined);
	QString AO();

	void setAudioChannels(int ID);
	int audioChannels();

	void setUseAudioEqualizer(bool b);
	bool useAudioEqualizer();

	void setAc3DTSPassthrough(bool b);
	bool Ac3DTSPassthrough();

	void setScaleTempoFilter(Settings::TPreferences::TOptionState value);
	Settings::TPreferences::TOptionState scaleTempoFilter();

	void setSoftVol(bool b);
	bool softVol();

	void setAmplification(int n);
	int amplification();

	void setInitialVolNorm(bool b);
	bool initialVolNorm();

	void setAutoSyncActivated(bool b);
	bool autoSyncActivated();

	void setAutoSyncFactor(int factor);
	int autoSyncFactor();

	void setMcActivated(bool b);
	bool mcActivated();

	void setMc(double value);
	double mc();

	void setAudioLang(const QString& lang);
	QString audioLang();

private slots:
	void ao_combo_changed(int);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_AUDIO_H

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

#ifndef GUI_PREF_AUDIO_H
#define GUI_PREF_AUDIO_H

#include "ui_audio.h"
#include "gui/pref/widget.h"
#include "log4qt/logger.h"
#include "player/info/playerinfo.h"
#include "settings/preferences.h"
#include "gui/deviceinfo.h"

#ifndef Q_OS_WIN
#define USE_ALSA_DEVICES 1
#endif


namespace Gui {
namespace Pref {

class TAudio : public TWidget, public Ui::TAudio {
	Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    Player::Info::InfoList ao_list;

    TAudio(QWidget* parent, const Player::Info::InfoList& aol);
	virtual ~TAudio();

	// Return the name of the section
	virtual QString sectionName();
	// Return the icon of the section
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

	void updateDriverCombo(Settings::TPreferences::TPlayerID player_id,
						   bool keep_current_drivers);

protected:
	virtual void retranslateStrings();

private:
	Settings::TPreferences::TPlayerID player_id;
	QString mplayer_ao;
	QString mpv_ao;

#if USE_ALSA_DEVICES
	TDeviceList alsa_devices;
#endif

	void createHelp();

	void setAO(const QString& ao_driver);
	QString AO();

	void setAudioChannels(int ID);
	int audioChannels();

	void setUseAudioEqualizer(bool b);
	bool useAudioEqualizer();

	void setAc3DTSPassthrough(bool b);
	bool Ac3DTSPassthrough();

	void setScaleTempoFilter(Settings::TPreferences::TOptionState value);
	Settings::TPreferences::TOptionState scaleTempoFilter();

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
	void onAOComboChanged(int);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_AUDIO_H

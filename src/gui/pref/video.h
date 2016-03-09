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

#ifndef GUI_PREF_VIDEO_H
#define GUI_PREF_VIDEO_H

#include "ui_video.h"
#include "gui/pref/widget.h"
#include "inforeader.h"
#include "settings/preferences.h"
#include "gui/deviceinfo.h"

#ifndef Q_OS_WIN
#define USE_XV_ADAPTORS 1
#endif


namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TVideo : public TWidget, public Ui::TVideo {
	Q_OBJECT

public:
	TVideo(QWidget* parent, InfoList vol);
	virtual ~TVideo();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences* pref);

    // Apply changes
	void getData(Settings::TPreferences* pref);

protected:
	InfoList vo_list;

#if USE_XV_ADAPTORS
	TDeviceList xv_adaptors;
#endif

	virtual void createHelp();
	virtual void retranslateStrings();
	void updateDriverCombo();

	void setVO(const QString& vo_driver);
	QString VO();

protected slots:
	void vo_combo_changed(int);

#ifndef Q_OS_WIN
	void on_vdpau_button_clicked();
#endif

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

private:

#ifndef Q_OS_WIN
	struct Settings::TPreferences::VDPAU_settings vdpau;
#endif

};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_VIDEO_H

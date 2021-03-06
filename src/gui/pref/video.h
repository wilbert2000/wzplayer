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

#ifndef GUI_PREF_VIDEO_H
#define GUI_PREF_VIDEO_H

#include "ui_video.h"
#include "wzdebug.h"
#include "gui/pref/section.h"
#include "gui/deviceinfo.h"
#include "settings/preferences.h"
#include "player/info/playerinfo.h"

#ifndef Q_OS_WIN
#define USE_XV_ADAPTORS 1
#endif


namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TVideo : public TSection, public Ui::TVideo {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    Player::Info::TNameDescList vo_list;

    TVideo(QWidget* parent, const Player::Info::TNameDescList& vol);

    virtual QString sectionName();
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);

    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

    void updateDriverCombo(Settings::TPreferences::TPlayerID player_id,
                           bool keep_driver);

protected:
    virtual void retranslateStrings();

private:
    Settings::TPreferences::TPlayerID player_id;
    QString mplayer_vo;
    QString mpv_vo;

#if USE_XV_ADAPTORS
    TDeviceList xv_adaptors;
#endif

#ifndef Q_OS_WIN
    struct Settings::TPreferences::VDPAU_settings vdpau;
#endif

    void createHelp();

    void setVO(const QString& vo_driver);
    QString VO();

    void setHwdec(const QString& v);
    QString hwdec();

    void setSoftwareVideoEqualizer(bool b);
    bool softwareVideoEqualizer();

    void setFrameDrop(bool b);
    bool frameDrop();

    void setHardFrameDrop(bool b);
    bool hardFrameDrop();

    void setInitialPostprocessing(bool b);
    bool initialPostprocessing();

    void setPostprocessingQuality(int n);
    int postprocessingQuality();

    void setInitialDeinterlace(int ID);
    int initialDeinterlace();

    void setInitialDeinterlaceTV(int ID);
    int initialDeinterlaceTV();

    void setInitialZoom(double v);
    double initialZoom();

private slots:
    void onVOComboChanged(int);

#ifndef Q_OS_WIN
    void on_vdpau_button_clicked();
#endif

    void setMonitorAspect(const QString& asp);
    QString monitorAspect();
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_VIDEO_H

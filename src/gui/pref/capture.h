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

#ifndef GUI_PREF_CAPTURE_H
#define GUI_PREF_CAPTURE_H

#include "ui_capture.h"
#include "gui/pref/section.h"
#include "settings/preferences.h"


namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TCapture : public TSection, public Ui::TCapture {
    Q_OBJECT

public:
    TCapture(QWidget* parent);
    virtual ~TCapture();

    virtual QString sectionName();
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);

    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

protected:
    virtual void retranslateStrings();

private:
    void createHelp();

    void setUseScreenshots(bool b);
    bool useScreenshots();

    void setScreenshotDir(const QString& path);
    QString screenshotDir();

    void setScreenshotFormat(const QString& format);
    QString screenshotFormat();

    void setSubtitlesOnScreenshots(bool b);
    bool subtitlesOnScreenshots();
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_CAPTURE_H

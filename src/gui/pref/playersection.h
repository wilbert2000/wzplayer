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

#ifndef GUI_PREF_PLAYERSECTION_H
#define GUI_PREF_PLAYERSECTION_H

#include "ui_playersection.h"
#include "gui/pref/section.h"
#include "settings/preferences.h"


namespace Gui {
namespace Pref {

class TPlayerSection : public TSection, public Ui::TPlayerSection {
    Q_OBJECT

public:
    TPlayerSection(QWidget* parent);

    // Return the name of the section
    virtual QString sectionName();

    // Return the icon of the section
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);

    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

signals:
    void binChanged(Settings::TPreferences::TPlayerID,
                    bool keep_current_drivers,
                    const QString& path);

protected:
    virtual void retranslateStrings();

private:
    virtual void createHelp();

    void setPlayerID(Settings::TPreferences::TPlayerID id);
    void setPlayerPath(const QString& mplayer, const QString& mpv);

    void setFileSettingsMethod(const QString& method);
    QString fileSettingsMethod();

    void setLogLevel(Log4Qt::Level level);
    Log4Qt::Level logLevel();

private slots:
    void onMPlayerFileChanged(QString file);
    void onMPVFileChanged(QString file);
    void onPlayerRadioClicked(bool);
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_PLAYERSECTION_H

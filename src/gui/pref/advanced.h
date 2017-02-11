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

#ifndef PREF_ADVANCED_H
#define PREF_ADVANCED_H

#include "ui_advanced.h"
#include "gui/pref/section.h"


namespace Settings {
class TPreferences;
}


namespace Gui {
namespace Pref {

class TAdvanced : public TSection, public Ui::TAdvanced {
    Q_OBJECT

public:
    TAdvanced(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~TAdvanced();

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

    void setLogLevel(Log4Qt::Level level);
    Log4Qt::Level logLevel();

    void setLogVerbose(bool b);
    bool logVerbose();

    void setActionsToRun(QString actions);
    QString actionsToRun();

    void setPlayerAdditionalArguments(QString args);
    QString playerAdditionalArguments();

    void setPlayerAdditionalVideoFilters(QString s);
    QString playerAdditionalVideoFilters();

    void setPlayerAdditionalAudioFilters(QString s);
    QString playerAdditionalAudioFilters();
};

} // namespace Pref
} // namespace Gui

#endif // PREF_ADVANCED_H

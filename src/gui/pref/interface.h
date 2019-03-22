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

#ifndef PREF_INTERFACE_H
#define PREF_INTERFACE_H

#include "ui_interface.h"
#include "gui/pref/section.h"
#include "wzdebug.h"

namespace Settings {
class TPreferences;
}

namespace Gui { namespace Pref {

class TInterface : public TSection, public Ui::TInterface {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TInterface(QWidget* parent = 0, Qt::WindowFlags f = 0);

    virtual QString sectionName();
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);

    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

protected:
    void createLanguageCombo();

    void setLanguage(const QString& lang);
    QString language();

    void setIconSet(const QString& set);
    QString iconSet();

    void setSaveSize(bool b);
    bool saveSize();

    void setStyle(const QString& style);
    QString style();

    void setUseSingleInstance(bool b);
    bool useSingleInstance();

    void setHideVideoOnAudioFiles(bool b);
    bool hideVideoOnAudioFiles();

    // History tab
    void setRecentsMaxItems(int n);
    int recentsMaxItems();

    void setURLMaxItems(int n);
    int urlMaxItems();

    void setRememberDirs(bool b);
    bool rememberDirs();

    virtual void retranslateStrings();

protected slots:
    void changeInstanceImages();

private:
    void createHelp();

    void setPauseWhenHidden(bool b);
    bool pauseWhenHidden();

    void setCloseOnFinish(bool b);
    bool closeOnFinish();

    void setStartInFullscreen(bool b);
    bool startInFullscreen();
};

}} // namespace Gui::Pref

#endif // PREF_INTERFACE_H

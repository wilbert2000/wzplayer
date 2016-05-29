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

#ifndef GUI_PREF_PLAYLISTSECTION_H
#define GUI_PREF_PLAYLISTSECTION_H

#include "ui_playlistsection.h"
#include "gui/pref/widget.h"
#include "settings/preferences.h"
#include "wzdebug.h"


namespace Settings{
class TPreferences;
}

namespace Gui { namespace Pref {

class TPlaylistSection : public TWidget, public Ui::TPlaylistSection {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TPlaylistSection(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~TPlaylistSection();

    virtual QString sectionName();
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);

    // Apply changes
    void getData(Settings::TPreferences* pref);

protected:
    virtual void retranslateStrings();

private:
    void createHelp();

    void setMediaToAddToPlaylist(Settings::TPreferences::TAddToPlaylist type);
    Settings::TPreferences::TAddToPlaylist mediaToAddToPlaylist();

    void setDirectoryRecursion(bool b);
    bool directoryRecursion();
};

}} // namespace Gui::Pref

#endif // GUI_PREF_PLAYLISTSECTION_H

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

#ifndef SETTINGS_FILESETTINGS_HASH_H
#define SETTINGS_FILESETTINGS_HASH_H

#include "settings/filesettingsbase.h"
#include "log4qt/logger.h"

namespace Settings {

class TFileSettingsHash : public TFileSettingsBase {
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TFileSettingsHash(const QString& filename);
    virtual ~TFileSettingsHash();

    virtual bool existSettingsFor(const QString& filename);
    virtual void loadSettingsFor(const QString& filename, TMediaSettings& mset);
    virtual void saveSettingsFor(const QString& filename, TMediaSettings& mset);

private:
    static QString iniFilenameFor(const QString& filename);
};

} // namespace Settings

#endif // SETTINGS_FILESETTINGS_HASH_H


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


#ifndef HELPER_H
#define HELPER_H

#include <QString>
#include <QStringList>
#include "settings/preferences.h"


class Helper {
public:
    // Format time as hh:mm:ss
    static QString formatTime(int secs);

    static bool directoryContainsDVD(QString directory);

    //! Returns an int with the version number of Qt at run-time.
    //! If version is 4.3.2 it returns 40302.
    static int qtVersion();

    static QStringList filesForPlaylist(
            const QString& initial_file,
            Settings::TPreferences::TAddToPlaylist filter);

    //! Tries to find the executable in the path.
    //! Returns the path if found or empty string if not.
    static QString findExecutable(const QString& name);

    static QString baseNameForURL(QString url);
    static QString clean(const QString& name);
    static QString cleanName(const QString& name);
    static QString cleanTitle(const QString& name);


private:
    static QStringList searchForConsecutiveFiles(const QString& initial_file);
    static QStringList filesInDirectory(const QString& initial_file,
                                        const QStringList& filter);
};

#endif // HELPER_H

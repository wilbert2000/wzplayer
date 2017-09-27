/*  WZPlayer, GUI front-end for mplayer and MPV.
    Copyright (C) 2006-2016 Ricardo Villalba <rvm@users.sourceforge.net>

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

#ifndef VERSION_H
#define VERSION_H

#include <QString>


class TVersion {
public:
    static const QString version;

    //! Returns an int with the version number of Qt at run-time.
    //! If version is 4.3.2 it returns 40302.
    static int qtVersion();
};

#endif // VERSION_H


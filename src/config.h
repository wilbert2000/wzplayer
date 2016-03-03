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

#ifndef CONFIG_H
#define CONFIG_H

#include <Qt>

// USE_INFOPROVIDER
// if 1, the playlist will read info about the files when they are added
// to the list.
// It's slow but allows the user to see the length and even the name of
// a mp3 song.
#define USE_INFOPROVIDER 1

// PROGRAM_SWITCH
// support for program switch in ts files
#define PROGRAM_SWITCH 0

// Enables/disables the use of -adapter
#ifdef Q_OS_WIN
#define USE_ADAPTER 1
#endif

#endif // CONFIG_H

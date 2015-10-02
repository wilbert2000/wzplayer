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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Qt>

// STYLE_SWITCHING
// if 1, the preferences dialog will have an option to switch
// the Qt style
#define STYLE_SWITCHING 1

// SEEKBAR_RESOLUTION
// specifies the maximum value of the time slider
#define SEEKBAR_RESOLUTION 1000

// Just for testing, possibility to disable the use of the colorkey
#define USE_COLORKEY 1

// USE_MINIMUMSIZE
// if 1, the main window will not be smaller than the control widget 
// size hint or pref->gui_minimum_width.
#define USE_MINIMUMSIZE 1

// PROGRAM_SWITCH
// support for program switch in ts files
#define PROGRAM_SWITCH 0

// ALLOW_DEMUXER_CODE_CHANGE
// support changing of demuxer and video and audio codecs
#define ALLOW_DEMUXER_CODEC_CHANGE 1

// Enables/disables the use of -adapter
#ifdef Q_OS_WIN
#define USE_ADAPTER 1
#define OVERLAY_VO "directx"
//#define OVERLAY_VO "xv"
#endif

// If 1, smplayer will check if mplayer is old
// and in that case it will report to the user
#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
#define REPORT_OLD_MPLAYER 1
#endif

// If 1, the background logo will be animated
#if QT_VERSION >= 0x040600
/* #define LOGO_ANIMATION 1 */
#endif

#endif

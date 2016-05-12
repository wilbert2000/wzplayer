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

#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <QStringList>


class ExtensionList : public QStringList {
public:
	ExtensionList();

    QString forFilter() const;
    QStringList forDirFilter() const;
    QString forRegExp() const;
};

class TExtensions {
public:
	TExtensions();
	virtual ~TExtensions();

    ExtensionList video() const { return _video; }
    ExtensionList audio() const { return _audio; }
    ExtensionList playlist() const { return _playlist; }
    ExtensionList subtitles() const { return _subtitles; }
    ExtensionList multimedia() const { return _multimedia; }
    ExtensionList allPlayable() const { return _all_playable; }

protected:
	ExtensionList _video, _audio, _playlist, _subtitles;
	ExtensionList _multimedia; //!< video and audio
	ExtensionList _all_playable; //!< video, audio and playlist
};

extern TExtensions extensions;

#endif

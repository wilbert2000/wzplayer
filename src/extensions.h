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
#include <QFileInfo>


class TExtensionList : public QStringList {
public:
    TExtensionList();

    QString forFilter() const;
    QStringList forDirFilter() const;
    QString forRegExp() const;

    void addList(const TExtensionList& list);
};

class TExtensions {
public:
    TExtensions();
    virtual ~TExtensions();

    TExtensionList video() const { return _video; }
    TExtensionList audio() const { return _audio; }
    TExtensionList playlists() const { return _playlists; }
    TExtensionList subtitles() const { return _subtitles; }
    TExtensionList images() const { return _images; }
    TExtensionList videoAndAudio() const { return _videoAndAudio; }
    TExtensionList allPlayable() const { return _all_playable; }

    bool isMultiMedia(const QFileInfo& fi) const;
    bool isPlaylist(const QFileInfo& fi) const;
    bool isPlaylist(const QString& filename) const;
    bool isImage(const QFileInfo& fi) const;
    bool isImage(const QString& filename) const;

private:
    TExtensionList _video, _audio, _playlists, _subtitles, _images;
    TExtensionList _videoAndAudio; //!< video and audio
    TExtensionList _all_playable; //!< video, audio and playlist
};

extern TExtensions extensions;

#endif

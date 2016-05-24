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

#include "extensions.h"
#include <QFileInfo>


TExtensions extensions;


ExtensionList::ExtensionList() : QStringList() {
}

QString ExtensionList::forFilter() const {

	QString s;
    for (int n = 0; n < count(); n++) {
		s = s + "*." + at(n) + " ";
	}
    if (!s.isEmpty())
        s = " (" + s + ")";
	return s;
}

QStringList ExtensionList::forDirFilter() const {

	QStringList l;
    for (int n = 0; n < count(); n++) {
		QString s = "*." + at(n);
		l << s;
	}
	return l;
}

QString ExtensionList::forRegExp() const {

	QString s;
    for (int n = 0; n < count(); n++) {
        if (!s.isEmpty())
            s = s + "|";
		s = s + "^" + at(n) + "$";
	}
	return s;
}

TExtensions::TExtensions() {

	_video << "avi" << "vfw" << "divx" 
           << "mpg" << "mpeg" << "m1v" << "m2v" << "mpv" << "dv" << "3gp"
           << "mov" << "mp4" << "m4v" << "mqv"
           << "dat" << "vcd"
           << "ogg" << "ogm" << "ogv" << "ogx"
           << "asf" << "wmv"
           << "bin" << "iso" << "vob"
           << "mkv" << "nsv" << "flv" << "swf"
           << "rm" << "ram"
           << "ts" << "rmvb" << "dvr-ms" << "m2t" << "m2ts" << "mts" << "rec"
           << "wtv"
           << "f4v" << "hdmov" << "webm" << "vp8"
           << "bik" << "smk" << "m4b" << "wtv";

    _audio << "mp3" << "ogg" << "oga" << "wav" << "wma" <<  "aac" << "ac3"
           << "dts" << "ra" << "ape" << "flac" << "thd" << "mka" << "m4a"
           << "au";

	_subtitles << "srt" << "sub" << "ssa" << "ass" << "idx" << "txt" << "smi"
               << "rt" << "utf" << "aqt";

    _images << "bmp" << "gif" << "jpg" << "jpeg" << "mj2c" << "mjp2" << "pcx"
            << "png" << "tga" << "sgi" << "tiff";

    _playlists << "m3u8" << "m3u" << "pls";

    _videoAndAudio = _video;
    for (int n = 0; n < _audio.count(); n++) {
        if (!_videoAndAudio.contains(_audio[n])) _videoAndAudio << _audio[n];
    }

    _all_playable << _videoAndAudio << _playlists << _images;
}

TExtensions::~TExtensions() {
}

bool TExtensions::isMultiMedia(const QFileInfo &fi) const {
    return _videoAndAudio.contains(fi.suffix().toLower());
}

bool TExtensions::isPlaylist(const QFileInfo &fi) const {
    return _playlists.contains(fi.suffix().toLower());
}

bool TExtensions::isImage(const QFileInfo &fi) const {
    return _images.contains(fi.suffix().toLower());
}

bool TExtensions::isImage(const QString& filename) const {
    return isImage(QFileInfo(filename));
}

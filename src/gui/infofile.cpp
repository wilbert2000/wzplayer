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

#include "gui/infofile.h"

#include <QFileInfo>
#include <QCoreApplication>

#include "discname.h"
#include "images.h"
#include "maps/tracks.h"

namespace Gui {

TInfoFile::TInfoFile() {
	row = 0;
}

TInfoFile::~TInfoFile() {
}

QString TInfoFile::getInfo(MediaData md) {
	QString s;

	// General
	QFileInfo fi(md.filename);

	QString icon;
	switch (md.selected_type) {
		case MediaData::TYPE_FILE:	if (md.noVideo())
										icon = "type_audio.png";
									else
										icon = "type_video.png";
									break;
		case MediaData::TYPE_DVD	:	icon = "type_dvd.png"; break;
		case MediaData::TYPE_DVDNAV	:	icon = "type_dvd.png"; break;
		case MediaData::TYPE_VCD	:	icon = "type_vcd.png"; break;
		case MediaData::TYPE_CDDA	:	icon = "type_vcd.png"; break;
		case MediaData::TYPE_TV		:	icon = "type_tv.png"; break;
		case MediaData::TYPE_STREAM :	icon = "type_url.png"; break;
		case MediaData::TYPE_BLURAY :	icon = "type_bluray.png"; break;
		default						:	icon = "type_unknown.png";
	}
	icon = icon.replace(".png", ""); // FIXME
	icon = "<img src=\"" + Images::file(icon) + "\"> ";

	if (md.selected_type == MediaData::TYPE_DVD
		|| md.selected_type == MediaData::TYPE_DVDNAV
		|| md.selected_type == MediaData::TYPE_BLURAY)
	{
		DiscData disc_data = DiscName::split(md.filename);
		s += title( icon + disc_data.protocol + "://" + QString::number(disc_data.title) );
	} else {
		s += title( icon + md.displayName() );
	}

	s += openPar( tr("General") );
	if (fi.exists()) {
		//s += addItem( tr("Path"), fi.dirPath() );
		s += addItem( tr("File"), fi.absoluteFilePath() );
		s += addItem( tr("Size"), tr("%1 KB (%2 MB)").arg(fi.size()/1024)
                                  .arg(fi.size()/1048576) );
	} else {
		QString url = md.filename;
		s += addItem( tr("URL"), url );
	}
	s += addItem( tr("Length"), Helper::formatTime((int)md.duration) );
	s += addItem( tr("Demuxer"), md.demuxer );
	s += closePar();

	// Clip info
	QString c;
	if (md.meta_data.contains("NAME")) c+= addItem( tr("Name"), md.meta_data["NAME"] );
	if (md.meta_data.contains("ARTIST")) c+= addItem( tr("Artist"), md.meta_data["ARTIST"] );
	if (md.meta_data.contains("AUTHOR")) c+= addItem( tr("Author"), md.meta_data["AUTHOR"] );
	if (md.meta_data.contains("ALBUM")) c+= addItem( tr("Album"), md.meta_data["ALBUM"] );
	if (md.meta_data.contains("GENRE")) c+= addItem( tr("Genre"), md.meta_data["GENRE"] );
	if (md.meta_data.contains("DATE")) c+= addItem( tr("Date"), md.meta_data["DATE"] );
	if (md.meta_data.contains("TRACK")) c+= addItem( tr("Track"), md.meta_data["TRACK"] );
	if (md.meta_data.contains("COPYRIGHT")) c+= addItem( tr("Copyright"), md.meta_data["COPYRIGHT"] );
	if (md.meta_data.contains("COMMENT")) c+= addItem( tr("Comment"), md.meta_data["COMMENT"] );
	if (md.meta_data.contains("SOFTWARE")) c+= addItem( tr("Software"), md.meta_data["SOFTWARE"] );

	if (!md.stream_title.isEmpty()) c+= addItem( tr("Stream title"), md.stream_title );
	if (!md.stream_url.isEmpty()) c+= addItem( tr("Stream URL"), md.stream_url );

	if (!c.isEmpty()) {
		s += openPar( tr("Clip info") );
		s += c;
		s += closePar();
	}

	// Video info
	if (!md.noVideo()) {
		s += openPar( tr("Video") );
		s += addItem( tr("Resolution"), QString("%1 x %2").arg(md.video_width).arg(md.video_height) );
		s += addItem( tr("Aspect ratio"), QString::number(md.video_aspect) );
		s += addItem( tr("Format"), md.video_format );
		s += addItem( tr("Bitrate"), tr("%1 kbps").arg(md.video_bitrate / 1000) );
		s += addItem( tr("Frames per second"), QString::number(md.video_fps) );
		s += addItem( tr("Selected codec"), md.video_codec );
		s += closePar();
	}

	// Audio info
	s += openPar( tr("Initial Audio Stream") );
	s += addItem( tr("Format"), md.audio_format );
	s += addItem( tr("Bitrate"), tr("%1 kbps").arg(md.audio_bitrate / 1000) );
	s += addItem( tr("Rate"), tr("%1 Hz").arg(md.audio_rate) );
	s += addItem( tr("Channels"), QString::number(md.audio_nch) );
	s += addItem( tr("Selected codec"), md.audio_codec );
	s += closePar();

	// Audio Tracks
	if (md.audios.count() > 0) {
		s += openPar( tr("Audio Streams") );
		row++;
		s += openItem();
		s += "<td>" + tr("#", "Info for translators: this is a abbreviation for number") + "</td><td>" + 
              tr("Language") + "</td><td>" + tr("Name") +"</td><td>" +
              tr("ID", "Info for translators: this is a identification code") + "</td>";
		s += closeItem();

		Maps::TTracks::TTrackIterator i = md.audios.getIterator();
		int n = 0;
		do {
			i.next();
			Maps::TTrackData track = i.value();
			row++;
			n++;
			s += openItem();
			QString lang = track.getLang();
			if (lang.isEmpty()) lang = "<i>&lt;"+tr("empty")+"&gt;</i>";
			QString name = track.getName();
			if (name.isEmpty()) name = "<i>&lt;"+tr("empty")+"&gt;</i>";
			s += QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td>")
                 .arg(n).arg(lang).arg(name)
				 .arg(track.getID());
			s += closeItem();
		} while (i.hasNext());
		s += closePar();
	}

	// Subtitles
	if (md.subs.count() > 0) {
		s += openPar( tr("Subtitles") );
		row++;
		s += openItem();
		s += "<td>" + tr("#", "Info for translators: this is a abbreviation for number") + "</td><td>" + 
              tr("Type") + "</td><td>" +
              tr("Language") + "</td><td>" + tr("Name") +"</td><td>" +
              tr("ID", "Info for translators: this is a identification code") + "</td>";
		s += closeItem();
		for (int n = 0; n < md.subs.count(); n++) {
			row++;
			s += openItem();
			QString t;
			switch (md.subs.itemAt(n).type()) {
				case SubData::File: t = "FILE_SUB"; break;
				case SubData::Vob:	t = "VOB"; break;
				default:			t = "SUB";
			}
			QString lang = md.subs.itemAt(n).lang();
			if (lang.isEmpty()) lang = "<i>&lt;"+tr("empty")+"&gt;</i>";
			QString name = md.subs.itemAt(n).name();
			if (name.isEmpty()) name = "<i>&lt;"+tr("empty")+"&gt;</i>";
			/*
			s += QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td>")
                 .arg(n).arg(t).arg(lang).arg(name)
                 .arg(md.subs.itemAt(n).ID());
			*/
            s += "<td>" + QString::number(n) + "</td><td>" + t + 
                 "</td><td>" + lang + "</td><td>" + name + 
                 "</td><td>" + QString::number(md.subs.itemAt(n).ID()) + "</td>";
			s += closeItem();
		}
		s += closePar();
	}

	return "<html><body bgcolor=\"white\"><font color=\"black\">"+ s + "</font></body></html>";
}

QString TInfoFile::title(QString text) {
	return "<h1>" + text + "</h1>";
}

QString TInfoFile::openPar(QString text) {
	return "<h2>" + text + "</h2>"
           "<table width=\"100%\">";
}

QString TInfoFile::closePar() {
	row = 0;
	return "</table>";
}

QString TInfoFile::openItem() {
	if (row % 2 == 1)
		return "<tr bgcolor=\"lavender\">";
	else
		return "<tr bgcolor=\"powderblue\">";
}

QString TInfoFile::closeItem() {
	return "</tr>";
}

QString TInfoFile::addItem( QString tag, QString value ) {
	row++;
	return openItem() + 
           "<td><b>" + tag + "</b></td>" +
           "<td>" + value + "</td>" +
           closeItem();
}


inline QString TInfoFile::tr( const char * sourceText, const char * comment, int n )  {
#if QT_VERSION >= 0x050000
	return QCoreApplication::translate("TInfoFile", sourceText, comment, n );
#else
	return QCoreApplication::translate("TInfoFile", sourceText, comment, QCoreApplication::CodecForTr, n );
#endif
}

} // namespace Gui

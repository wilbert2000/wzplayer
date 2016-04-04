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

#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

#include "discname.h"
#include "images.h"
#include "maps/tracks.h"
#include "settings/aspectratio.h"

namespace Gui {

TInfoFile::TInfoFile() {
	row = 0;
}

TInfoFile::~TInfoFile() {
}

QString TInfoFile::formatSize(qint64 size) {

	const qint64 MB = 1024 * 1024;

	QLocale locale;
	if (size < MB) {
		double kb = (double) size / 1024;
		return tr("%1 KiB (%2 bytes)").arg(locale.toString(kb, 'f', 2), locale.toString(size));
	}

	double mb = (double) size / MB;
	return tr("%1 MiB (%2 bytes)").arg(locale.toString(mb, 'f', 2), locale.toString(size));
}

void TInfoFile::addTracks(QString& s, const Maps::TTracks& tracks, const QString& name) {

	if (tracks.count() > 0) {
		s += openPar(name);
		row++;
		s += openItem();
		s += "<td>" + tr("#", "Info for translators: this is a abbreviation for number") + "</td><td>" +
			 tr("Language") + "</td><td>" + tr("Name") +"</td><td>" +
			 tr("ID", "Info for translators: this is a identification code") + "</td>";
		s += closeItem();

		int n = 0;
		Maps::TTracks::TTrackIterator i = tracks.getIterator();
		do {
			i.next();
			Maps::TTrackData track = i.value();
			row++;
			n++;
			s += openItem();
			QString lang = track.getLang();
			if (lang.isEmpty())
				lang = "<i>&lt;"+tr("empty")+"&gt;</i>";
			QString name = track.getName();
			if (name.isEmpty())
				name = "<i>&lt;"+tr("empty")+"&gt;</i>";
			s += QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td>")
				 .arg(n).arg(lang).arg(name).arg(track.getID());
			s += closeItem();
		} while (i.hasNext());
		s += closePar();
	}
}

QString TInfoFile::getInfo(const TMediaData& md) {

	QString s;

	// General
	QFileInfo fi(md.filename);

	QString icon;
	switch (md.selected_type) {
		case TMediaData::TYPE_FILE:
			if (md.noVideo()) {
				if (md.noAudio()) {
					icon = "type_unknown";
				} else {
					icon = "type_audio";
				}
			} else {
				icon = "type_video";
			}
			break;
		case TMediaData::TYPE_DVD:		icon = "type_dvd"; break;
		case TMediaData::TYPE_DVDNAV:	icon = "type_dvd"; break;
		case TMediaData::TYPE_VCD:		icon = "type_vcd"; break;
		case TMediaData::TYPE_CDDA:		icon = "type_vcd"; break;
		case TMediaData::TYPE_TV:		icon = "type_tv"; break;
		case TMediaData::TYPE_STREAM:	icon = "type_url"; break;
		case TMediaData::TYPE_BLURAY:	icon = "type_bluray"; break;
		default:						icon = "type_unknown";
	}
	icon = "<img src=\"" + Images::file(icon) + "\"> ";
	s += title(icon + md.displayName());

	s += openPar(tr("General"));
	if (fi.exists()) {
		s += addItem(tr("File"), fi.absoluteFilePath());
		s += addItem(tr("Size"), formatSize(fi.size()));
	} else {
		QString url = md.filename;
		s += addItem(tr("URL"), url);
	}
	if (!md.stream_url.isEmpty())
		s += addItem(tr("Stream URL"), md.stream_url);
	s += addItem(tr("Length"), Helper::formatTime((int)md.duration));
	s += addItem(tr("Demuxer"), md.demuxer + " - " + md.demuxer_description);
	s += closePar();

	// Meta data
	QString c;
	QMapIterator<QString, QString> i(md.meta_data);
	while (i.hasNext()) {
		i.next();
		c += addItem(i.key(), i.value());
	 }

	if (!c.isEmpty()) {
		s += openPar(tr("Meta data"));
		s += c;
		s += closePar();
	}

	// Video
	if (md.hasVideo()) {
		s += openPar(tr("Video"));
		s += addItem(tr("Resolution source"), QString("%1 x %2").arg(md.video_width).arg(md.video_height));
		s += addItem(tr("Resolution video out"), QString("%1 x %2").arg(md.video_out_width).arg(md.video_out_height));
		s += addItem(tr("Aspect ratio reported by player"), md.video_aspect);
		s += addItem(tr("Original aspect ratio"), md.video_aspect_original == -1
			? tr("Play with aspect ratio set to auto to detect")
			: Settings::TAspectRatio::doubleToString(md.video_aspect_original));
		s += addItem(tr("Current aspect ratio"),
			Settings::TAspectRatio::doubleToString((double) md.video_out_width / md.video_out_height));
		s += addItem(tr("Format"), md.video_format);
		s += addItem(tr("Bitrate"), md.video_bitrate == -1
			? (Settings::pref->isMPV() ? tr("Still testing, wait a few seconds") : tr("Unknown"))
			: tr("%1 kbps").arg(md.video_bitrate / 1000));
		s += addItem(tr("Frames per second"), QString::number(md.video_fps));
		s += addItem(tr("Selected codec"), md.video_codec + " - " + md.video_codec_description);
		s += addItem(tr("Number of tracks"), QString::number(md.videos.count()));
		s += closePar();
	}

	// Video tracks
	addTracks(s, md.videos, tr("Video tracks"));

	// Audio
	s += openPar(tr("Audio"));
	s += addItem(tr("Format"), md.audio_format);
	s += addItem(tr("Bitrate"), md.audio_bitrate == -1
		 ?  (Settings::pref->isMPV() ? tr("Still testing, wait a few seconds") : tr("Unknown"))
		 : tr("%1 kbps").arg(md.audio_bitrate / 1000));
	s += addItem(tr("Rate"), tr("%1 Hz").arg(md.audio_rate));
	s += addItem(tr("Channels"), QString::number(md.audio_nch));
	s += addItem(tr("Selected codec"), md.audio_codec + " - " + md.audio_codec_description);
	s += addItem(tr("Number of tracks"), QString::number(md.audios.count()));
	s += closePar();

	// Audio tracks
	addTracks(s, md.audios, tr("Audio tracks"));

	// Subtitles
	if (md.subs.count() > 0) {
		s += openPar(tr("Subtitles"));
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

QString TInfoFile::addItem(QString tag, QString value) {
	row++;
	return openItem()
			+ "<td><b>" + tag + "</b></td>"
			+ "<td>" + value + "</td>"
			+ closeItem();
}


inline QString TInfoFile::tr(const char* sourceText, const char* comment, int n)  {
#if QT_VERSION >= 0x050000
	return QCoreApplication::translate("TInfoFile", sourceText, comment, n);
#else
	return QCoreApplication::translate("TInfoFile", sourceText, comment, QCoreApplication::CodecForTr, n);
#endif
}

} // namespace Gui

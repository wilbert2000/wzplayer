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

#include "gui/infofile.h"

#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>

#include "settings/preferences.h"
#include "settings/aspectratio.h"
#include "maps/tracks.h"
#include "discname.h"
#include "images.h"

namespace Gui {

TInfoFile::TInfoFile() :
    row(0) {
}

TInfoFile::~TInfoFile() {
}

QString TInfoFile::formatSize(qint64 size) {

	const qint64 MB = 1024 * 1024;

	QLocale locale;
	if (size < MB) {
		double kb = (double) size / 1024;
        return tr("%1 KiB (%2 bytes)").arg(locale.toString(kb, 'f', 2),
                                           locale.toString(size));
	}

	double mb = (double) size / MB;
    return tr("%1 MiB (%2 bytes)").arg(locale.toString(mb, 'f', 2),
                                       locale.toString(size));
}

void TInfoFile::addTracks(QString& s,
                          const Maps::TTracks& tracks,
                          const QString& name) {

    if (tracks.count() > 0) {
        s += "<tr><td>&nbsp;</td></tr>"
             "<tr><td colspan=\"5\"><h3>" + name + "</h3></td></tr>";
        row++;
        s += "<tr><th align=\"left\">" + tr("#") + "</th>"
             "<th align=\"left\">" + tr("ID") + "</th>"
             "<th align=\"left\" colspan=\"2\">" + tr("Name") +"</th>"
             "<th align=\"left\">" + tr("Language") + "</th></tr>";

        int n = 0;
        Maps::TTracks::TTrackIterator i = tracks.getIterator();
        do {
            i.next();
            Maps::TTrackData track = i.value();
            row++;
            n++;
            s += "<tr><th align=\"left\">" + QString::number(n) + "</th>"
                 "<td>" + QString::number(track.getID()) + "</td>"
                 "<td colspan=\"2\">" + track.getName() + "</td>"
                 "<td>" + track.getLang() + "</td></tr>";
        } while (i.hasNext());
    }
}

QString TInfoFile::getInfo(const TMediaData& md) {

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

    QString s = "<html><body bgcolor=\"white\"><font color=\"black\">";
    s += "<h1><img src=\"" + Images::file(icon) + "\">" + md.displayName()
         + "</h1>\n";

    s += "<table width=\"100%\">";

    s += openPar(tr("General"));
    if (fi.exists()) {
        s += addItem(tr("File name"), fi.absoluteFilePath());
        s += addItem(tr("Size"), formatSize(fi.size()));
    } else {
        s += addItem(tr("URL"), md.filename);
    }
    if (!md.stream_url.isEmpty())
        s += addItem(tr("Stream URL"), md.stream_url);
    s += addItem(tr("Duration"), Helper::formatTime(qRound(md.duration)));
    s += addItem(tr("Demuxer"), md.demuxer + " - " + md.demuxer_description);
    s += closePar();

    // Video
    if (md.hasVideo()) {
        s += openPar(tr("Video"));
        s += addItem(tr("Video out driver"), md.vo);
        s += addItem(tr("Resolution source"), QString("%1 x %2").arg(md.video_width).arg(md.video_height));
        s += addItem(tr("Resolution video out"), QString("%1 x %2").arg(md.video_out_width).arg(md.video_out_height));
        s += addItem(tr("Aspect reported by player"), md.video_aspect);
        s += addItem(tr("Original aspect ratio"), md.video_aspect_original == -1
            ? tr("Play with aspect ratio set to auto to detect")
            : Settings::TAspectRatio::doubleToString(md.video_aspect_original));
        s += addItem(tr("Current aspect ratio"),
            Settings::TAspectRatio::doubleToString((double) md.video_out_width / md.video_out_height));
        s += addItem(tr("Format"), md.video_format);
        if (md.video_bitrate == -1) {
            if (Settings::pref->isMPV()) {
                s += addItem(tr("Bitrate"),
                             tr("Not received, wait a few seconds..."));
            } else {
                s += addItem(tr("Bitrate"), tr("Unknown"));
            }
        } else {
            s += addItem(tr("Bitrate"),
                         tr("%1 kbps").arg(md.video_bitrate / 1000));
        }
        s += addItem(tr("Frames per second"), QString::number(md.video_fps));
        s += addItem(tr("Selected codec"), md.video_codec + " - " + md.video_codec_description);
        s += addItem(tr("Number of tracks"), QString::number(md.videos.count()));
        s += addItem(tr("Current angle"), QString::number(md.angle));
        s += addItem(tr("Number of angles"), QString::number(md.angles));
        s += closePar();
    }

    // Video tracks
    addTracks(s, md.videos, tr("Video tracks"));

    // Audio
    s += openPar(tr("Audio"));
    s += addItem(tr("Audio out driver"), md.ao);
    s += addItem(tr("Format"), md.audio_format);
    if (md.audio_bitrate == -1) {
        if (Settings::pref->isMPV()) {
            s += addItem(tr("Bitrate"),
                         tr("Not received yet, wait a few seconds..."));
        } else {
            s += addItem(tr("Bitrate"), tr("Unknown"));
        }
    } else {
        s += addItem(tr("Bitrate"),
                     tr("%1 kbps").arg(md.video_bitrate / 1000));
    }
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
        s += "<tr><th align=\"left\">" + tr("#") + "</th>";
        if (Settings::pref->isMPlayer()) {
             s += "<th align=\"left\">" +  tr("Source") + "</th>";
        }
        s += "<th align=\"left\">" + tr("ID") + "</th>";
        if (Settings::pref->isMPlayer()) {
             s += "<th align=\"left\">" + tr("Name") +"</th>";
        } else {
            s += "<th align=\"left\" colspan=\"2\">" + tr("Name") +"</th>";
        }
        s += "<th align=\"left\">" + tr("Language") + "</th></tr>";
        for (int n = 0; n < md.subs.count(); n++) {
            row++;
            QString t;
            switch (md.subs.itemAt(n).type()) {
                case SubData::File: t = tr("file"); break;
                case SubData::Vob:	t = tr("VOB"); break;
                default:			t = tr("demuxer"); break;
            }
            s += "<tr><th align=\"left\">" + QString::number(n) + "</th>";
            if (Settings::pref->isMPlayer()) {
                 s += "<td>" + t + "</td>";
            }
            s += "<td>" + QString::number(md.subs.itemAt(n).ID()) + "</td>";
            if (Settings::pref->isMPlayer()) {
                  s += "<td>" + md.subs.itemAt(n).name() + "</td>";
            } else {
                s += "<td colspan=\"2\">" + md.subs.itemAt(n).name() + "</td>";
            }
            s += "<td>" + md.subs.itemAt(n).lang() + "</td></tr>";
        }
        s += closePar();
    }

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

    s += "</table></font></body></html>";
    return s;
}

QString TInfoFile::title(QString text) {
    return "<h1>" + text + "</h1>";
}

QString TInfoFile::openPar(QString text) {
    return "<tr><td>&nbsp;</td></tr>"
           "<tr><td colspan=\"5\"><h2>" + text + "</h2></td></tr>";
}

QString TInfoFile::closePar() {

    row = 0;
    return "\n";
}

QString TInfoFile::addItem(QString tag, QString value) {
	row++;
    return "<tr><th align=\"left\" colspan=\"2\">" + tag+ "</th>"
            "<td align=\"left\" colspan=\"3\">" + value + "</td></tr>";
}


inline QString TInfoFile::tr(const char* sourceText,
                             const char* comment,
                             int n)  {

#if QT_VERSION >= 0x050000
	return QCoreApplication::translate("TInfoFile", sourceText, comment, n);
#else
    return QCoreApplication::translate("TInfoFile", sourceText, comment,
                                       QCoreApplication::CodecForTr, n);
#endif
}

} // namespace Gui

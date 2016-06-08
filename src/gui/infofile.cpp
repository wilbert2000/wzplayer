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

#include <QFileInfo>
#include <QCoreApplication>

#include "settings/preferences.h"
#include "settings/aspectratio.h"
#include "maps/tracks.h"
#include "discname.h"
#include "images.h"

namespace Gui {

TInfoFile::TInfoFile() :
    debug(logger()) {
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
        s += "<tr><th align=\"left\">" + tr("#") + "</th>"
             "<th align=\"left\">" + tr("ID") + "</th>"
             "<th align=\"left\" colspan=\"2\">" + tr("Name") +"</th>"
             "<th align=\"left\">" + tr("Language") + "</th></tr>";

        int n = 0;
        Maps::TTracks::TTrackIterator i = tracks.getIterator();
        do {
            i.next();
            Maps::TTrackData track = i.value();
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

    s += openPar(tr("File"));
    if (fi.exists()) {
        s += addItem(tr("Name"), fi.absoluteFilePath());
        s += addItem(tr("Size"), formatSize(fi.size()));
    } else {
        s += addItem(tr("URL"), md.filename);
    }
    if (!md.stream_url.isEmpty()) {
        s += addItem(tr("Stream URL"), md.stream_url);
    }
    s += addItem(tr("Duration"), Helper::formatTime(qRound(md.duration)));
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
        s += addItem(tr("Driver"), md.vo);
        s += addItem(tr("Format"), md.video_format);
        s += addItem(tr("Codec"), tr("%1 - %2")
                     .arg(md.video_codec).arg(md.video_codec_description));
        s += addItem(tr("Resolution"), QString("%1 x %2")
                     .arg(md.video_width).arg(md.video_height));
        s += addItem(tr("Video out"), QString("%1 x %2")
                     .arg(md.video_out_width).arg(md.video_out_height));
        s += addItem(tr("Aspect ratio"), md.video_aspect);
        s += addItem(tr("Original aspect"),
                     md.video_aspect_original == -1
                     ? tr("Play with aspect auto to detect")
                     : Settings::TAspectRatio::doubleToString(
                           md.video_aspect_original));
        s += addItem(tr("Current aspect"),
                     Settings::TAspectRatio::doubleToString(
                         (double) md.video_out_width / md.video_out_height));
        if (md.video_bitrate == -1) {
            if (Settings::pref->isMPV() && !md.image) {
                s += addItem(tr("Bitrate"), tr("Wait a few seconds..."));
            } else {
                s += addItem(tr("Bitrate"), tr("Not set"));
            }
        } else {
            s += addItem(tr("Bitrate"),
                         tr("%1 kbps").arg(md.video_bitrate / 1000));
        }
        s += addItem(tr("Frames per second"), QString::number(md.video_fps));
        s += addItem(tr("Angle"), tr("%1/%2").arg(md.angle).arg(md.angles));
        s += addItem(tr("Number of tracks"),
                     QString::number(md.videos.count()));
        s += closePar();

        // Video tracks
        addTracks(s, md.videos, tr("Video tracks"));
    }

    // Audio
    if (md.hasAudio()) {
        s += openPar(tr("Audio"));
        s += addItem(tr("Driver"), md.ao);
        s += addItem(tr("Format"), md.audio_format);
        s += addItem(tr("Codec"),
            tr("%1 - %2").arg(md.audio_codec).arg(md.audio_codec_description));
        s += addItem(tr("Channels"), QString::number(md.audio_nch));
        s += addItem(tr("Rate"), tr("%1 Hz").arg(md.audio_rate));
        if (md.audio_bitrate == -1) {
            if (Settings::pref->isMPV() && !md.image) {
                s += addItem(tr("Bitrate"), tr("Wait a few seconds..."));
            } else {
                s += addItem(tr("Bitrate"), tr("Not set"));
            }
        } else {
            s += addItem(tr("Bitrate"),
                         tr("%1 kbps").arg(md.audio_bitrate / 1000));
        }
        s += addItem(tr("Number of tracks"),
                     QString::number(md.audios.count()));
        s += closePar();

        // Audio tracks
        addTracks(s, md.audios, tr("Audio tracks"));
    }

    // Subtitles
    if (md.subs.count() > 0) {
        s += openPar(tr("Subtitles"));

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

    s += "</table></font></body></html>";
    return s;
}

QString TInfoFile::openPar(QString text) {
    return "<tr><td>&nbsp;</td></tr>"
           "<tr><td colspan=\"5\"><h2>" + text + "</h2></td></tr>";
}

QString TInfoFile::closePar() {
    return "\n";
}

QString TInfoFile::addItem(QString tag, QString value) {
    return "<tr><th align=\"left\" colspan=\"2\">" + tag+ "</th>"
            "<td align=\"left\" colspan=\"3\">" + value + "</td></tr>";
}

} // namespace Gui

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

#include "gui/propertiesdialog.h"
#include "settings/preferences.h"
#include "player/player.h"
#include "images.h"
#include "desktop.h"
#include "config.h"
#include "wzdebug.h"
#include "wztime.h"

#include <QScrollBar>


namespace Gui {

TPropertiesDialog::TPropertiesDialog(QWidget* parent)
    : QDialog(parent, TConfig::DIALOG_FLAGS) {

    setupUi(this);
    retranslateUi(this);
    setWindowIcon(Images::icon("logo"));
    setModal(false);

    // Restore pos and size
    Settings::pref->beginGroup("propertiesdialog");
    QPoint p = Settings::pref->value("pos", QPoint()).toPoint();
    QSize s = Settings::pref->value("size", QPoint()).toSize();
    Settings::pref->endGroup();

    if (s.width() > 400 && s.height() > 400) {
        move(p);
        resize(s);
        TDesktop::keepInsideDesktop(this);
    }

    // Setup buttons
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &TPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &TPropertiesDialog::reject);
    connect(applyButton, &QPushButton::clicked,
            this, &TPropertiesDialog::apply);
}

void TPropertiesDialog::saveSettings() {

    Settings::pref->beginGroup("propertiesdialog");
    Settings::pref->setValue("pos", pos());
    Settings::pref->setValue("size", size());
    Settings::pref->endGroup();
}

void TPropertiesDialog::closeEvent(QCloseEvent* event) {

    emit visibilityChanged(false);
    event->accept();
}

void TPropertiesDialog::accept() {

    setResult(QDialog::Accepted);
    hide();
    emit visibilityChanged(false);
    emit applied();
}

void TPropertiesDialog::reject() {

    setResult(QDialog::Rejected);
    hide();
    emit visibilityChanged(false);
}

void TPropertiesDialog::apply() {

    setResult(QDialog::Accepted);
    emit applied();
}

void TPropertiesDialog::setCodecs(const Player::Info::TNameDescList& vc,
                                  const Player::Info::TNameDescList& ac,
                                  const Player::Info::TNameDescList& demuxer) {

    vclist = vc;
    aclist = ac;
    demuxerlist = demuxer;

    qSort(vclist);
    qSort(aclist);
    qSort(demuxerlist);

    vc_listbox->clear();
    ac_listbox->clear();
    demuxer_listbox->clear();

    Player::Info::TNameDescList::iterator it;

    for (it = vclist.begin(); it != vclist.end(); ++it) {
        vc_listbox->addItem((*it).name() + " - " + (*it).desc());
    }
    for (it = aclist.begin(); it != aclist.end(); ++it) {
        ac_listbox->addItem((*it).name() + " - " + (*it).desc());
    }
    for (it = demuxerlist.begin(); it != demuxerlist.end(); ++it) {
        demuxer_listbox->addItem((*it).name() + " - " + (*it).desc());
    }
}

void TPropertiesDialog::setDemuxer(const QString& demuxer,
                                   const QString& original_demuxer) {

    if (!original_demuxer.isEmpty()) {
        orig_demuxer = original_demuxer;
        int pos = find(orig_demuxer, demuxerlist);
        if (pos >= 0) {
            player->mdat.demuxer_description = demuxerlist.at(pos).desc();
        }
    }

    int pos = find(demuxer, demuxerlist);
    if (pos >= 0) {
        demuxer_listbox->setCurrentRow(pos);
    }

    WZDEBUG("'" + demuxer + "'");
}

QString TPropertiesDialog::demuxer() {

    int pos = demuxer_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return demuxerlist[pos].name();
}

void TPropertiesDialog::setVideoCodec(const QString& vc,
                                      const QString& original_vc) {

    WZDEBUG("'" + vc + "'");

    if (!original_vc.isEmpty()) {
        orig_vc = original_vc;
        int pos = find(orig_vc, vclist);
        if (pos >= 0) {
            vc_listbox->setCurrentRow(pos);
            // player->mdat.video_codec_description is used by info page
            player->mdat.video_codec_description = vclist.at(pos).desc();
            WZTRACE(QString("Found video codec '%1'").arg(orig_vc));
        } else {
            WZWARN(QString("Video codec '%1' not found").arg(orig_vc));
        }
    }

    int pos = find(vc, vclist);
    if (pos >= 0) {
        vc_listbox->setCurrentRow(pos);
        WZTRACE(QString("Found video codec '%1'").arg(vc));
    } else {
        WZWARN(QString("Video codec '%1' not found").arg(vc));
    }
}

QString TPropertiesDialog::videoCodec() {

    int pos = vc_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return vclist[pos].name();
}

void TPropertiesDialog::setAudioCodec(const QString& ac, const QString& original_ac) {

    if (!original_ac.isEmpty()) {
        orig_ac = original_ac;
        int pos = find(orig_ac, aclist);
        if (pos >= 0) {
            player->mdat.audio_codec_description = aclist[pos].desc();
        }
    }
    int pos = find(ac, aclist);
    if (pos >= 0) {
        ac_listbox->setCurrentRow(pos);
    }

    WZDEBUG("'" + ac + "'");
}

QString TPropertiesDialog::audioCodec() {

    int pos = ac_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return aclist[pos].name();
}

void TPropertiesDialog::on_resetDemuxerButton_clicked() {
    setDemuxer(orig_demuxer);
}

void TPropertiesDialog::on_resetACButton_clicked() {
    setAudioCodec(orig_ac);
}

void TPropertiesDialog::on_resetVCButton_clicked() {
    setVideoCodec(orig_vc);
}

int TPropertiesDialog::find(const QString& s,
                            const Player::Info::TNameDescList& list) const {

    int n = 0;
    Player::Info::TNameDescList::const_iterator it;

    for (it = list.constBegin(); it != list.constEnd(); ++it) {
        if ((*it).name() == s) {
            return n;
        }
        n++;
    }

    return -1;
}

void TPropertiesDialog::setPlayerAdditionalArguments(const QString& args) {
    player_args_edit->setText(args);
}

QString TPropertiesDialog::playerAdditionalArguments() {
    return player_args_edit->text();
}

void TPropertiesDialog::setPlayerAdditionalVideoFilters(const QString& s) {
    player_vfilters_edit->setText(s);
}

QString TPropertiesDialog::playerAdditionalVideoFilters() {
    return player_vfilters_edit->text();
}

void TPropertiesDialog::setPlayerAdditionalAudioFilters(const QString& s) {
    player_afilters_edit->setText(s);
}

QString TPropertiesDialog::playerAdditionalAudioFilters() {
    return player_afilters_edit->text();
}

QString TPropertiesDialog::formatSize(qint64 size) {

    const qint64 MB = 1024 * 1024;
    const qint64 GB = 1024 * MB;

    QLocale locale;
    if (size < MB) {
        double kb = (double) size / 1024;
        return tr("%1 KiB (%2 bytes)").arg(locale.toString(kb, 'f', 2),
                                           locale.toString(size));
    }

    if (size < GB) {
        double mb = (double) size / MB;
        return tr("%1 MiB (%2 bytes)").arg(locale.toString(mb, 'f', 2),
                                           locale.toString(size));
    }

    double gb = (double) size / GB;
    return tr("%1 GiB (%2 bytes)").arg(locale.toString(gb, 'f', 2),
                                       locale.toString(size));
}

void TPropertiesDialog::addTracks(QString& s,
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

QString TPropertiesDialog::openPar(QString text) {
    return "<tr><td>&nbsp;</td></tr>"
           "<tr><td colspan=\"5\"><h2>" + text + "</h2></td></tr>";
}

QString TPropertiesDialog::closePar() {
    return "\n";
}

QString TPropertiesDialog::addItem(QString tag, QString value) {
    return "<tr><th align=\"left\" colspan=\"2\">" + tag+ "</th>"
            "<td align=\"left\" colspan=\"3\">" + value + "</td></tr>";
}

QString TPropertiesDialog::getInfo(const QString& title) {

    const TMediaData& md = player->mdat;

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
        case TMediaData::TYPE_DVD: icon = "type_dvd"; break;
        case TMediaData::TYPE_DVDNAV: icon = "type_dvd"; break;
        case TMediaData::TYPE_VCD: icon = "type_vcd"; break;
        case TMediaData::TYPE_CDDA: icon = "type_vcd"; break;
        case TMediaData::TYPE_TV: icon = "type_tv"; break;
        case TMediaData::TYPE_STREAM: icon = "type_url"; break;
        case TMediaData::TYPE_BLURAY: icon = "type_bluray"; break;
        default: icon = "type_unknown";
    }

    QString s = "<html><body>\n";
    s += "<h1><img src=\"" + Images::iconFilename(icon) + "\">"
            + title + "</h1>\n";

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
    s += addItem(tr("Duration"), TWZTime::formatMS(md.duration_ms)
                 + " " + tr("(h:min:sec%1ms)").arg(QLocale().decimalPoint()));
    s += addItem(tr("Start time"),
                 TWZTime::formatMS(md.start_ms_player)
                 + tr(" (%1 seconds)")
                 .arg(QString::number(md.getStartSecPlayer())));
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
        if (md.video_bitrate > 0) {
            s += addItem(tr("Bitrate"),tr("%1 kbps")
                         .arg(md.video_bitrate / 1000));
        }
        s += addItem(tr("Frames per second"), QString::number(md.video_fps));
        s += addItem(tr("Angle"), tr("%1/%2").arg(md.angle).arg(md.angles));
        if (!md.video_colorspace.isEmpty()) {
            s += addItem(tr("Colorspace"), md.video_colorspace);
        }
        if (!md.video_out_colorspace.isEmpty()) {
            s += addItem(tr("Colorspace out"),
                         tr("%1 (from last open)")
                         .arg(md.video_out_colorspace));
        }
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
        if (md.audio_bitrate > 0) {
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
                case SubData::Vob:    t = tr("VOB"); break;
                default:            t = tr("demuxer"); break;
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

void TPropertiesDialog::showInfo(const QString& title) {

    QScrollBar* bar = info_edit->verticalScrollBar();
    int scrollPos = bar->value();
    info_edit->setText(getInfo(title));
    bar->setValue(scrollPos);
}

} // namespace Gui

#include "moc_propertiesdialog.cpp"

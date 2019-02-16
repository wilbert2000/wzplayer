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

#include "gui/filepropertiesdialog.h"

#include "images.h"
#include "gui/infofile.h"
#include "config.h"


namespace Gui {

TFilePropertiesDialog::TFilePropertiesDialog(QWidget* parent,
                                             TMediaData* md)
    : QDialog(parent, TConfig::DIALOG_FLAGS),
    media_data(md) {

    setupUi(this);

    // Setup buttons
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked,
            this, &TFilePropertiesDialog::apply);

    codecs_set = false;

    retranslateStrings();
}

TFilePropertiesDialog::~TFilePropertiesDialog() {
}

void TFilePropertiesDialog::closeEvent(QCloseEvent* event) {

    emit visibilityChanged(false);
    event->accept();
}

void TFilePropertiesDialog::showInfo() {

    TInfoFile info;
    info_edit->setText(info.getInfo(*media_data));
}

void TFilePropertiesDialog::retranslateStrings() {

    retranslateUi(this);
    setWindowIcon(Images::icon("logo"));
}

void TFilePropertiesDialog::accept() {

    setResult(QDialog::Accepted);
    hide();
    emit visibilityChanged(false);
    emit applied();
}

void TFilePropertiesDialog::reject() {

    setResult(QDialog::Rejected);
    hide();
    emit visibilityChanged(false);
}

void TFilePropertiesDialog::apply() {

    setResult(QDialog::Accepted);
    emit applied();
}

void TFilePropertiesDialog::setCodecs(const Player::Info::InfoList& vc,
                                      const Player::Info::InfoList& ac,
                                      const Player::Info::InfoList& demuxer) {

    vclist = vc;
    aclist = ac;
    demuxerlist = demuxer;

    qSort(vclist);
    qSort(aclist);
    qSort(demuxerlist);

    vc_listbox->clear();
    ac_listbox->clear();
    demuxer_listbox->clear();

    Player::Info::InfoList::iterator it;

    for (it = vclist.begin(); it != vclist.end(); ++it) {
        vc_listbox->addItem((*it).name() + " - " + (*it).desc());
    }

    for (it = aclist.begin(); it != aclist.end(); ++it) {
        ac_listbox->addItem((*it).name() + " - " + (*it).desc());
    }

    for (it = demuxerlist.begin(); it != demuxerlist.end(); ++it) {
        demuxer_listbox->addItem((*it).name() + " - " + (*it).desc());
    }

    codecs_set = true;
}

void TFilePropertiesDialog::setDemuxer(const QString& demuxer, const QString& original_demuxer) {

    if (!original_demuxer.isEmpty()) {
        orig_demuxer = original_demuxer;
        int pos = find(orig_demuxer, demuxerlist);
        if (pos >= 0) {
            media_data->demuxer_description = demuxerlist[pos].desc();
        }
    }

    int pos = find(demuxer, demuxerlist);
    if (pos >= 0) {
        demuxer_listbox->setCurrentRow(pos);
    }

    WZDEBUG("'" + demuxer + "'");
}

QString TFilePropertiesDialog::demuxer() {

    int pos = demuxer_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return demuxerlist[pos].name();
}

void TFilePropertiesDialog::setVideoCodec(const QString& vc, const QString& original_vc) {

    if (!original_vc.isEmpty()) {
        orig_vc = original_vc;
        int pos = find(orig_vc, vclist);
        if (pos >= 0) {
            media_data->video_codec_description = vclist[pos].desc();
        }
    }

    int pos = find(vc, vclist);
    if (pos >= 0) {
        vc_listbox->setCurrentRow(pos);
    }

    WZDEBUG("'" + vc + "'");
}

QString TFilePropertiesDialog::videoCodec() {

    int pos = vc_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return vclist[pos].name();
}

void TFilePropertiesDialog::setAudioCodec(const QString& ac, const QString& original_ac) {

    if (!original_ac.isEmpty()) {
        orig_ac = original_ac;
        int pos = find(orig_ac, aclist);
        if (pos >= 0) {
            media_data->audio_codec_description = aclist[pos].desc();
        }
    }
    int pos = find(ac, aclist);
    if (pos >= 0) {
        ac_listbox->setCurrentRow(pos);
    }

    WZDEBUG("'" + ac + "'");
}

QString TFilePropertiesDialog::audioCodec() {

    int pos = ac_listbox->currentRow();
    if (pos < 0) {
        return "";
    }
    return aclist[pos].name();
}

void TFilePropertiesDialog::on_resetDemuxerButton_clicked() {
    setDemuxer(orig_demuxer);
}

void TFilePropertiesDialog::on_resetACButton_clicked() {
    setAudioCodec(orig_ac);
}

void TFilePropertiesDialog::on_resetVCButton_clicked() {
    setVideoCodec(orig_vc);
}

int TFilePropertiesDialog::find(const QString& s,
                                const Player::Info::InfoList& list) const {

    int n = 0;
    Player::Info::InfoList::const_iterator it;

    for (it = list.constBegin(); it != list.constEnd(); ++it) {
        if ((*it).name() == s) {
            return n;
        }
        n++;
    }

    return -1;
}

void TFilePropertiesDialog::setPlayerAdditionalArguments(const QString& args) {
    player_args_edit->setText(args);
}

QString TFilePropertiesDialog::playerAdditionalArguments() {
    return player_args_edit->text();
}

void TFilePropertiesDialog::setPlayerAdditionalVideoFilters(const QString& s) {
    player_vfilters_edit->setText(s);
}

QString TFilePropertiesDialog::playerAdditionalVideoFilters() {
    return player_vfilters_edit->text();
}

void TFilePropertiesDialog::setPlayerAdditionalAudioFilters(const QString& s) {
    player_afilters_edit->setText(s);
}

QString TFilePropertiesDialog::playerAdditionalAudioFilters() {
    return player_afilters_edit->text();
}

// Language change stuff
void TFilePropertiesDialog::changeEvent(QEvent *e) {

    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QDialog::changeEvent(e);
    }
}

} // namespace Gui

#include "moc_filepropertiesdialog.cpp"

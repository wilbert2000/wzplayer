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

#ifndef GUI_PROPERTIESDIALOG_H
#define GUI_PROPERTIESDIALOG_H

#include "ui_propertiesdialog.h"
#include "player/info/playerinfo.h"
#include "mediadata.h"
#include "wzdebug.h"


class QPushButton;

namespace Gui {

class TPropertiesDialog : public QDialog, public Ui::TPropertiesDialog {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    TPropertiesDialog(QWidget* parent);

    void setDemuxer(const QString& demuxer,
                    const QString& original_demuxer = "");
    QString demuxer();

    void setCodecs(const Player::Info::TNameDescList& vc,
                   const Player::Info::TNameDescList& ac,
                   const Player::Info::TNameDescList& demuxer);
    void setVideoCodec(const QString& vc, const QString& original_vc = "");
    QString videoCodec();
    void setAudioCodec(const QString& ac, const QString& original_ac = "");
    QString audioCodec();

    void setPlayerAdditionalArguments(const QString& args);
    QString playerAdditionalArguments();
    void setPlayerAdditionalVideoFilters(const QString& s);
    QString playerAdditionalVideoFilters();
    void setPlayerAdditionalAudioFilters(const QString& s);
    QString playerAdditionalAudioFilters();

    void showInfo(const QString& title);

    void saveSettings();

public slots:
    void accept(); // Reimplemented to send a signal
    void reject();
    void apply();

signals:
    void applied();
    void visibilityChanged(bool visible);

protected slots:
    virtual void on_resetDemuxerButton_clicked();
    virtual void on_resetACButton_clicked();
    virtual void on_resetVCButton_clicked();

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    Player::Info::TNameDescList demuxerlist, vclist, aclist;
    QString orig_demuxer, orig_vc, orig_ac;

    QPushButton* okButton;
    QPushButton* cancelButton;
    QPushButton* applyButton;

    int find(const QString& s, const Player::Info::TNameDescList& list) const;

    QString openPar(QString text);
    QString closePar();
    QString addItem(QString tag, QString value);
    QString formatSize(qint64 size);
    void addTracks(QString& s,
                   const Maps::TTracks& tracks,
                   const QString& name);
    QString getInfo(const QString& title);
};

} // namespace Gui

#endif // GUI_PROPERTIESDIALOG_H

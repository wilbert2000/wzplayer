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

#ifndef _GUI_FILEPROPERTIESDIALOG_H_
#define _GUI_FILEPROPERTIESDIALOG_H_

#include "ui_filepropertiesdialog.h"
#include "inforeader.h"
#include "mediadata.h"
#include "config.h"

class QPushButton;

namespace Gui {

class TFilePropertiesDialog : public QDialog, public Ui::TFilePropertiesDialog
{
	Q_OBJECT

public:
	TFilePropertiesDialog(QWidget* parent, const TMediaData& md);
	virtual ~TFilePropertiesDialog();

	// Call it as soon as possible
	void setCodecs(const InfoList& vc, const InfoList& ac, const InfoList& demuxer);

	void setDemuxer(const QString& demuxer, const QString& original_demuxer="");
	QString demuxer();

	void setVideoCodec(const QString& vc, const QString& original_vc = "");
	QString videoCodec();

	void setAudioCodec(const QString& ac, const QString& original_ac="");
	QString audioCodec();

	void setMplayerAdditionalArguments(const QString& args);
	QString mplayerAdditionalArguments();

	void setMplayerAdditionalVideoFilters(const QString& s);
	QString mplayerAdditionalVideoFilters();

	void setMplayerAdditionalAudioFilters(const QString& s);
	QString mplayerAdditionalAudioFilters();

	void showInfo();

public slots:
	void accept(); // Reimplemented to send a signal
	void apply();

signals:
	void applied();

protected slots:
	virtual void on_resetDemuxerButton_clicked();
	virtual void on_resetACButton_clicked();
	virtual void on_resetVCButton_clicked();

protected:
	bool hasCodecsList() { return codecs_set; }
	int find(const QString s, InfoList &list);

protected:
	virtual void retranslateStrings();
	virtual void changeEvent(QEvent* event);

private:
	bool codecs_set;
	InfoList vclist, aclist, demuxerlist;
	QString orig_demuxer, orig_ac, orig_vc;
	QString demuxer_description;
	QString video_codec_description;
	QString audio_codec_description;
	const TMediaData* media_data;

	QPushButton* okButton;
	QPushButton* cancelButton;
	QPushButton* applyButton;
};

} // namespace Gui

#endif // _GUI_FILEPROPERTIESDIALOG_H_

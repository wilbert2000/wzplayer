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

#include "gui/tvlist.h"
#include "gui/favoriteeditor.h"
#include "images.h"

#include <QFile>
#include <QDir>
#include <QTextStream>

namespace Gui {

TTVList::TTVList(bool check_channels_conf, Services services, QString filename, QWidget * parent)
	: TFavorites(filename,parent)
{
#ifndef Q_OS_WIN
	if (check_channels_conf) {
		/* f_list.clear(); */
		parse_channels_conf(services);
		updateMenu();
	}
#endif
}

TTVList::~TTVList() {
}

TFavorites * TTVList::createNewObject(QString filename, QWidget * parent) {
	return new TTVList(false, TV, filename, parent);
}

#ifndef Q_OS_WIN
void TTVList::parse_channels_conf(Services services) {
	qDebug("Gui::TTVList::parse_channels_conf");

	QString file = QDir::homePath() + "/.mplayer/channels.conf.ter";

	if (!QFile::exists(file)) {
		qDebug("VList::parse_channels_conf: %s doesn't exist", file.toUtf8().constData());
		file = QDir::homePath() + "/.mplayer/channels.conf";
	}

	QFile f( file );
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug("Gui::TTVList::parse_channels_conf: can't open %s", file.toUtf8().constData());
		return;
	}

	QTextStream in(&f);
	while (!in.atEnd()) {
		QString line = in.readLine();
		qDebug("Gui::TTVList::parse_channels_conf: '%s'", line.toUtf8().constData());
		QString channel = line.section(':', 0, 0);
		QString video_pid = line.section(':', 10, 10);
		QString audio_pid = line.section(':', 11, 11);
		bool is_radio = (video_pid == "0" && audio_pid != "0");
		bool is_data = (video_pid == "0" && audio_pid == "0");
		bool is_tv = (!is_radio && !is_data);
		if (!channel.isEmpty()) {
			qDebug("Gui::TTVList::parse_channels_conf: channel: '%s' video_pid: %s audio_pid: %s", channel.toUtf8().constData(),video_pid.toUtf8().constData(),audio_pid.toUtf8().constData());
			QString channel_id = "dvb://"+channel;
			if (findFile(channel_id) == -1) {
				if ( (services.testFlag(TTVList::TV) && is_tv) ||
					 (services.testFlag(TTVList::Radio) && is_radio) ||
					 (services.testFlag(TTVList::Data) && is_data) )
				{
					f_list.append( TFavorite(channel, channel_id) );
				}
			}
		}
	}
}

QString TTVList::findChannelsFile() {
	QString channels_file;

	QString file = QDir::homePath() + "/.mplayer/channels.conf.ter";
	if (QFile::exists(file)) return file;

	file = QDir::homePath() + "/.mplayer/channels.conf";
	if (QFile::exists(file)) return file;

	file = QDir::homePath() + "/.config/mpv/channels.conf.ter";
	if (QFile::exists(file)) return file;

	file = QDir::homePath() + "/.config/mpv/channels.conf";
	if (QFile::exists(file)) return file;

	return QString::null;
}
#endif

void TTVList::edit() {
	qDebug("Gui::TTVList::edit");

	TFavoriteEditor e(parent_widget);

	e.setWindowTitle( tr("Channel editor") );
	e.setCaption( tr("TV/Radio list") );
	e.setDialogIcon( Images::icon("open_tv") );

	e.setData(f_list);
	e.setStorePath( QFileInfo(_filename).absolutePath() );

	if (e.exec() == QDialog::Accepted) {
		f_list = e.data();
		updateMenu();
	}
}

} // namespace Gui

#include "moc_tvlist.cpp"
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

#include "inforeader.h"
#include "settings/preferences.h"
#include "proc/playerprocess.h"
#include "settings/paths.h"
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QDebug>

#ifdef MPV_SUPPORT
#include "inforeadermpv.h"
#endif

#ifdef MPLAYER_SUPPORT
#include "inforeadermplayer.h"
#endif

#define INFOREADER_SAVE_VERSION 3

using namespace Settings;

InfoReader* InfoReader::static_obj = 0;

InfoReader* InfoReader::obj() {

	if (!static_obj) {
		static_obj = new InfoReader();
	}
	return static_obj;
}

InfoReader::InfoReader()
	: QObject()
	, bin_size(0) {
}

InfoReader::~InfoReader() {
}

void InfoReader::getInfo() {
	getInfo(Settings::pref->playerAbsolutePath());
}

QString InfoReader::getGroup() {
	QString group = bin;
	return group.replace("/", "_").replace("\\", "_").replace(".", "_").replace(":", "_");
}

void InfoReader::clearInfo() {

	vo_list.clear();
	ao_list.clear();
	demuxerList().clear();
	vc_list.clear();
	ac_list.clear();
	vf_list.clear();
	option_list.clear();
}

void InfoReader::getInfo(const QString& path) {
	qDebug() << "InfoReader::getInfo:" << path;

	// Player not existing
	QFileInfo fi(path);
	if (!fi.exists()) {
		bin = path;
		bin_size = 0;
		clearInfo();
		qWarning() << "InforReader::getInfo: player" << path << "not found";
		return;
	}

	// Already loaded info
	qint64 size = fi.size();
	if (path == bin && size == bin_size) {
		qDebug("InfoReader::getInfo: reusing player info");
		return;
	}

	bin = path;
	bin_size = size;

	// Get info from ini file
	QString inifile = TPaths::configPath() + "/player_info_version_"
					  + QString::number(INFOREADER_SAVE_VERSION) + ".ini";
	QSettings set(inifile, QSettings::IniFormat);
	set.beginGroup(getGroup());

	if (set.value("size", -1).toInt() == bin_size) {
		vo_list = convertListToInfoList(set.value("vo_list").toStringList());
		ao_list = convertListToInfoList(set.value("ao_list").toStringList());
		demuxer_list = convertListToInfoList(set.value("demuxer_list").toStringList());
		vc_list = convertListToInfoList(set.value("vc_list").toStringList());
		ac_list = convertListToInfoList(set.value("ac_list").toStringList());
		vf_list = set.value("vf_list").toStringList();
		option_list = set.value("option_list").toStringList();

		qDebug() << "InfoReader::getInfo: loaded player info from" << inifile;
		return;
	}

	// Get info from player
	bool save = false;
	if (TPreferences::getPlayerID(bin) == TPreferences::ID_MPLAYER) {
#ifdef MPLAYER_SUPPORT
		InfoReaderMplayer ir(bin);
		ir.getInfo();
		vo_list = ir.voList();
		ao_list = ir.aoList();
		demuxer_list = ir.demuxerList();
		vc_list = ir.vcList();
		ac_list = ir.acList();
		vf_list.clear();
		option_list.clear();
		save = true;
#endif
	} else {
#ifdef MPV_SUPPORT
		InfoReaderMPV ir(bin);
		ir.getInfo();
		vo_list = ir.voList();
		ao_list = ir.aoList();
		demuxer_list = ir.demuxerList();
		vc_list = ir.vcList();
		ac_list = ir.acList();
		vf_list = ir.vfList();
		option_list = ir.optionList();
		save = true;
#endif
	}

	if (save) {
		qDebug() << "InfoReader::getInfo: saving info to" << inifile;
		set.setValue("size", bin_size);
		set.setValue("date", fi.lastModified());
		set.setValue("vo_list", convertInfoListToList(vo_list));
		set.setValue("ao_list", convertInfoListToList(ao_list));
		set.setValue("demuxer_list", convertInfoListToList(demuxer_list));
		set.setValue("vc_list", convertInfoListToList(vc_list));
		set.setValue("ac_list", convertInfoListToList(ac_list));
		set.setValue("vf_list", vf_list);
		set.setValue("option_list", option_list);
	} else {
		clearInfo();
		qWarning("InfoReader::getInfo: support for player not compiled");
	}
}

QStringList InfoReader::convertInfoListToList(InfoList l) {
	QStringList r;
	for (int n = 0; n < l.count(); n++) {
		r << l[n].name() + "|" + l[n].desc();
	}
	return r;
}

InfoList InfoReader::convertListToInfoList(QStringList l) {
	InfoList r;
	for (int n = 0; n < l.count(); n++) {
		QStringList s = l[n].split("|");
		if (s.count() >= 2) {
			r.append(InfoData(s[0], s[1]));
		}
	}
	return r;
}

#include "moc_inforeader.cpp"

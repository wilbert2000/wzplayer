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

#define INFOREADER_SAVE_VERSION 2

using namespace Settings;

InfoReader* InfoReader::static_obj = 0;

InfoReader* InfoReader::obj() {

	if (!static_obj) {
		static_obj = new InfoReader();
	}
	return static_obj;
}

InfoReader::InfoReader(QObject* parent)
	: QObject(parent) {
}

InfoReader::~InfoReader() {
}

void InfoReader::getInfo() {

	QString inifile = TPaths::configPath() + "/player_info.ini";
	QSettings set(inifile, QSettings::IniFormat);

	QString version_group = "version_" + QString::number(INFOREADER_SAVE_VERSION);

	QString bin = Settings::pref->playerAbsolutePath();
	QString sname = bin;
	sname = sname.replace("/", "_").replace("\\", "_").replace(".", "_").replace(":", "_");
	QFileInfo fi(bin);
	if (fi.exists()) {
		sname += "_" + QString::number(fi.size());
		qDebug() << "InfoReader::getInfo: sname:" << sname;

		// Check if we already have info about the player in the ini file
		bool got_info = false;
		set.beginGroup(version_group +"/"+ sname);
		if (set.value("size", -1).toInt() == fi.size()) {
			got_info = true;
			vo_list = convertListToInfoList(set.value("vo_list").toStringList());
			ao_list = convertListToInfoList(set.value("ao_list").toStringList());
			demuxer_list = convertListToInfoList(set.value("demuxer_list").toStringList());
			vc_list = convertListToInfoList(set.value("vc_list").toStringList());
			ac_list = convertListToInfoList(set.value("ac_list").toStringList());
			vf_list = set.value("vf_list").toStringList();
			option_list = set.value("option_list").toStringList();
		}
		set.endGroup();
		if (got_info) {
			qDebug() << "InfoReader::getInfo: loaded info from" << inifile;
			return;
		}
	}

	if (pref->player_id == TPreferences::MPV) {
#ifdef MPV_SUPPORT
		qDebug("InfoReader::getInfo: mpv");
		InfoReaderMPV ir(this);
		ir.getInfo();
		vo_list = ir.voList();
		ao_list = ir.aoList();
		demuxer_list = ir.demuxerList();
		vc_list = ir.vcList();
		ac_list = ir.acList();
		vf_list = ir.vfList();
		option_list = ir.optionList();
#endif
	} else {
#ifdef MPLAYER_SUPPORT
		qDebug("InfoReader::getInfo: mplayer");
		InfoReaderMplayer ir(this);
		ir.getInfo();
		vo_list = ir.voList();
		ao_list = ir.aoList();
		demuxer_list = ir.demuxerList();
		vc_list = ir.vcList();
		ac_list = ir.acList();
		vf_list.clear();
		option_list.clear();
#endif
	}

	if (fi.exists()) {
		qDebug() << "InfoReader::getInfo: saving info to" << inifile;
		set.beginGroup(version_group +"/"+ sname);
		set.setValue("size", fi.size());
		set.setValue("date", fi.lastModified());
		set.setValue("vo_list", convertInfoListToList(vo_list));
		set.setValue("ao_list", convertInfoListToList(ao_list));
		set.setValue("demuxer_list", convertInfoListToList(demuxer_list));
		set.setValue("vc_list", convertInfoListToList(vc_list));
		set.setValue("ac_list", convertInfoListToList(ac_list));
		set.setValue("vf_list", vf_list);
		set.setValue("option_list", option_list);
		set.endGroup();
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

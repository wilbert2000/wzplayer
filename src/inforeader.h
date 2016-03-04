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


#ifndef INFOREADER_H
#define INFOREADER_H

#include <QObject>
#include <QList>
#include <QStringList>


class InfoData {

public:
	InfoData() {}
	InfoData(const QString& name, const QString& desc) {
		_name = name;
		_desc = desc;
	};
	virtual ~InfoData() {}

	void setName(const QString& name) { _name = name; }
	void setDesc(const QString& desc) { _desc = desc; }

	QString name() const { return _name; }
	QString desc() const { return _desc; }

	bool operator<(const InfoData& other) const {
		return name() < other.name();
	}

	bool operator==(const InfoData& other) const {
		return name() == other.name();
	}

private:
	QString _name, _desc;
};


typedef QList<InfoData> InfoList;


class InfoReader : QObject {
	Q_OBJECT

public:
	InfoReader(QObject* parent = 0);
	virtual ~InfoReader();

	void getInfo();

	InfoList voList() { return vo_list; }
	InfoList aoList() { return ao_list; }

	InfoList demuxerList() { return demuxer_list; }
	InfoList vcList() { return vc_list; }
	InfoList acList() { return ac_list; }
	QStringList vfList() { return vf_list; }
	QStringList optionList() { return option_list; }

	//! Returns an InfoReader object. If it didn't exist before, one
	//! is created.
	static InfoReader* obj();

protected:
	InfoList vo_list;
	InfoList ao_list;

	InfoList demuxer_list;
	InfoList vc_list;
	InfoList ac_list;
	QStringList vf_list;
	QStringList option_list;

private:
	static InfoReader* static_obj;
	static QStringList convertInfoListToList(InfoList l);
	static InfoList convertListToInfoList(QStringList l);
};

#endif

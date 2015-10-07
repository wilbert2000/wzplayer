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

#ifndef _ASSSTYLES_H_
#define _ASSSTYLES_H_

#include <QString>

class QSettings;

class TAssStyles {

public:
	enum HAlignment { Left = 1, HCenter = 2, Right = 3 };
	enum VAlignment { Bottom = 0, VCenter = 1, Top = 2 };
	enum BorderStyle { Outline = 1, Opaque = 3 };

	TAssStyles();
	virtual ~TAssStyles() {}

	QString fontname;
	int fontsize;
	unsigned int primarycolor;
	unsigned int backcolor;
	unsigned int outlinecolor;
	bool bold;
	bool italic;
	int halignment;
	int valignment;
	int borderstyle;
	double outline;
	double shadow;
	int marginl;
	int marginr;
	int marginv;

	void save(QSettings* set);
	void load(QSettings* set);

	bool exportStyles(const QString & filename) const;
	QString toString();
};

#endif

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

#ifndef _GUI_INFOFILE_H_
#define _GUI_INFOFILE_H_

#include "mediadata.h"
#include <QString>

namespace Gui {

class TInfoFile {

public:
	TInfoFile();
	virtual ~TInfoFile();

	QString getInfo(const TMediaData& md);

protected:
	QString title(QString text);
	QString openPar(QString text);
	QString closePar();
	QString openItem();
	QString closeItem();

	QString addItem(QString tag, QString value);

	int row;

private:
	inline QString tr(const char* sourceText, const char* comment = 0, int n = -1);
	QString formatSize(qint64 size);
	void addTracks(QString& s, const Maps::TTracks& tracks, const QString& name);
};

} // namespace Gui

#endif // _GUI_INFOFILE_H_
